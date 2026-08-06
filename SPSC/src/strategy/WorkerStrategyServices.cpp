/**
 * Implementation of the worker-side strategy services proxy.
 *
 * All write operations (place, cancel, subscribe, etc.) go through the
 * sendAction<T>() template which wraps the C++ type in a StrategyAction
 * variant and pushes it through the transport's actionRing to the engine.
 *
 * handleEvent() uses std::visit with if constexpr to dispatch incoming
 * events and update the local cache (connectedExchanges_, accountInfos_,
 * walletInfos_, etc.) before the strategy's callback is invoked.
 *
 * Market data read methods (getLatestTopOfBook, getLatestBook, etc.)
 * currently return std::nullopt. The strategy receives market data via
 * event callbacks instead. Local caching is planned for a future iteration.
 */
#include "strategy/WorkerStrategyServices.hpp"

#include "logging/Log.hpp"

#include <type_traits>
#include <utility>

namespace {

template <typename T>
std::optional<T> missingMarketData() {
    return std::nullopt;
}

} // namespace

WorkerStrategyServices::WorkerStrategyServices(StrategyHandle handle, IStrategyTransport& transport)
    : handle_(handle)
    , transport_(transport) {
    defaultOehCapabilities_.amend = true;
    defaultOehCapabilities_.replace = true;
    defaultOehCapabilities_.cancelAll = true;
    defaultOehCapabilities_.postOnly = true;
    defaultOehCapabilities_.transactions = true;
}

template <typename Action>
bool WorkerStrategyServices::sendAction(Action&& action) {
    if (transport_.sendAction(StrategyAction{std::forward<Action>(action)})) {
        return true;
    }
    IOUR_LOG_ERROR("WorkerStrategyServices failed to send action for strategy {}", static_cast<int>(handle_));
    return false;
}

void WorkerStrategyServices::handleEvent(const StrategyEvent& event) {
    std::visit([this](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, StrategyEventBookUpdate> ||
                      std::is_same_v<T, StrategyEventIndexPrice> ||
                      std::is_same_v<T, StrategyEventSnapshot> ||
                      std::is_same_v<T, StrategyEventTopOfBook> ||
                      std::is_same_v<T, StrategyEventMarkPrice> ||
                      std::is_same_v<T, StrategyEventInstrumentState> ||
                      std::is_same_v<T, StrategyEventFundingRate> ||
                      std::is_same_v<T, StrategyEventTrades>) {
            connectedExchanges_.insert(payload.exchangeId);
        } else if constexpr (std::is_same_v<T, StrategyEventAccountStatus> ||
                             std::is_same_v<T, StrategyEventLoginResponse>) {
            const auto key = makeAccountKey(payload.exchangeId, payload.accountId);
            accountInfos_[key] = payload.info;
            walletInfos_[key] = payload.info.walletInfo;
            rateLimitLoads_[key] = payload.info.initialRateLimitLoads;
            readyAccounts_.insert(key);
            connectedExchanges_.insert(payload.exchangeId);
        } else if constexpr (std::is_same_v<T, StrategyEventWalletUpdate>) {
            const auto key = makeAccountKey(payload.exchangeId, payload.accountId);
            walletInfos_[key] = payload.info;
            connectedExchanges_.insert(payload.exchangeId);
        } else if constexpr (std::is_same_v<T, StrategyEventPositionUpdate>) {
            const auto key = makeAccountKey(payload.exchangeId, payload.accountId);
            readyAccounts_.insert(key);
            connectedExchanges_.insert(payload.exchangeId);
        }
    }, event);
}

bool WorkerStrategyServices::subscribe(StrategyHandle strategyHandle,
                                       exchange_id_type exchangeId,
                                       instrument_id_type instrumentId,
                                       SubscribeOptions options) {
    (void)strategyHandle;
    const bool sent = sendAction(StrategyActionSubscribe{
        .exchangeId = exchangeId,
        .instrumentId = instrumentId,
        .options = std::move(options),
    });
    if (sent) {
        subscribedInstruments_[exchangeId].insert(instrumentId);
        connectedExchanges_.insert(exchangeId);
    }
    return sent;
}

bool WorkerStrategyServices::subscribeMarketDataOnly(StrategyHandle strategyHandle,
                                                     exchange_id_type exchangeId,
                                                     instrument_id_type instrumentId,
                                                     SubscribeOptions options) {
    (void)strategyHandle;
    const bool sent = sendAction(StrategyActionSubscribeMarketDataOnly{
        .exchangeId = exchangeId,
        .instrumentId = instrumentId,
        .options = std::move(options),
    });
    if (sent) {
        subscribedInstruments_[exchangeId].insert(instrumentId);
        connectedExchanges_.insert(exchangeId);
    }
    return sent;
}

