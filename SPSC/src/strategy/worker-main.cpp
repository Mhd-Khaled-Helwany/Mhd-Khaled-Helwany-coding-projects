/**
 * Entry point for the strategy worker process (compiled as "strategy-worker").
 *
 * This is the binary that OutOfProcessStrategyRuntime launches via fork()+execl().
 * It receives CLI arguments (ring names, eventfd numbers, strategy config, etc.),
 * opens the existing SHM rings created by the engine, instantiates the strategy
 * via the factory, and runs the event loop:
 *
 *   1. Parse CLI args (--events-ring, --actions-ring, --worker-wake-fd, etc.)
 *   2. Open SHM transport with createRings=false (rings already exist)
 *   3. Create strategy instance via StrategyFactory
 *   4. Call strategy->start()
 *   5. Send WorkerReady back to engine
 *   6. Event loop: poll(workerWakeFd) -> read events -> dispatch to strategy -> ack + heartbeat
 */
#include "engine/ShmStrategyTransport.hpp"
#include "metrics/Recorder.hpp"
#include "strategy/MockTestStrategy.hpp"
#include "strategy/WorkerStrategyServices.hpp"

#include <cerrno>
#include <csignal>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <poll.h>
#include <string>
#include <string_view>
#include <type_traits>

namespace {

volatile std::sig_atomic_t g_keepRunning = 1;

void workerSignalHandler(int) {
    g_keepRunning = 0;
}

struct WorkerOptions {
    std::string eventsRingName;
    std::string actionsRingName;
    std::string metricsShmName;
    std::string instanceId;
    std::string strategyName;
    std::string userId;
    std::string configJson;
    StrategyHandle handle{urexchange::config::INVALID_STRATEGY_HANDLE};
    int workerWakeFd{-1};
    int engineWakeFd{-1};
};

bool parseInt(std::string_view text, int& out) {
    try {
        out = std::stoi(std::string(text));
        return true;
    } catch (...) {
        return false;
    }
}

bool parseHandle(std::string_view text, StrategyHandle& out) {
    try {
        const auto value = std::stoul(std::string(text));
        out = static_cast<StrategyHandle>(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseWorkerOptions(int argc, char* argv[], WorkerOptions& out) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--events-ring" && i + 1 < argc) {
            out.eventsRingName = argv[++i];
        } else if (arg == "--actions-ring" && i + 1 < argc) {
            out.actionsRingName = argv[++i];
        } else if (arg == "--metrics-shm" && i + 1 < argc) {
            out.metricsShmName = argv[++i];
        } else if (arg == "--instance-id" && i + 1 < argc) {
            out.instanceId = argv[++i];
        } else if (arg == "--strategy-name" && i + 1 < argc) {
            out.strategyName = argv[++i];
        } else if (arg == "--user-id" && i + 1 < argc) {
            out.userId = argv[++i];
        } else if (arg == "--config-json" && i + 1 < argc) {
            out.configJson = argv[++i];
        } else if (arg == "--handle" && i + 1 < argc) {
            if (!parseHandle(argv[++i], out.handle)) {
                return false;
            }
        } else if (arg == "--worker-wake-fd" && i + 1 < argc) {
            if (!parseInt(argv[++i], out.workerWakeFd)) {
                return false;
            }
        } else if (arg == "--engine-wake-fd" && i + 1 < argc) {
            if (!parseInt(argv[++i], out.engineWakeFd)) {
                return false;
            }
        } else {
            return false;
        }
    }

    return !out.eventsRingName.empty() &&
           !out.actionsRingName.empty() &&
           !out.instanceId.empty() &&
           !out.strategyName.empty() &&
           !out.userId.empty() &&
           !out.configJson.empty() &&
           out.handle != urexchange::config::INVALID_STRATEGY_HANDLE &&
           out.workerWakeFd != -1 &&
           out.engineWakeFd != -1;
}

/**
 * @brief Creates the strategy instance from the factory.
 * 
 * Checks for MockTestStrategy first (used in tests), then falls back to the
 * registered factory system.
 * 
 * @param opts Configuration options for the worker.
 * @param services The services proxy to inject into the strategy.
 * @return A unique pointer to the instantiated strategy.
 */
std::unique_ptr<Strategy> makeWorkerStrategy(const WorkerOptions& opts, IStrategyServices& services) {
    try {
        if (opts.strategyName == "MockTestStrategy") {
            return std::make_unique<MockTestStrategy>(opts.handle, services, glz::raw_json{opts.configJson});
        }

        if (auto strategy = Strategy::instantiateRegisteredServiceStrategy(
                opts.userId,
                opts.strategyName,
                opts.handle,
                services,
                glz::raw_json{opts.configJson})) {
            return strategy;
        }
    } catch (const std::exception& ex) {
        std::cerr << "strategy-worker: failed to construct strategy " << opts.strategyName
                  << ": " << ex.what() << "\n";
        return nullptr;
    }

    std::cerr << "strategy-worker: no worker-capable factory registered for strategy "
              << opts.strategyName << " (user " << opts.userId << ")\n";
    return nullptr;
}

/**
 * @brief Handles an incoming event from the engine.
 * 
 * First updates WorkerStrategyServices' local cache via handleEvent(), then
 * dispatches to the strategy's actual callback (onBookUpdate, onPlaceResponse,
 * etc.) using std::visit + if constexpr on the event variant.
 * 
 * @param strategy The active strategy instance.
 * @param services The worker's services proxy.
 * @param event The event payload received from the transport.
 */
void dispatchStrategyEvent(Strategy& strategy,
                           WorkerStrategyServices& services,
                           const StrategyEvent& event) {
    services.handleEvent(event);
    std::visit([&](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, StrategyEventBookUpdate>) {
            strategy.onBookUpdate(payload.instrumentId, payload.exchangeId);
        } else if constexpr (std::is_same_v<T, StrategyEventIndexPrice>) {
            strategy.onIndexPrice(payload.instrumentId, payload.exchangeId);
        } else if constexpr (std::is_same_v<T, StrategyEventSnapshot>) {
            strategy.onSnapshot(payload.instrumentId, payload.exchangeId);
        } else if constexpr (std::is_same_v<T, StrategyEventTopOfBook>) {
            strategy.onTopOfBook(payload.instrumentId, payload.exchangeId);
        } else if constexpr (std::is_same_v<T, StrategyEventMarkPrice>) {
            strategy.onMarkPrice(payload.instrumentId, payload.exchangeId);
        } else if constexpr (std::is_same_v<T, StrategyEventInstrumentState>) {
            strategy.onInstrumentState(payload.instrumentId, payload.exchangeId);
        } else if constexpr (std::is_same_v<T, StrategyEventFundingRate>) {
            strategy.onFundingRate(payload.instrumentId, payload.exchangeId);
        } else if constexpr (std::is_same_v<T, StrategyEventTrades>) {
            strategy.onTrades(payload.instrumentId, payload.exchangeId);
        } else if constexpr (std::is_same_v<T, StrategyEventReplaceResponse>) {
            strategy.onReplaceResponse(payload.entry);
        } else if constexpr (std::is_same_v<T, StrategyEventCancelResponse>) {
            strategy.onCancelResponse(payload.entry);
        } else if constexpr (std::is_same_v<T, StrategyEventAmendResponse>) {
            strategy.onAmendResponse(payload.entry);
        } else if constexpr (std::is_same_v<T, StrategyEventPlaceResponse>) {
            strategy.onPlaceResponse(payload.entry);
        } else if constexpr (std::is_same_v<T, StrategyEventOrderUpdate>) {
            strategy.onOrderResponse(payload.entry);
        } else if constexpr (std::is_same_v<T, StrategyEventFill>) {
            strategy.onFillEvent(payload.entry);
        } else if constexpr (std::is_same_v<T, StrategyEventCancelAllResponse>) {
            strategy.onCancelAllResponse(payload.accountId, payload.exchangeId, payload.event);
        } else if constexpr (std::is_same_v<T, StrategyEventAccountStatus>) {
            strategy.onAccountStatus(payload.accountId, payload.exchangeId, payload.info);
        } else if constexpr (std::is_same_v<T, StrategyEventWalletUpdate>) {
            strategy.onWalletUpdate(payload.accountId, payload.exchangeId, payload.info);
        } else if constexpr (std::is_same_v<T, StrategyEventPositionUpdate>) {
            strategy.onPositionUpdate(payload.accountId, payload.exchangeId, payload.entry);
        } else if constexpr (std::is_same_v<T, StrategyEventLoginResponse>) {
            strategy.onLoginResponse(payload.accountId, payload.exchangeId, payload.info);
        }
    }, event);
}

