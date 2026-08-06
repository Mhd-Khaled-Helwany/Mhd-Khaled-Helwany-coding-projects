/**
 * IPC transport layer connecting ShmSpscRing with StrategyTransportCodec.
 *
 * Implements IStrategyTransport using two SPSC rings over shared memory:
 *   - eventRing: engine (writer) -> worker (reader)
 *   - actionRing: worker (writer) -> engine (reader)
 *
 * Role-based access control: engine can only sendEvent/tryRecvAction,
 * worker can only sendAction/tryRecvEvent. This enforces the SPSC guarantee
 * (one writer, one reader per ring) at the API level.
 *
 * Wakeup is event-driven via eventfd: each send does eventfd_write(peerWakeFd)
 * so the other side wakes up from poll()/epoll without busy-waiting.
 *
 * Factory: make() with createRings=true creates new SHM regions (engine side),
 * createRings=false opens existing ones (worker side).
 */
#pragma once

#include "engine/ShmSpscRing.hpp"
#include "engine/StrategyTransport.hpp"

#include <expected>
#include <memory>
#include <string>
#include <string_view>

enum class ShmStrategyTransportRole {
    Engine,
    Worker
};

struct ShmStrategyTransportConfig {
    ShmStrategyTransportRole role{ShmStrategyTransportRole::Engine};
    std::string eventsRingName;
    std::string actionsRingName;
    std::size_t ringRequestedCapacity{1024};
    std::size_t maxMessageBytes{4096};
    int localWakeFd{-1};
    int peerWakeFd{-1};
    bool createRings{false};
    bool unlinkOnDestroy{false};
};

class ShmStrategyTransport final : public IStrategyTransport {
public:
    /**
     * @brief Factory: creates/opens rings and returns a ready-to-use transport.
     * 
     * @param config The configuration specifying role (Engine/Worker), names, and capacities.
     * @return Expected containing the unique pointer to the transport, or an error string.
     */
    static std::expected<std::unique_ptr<ShmStrategyTransport>, std::string> make(ShmStrategyTransportConfig config);

    static std::string eventsRingName(std::string_view prefix);
    static std::string actionsRingName(std::string_view prefix);
    static std::string metricsShmName(std::string_view prefix);

    ~ShmStrategyTransport() override = default;

    /**
     * @brief Engine -> worker: encode event and push to eventRing.
     * 
     * Requires the transport to be acting in the Engine role.
     * 
     * @param event The event object to serialize and send.
     * @return true on success, false if ring is full or serialization fails.
     */
    bool sendEvent(const StrategyEvent& event) override;

    /**
     * @brief Worker side: pop from eventRing and decode.
     * 
     * Requires the transport to be acting in the Worker role.
     * 
     * @param event Output parameter populated with the decoded event.
     * @return true if an event was popped and decoded, false if empty.
     */
    bool tryRecvEvent(StrategyEvent& event) override;

    /**
     * @brief Worker -> engine: encode action and push to actionRing.
     * 
     * Requires the transport to be acting in the Worker role.
     * 
     * @param action The action object to serialize and send.
     * @return true on success, false if ring is full or serialization fails.
     */
    bool sendAction(const StrategyAction& action) override;

    /**
     * @brief Engine side: pop from actionRing and decode.
     * 
     * Requires the transport to be acting in the Engine role.
     * 
     * @param action Output parameter populated with the decoded action.
     * @return true if an action was popped and decoded, false if empty.
     */
    bool tryRecvAction(StrategyAction& action) override;

    /**
     * @brief Returns the local eventfd, for use in poll()/epoll.
     */
    int wakeupFd() const override;

    /**
     * @brief Drains accumulated wakeup counter so poll() blocks on next call.
     */
    void drainWakeups() override;

    std::string_view lastError() const noexcept;
    std::size_t maxMessageBytes() const noexcept;
    std::size_t ringCapacity() const noexcept;

private:
    ShmStrategyTransport(ShmStrategyTransportRole role,
                         ShmSpscRing eventRing,
                         ShmSpscRing actionRing,
                         int localWakeFd,
                         int peerWakeFd);

    bool notifyPeer() noexcept;
    void setError(std::string message) noexcept;

    ShmStrategyTransportRole role_;
    ShmSpscRing eventRing_;
    ShmSpscRing actionRing_;
    int localWakeFd_{-1};
    int peerWakeFd_{-1};
    std::string lastError_{};
};
