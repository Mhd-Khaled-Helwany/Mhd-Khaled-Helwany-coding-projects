/*
Build & run
-----------

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target strategy-worker OutOfProcessRuntimeTests
ctest --test-dir build -R OutOfProcessRuntimeTests -V
*/

#include "engine/StrategyRuntime.hpp"
#include "engine/StrategyEngine.hpp"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <poll.h>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

struct TestContext { int failed = 0; int passed = 0; std::ostringstream os; } ctx;

#define TEST_CASE(name) void name()
#define REQUIRE_TRUE(cond) do { if (!(cond)) { ++ctx.failed; ctx.os << __FUNCTION__ << ": line " << __LINE__ << ": REQUIRE_TRUE(" #cond ") failed\n"; return; } } while (0)
#define EXPECT_TRUE(cond) do { if (cond) { ++ctx.passed; } else { ++ctx.failed; ctx.os << __FUNCTION__ << ": line " << __LINE__ << ": EXPECT_TRUE(" #cond ") failed\n"; } } while (0)

namespace {

std::string makeWorkerStrategyConfigJson(std::string_view instanceId) {
    return std::string("{\"base\":{\"instance_id\":\"") + std::string(instanceId) +
           "\",\"strategy_name\":\"MockTestStrategy\","
           "\"instruments\":[{\"id\":42,\"exchange_id\":7,\"code\":\"BTCUSDT\",\"ticksize\":0.01,"
           "\"tick_decimals\":2,\"max_decimals\":2,\"significant_digits\":6,\"min_price\":1.0,"
           "\"max_price\":1000000.0,\"lot_size\":0.001,\"lot_decimals\":3,\"min_order\":0.001,"
           "\"max_order\":10.0,\"min_value\":1.0,\"multiplier\":1.0,\"maker_fee\":0.0,\"taker_fee\":0.0}],"
           "\"accounts\":[{\"exchange_id\":7,\"account_id\":7001,\"instrument_ids\":[42]}]}}";
}

template <typename Predicate>
bool waitUntilReady(OutOfProcessStrategyRuntime& runtime, Predicate&& predicate) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline) {
        runtime.pollHealth(std::chrono::steady_clock::now());
        const int fd = runtime.wakeupFd();
        if (fd == -1) {
            return false;
        }

        struct pollfd pfd {
            fd, POLLIN, 0
        };
        const int rc = poll(&pfd, 1, 100);
        if (rc == -1 && errno == EINTR) {
            continue;
        }

        std::vector<StrategyAction> actions;
        runtime.drainActions(actions, 16);
        for (auto& action : actions) {
            if (predicate(action)) {
                return true;
            }
        }
    }
    return false;
}

bool waitForWorkerRestart(OutOfProcessStrategyRuntime& runtime,
                          int previousPid,
                          std::chrono::milliseconds timeout = std::chrono::seconds(4)) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        runtime.pollHealth(std::chrono::steady_clock::now());
        if (runtime.workerPidForTest() > 0 && runtime.workerPidForTest() != previousPid) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

} // namespace

TEST_CASE(test_out_of_process_runtime_launches_worker_and_receives_ready) {
    OutOfProcessStrategyRuntime runtime(
        static_cast<StrategyHandle>(17),
        "MockTestStrategy",
        "user-1",
        glz::raw_json{"{\"mode\":\"test\"}"});
    runtime.setInstanceId("instance-outproc-1");
    runtime.start();

    const bool sawReady = waitUntilReady(runtime, [](const StrategyAction& action) {
        bool matched = false;
        std::visit([&](const auto& payload) {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<T, StrategyActionWorkerReady>) {
                matched = payload.instanceId == "instance-outproc-1" &&
                          payload.strategyName == "MockTestStrategy";
            }
        }, action);
        return matched;
    });
    EXPECT_TRUE(sawReady);

    runtime.stop();
}

TEST_CASE(test_out_of_process_runtime_roundtrips_event_ack_from_worker) {
    OutOfProcessStrategyRuntime runtime(
        static_cast<StrategyHandle>(18),
        "MockTestStrategy",
        "user-2",
        glz::raw_json{"{\"mode\":\"test\"}"});
    runtime.setInstanceId("instance-outproc-2");
    runtime.start();

    REQUIRE_TRUE(waitUntilReady(runtime, [](const StrategyAction& action) {
        return std::holds_alternative<StrategyActionWorkerReady>(action);
    }));

    runtime.onTopOfBook(42, 7);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    runtime.onTopOfBook(42, 7);

    const bool sawAck = waitUntilReady(runtime, [](const StrategyAction& action) {
        bool matched = false;
        std::visit([&](const auto& payload) {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<T, StrategyActionWorkerEventAck>) {
                matched = payload.eventsSeen >= 1;
            }
        }, action);
        return matched;
    });
    EXPECT_TRUE(sawAck);

    runtime.stop();
}

