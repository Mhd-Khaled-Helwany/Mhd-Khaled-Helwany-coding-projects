/*
How to build and run these tests
--------------------------------

1) Configure the build directory:
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

2) Build just this test target (or build all):
    cmake --build build --target MetricsTests
    # or
    cmake --build build

3) Run the tests directly or via CTest:
    ./build/MetricsTests
    # or
    ctest --test-dir build -R outOfProcessMetricsTest -V
*/

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <vector>
#include <sys/mman.h>

#include "metrics/Recorder.hpp"
#include "logging/SystemLog.hpp"
#include "metrics/Spsc.hpp"
#include "engine/ShmSpscRing.hpp"

struct TestContext {
    int failed = 0;
    int passed = 0;
    std::ostringstream os;
} ctx;

#define TEST_CASE(name) void name()

#define REQUIRE_TRUE(cond)                                                                              \
    do {                                                                                                \
        if (!(cond)) {                                                                                  \
            ++ctx.failed;                                                                               \
            ctx.os << __FUNCTION__ << ": line " << __LINE__ << ": REQUIRE_TRUE(" #cond ") failed\n";    \
            return;                                                                                     \
        }                                                                                               \
    } while (0)

#define EXPECT_TRUE(cond)                                                                               \
    do {                                                                                                \
        if (cond) {                                                                                     \
            ++ctx.passed;                                                                               \
        } else {                                                                                        \
            ++ctx.failed;                                                                               \
            ctx.os << __FUNCTION__ << ": line " << __LINE__ << ": EXPECT_TRUE(" #cond ") failed\n";     \
        }                                                                                               \
    } while (0)

#define EXPECT_FALSE(cond) EXPECT_TRUE(!(cond))

template <class A, class B>
static void expect_eq_impl(const A& a, const B& b, const char* aExpr, const char* bExpr, const char* func, int line) {
    if (a == b) {
        ++ctx.passed;
    } else {
        ++ctx.failed;
        ctx.os << func << ": line " << line << ": EXPECT_EQ(" << aExpr << ", " << bExpr << ") failed: "
               << a << " != " << b << "\n";
    }
}

#define EXPECT_EQ(a, b) expect_eq_impl((a), (b), #a, #b, __FUNCTION__, __LINE__)

namespace {

struct RecordingPublisher final : IMetricsPublisher {
    void publish(const MetricsBatch& batch) override {
        std::lock_guard lock(mutex_);
        batches_.push_back(batch);
        cv_.notify_all();
    }

    bool waitForCount(size_t expected, std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        return cv_.wait_for(lock, timeout, [&] { return batches_.size() >= expected; });
    }