void WorkerStrategyServices::unsubscribe(StrategyHandle strategyHandle,
                                         exchange_id_type exchangeId,
                                         instrument_id_type instrumentId) {
    (void)strategyHandle;
    if (sendAction(StrategyActionUnsubscribe{
            .exchangeId = exchangeId,
            .instrumentId = instrumentId,
        })) {
        if (auto it = subscribedInstruments_.find(exchangeId); it != subscribedInstruments_.end()) {
            it->second.erase(instrumentId);
        }
    }
}

void WorkerStrategyServices::place(AccountId accountId,
                                   StrategyHandle strategyHandle,
                                   exchange_id_type exchangeId,
                                   std::vector<OrderEntry> entries) {
    (void)strategyHandle;
    (void)sendAction(StrategyActionPlace{
        .accountId = accountId,
        .exchangeId = exchangeId,
        .entries = std::move(entries),
    });
}

void WorkerStrategyServices::cancel(AccountId accountId,
                                    StrategyHandle strategyHandle,
                                    exchange_id_type exchangeId,
                                    std::vector<CancelEntry> entries) {
    (void)strategyHandle;
    (void)sendAction(StrategyActionCancel{
        .accountId = accountId,
        .exchangeId = exchangeId,
        .entries = std::move(entries),
    });
}

void WorkerStrategyServices::amend(AccountId accountId,
                                   StrategyHandle strategyHandle,
                                   exchange_id_type exchangeId,
                                   std::vector<AmendEntry> entries) {
    (void)strategyHandle;
    (void)sendAction(StrategyActionAmend{
        .accountId = accountId,
        .exchangeId = exchangeId,
        .entries = std::move(entries),
    });
}

void WorkerStrategyServices::replace(AccountId accountId,
                                     StrategyHandle strategyHandle,
                                     exchange_id_type exchangeId,
                                     std::vector<ReplaceEntry> entries) {
    (void)strategyHandle;
    (void)sendAction(StrategyActionReplace{
        .accountId = accountId,
        .exchangeId = exchangeId,
        .entries = std::move(entries),
    });
}

void WorkerStrategyServices::setupExchange(StrategyHandle strategyHandle,
                                           exchange_id_type exchangeId,
                                           AccountId accountId,
                                           CSOEHConfig config) {
    (void)strategyHandle;
    if (sendAction(StrategyActionSetupExchange{
            .exchangeId = exchangeId,
            .accountId = accountId,
            .config = std::move(config),
        })) {
        readyAccounts_.insert(makeAccountKey(exchangeId, accountId));
        connectedExchanges_.insert(exchangeId);
    }
}

std::optional<SnapshotEvent> WorkerStrategyServices::getLatestSnapshot(instrument_id_type, exchange_id_type) const {
    return missingMarketData<SnapshotEvent>();
}

std::optional<BookUpdateEvent> WorkerStrategyServices::getLatestBook(instrument_id_type, exchange_id_type) const {
    return missingMarketData<BookUpdateEvent>();
}

std::optional<TopOfBookEvent> WorkerStrategyServices::getLatestTopOfBook(instrument_id_type, exchange_id_type) const {
    return missingMarketData<TopOfBookEvent>();
}

std::optional<IndexPriceEvent> WorkerStrategyServices::getLatestIndexPrice(instrument_id_type, exchange_id_type) const {
    return missingMarketData<IndexPriceEvent>();
}

std::optional<MarkPriceEvent> WorkerStrategyServices::getLatestMarkPrice(instrument_id_type, exchange_id_type) const {
    return missingMarketData<MarkPriceEvent>();
}

std::optional<InstrumentStateEvent> WorkerStrategyServices::getLatestInstrumentState(instrument_id_type, exchange_id_type) const {
    return missingMarketData<InstrumentStateEvent>();
}

std::optional<FundingRateEvent> WorkerStrategyServices::getLatestFundingRate(instrument_id_type, exchange_id_type) const {
    return missingMarketData<FundingRateEvent>();
}

std::optional<TradesEvent> WorkerStrategyServices::getLatestTrades(instrument_id_type, exchange_id_type) const {
    return missingMarketData<TradesEvent>();
}