TEST_CASE(test_out_of_process_runtime_restarts_worker_after_sigkill) {
    OutOfProcessStrategyRuntime runtime(
        static_cast<StrategyHandle>(19),
        "MockTestStrategy",
        "user-3",
        glz::raw_json{"{\"mode\":\"restart-test\"}"});
    runtime.setInstanceId("instance-outproc-3");
    runtime.start();

    REQUIRE_TRUE(waitUntilReady(runtime, [](const StrategyAction& action) {
        return std::holds_alternative<StrategyActionWorkerReady>(action);
    }));

    const int firstPid = runtime.workerPidForTest();
    REQUIRE_TRUE(firstPid > 0);
    REQUIRE_TRUE(::kill(firstPid, SIGKILL) == 0);

    const bool sawRestartedReady = waitUntilReady(runtime, [](const StrategyAction& action) {
        return std::holds_alternative<StrategyActionWorkerReady>(action);
    });
    EXPECT_TRUE(sawRestartedReady);
    EXPECT_TRUE(runtime.workerPidForTest() > 0);
    EXPECT_TRUE(runtime.workerPidForTest() != firstPid);

    runtime.onTopOfBook(42, 7);
    const bool sawAck = waitUntilReady(runtime, [](const StrategyAction& action) {
        bool matched = false;
        std::visit([&](const auto& payload) {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<T, StrategyActionWorkerEventAck>) {
                matched = payload.eventsSeen >= 1;
            }
        }, action);
        return matched;
    });
    EXPECT_TRUE(sawAck);

    runtime.stop();
}

TEST_CASE(test_out_of_process_runtime_restarts_worker_after_sigsegv) {
    OutOfProcessStrategyRuntime runtime(
        static_cast<StrategyHandle>(119),
        "MockTestStrategy",
        "user-3b",
        glz::raw_json{"{\"mode\":\"restart-segv-test\"}"});
    runtime.setInstanceId("instance-outproc-3b");
    runtime.start();

    REQUIRE_TRUE(waitUntilReady(runtime, [](const StrategyAction& action) {
        return std::holds_alternative<StrategyActionWorkerReady>(action);
    }));

    const int firstPid = runtime.workerPidForTest();
    REQUIRE_TRUE(firstPid > 0);
    REQUIRE_TRUE(::kill(firstPid, SIGSEGV) == 0);

    const bool sawRestartedReady = waitUntilReady(runtime, [](const StrategyAction& action) {
        return std::holds_alternative<StrategyActionWorkerReady>(action);
    });
    EXPECT_TRUE(sawRestartedReady);
    EXPECT_TRUE(runtime.workerPidForTest() > 0);
    EXPECT_TRUE(runtime.workerPidForTest() != firstPid);

    runtime.stop();
}

TEST_CASE(test_out_of_process_runtime_restarts_hung_worker_after_sigstop) {
    OutOfProcessStrategyRuntime runtime(
        static_cast<StrategyHandle>(120),
        "MockTestStrategy",
        "user-3c",
        glz::raw_json{"{\"mode\":\"restart-hang-test\"}"});
    runtime.setInstanceId("instance-outproc-3c");
    runtime.start();

    REQUIRE_TRUE(waitUntilReady(runtime, [](const StrategyAction& action) {
        return std::holds_alternative<StrategyActionWorkerReady>(action);
    }));

    const int firstPid = runtime.workerPidForTest();
    REQUIRE_TRUE(firstPid > 0);
    REQUIRE_TRUE(::kill(firstPid, SIGSTOP) == 0);

    const bool restarted = waitForWorkerRestart(runtime, firstPid);
    EXPECT_TRUE(restarted);

    const bool sawRestartedReady = waitUntilReady(runtime, [](const StrategyAction& action) {
        return std::holds_alternative<StrategyActionWorkerReady>(action);
    });
    EXPECT_TRUE(sawRestartedReady);
    EXPECT_TRUE(runtime.workerPidForTest() > 0);
    EXPECT_TRUE(runtime.workerPidForTest() != firstPid);

    runtime.stop();
}