    std::vector<MetricsBatch> copyBatches() const {
        std::lock_guard lock(mutex_);
        return batches_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<MetricsBatch> batches_;
};

static RecorderConfig makeConfig(DropPolicy policy, uint32_t queueCapacity = 8) {
    RecorderConfig config{};
    config.queueCapacity = queueCapacity;
    config.dropPolicy = policy;
    config.flushInterval = std::chrono::microseconds(50);
    config.enableSelfMetricsReport = false;
    return config;
}

static void drainSystemLogQueue() {
    std::array<SystemLogEntry, 256> buffer;
    while (g_systemLogQueue.try_pop_bulk(buffer.data(), buffer.size()) > 0) {
    }
}

static std::filesystem::path makeTempSystemLogPath(const std::string& testName) {
    auto base = std::filesystem::temp_directory_path() / "iour_metrics_tests";
    auto dir = base / (testName + "_" + std::to_string(std::random_device{}()));
    std::error_code ec;
    std::filesystem::create_directories(dir.parent_path(), ec);
    return dir / "system-events.jsonl";
}

static std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream in(path);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

static void cleanupTempPath(const std::filesystem::path& path) {
    if (path.empty()) {
        return;
    }
    std::error_code ec;
    std::filesystem::remove_all(path.parent_path(), ec);
}

static OrderEntry makeOrderEntry(BatchSequence seq) {
    OrderEntry entry{};
    entry.ownOrderId = "order_" + std::to_string(seq);
    entry.clientOrderId = entry.ownOrderId;
    entry.instrumentId = 1;
    entry.orderType = OrderType::LIMIT;
    entry.orderSide = OrderSide::BID;
    entry.timeInForce = TimeInForce::GOOD_TILL_CANCEL;
    entry.price = "100.0";
    entry.totalQuantity = "1";
    entry.postOnly = true;
    entry.closeOnly = false;
    entry.reduceOnly = false;
    entry.autoBorrow = false;
    entry.autoRepay = false;
    entry.useSpare = false;
    entry.walletType = WalletType::ACCOUNT;
    entry.owner.instanceId = "tests";
    entry.owner.instanceName = "tests";
    entry.owner.processId = "tests";
    return entry;
}

static MetricEvent makeDecisionEvent(StrategyId strategyId, BatchSequence seq) {
    MetricEvent event{};
    event.monotonicTimestampNs = 1'000 + seq;
    event.unixTimestampNs = 2'000 + seq;
    event.strategyId = strategyId;
    event.exchangeId = 7;
    event.accountId = 11;
    event.sequence = seq;
    event.kind = MetricKind::Decision;
    event.payload = makeOrderEntry(seq);
    event.reason = "test_metric";
    return event;
}

struct ShmCleaner {
    const char* name;
    explicit ShmCleaner(const char* n) : name(n) {
        shm_unlink(name); // Rensa ifall minnet fanns kvar från en tidigare krasch
    }
    ~ShmCleaner() {
        shm_unlink(name); // Rensa automatiskt när testet går ur scope
    }
};

} // namespace

/**
 * Test: register_strategy_returns_handle
 * - Arrange: Create recorder with drop-newest policy.
 * - Act: Register strategy and inspect handle.
 * - Assert: Handle is non-null and holds the requested strategy id.
 */
TEST_CASE(test_register_strategy_returns_handle) {
    ShmCleaner cleaner("ogaboga");

    RecordingPublisher publisher;
    RecorderConfig config = makeConfig(DropPolicy::DropNewest);
    MetricsRecorder recorder(config, publisher);

    auto queue = ShmSpscRing::create("ogaboga", 16, 4096);
    REQUIRE_TRUE(queue.has_value());

    auto handle = recorder.registerStrategy(std::move(queue.value()), 1234);
    REQUIRE_TRUE(handle != nullptr);
    EXPECT_EQ(handle->getStrategyId(), static_cast<StrategyId>(1234));

    recorder.unregisterStrategy(handle);

}

/**
 * Test: drop_newest_rejects_when_full
 * - Arrange: Recorder with small queue and DropNewest policy.
 * - Act: Fill queue to capacity and push one extra event.
 * - Assert: Extra push fails while initial pushes succeed.
 */
TEST_CASE(test_drop_newest_rejects_when_full) {
    ShmCleaner cleaner("ogaboga");

    RecordingPublisher publisher;
    RecorderConfig config = makeConfig(DropPolicy::DropNewest, 4);
    MetricsRecorder recorder(config, publisher);
    auto queue = ShmSpscRing::create("ogaboga", config.queueCapacity, 4096);

    EXPECT_TRUE(config.queueCapacity == 4);
    auto handle = recorder.registerStrategy(std::move(queue.value()), 1);
    REQUIRE_TRUE(handle != nullptr);

    const size_t maxEvents = config.queueCapacity - 1;
    for (size_t i = 0; i < maxEvents; ++i) {
        EXPECT_TRUE(handle->tryEmitEvent(makeDecisionEvent(handle->getStrategyId(), handle->nextSequence())));
    }

    auto extra = makeDecisionEvent(handle->getStrategyId(), handle->nextSequence());
    EXPECT_FALSE(handle->tryEmitEvent(extra));

    recorder.unregisterStrategy(handle);
}

/**
 * Test: drop_oldest_accepts_extra_event
 * - Arrange: Recorder with DropOldest policy and tiny queue.
 * - Act: Push more events than fit.
 * - Assert: All pushes succeed, demonstrating overwrite behavior.
 */
TEST_CASE(test_drop_oldest_accepts_extra_event) {
    ShmCleaner cleaner("ogaboga");

    RecordingPublisher publisher;
    RecorderConfig config = makeConfig(DropPolicy::DropOldest, 4);
    MetricsRecorder recorder(config, publisher);
    auto queue = ShmSpscRing::create("ogaboga", 16, 4096);
    auto handle = recorder.registerStrategy(std::move(queue.value()), 2);
    REQUIRE_TRUE(handle != nullptr);

    const size_t pushes = (config.queueCapacity - 1) + 3;
    for (size_t i = 0; i < pushes; ++i) {
        EXPECT_TRUE(handle->tryEmitEvent(makeDecisionEvent(handle->getStrategyId(), handle->nextSequence())));
    }

    recorder.unregisterStrategy(handle);
}

/**
 * Test: next_sequence_is_monotonic
 * - Arrange: Registered strategy handle.
 * - Act: Call nextSequence several times.
 * - Assert: Returned values increment by one.
 */
TEST_CASE(test_next_sequence_is_monotonic) {
    ShmCleaner cleaner("ogaboga");

    RecordingPublisher publisher;
    RecorderConfig config = makeConfig(DropPolicy::DropNewest);
    MetricsRecorder recorder(config, publisher);
    auto queue = ShmSpscRing::create("ogaboga", 16, 4096);
    auto handle = recorder.registerStrategy(std::move(queue.value()), 3);
    REQUIRE_TRUE(handle != nullptr);

    auto first = handle->nextSequence();
    auto second = handle->nextSequence();
    auto third = handle->nextSequence();

    EXPECT_EQ(second, first + 1);
    EXPECT_EQ(third, second + 1);

    recorder.unregisterStrategy(handle);
}

/**
 * Test: recorder_publishes_events
 * - Arrange: Recorder with running worker and pending events.
 * - Act: Start recorder, emit a few events, wait for publisher callback.
 * - Assert: Publisher receives batch with matching strategy id and event count.
 */
TEST_CASE(test_recorder_publishes_events) {
    ShmCleaner cleaner("ogaboga");

    RecordingPublisher publisher;
    RecorderConfig config = makeConfig(DropPolicy::DropNewest);
    MetricsRecorder recorder(config, publisher);
    auto queue = ShmSpscRing::create("ogaboga", 16, 4096);

    auto handle = recorder.registerStrategy(std::move(queue.value()), 42);
    REQUIRE_TRUE(handle != nullptr);

    constexpr size_t kEventCount = 3;
    for (size_t i = 0; i < kEventCount; ++i) {
        EXPECT_TRUE(handle->tryEmitEvent(makeDecisionEvent(handle->getStrategyId(), handle->nextSequence())));
    }

    recorder.start();
    bool gotBatch = publisher.waitForCount(1, std::chrono::milliseconds(500));
    recorder.stop();

    REQUIRE_TRUE(gotBatch);

    auto batches = publisher.copyBatches();
    REQUIRE_TRUE(!batches.empty());
    const auto& batch = batches.front();
    EXPECT_EQ(batch.strategyId, handle->getStrategyId());
    EXPECT_EQ(batch.events.size(), kEventCount);
    EXPECT_EQ(batch.events.front().sequence, static_cast<BatchSequence>(0));
    EXPECT_EQ(batch.events.back().sequence, static_cast<BatchSequence>(kEventCount - 1));

    recorder.unregisterStrategy(handle);
}

TEST_CASE(test_system_logs_dump_to_local_file) {
    RecordingPublisher publisher;
    RecorderConfig config = makeConfig(DropPolicy::DropNewest);
    config.enableLocalSystemLogDump = true;
    const auto targetPath = makeTempSystemLogPath("system_log_dump");
    cleanupTempPath(targetPath);
    config.localSystemLogPath = targetPath;

    MetricsRecorder recorder(config, publisher);
    drainSystemLogQueue();

    recorder.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_TRUE(isSystemLogEnabled());

    SystemLogEntry entry{};
    entry.monotonicTimestampNs = 987654321ULL;
    entry.eventType = SystemEventType::EXTERNAL_DEPENDENCY_DOWN;
    entry.severity = AlertSeverity::SEV1;
    entry.component = ComponentType::INFRA;
    std::snprintf(entry.message, sizeof(entry.message), "Central unavailable");
    entry.hasErrorCode = false;
    entry.context.emplace("host", "recorder-tests");

    EXPECT_TRUE(g_systemLogQueue.try_push(entry));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    recorder.stop();

    EXPECT_FALSE(isSystemLogEnabled());
    EXPECT_TRUE(std::filesystem::exists(targetPath));
    const auto content = readTextFile(targetPath);
    EXPECT_TRUE(content.find("\"event_type\":\"EXTERNAL_DEPENDENCY_DOWN\"") != std::string::npos);
    EXPECT_TRUE(content.find("\"host\":\"recorder-tests\"") != std::string::npos);

    drainSystemLogQueue();
    cleanupTempPath(targetPath);
}

TEST_CASE(test_system_logs_remain_disabled_without_sinks) {
    RecordingPublisher publisher;
    RecorderConfig config = makeConfig(DropPolicy::DropNewest);
    config.enableLocalSystemLogDump = false;
    config.localSystemLogPath.clear();

    MetricsRecorder recorder(config, publisher);
    drainSystemLogQueue();

    recorder.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_FALSE(isSystemLogEnabled());

    SystemLogEntry entry{};
    entry.monotonicTimestampNs = 1;
    entry.eventType = SystemEventType::PROCESS_RESTART;
    entry.severity = AlertSeverity::LOW;
    entry.component = ComponentType::METRICS_RECORDER;
    std::snprintf(entry.message, sizeof(entry.message), "noop");

    EXPECT_TRUE(g_systemLogQueue.try_push(entry));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(g_systemLogQueue.size(), static_cast<size_t>(1));

    recorder.stop();
    drainSystemLogQueue();
}

TEST_CASE(test_system_logs_stay_disabled_when_path_empty) {
    RecordingPublisher publisher;
    RecorderConfig config = makeConfig(DropPolicy::DropNewest);
    config.enableLocalSystemLogDump = true;
    config.localSystemLogPath.clear();

    MetricsRecorder recorder(config, publisher);
    drainSystemLogQueue();

    recorder.start();
    EXPECT_FALSE(isSystemLogEnabled());

    SystemLogEntry entry{};
    entry.monotonicTimestampNs = 3;
    entry.eventType = SystemEventType::HOTFIX_APPLIED;
    entry.severity = AlertSeverity::LOW;
    entry.component = ComponentType::STRATEGY;
    std::snprintf(entry.message, sizeof(entry.message), "not persisted");
    EXPECT_TRUE(g_systemLogQueue.try_push(entry));

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(g_systemLogQueue.size(), static_cast<size_t>(1));

    recorder.stop();
    drainSystemLogQueue();
}

int main() {
    test_register_strategy_returns_handle();
    test_drop_newest_rejects_when_full();
    test_drop_oldest_accepts_extra_event();
    test_next_sequence_is_monotonic();
    test_recorder_publishes_events();
    test_system_logs_dump_to_local_file();
    test_system_logs_remain_disabled_without_sinks();
    test_system_logs_stay_disabled_when_path_empty();

    if (ctx.failed) {
        std::cerr << "\nTEST FAILURES: " << ctx.failed << " failed, " << ctx.passed << " passed\n";
        std::cerr << ctx.os.str();
        return 1;
    }
    std::cout << "All Metrics tests passed. Assertions: " << ctx.passed << "\n";
    return 0;
}