const Capabilities* WorkerStrategyServices::getMdhCapabilities(exchange_id_type exchangeId) const {
    return connectedExchanges_.contains(exchangeId) ? &defaultMdhCapabilities_ : nullptr;
}

bool WorkerStrategyServices::isMdhReady(exchange_id_type exchangeId) const {
    return connectedExchanges_.contains(exchangeId);
}

bool WorkerStrategyServices::isMdhConnected(exchange_id_type exchangeId) const {
    return connectedExchanges_.contains(exchangeId);
}

const LoginCapabilities& WorkerStrategyServices::getOehCapabilities(exchange_id_type exchangeId,
                                                                    AccountId accountId) const {
    const auto key = makeAccountKey(exchangeId, accountId);
    if (auto it = accountInfos_.find(key); it != accountInfos_.end()) {
        return it->second.capabilities;
    }
    return defaultOehCapabilities_;
}

const WalletInfo* WorkerStrategyServices::getWalletInfo(exchange_id_type exchangeId,
                                                        AccountId accountId) const {
    const auto key = makeAccountKey(exchangeId, accountId);
    auto it = walletInfos_.find(key);
    return it == walletInfos_.end() ? nullptr : &it->second;
}

const AccountInfo* WorkerStrategyServices::getAccountInfo(exchange_id_type exchangeId,
                                                          AccountId accountId) const {
    const auto key = makeAccountKey(exchangeId, accountId);
    auto it = accountInfos_.find(key);
    return it == accountInfos_.end() ? nullptr : &it->second;
}

std::optional<WalletDataEntry> WorkerStrategyServices::getWalletBalance(exchange_id_type exchangeId,
                                                                        AccountId accountId,
                                                                        uint64_t underlyingId) const {
    if (const auto* walletInfo = getWalletInfo(exchangeId, accountId)) {
        auto it = walletInfo->wallets.find(underlyingId);
        if (it != walletInfo->wallets.end()) {
            return it->second;
        }
    }
    return std::nullopt;
}

const std::vector<WalletMaintenanceMargin>* WorkerStrategyServices::getMaintenanceMargin(exchange_id_type exchangeId,
                                                                                         AccountId accountId) const {
    if (const auto* walletInfo = getWalletInfo(exchangeId, accountId)) {
        return &walletInfo->maintenanceMargin;
    }
    return nullptr;
}

const std::vector<WalletTotalBalance>* WorkerStrategyServices::getTotalBalances(exchange_id_type exchangeId,
                                                                                AccountId accountId) const {
    if (const auto* walletInfo = getWalletInfo(exchangeId, accountId)) {
        return &walletInfo->totalBalances;
    }
    return nullptr;
}

const std::vector<RateLimitLoad>* WorkerStrategyServices::getRateLimitLoads(exchange_id_type exchangeId,
                                                                            AccountId accountId) const {
    const auto key = makeAccountKey(exchangeId, accountId);
    auto it = rateLimitLoads_.find(key);
    return it == rateLimitLoads_.end() ? nullptr : &it->second;
}

std::vector<instrument_id_type> WorkerStrategyServices::getSubscribedInstruments(exchange_id_type exchangeId) const {
    std::vector<instrument_id_type> instruments;
    auto it = subscribedInstruments_.find(exchangeId);
    if (it == subscribedInstruments_.end()) {
        return instruments;
    }
    instruments.reserve(it->second.size());
    for (const auto instrumentId : it->second) {
        instruments.push_back(instrumentId);
    }
    return instruments;
}

bool WorkerStrategyServices::isInstrumentSubscribed(exchange_id_type exchangeId,
                                                    instrument_id_type instrumentId) const {
    auto it = subscribedInstruments_.find(exchangeId);
    return it != subscribedInstruments_.end() && it->second.contains(instrumentId);
}

std::optional<std::string> WorkerStrategyServices::getBestBid(exchange_id_type, instrument_id_type) const {
    return std::nullopt;
}

std::optional<std::string> WorkerStrategyServices::getBestAsk(exchange_id_type, instrument_id_type) const {
    return std::nullopt;
}

bool WorkerStrategyServices::isExchangeReady(exchange_id_type exchangeId, AccountId accountId) const {
    return readyAccounts_.contains(makeAccountKey(exchangeId, accountId));
}

std::uint64_t WorkerStrategyServices::makeAccountKey(exchange_id_type exchangeId, AccountId accountId) {
    return (static_cast<std::uint64_t>(exchangeId) << 32U) | static_cast<std::uint64_t>(accountId);
}