TEST_CASE(test_out_of_process_runtime_schedules_restart_backoff_after_crash) {
    OutOfProcessStrategyRuntime runtime(
        static_cast<StrategyHandle>(121),
        "MockTestStrategy",
        "user-3d",
        glz::raw_json{"{\"mode\":\"restart-backoff-test\"}"});
    runtime.setInstanceId("instance-outproc-3d");
    runtime.start();

    REQUIRE_TRUE(waitUntilReady(runtime, [](const StrategyAction& action) {
        return std::holds_alternative<StrategyActionWorkerReady>(action);
    }));

    const int firstPid = runtime.workerPidForTest();
    REQUIRE_TRUE(firstPid > 0);
    REQUIRE_TRUE(runtime.restartDelayForTest() == std::chrono::milliseconds(100));
    REQUIRE_TRUE(::kill(firstPid, SIGKILL) == 0);

    runtime.pollHealth(std::chrono::steady_clock::now());
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    runtime.handleSupervisorReadable();

    EXPECT_TRUE(runtime.workerPidForTest() == -1 || runtime.workerPidForTest() != firstPid);
    EXPECT_TRUE(runtime.restartDelayForTest() >= std::chrono::milliseconds(200));

    runtime.stop();
}

TEST_CASE(test_out_of_process_runtime_executes_worker_strategy_actions) {
    OutOfProcessStrategyRuntime runtime(
        static_cast<StrategyHandle>(20),
        "MockTestStrategy",
        "user-4",
        glz::raw_json{makeWorkerStrategyConfigJson("instance-outproc-4")});
    runtime.setInstanceId("instance-outproc-4");
    runtime.start();

    REQUIRE_TRUE(waitUntilReady(runtime, [](const StrategyAction& action) {
        bool matched = false;
        std::visit([&](const auto& payload) {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<T, StrategyActionSubscribe>) {
                matched = payload.exchangeId == 7 && payload.instrumentId == 42;
            }
        }, action);
        return matched;
    }));

    runtime.onTopOfBook(42, 7);
    const bool sawPlace = waitUntilReady(runtime, [](const StrategyAction& action) {
        bool matched = false;
        std::visit([&](const auto& payload) {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<T, StrategyActionPlace>) {
                matched = payload.accountId == 7001 &&
                          payload.exchangeId == 7 &&
                          payload.entries.size() == 1 &&
                          payload.entries.front().instrumentId == 42 &&
                          !payload.entries.front().ownOrderId.empty();
            }
        }, action);
        return matched;
    });
    EXPECT_TRUE(sawPlace);

    runtime.stop();
}

TEST_CASE(test_strategy_engine_reactor_drains_runtime_actions_from_worker_fd) {
    StrategyEngineConfig config{};
    config.strategyExecutionMode = StrategyExecutionMode::OutOfProcess;
    config.pinTradingThread = false;
    config.pinMetricsThread = false;
    config.enableMetricsRecorder = false;
    config.mlockTradingMemory = false;

    auto engine = std::make_unique<StrategyEngine>(config);

    bool sawReady = false;
    bool sawAck = false;
    engine->setRuntimeActionHookForTest([&](StrategyHandle, const StrategyAction& action) {
        std::visit([&](const auto& payload) {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<T, StrategyActionWorkerReady>) {
                if (payload.instanceId == "engine-instance-1") {
                    sawReady = true;
                }
            } else if constexpr (std::is_same_v<T, StrategyActionWorkerEventAck>) {
                if (payload.eventsSeen >= 1) {
                    sawAck = true;
                }
            }
        }, action);
    });

    const auto createResult = engine->createStrategy(
        "MockTestStrategy",
        glz::raw_json{"{\"mode\":\"engine-test\"}"},
        "engine-instance-1",
        "engine-user-1");
    REQUIRE_TRUE(createResult.success);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!sawReady && std::chrono::steady_clock::now() < deadline) {
        engine->getReactor().runOnce(100);
    }
    EXPECT_TRUE(sawReady);

    engine->onTopOfBook(42, 7, createResult.handle);
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!sawAck && std::chrono::steady_clock::now() < deadline) {
        engine->getReactor().runOnce(100);
    }
    EXPECT_TRUE(sawAck);

    EXPECT_TRUE(engine->unregisterStrategy(createResult.handle));
}

