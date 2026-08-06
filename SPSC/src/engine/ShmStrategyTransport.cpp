/**
 * Implementation of the SHM-based IPC transport.
 *
 * Connects ShmSpscRing (ring buffer) with StrategyTransportCodec (JSON
 * serialization) to provide sendEvent/tryRecvEvent/sendAction/tryRecvAction.
 *
 * Key details:
 *   - Role enforcement: engine can only write to eventRing / read from actionRing.
 *     Worker can only write to actionRing / read from eventRing. Wrong-role
 *     calls are rejected, protecting the SPSC one-writer-one-reader guarantee.
 *   - Wakeup: each send() does eventfd_write(peerWakeFd_) to wake the other side.
 *   - drainWakeups(): eventfd is a counter; after processing, drain all accumulated
 *     wakeups so the next poll() blocks correctly.
 *   - make() factory: createRings=true -> shm_open(O_CREAT) [engine side],
 *     createRings=false -> shm_open(O_RDWR) [worker side, opens existing rings].
 *   - Backpressure: if the ring is full, tryPush returns false and the send
 *     method propagates the failure upward.
 */
#include "engine/ShmStrategyTransport.hpp"

#include "engine/StrategyTransportCodec.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <sys/eventfd.h>
#include <system_error>
#include <unistd.h>

namespace {

std::string makeErrorMessage(std::string_view prefix, int err) {
    std::error_code ec(err, std::system_category());
    return std::string(prefix) + ": " + ec.message() + " (" + std::to_string(err) + ")";
}

} // namespace

std::expected<std::unique_ptr<ShmStrategyTransport>, std::string>
ShmStrategyTransport::make(ShmStrategyTransportConfig config) {
    std::expected<ShmSpscRing, std::string> eventRing = std::unexpected("not initialized");
    std::expected<ShmSpscRing, std::string> actionRing = std::unexpected("not initialized");

    if (config.createRings) {
        eventRing = ShmSpscRing::create(config.eventsRingName,
                                        config.ringRequestedCapacity,
                                        config.maxMessageBytes,
                                        config.unlinkOnDestroy);
        if (!eventRing) {
            return std::unexpected(eventRing.error());
        }
        actionRing = ShmSpscRing::create(config.actionsRingName,
                                         config.ringRequestedCapacity,
                                         config.maxMessageBytes,
                                         config.unlinkOnDestroy);
        if (!actionRing) {
            return std::unexpected(actionRing.error());
        }
    } else {
        eventRing = ShmSpscRing::open(config.eventsRingName);
        if (!eventRing) {
            return std::unexpected(eventRing.error());
        }
        actionRing = ShmSpscRing::open(config.actionsRingName);
        if (!actionRing) {
            return std::unexpected(actionRing.error());
        }
    }

    return std::unique_ptr<ShmStrategyTransport>(new ShmStrategyTransport(
        config.role,
        std::move(eventRing.value()),
        std::move(actionRing.value()),
        config.localWakeFd,
        config.peerWakeFd));
}

std::string ShmStrategyTransport::eventsRingName(std::string_view prefix) {
    return std::string(prefix) + ".events";
}

std::string ShmStrategyTransport::actionsRingName(std::string_view prefix) {
    return std::string(prefix) + ".actions";
}

std::string ShmStrategyTransport::metricsShmName(std::string_view prefix) {
    return std::string(prefix) + ".metrics";
}

ShmStrategyTransport::ShmStrategyTransport(ShmStrategyTransportRole role,
                                           ShmSpscRing eventRing,
                                           ShmSpscRing actionRing,
                                           int localWakeFd,
                                           int peerWakeFd)
    : role_(role)
    , eventRing_(std::move(eventRing))
    , actionRing_(std::move(actionRing))
    , localWakeFd_(localWakeFd)
    , peerWakeFd_(peerWakeFd) {}

bool ShmStrategyTransport::sendEvent(const StrategyEvent& event) {
    if (role_ != ShmStrategyTransportRole::Engine) {
        setError("sendEvent is only valid for engine-side transport");
        return false;
    }

    auto encoded = encodeStrategyEventMessage(event);
    if (!encoded) {
        setError(encoded.error());
        return false;
    }
    if (encoded->size() > eventRing_.maxPayloadBytes()) {
        setError("encoded event exceeds ring slot capacity");
        return false;
    }
    if (!eventRing_.tryPush(*encoded)) {
        setError("event ring is full");
        return false;
    }
    if (!notifyPeer()) {
        return false;
    }
    return true;
}

bool ShmStrategyTransport::tryRecvEvent(StrategyEvent& event) {
    if (role_ != ShmStrategyTransportRole::Worker) {
        setError("tryRecvEvent is only valid for worker-side transport");
        return false;
    }

    std::string wire;
    if (!eventRing_.tryPop(wire)) {
        return false;
    }

    auto decoded = decodeStrategyEventMessage(wire);
    if (!decoded) {
        setError(decoded.error());
        return false;
    }
    event = std::move(decoded.value());
    return true;
}

bool ShmStrategyTransport::sendAction(const StrategyAction& action) {
    if (role_ != ShmStrategyTransportRole::Worker) {
        setError("sendAction is only valid for worker-side transport");
        return false;
    }

    auto encoded = encodeStrategyActionMessage(action);
    if (!encoded) {
        setError(encoded.error());
        return false;
    }
    if (encoded->size() > actionRing_.maxPayloadBytes()) {
        setError("encoded action exceeds ring slot capacity");
        return false;
    }
    if (!actionRing_.tryPush(*encoded)) {
        setError("action ring is full");
        return false;
    }
    if (!notifyPeer()) {
        return false;
    }
    return true;
}

bool ShmStrategyTransport::tryRecvAction(StrategyAction& action) {
    if (role_ != ShmStrategyTransportRole::Engine) {
        setError("tryRecvAction is only valid for engine-side transport");
        return false;
    }

    std::string wire;
    if (!actionRing_.tryPop(wire)) {
        return false;
    }

    auto decoded = decodeStrategyActionMessage(wire);
    if (!decoded) {
        setError(decoded.error());
        return false;
    }
    action = std::move(decoded.value());
    return true;
}

int ShmStrategyTransport::wakeupFd() const {
    return localWakeFd_;
}

void ShmStrategyTransport::drainWakeups() {
    if (localWakeFd_ == -1) {
        return;
    }

    for (;;) {
        std::uint64_t value = 0;
        const ssize_t n = read(localWakeFd_, &value, sizeof(value));
        if (n == static_cast<ssize_t>(sizeof(value))) {
            continue;
        }
        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }
        if (n == -1 && errno == EINTR) {
            continue;
        }
        break;
    }
}

std::string_view ShmStrategyTransport::lastError() const noexcept {
    return lastError_;
}

std::size_t ShmStrategyTransport::maxMessageBytes() const noexcept {
    return eventRing_.maxPayloadBytes();
}

std::size_t ShmStrategyTransport::ringCapacity() const noexcept {
    return eventRing_.capacity();
}

bool ShmStrategyTransport::notifyPeer() noexcept {
    if (peerWakeFd_ == -1) {
        return true;
    }

    int rc = eventfd_write(peerWakeFd_, 1);
    if (rc == -1 && errno == EINTR) {
        rc = eventfd_write(peerWakeFd_, 1);
    }
    if (rc == -1 && errno != EAGAIN) {
        setError(makeErrorMessage("eventfd_write failed", errno));
        return false;
    }
    return true;
}

void ShmStrategyTransport::setError(std::string message) noexcept {
    lastError_ = std::move(message);
}
