/**
 * Worker-side implementation of IStrategyServices.
 *
 * In the in-process model, strategies call ApiManager directly. In the
 * out-of-process model, there is no ApiManager in the worker process.
 * This class bridges the gap by intercepting all service calls (place,
 * cancel, subscribe, etc.) and serializing them as StrategyActions sent
 * back to the engine via the IPC transport (actionRing).
 *
 * Write operations: forwarded to engine via sendAction<T>().
 * Read operations (getLatestBook, etc.): return from local cache or nullopt.
 * handleEvent() is called by worker-main before dispatching to the strategy,
 * updating the local cache (account info, wallet info, subscriptions).
 */
#pragma once

#include "engine/StrategyRuntimeMessages.hpp"
#include "engine/StrategyTransport.hpp"
#include "strategy/StrategyServices.hpp"

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

class WorkerStrategyServices final : public IStrategyServices {
public:
    WorkerStrategyServices(StrategyHandle handle, IStrategyTransport& transport);

    /**
     * @brief Called by worker-main before dispatching to the strategy's callback.
     * 
     * Updates the local caches (connectedExchanges, accountInfos, walletInfos,
     * rateLimitLoads, readyAccounts) based on the event type so that the
     * strategy's synchronous read methods (isExchangeReady, getAccountInfo, etc.) work.
     * 
     * @param event The received event to process.
     */
    void handleEvent(const StrategyEvent& event);

    bool subscribe(StrategyHandle strategyHandle,
                   exchange_id_type exchangeId,
                   instrument_id_type instrumentId,
                   SubscribeOptions options) override;
    bool subscribeMarketDataOnly(StrategyHandle strategyHandle,
                                 exchange_id_type exchangeId,
                                 instrument_id_type instrumentId,
                                 SubscribeOptions options) override;
    void unsubscribe(StrategyHandle strategyHandle,
                     exchange_id_type exchangeId,
                     instrument_id_type instrumentId) override;

    void place(AccountId accountId,
               StrategyHandle strategyHandle,
               exchange_id_type exchangeId,
               std::vector<OrderEntry> entries) override;
    void cancel(AccountId accountId,
                StrategyHandle strategyHandle,
                exchange_id_type exchangeId,
                std::vector<CancelEntry> entries) override;
    void amend(AccountId accountId,
               StrategyHandle strategyHandle,
               exchange_id_type exchangeId,
               std::vector<AmendEntry> entries) override;
    void replace(AccountId accountId,
                 StrategyHandle strategyHandle,
                 exchange_id_type exchangeId,
                 std::vector<ReplaceEntry> entries) override;

    void setupExchange(StrategyHandle strategyHandle,
                       exchange_id_type exchangeId,
                       AccountId accountId,
                       CSOEHConfig config) override;

    std::optional<SnapshotEvent> getLatestSnapshot(instrument_id_type instrumentId,
                                                   exchange_id_type exchangeId) const override;
    std::optional<BookUpdateEvent> getLatestBook(instrument_id_type instrumentId,
                                                 exchange_id_type exchangeId) const override;
    std::optional<TopOfBookEvent> getLatestTopOfBook(instrument_id_type instrumentId,
                                                     exchange_id_type exchangeId) const override;
    std::optional<IndexPriceEvent> getLatestIndexPrice(instrument_id_type instrumentId,
                                                       exchange_id_type exchangeId) const override;
    std::optional<MarkPriceEvent> getLatestMarkPrice(instrument_id_type instrumentId,
                                                     exchange_id_type exchangeId) const override;
    std::optional<InstrumentStateEvent> getLatestInstrumentState(instrument_id_type instrumentId,
                                                                 exchange_id_type exchangeId) const override;
    std::optional<FundingRateEvent> getLatestFundingRate(instrument_id_type instrumentId,
                                                         exchange_id_type exchangeId) const override;
    std::optional<TradesEvent> getLatestTrades(instrument_id_type instrumentId,
                                               exchange_id_type exchangeId) const override;

    const Capabilities* getMdhCapabilities(exchange_id_type exchangeId) const override;
    bool isMdhReady(exchange_id_type exchangeId) const override;
    bool isMdhConnected(exchange_id_type exchangeId) const override;
    const LoginCapabilities& getOehCapabilities(exchange_id_type exchangeId,
                                                AccountId accountId) const override;

    const WalletInfo* getWalletInfo(exchange_id_type exchangeId,
                                    AccountId accountId) const override;
    const AccountInfo* getAccountInfo(exchange_id_type exchangeId,
                                      AccountId accountId) const override;
    std::optional<WalletDataEntry> getWalletBalance(exchange_id_type exchangeId,
                                                    AccountId accountId,
                                                    uint64_t underlyingId) const override;
    const std::vector<WalletMaintenanceMargin>* getMaintenanceMargin(exchange_id_type exchangeId,
                                                                     AccountId accountId) const override;
    const std::vector<WalletTotalBalance>* getTotalBalances(exchange_id_type exchangeId,
                                                            AccountId accountId) const override;
    const std::vector<RateLimitLoad>* getRateLimitLoads(exchange_id_type exchangeId,
                                                        AccountId accountId) const override;

    std::vector<instrument_id_type> getSubscribedInstruments(exchange_id_type exchangeId) const override;
    bool isInstrumentSubscribed(exchange_id_type exchangeId,
                                instrument_id_type instrumentId) const override;
    std::optional<std::string> getBestBid(exchange_id_type exchangeId,
                                          instrument_id_type instrumentId) const override;
    std::optional<std::string> getBestAsk(exchange_id_type exchangeId,
                                          instrument_id_type instrumentId) const override;
    bool isExchangeReady(exchange_id_type exchangeId, AccountId accountId) const override;

private:
    static std::uint64_t makeAccountKey(exchange_id_type exchangeId, AccountId accountId);

    /**
     * @brief Generic helper used by all write methods (place, cancel, subscribe, etc.).
     * 
     * Wraps the concrete action type in the StrategyAction variant and sends it
     * through the transport's actionRing to the engine process.
     * 
     * @tparam Action The concrete action type.
     * @param action The action payload to send.
     * @return true if successfully queued to transport, false otherwise.
     */
    template <typename Action>
    bool sendAction(Action&& action);

    StrategyHandle handle_;
    IStrategyTransport& transport_;

    LoginCapabilities defaultOehCapabilities_{};
    Capabilities defaultMdhCapabilities_{};
    absl::flat_hash_set<exchange_id_type> connectedExchanges_{};
    absl::flat_hash_set<std::uint64_t> readyAccounts_{};
    absl::flat_hash_map<exchange_id_type, absl::flat_hash_set<instrument_id_type>> subscribedInstruments_{};
    absl::flat_hash_map<std::uint64_t, AccountInfo> accountInfos_{};
    absl::flat_hash_map<std::uint64_t, WalletInfo> walletInfos_{};
    absl::flat_hash_map<std::uint64_t, std::vector<RateLimitLoad>> rateLimitLoads_{};
};