TEST_CASE(test_strategy_engine_observes_worker_strategy_actions) {
    StrategyEngineConfig config{};
    config.strategyExecutionMode = StrategyExecutionMode::OutOfProcess;
    config.pinTradingThread = false;
    config.pinMetricsThread = false;
    config.enableMetricsRecorder = false;
    config.mlockTradingMemory = false;

    auto engine = std::make_unique<StrategyEngine>(config);

    bool sawSubscribe = false;
    bool sawPlace = false;
    engine->setRuntimeActionHookForTest([&](StrategyHandle, const StrategyAction& action) {
        std::visit([&](const auto& payload) {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<T, StrategyActionSubscribe>) {
                if (payload.exchangeId == 7 && payload.instrumentId == 42) {
                    sawSubscribe = true;
                }
            } else if constexpr (std::is_same_v<T, StrategyActionPlace>) {
                if (payload.accountId == 7001 &&
                    payload.exchangeId == 7 &&
                    payload.entries.size() == 1 &&
                    payload.entries.front().instrumentId == 42 &&
                    !payload.entries.front().ownOrderId.empty()) {
                    sawPlace = true;
                }
            }
        }, action);
    });

    const auto createResult = engine->createStrategy(
        "MockTestStrategy",
        glz::raw_json{makeWorkerStrategyConfigJson("engine-instance-3")},
        "engine-instance-3",
        "engine-user-3");
    REQUIRE_TRUE(createResult.success);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!sawSubscribe && std::chrono::steady_clock::now() < deadline) {
        engine->getReactor().runOnce(100);
    }
    EXPECT_TRUE(sawSubscribe);

    auto* runtime = dynamic_cast<OutOfProcessStrategyRuntime*>(engine->getRuntime(createResult.handle));
    REQUIRE_TRUE(runtime != nullptr);
    runtime->resumeAfterRecovery(7);

    engine->onTopOfBook(42, 7, createResult.handle);
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!sawPlace && std::chrono::steady_clock::now() < deadline) {
        engine->getReactor().runOnce(100);
    }
    EXPECT_TRUE(sawPlace);

    EXPECT_TRUE(engine->unregisterStrategy(createResult.handle));
}

TEST_CASE(test_strategy_engine_restarts_worker_after_sigkill) {
    StrategyEngineConfig config{};
    config.strategyExecutionMode = StrategyExecutionMode::OutOfProcess;
    config.pinTradingThread = false;
    config.pinMetricsThread = false;
    config.enableMetricsRecorder = false;
    config.mlockTradingMemory = false;

    auto engine = std::make_unique<StrategyEngine>(config);

    int readyCount = 0;
    bool sawAck = false;
    engine->setRuntimeActionHookForTest([&](StrategyHandle, const StrategyAction& action) {
        std::visit([&](const auto& payload) {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<T, StrategyActionWorkerReady>) {
                if (payload.instanceId == "engine-instance-2") {
                    ++readyCount;
                }
            } else if constexpr (std::is_same_v<T, StrategyActionWorkerEventAck>) {
                if (payload.eventsSeen >= 1) {
                    sawAck = true;
                }
            }
        }, action);
    });

    const auto createResult = engine->createStrategy(
        "MockTestStrategy",
        glz::raw_json{"{\"mode\":\"engine-restart-test\"}"},
        "engine-instance-2",
        "engine-user-2");
    REQUIRE_TRUE(createResult.success);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (readyCount < 1 && std::chrono::steady_clock::now() < deadline) {
        engine->getReactor().runOnce(100);
    }
    REQUIRE_TRUE(readyCount >= 1);

    auto* runtime = dynamic_cast<OutOfProcessStrategyRuntime*>(engine->getRuntime(createResult.handle));
    REQUIRE_TRUE(runtime != nullptr);
    const int firstPid = runtime->workerPidForTest();
    REQUIRE_TRUE(firstPid > 0);
    REQUIRE_TRUE(::kill(firstPid, SIGKILL) == 0);

    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (readyCount < 2 && std::chrono::steady_clock::now() < deadline) {
        engine->getReactor().runOnce(100);
        runtime->pollHealth(std::chrono::steady_clock::now());
    }
    EXPECT_TRUE(readyCount >= 2);

    engine->onBookUpdate(42, 7, createResult.handle);
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!sawAck && std::chrono::steady_clock::now() < deadline) {
        engine->getReactor().runOnce(100);
        runtime->pollHealth(std::chrono::steady_clock::now());
    }
    EXPECT_TRUE(sawAck);
    EXPECT_TRUE(runtime->workerPidForTest() > 0);
    EXPECT_TRUE(runtime->workerPidForTest() != firstPid);

    EXPECT_TRUE(engine->unregisterStrategy(createResult.handle));
}

int main() {
    test_out_of_process_runtime_launches_worker_and_receives_ready();
    test_out_of_process_runtime_roundtrips_event_ack_from_worker();
    test_out_of_process_runtime_restarts_worker_after_sigkill();
    test_out_of_process_runtime_restarts_worker_after_sigsegv();
    test_out_of_process_runtime_restarts_hung_worker_after_sigstop();
    test_out_of_process_runtime_schedules_restart_backoff_after_crash();
    test_out_of_process_runtime_executes_worker_strategy_actions();
    test_strategy_engine_reactor_drains_runtime_actions_from_worker_fd();
    test_strategy_engine_observes_worker_strategy_actions();
    test_strategy_engine_restarts_worker_after_sigkill();

    if (ctx.failed) {
        std::cerr << "\nTEST FAILURES: " << ctx.failed << " failed\n";
        std::cerr << ctx.os.str();
        return 1;
    }

    std::cout << "All OutOfProcessRuntime tests passed. Assertions: " << ctx.passed << "\n";
    return 0;
}