/**
 * @brief Main worker initialization and event loop.
 * 
 * 1. Opens the existing SHM transport rings.
 * 2. Creates the strategy and calls start().
 * 3. Sends WorkerReady to the engine to trigger replay.
 * 4. Loops: poll(workerWakeFd) -> drain events -> dispatch -> send ack + periodic heartbeat.
 * 
 * @param opts Parsed command-line options.
 * @return Exit system status code.
 */
int runWorker(const WorkerOptions& opts) {
    std::signal(SIGINT, workerSignalHandler);
    std::signal(SIGTERM, workerSignalHandler);

    auto transport = ShmStrategyTransport::make({
        .role = ShmStrategyTransportRole::Worker,
        .eventsRingName = opts.eventsRingName,
        .actionsRingName = opts.actionsRingName,
        .ringRequestedCapacity = 0,
        .maxMessageBytes = 0,
        .localWakeFd = opts.workerWakeFd,
        .peerWakeFd = opts.engineWakeFd,
        .createRings = false,
        .unlinkOnDestroy = false,
    });
    if (!transport) {
        std::cerr << "strategy-worker: failed to open transport: " << transport.error() << "\n";
        return 2;
    }

    WorkerStrategyServices services(opts.handle, **transport);
    auto strategy = makeWorkerStrategy(opts, services);

    if (strategy) {
        strategy->setInstanceId(opts.instanceId);

        if (!opts.metricsShmName.empty()) {
            auto metricsRingResult = ShmSpscRing::open(opts.metricsShmName);
            if (metricsRingResult.has_value()) {
                auto metricsHandle = MetricsHandle::create(
                    std::move(metricsRingResult.value()),
                    static_cast<StrategyId>(opts.handle),
                    DropPolicy::DropOldest
                ); // TODO gör så att droppolicy läses in av i configJson, vilket vi tror den redan gör
                strategy->setMetricsHandle(metricsHandle);
            } else {
                // Engine did not create a metrics ring (recorder disabled). Continue
                // without a metrics handle; strategy code null-checks before use.
                std::cerr << "strategy-worker: metrics shm ring '"
                          << opts.metricsShmName << "' unavailable, continuing without metrics: "
                          << metricsRingResult.error() << "\n";
            }
        }


        try {
            strategy->start();
        } catch (const std::exception& ex) {
            std::cerr << "strategy-worker: strategy start failed: " << ex.what() << "\n";
            return 6;
        }
    }

    if (!(*transport)->sendAction(StrategyActionWorkerReady{
            .handle = opts.handle,
            .instanceId = opts.instanceId,
            .strategyName = opts.strategyName,
        })) {
        std::cerr << "strategy-worker: failed to send ready action\n";
        return 3;
    }

    std::uint64_t eventsSeen = 0;
    std::uint64_t heartbeatSeq = 0;
    auto lastHeartbeatSent = std::chrono::steady_clock::now();
    constexpr auto kHeartbeatInterval = std::chrono::milliseconds(250);
    while (g_keepRunning) {
        struct pollfd pfd {
            (*transport)->wakeupFd(), POLLIN, 0
        };

        const int rc = poll(&pfd, 1, 100);
        if (rc == -1) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "strategy-worker: poll failed\n";
            return 4;
        }
        if (rc > 0 && (pfd.revents & POLLIN)) {
            (*transport)->drainWakeups();
            StrategyEvent event;
            while ((*transport)->tryRecvEvent(event)) {
                if (strategy) {
                    dispatchStrategyEvent(*strategy, services, event);
                }
                ++eventsSeen;
                if (!(*transport)->sendAction(StrategyActionWorkerEventAck{eventsSeen})) {
                    std::cerr << "strategy-worker: failed to send event ack\n";
                    return 5;
                }
            }
        }

        const auto now = std::chrono::steady_clock::now();
        if (now - lastHeartbeatSent >= kHeartbeatInterval) {
            if (!(*transport)->sendAction(StrategyActionWorkerHeartbeat{++heartbeatSeq})) {
                std::cerr << "strategy-worker: failed to send heartbeat\n";
                return 7;
            }
            lastHeartbeatSent = now;
        }
    }

    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    WorkerOptions options;
    if (!parseWorkerOptions(argc, argv, options)) {
        std::cerr << "Usage: strategy-worker"
                  << " --events-ring <name>"
                  << " --actions-ring <name>"
                  << " --instance-id <id>"
                  << " --strategy-name <name>"
                  << " --user-id <id>"
                  << " --config-json <json>"
                  << " --handle <handle>"
                  << " --worker-wake-fd <fd>"
                  << " --engine-wake-fd <fd>\n";
        return 1;
    }

    return runWorker(options);
}
