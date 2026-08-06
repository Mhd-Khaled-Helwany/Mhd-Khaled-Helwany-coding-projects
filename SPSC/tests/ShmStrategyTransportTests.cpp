/*
Build & run
-----------

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target ShmStrategyTransportTests
ctest --test-dir build -R ShmStrategyTransportTests -V
*/

#include "engine/ShmStrategyTransport.hpp"

#include <chrono>
#include <iostream>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/eventfd.h>
#include <type_traits>
#include <unistd.h>

struct TestContext { int failed = 0; int passed = 0; std::ostringstream os; } ctx;

#define TEST_CASE(name) void name()
#define REQUIRE_TRUE(cond) do { if (!(cond)) { ++ctx.failed; ctx.os << __FUNCTION__ << ": line " << __LINE__ << ": REQUIRE_TRUE(" #cond ") failed\n"; return; } } while (0)
#define EXPECT_TRUE(cond) do { if (cond) { ++ctx.passed; } else { ++ctx.failed; ctx.os << __FUNCTION__ << ": line " << __LINE__ << ": EXPECT_TRUE(" #cond ") failed\n"; } } while (0)
#define EXPECT_FALSE(cond) EXPECT_TRUE(!(cond))
#define EXPECT_EQ(a, b) do { auto _a = (a); auto _b = (b); if (_a == _b) { ++ctx.passed; } else { ++ctx.failed; ctx.os << __FUNCTION__ << ": line " << __LINE__ << ": EXPECT_EQ failed got=" << _a << " exp=" << _b << "\n"; } } while (0)

namespace {

std::string unique_prefix() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::string("/shm_transport_test_") + std::to_string(getpid()) + "_" + std::to_string(now);
}

bool is_readable(int fd) {
    struct pollfd pfd {
        fd, POLLIN, 0
    };
    return poll(&pfd, 1, 0) == 1 && (pfd.revents & POLLIN);
}

struct EventFdPair {
    int engineFd{-1};
    int workerFd{-1};

    ~EventFdPair() {
        if (engineFd != -1) {
            close(engineFd);
        }
        if (workerFd != -1) {
            close(workerFd);
        }
    }
};

EventFdPair make_eventfds() {
    EventFdPair pair{};
    pair.engineFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    pair.workerFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    return pair;
}

} // namespace

TEST_CASE(test_engine_to_worker_event_roundtrip) {
    EventFdPair wakes = make_eventfds();
    REQUIRE_TRUE(wakes.engineFd != -1);
    REQUIRE_TRUE(wakes.workerFd != -1);

    const std::string prefix = unique_prefix();
    auto engine = ShmStrategyTransport::make({
        .role = ShmStrategyTransportRole::Engine,
        .eventsRingName = ShmStrategyTransport::eventsRingName(prefix),
        .actionsRingName = ShmStrategyTransport::actionsRingName(prefix),
        .ringRequestedCapacity = 16,
        .maxMessageBytes = 2048,
        .localWakeFd = wakes.engineFd,
        .peerWakeFd = wakes.workerFd,
        .createRings = true,
        .unlinkOnDestroy = true,
    });
    REQUIRE_TRUE(engine.has_value());

    auto worker = ShmStrategyTransport::make({
        .role = ShmStrategyTransportRole::Worker,
        .eventsRingName = ShmStrategyTransport::eventsRingName(prefix),
        .actionsRingName = ShmStrategyTransport::actionsRingName(prefix),
        .ringRequestedCapacity = 16,
        .maxMessageBytes = 2048,
        .localWakeFd = wakes.workerFd,
        .peerWakeFd = wakes.engineFd,
        .createRings = false,
        .unlinkOnDestroy = false,
    });
    REQUIRE_TRUE(worker.has_value());

    StrategyEvent sent = StrategyEventBookUpdate{42, 7};
    REQUIRE_TRUE((*engine)->sendEvent(sent));
    EXPECT_TRUE(is_readable(wakes.workerFd));

    (*worker)->drainWakeups();
    StrategyEvent received{};
    REQUIRE_TRUE((*worker)->tryRecvEvent(received));

    bool matched = false;
    std::visit([&](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, StrategyEventBookUpdate>) {
            matched = (payload.instrumentId == 42 && payload.exchangeId == 7);
        }
    }, received);
    EXPECT_TRUE(matched);
}

TEST_CASE(test_worker_to_engine_action_roundtrip) {
    EventFdPair wakes = make_eventfds();
    REQUIRE_TRUE(wakes.engineFd != -1);
    REQUIRE_TRUE(wakes.workerFd != -1);

    const std::string prefix = unique_prefix();
    auto engine = ShmStrategyTransport::make({
        .role = ShmStrategyTransportRole::Engine,
        .eventsRingName = ShmStrategyTransport::eventsRingName(prefix),
        .actionsRingName = ShmStrategyTransport::actionsRingName(prefix),
        .ringRequestedCapacity = 16,
        .maxMessageBytes = 4096,
        .localWakeFd = wakes.engineFd,
        .peerWakeFd = wakes.workerFd,
        .createRings = true,
        .unlinkOnDestroy = true,
    });
    REQUIRE_TRUE(engine.has_value());

    auto worker = ShmStrategyTransport::make({
        .role = ShmStrategyTransportRole::Worker,
        .eventsRingName = ShmStrategyTransport::eventsRingName(prefix),
        .actionsRingName = ShmStrategyTransport::actionsRingName(prefix),
        .ringRequestedCapacity = 16,
        .maxMessageBytes = 4096,
        .localWakeFd = wakes.workerFd,
        .peerWakeFd = wakes.engineFd,
        .createRings = false,
        .unlinkOnDestroy = false,
    });
    REQUIRE_TRUE(worker.has_value());

    StrategyAction action = StrategyActionSetupExchange{
        .exchangeId = 11,
        .accountId = 99,
        .config = CSOEHConfig{
            .accountId = 99,
            .requestId = "req-1",
            .processId = "worker-1",
            .version = "1.0",
            .options = {{"includeContext", true}, {"filterByProcessId", false}},
        },
    };

    REQUIRE_TRUE((*worker)->sendAction(action));
    EXPECT_TRUE(is_readable(wakes.engineFd));

    (*engine)->drainWakeups();
    StrategyAction received{};
    REQUIRE_TRUE((*engine)->tryRecvAction(received));

    bool matched = false;
    std::visit([&](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, StrategyActionSetupExchange>) {
            matched =
                payload.exchangeId == 11 &&
                payload.accountId == 99 &&
                payload.config.requestId == "req-1" &&
                payload.config.processId == "worker-1" &&
                payload.config.options.contains("includeContext");
        }
    }, received);
    EXPECT_TRUE(matched);
}

TEST_CASE(test_engine_side_backpressure_reports_full_ring) {
    EventFdPair wakes = make_eventfds();
    REQUIRE_TRUE(wakes.engineFd != -1);
    REQUIRE_TRUE(wakes.workerFd != -1);

    const std::string prefix = unique_prefix();
    auto engine = ShmStrategyTransport::make({
        .role = ShmStrategyTransportRole::Engine,
        .eventsRingName = ShmStrategyTransport::eventsRingName(prefix),
        .actionsRingName = ShmStrategyTransport::actionsRingName(prefix),
        .ringRequestedCapacity = 2,
        .maxMessageBytes = 512,
        .localWakeFd = wakes.engineFd,
        .peerWakeFd = wakes.workerFd,
        .createRings = true,
        .unlinkOnDestroy = true,
    });
    REQUIRE_TRUE(engine.has_value());

    auto worker = ShmStrategyTransport::make({
        .role = ShmStrategyTransportRole::Worker,
        .eventsRingName = ShmStrategyTransport::eventsRingName(prefix),
        .actionsRingName = ShmStrategyTransport::actionsRingName(prefix),
        .ringRequestedCapacity = 2,
        .maxMessageBytes = 512,
        .localWakeFd = wakes.workerFd,
        .peerWakeFd = wakes.engineFd,
        .createRings = false,
        .unlinkOnDestroy = false,
    });
    REQUIRE_TRUE(worker.has_value());

    bool sawFull = false;
    const std::size_t maxAttempts = (*engine)->ringCapacity() + 2;
    for (std::size_t i = 0; i < maxAttempts; ++i) {
        if (!(*engine)->sendEvent(StrategyEventBookUpdate{static_cast<instrument_id_type>(i + 1), 7})) {
            sawFull = true;
            break;
        }
    }

    EXPECT_TRUE(sawFull);
    EXPECT_TRUE(std::string((*engine)->lastError()).find("full") != std::string::npos);
}

int main() {
    test_engine_to_worker_event_roundtrip();
    test_worker_to_engine_action_roundtrip();
    test_engine_side_backpressure_reports_full_ring();

    if (ctx.failed) {
        std::cerr << "\nTEST FAILURES: " << ctx.failed << " failed, " << ctx.passed << " passed\n";
        std::cerr << ctx.os.str();
        return 1;
    }

    std::cout << "All ShmStrategyTransport tests passed. Assertions: " << ctx.passed << "\n";
    return 0;
}
