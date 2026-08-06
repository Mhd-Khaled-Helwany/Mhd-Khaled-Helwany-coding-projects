#pragma once

#include "exchanges/cryptostruct/ApiManager.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

class IStrategyServices {
public:
    virtual ~IStrategyServices() = default;

    virtual bool subscribe(StrategyHandle strategyHandle,
                           exchange_id_type exchangeId,
                           instrument_id_type instrumentId,
                           SubscribeOptions options) = 0;
    virtual bool subscribeMarketDataOnly(StrategyHandle strategyHandle,
                                         exchange_id_type exchangeId,
                                         instrument_id_type instrumentId,
                                         SubscribeOptions options) = 0;
    virtual void unsubscribe(StrategyHandle strategyHandle,
                             exchange_id_type exchangeId,
                             instrument_id_type instrumentId) = 0;

    virtual void place(AccountId accountId,
                       StrategyHandle strategyHandle,
                       exchange_id_type exchangeId,
                       std::vector<OrderEntry> entries) = 0;
    virtual void cancel(AccountId accountId,
                        StrategyHandle strategyHandle,
                        exchange_id_type exchangeId,
                        std::vector<CancelEntry> entries) = 0;
    virtual void amend(AccountId accountId,
                       StrategyHandle strategyHandle,
                       exchange_id_type exchangeId,
                       std::vector<AmendEntry> entries) = 0;
    virtual void replace(AccountId accountId,
                         StrategyHandle strategyHandle,
                         exchange_id_type exchangeId,
                         std::vector<ReplaceEntry> entries) = 0;

    virtual void setupExchange(StrategyHandle strategyHandle,
                               exchange_id_type exchangeId,
                               AccountId accountId,
                               CSOEHConfig config) = 0;

    virtual std::optional<SnapshotEvent> getLatestSnapshot(instrument_id_type instrumentId,
                                                           exchange_id_type exchangeId) const = 0;
    virtual std::optional<BookUpdateEvent> getLatestBook(instrument_id_type instrumentId,
                                                         exchange_id_type exchangeId) const = 0;
    virtual std::optional<TopOfBookEvent> getLatestTopOfBook(instrument_id_type instrumentId,
                                                             exchange_id_type exchangeId) const = 0;
    virtual std::optional<IndexPriceEvent> getLatestIndexPrice(instrument_id_type instrumentId,
                                                               exchange_id_type exchangeId) const = 0;
    virtual std::optional<MarkPriceEvent> getLatestMarkPrice(instrument_id_type instrumentId,
                                                             exchange_id_type exchangeId) const = 0;
    virtual std::optional<InstrumentStateEvent> getLatestInstrumentState(instrument_id_type instrumentId,
                                                                         exchange_id_type exchangeId) const = 0;
    virtual std::optional<FundingRateEvent> getLatestFundingRate(instrument_id_type instrumentId,
                                                                 exchange_id_type exchangeId) const = 0;
    virtual std::optional<TradesEvent> getLatestTrades(instrument_id_type instrumentId,
                                                       exchange_id_type exchangeId) const = 0;

    virtual const Capabilities* getMdhCapabilities(exchange_id_type exchangeId) const = 0;
    virtual bool isMdhReady(exchange_id_type exchangeId) const = 0;
    virtual bool isMdhConnected(exchange_id_type exchangeId) const = 0;
    virtual const LoginCapabilities& getOehCapabilities(exchange_id_type exchangeId,
                                                        AccountId accountId) const = 0;

    virtual const WalletInfo* getWalletInfo(exchange_id_type exchangeId,
                                            AccountId accountId) const = 0;
    virtual const AccountInfo* getAccountInfo(exchange_id_type exchangeId,
                                              AccountId accountId) const = 0;
    virtual std::optional<WalletDataEntry> getWalletBalance(exchange_id_type exchangeId,
                                                            AccountId accountId,
                                                            uint64_t underlyingId) const = 0;
    virtual const std::vector<WalletMaintenanceMargin>* getMaintenanceMargin(exchange_id_type exchangeId,
                                                                             AccountId accountId) const = 0;
    virtual const std::vector<WalletTotalBalance>* getTotalBalances(exchange_id_type exchangeId,
                                                                    AccountId accountId) const = 0;
    virtual const std::vector<RateLimitLoad>* getRateLimitLoads(exchange_id_type exchangeId,
                                                                AccountId accountId) const = 0;

    virtual std::vector<instrument_id_type> getSubscribedInstruments(exchange_id_type exchangeId) const = 0;
    virtual bool isInstrumentSubscribed(exchange_id_type exchangeId,
                                        instrument_id_type instrumentId) const = 0;
    virtual std::optional<std::string> getBestBid(exchange_id_type exchangeId,
                                                  instrument_id_type instrumentId) const = 0;
    virtual std::optional<std::string> getBestAsk(exchange_id_type exchangeId,
                                                  instrument_id_type instrumentId) const = 0;
    virtual bool isExchangeReady(exchange_id_type exchangeId, AccountId accountId) const = 0;
};

class InProcessStrategyServices final : public IStrategyServices {
public:
    explicit InProcessStrategyServices(ApiManager& apiManager)
        : apiManager_(apiManager) {}

    bool subscribe(StrategyHandle strategyHandle,
                   exchange_id_type exchangeId,
                   instrument_id_type instrumentId,
                   SubscribeOptions options) override {
        return apiManager_.subscribe(strategyHandle, exchangeId, instrumentId, options);
    }

    bool subscribeMarketDataOnly(StrategyHandle strategyHandle,
                                 exchange_id_type exchangeId,
                                 instrument_id_type instrumentId,
                                 SubscribeOptions options) override {
        return apiManager_.subscribeMarketDataOnly(strategyHandle, exchangeId, instrumentId, options);
    }

    void unsubscribe(StrategyHandle strategyHandle,
                     exchange_id_type exchangeId,
                     instrument_id_type instrumentId) override {
        apiManager_.unsubscribe(strategyHandle, exchangeId, instrumentId);
    }

    void place(AccountId accountId,
               StrategyHandle strategyHandle,
               exchange_id_type exchangeId,
               std::vector<OrderEntry> entries) override {
        apiManager_.place(accountId, strategyHandle, exchangeId, std::move(entries));
    }

    void cancel(AccountId accountId,
                StrategyHandle strategyHandle,
                exchange_id_type exchangeId,
                std::vector<CancelEntry> entries) override {
        apiManager_.cancel(accountId, strategyHandle, exchangeId, std::move(entries));
    }

    void amend(AccountId accountId,
               StrategyHandle strategyHandle,
               exchange_id_type exchangeId,
               std::vector<AmendEntry> entries) override {
        apiManager_.amend(accountId, strategyHandle, exchangeId, std::move(entries));
    }

    void replace(AccountId accountId,
                 StrategyHandle strategyHandle,
                 exchange_id_type exchangeId,
                 std::vector<ReplaceEntry> entries) override {
        apiManager_.replace(accountId, strategyHandle, exchangeId, std::move(entries));
    }

    void setupExchange(StrategyHandle strategyHandle,
                       exchange_id_type exchangeId,
                       AccountId accountId,
                       CSOEHConfig config) override {
        apiManager_.setupExchange(strategyHandle, exchangeId, accountId, std::move(config));
    }

    std::optional<SnapshotEvent> getLatestSnapshot(instrument_id_type instrumentId,
                                                   exchange_id_type exchangeId) const override {
        return apiManager_.getLatestSnapshot(instrumentId, exchangeId);
    }

    std::optional<BookUpdateEvent> getLatestBook(instrument_id_type instrumentId,
                                                 exchange_id_type exchangeId) const override {
        return apiManager_.getLatestBook(instrumentId, exchangeId);
    }

    std::optional<TopOfBookEvent> getLatestTopOfBook(instrument_id_type instrumentId,
                                                     exchange_id_type exchangeId) const override {
        return apiManager_.getLatestTopOfBook(instrumentId, exchangeId);
    }

    std::optional<IndexPriceEvent> getLatestIndexPrice(instrument_id_type instrumentId,
                                                       exchange_id_type exchangeId) const override {
        return apiManager_.getLatestIndexPrice(instrumentId, exchangeId);
    }

    std::optional<MarkPriceEvent> getLatestMarkPrice(instrument_id_type instrumentId,
                                                     exchange_id_type exchangeId) const override {
        return apiManager_.getLatestMarkPrice(instrumentId, exchangeId);
    }

    std::optional<InstrumentStateEvent> getLatestInstrumentState(instrument_id_type instrumentId,
                                                                 exchange_id_type exchangeId) const override {
        return apiManager_.getLatestInstrumentState(instrumentId, exchangeId);
    }

    std::optional<FundingRateEvent> getLatestFundingRate(instrument_id_type instrumentId,
                                                         exchange_id_type exchangeId) const override {
        return apiManager_.getLatestFundingRate(instrumentId, exchangeId);
    }

    std::optional<TradesEvent> getLatestTrades(instrument_id_type instrumentId,
                                               exchange_id_type exchangeId) const override {
        return apiManager_.getLatestTrades(instrumentId, exchangeId);
    }

    const Capabilities* getMdhCapabilities(exchange_id_type exchangeId) const override {
        return apiManager_.getMdhCapabilities(exchangeId);
    }

    bool isMdhReady(exchange_id_type exchangeId) const override {
        return apiManager_.isMdhReady(exchangeId);
    }

    bool isMdhConnected(exchange_id_type exchangeId) const override {
        return apiManager_.isMdhConnected(exchangeId);
    }

    const LoginCapabilities& getOehCapabilities(exchange_id_type exchangeId,
                                                AccountId accountId) const override {
        return apiManager_.getOehCapabilities(exchangeId, accountId);
    }

    const WalletInfo* getWalletInfo(exchange_id_type exchangeId,
                                    AccountId accountId) const override {
        return apiManager_.getWalletInfo(exchangeId, accountId);
    }

    const AccountInfo* getAccountInfo(exchange_id_type exchangeId,
                                      AccountId accountId) const override {
        return apiManager_.getAccountInfo(exchangeId, accountId);
    }

    std::optional<WalletDataEntry> getWalletBalance(exchange_id_type exchangeId,
                                                    AccountId accountId,
                                                    uint64_t underlyingId) const override {
        return apiManager_.getWalletBalance(exchangeId, accountId, underlyingId);
    }

    const std::vector<WalletMaintenanceMargin>* getMaintenanceMargin(exchange_id_type exchangeId,
                                                                     AccountId accountId) const override {
        return apiManager_.getMaintenanceMargin(exchangeId, accountId);
    }

    const std::vector<WalletTotalBalance>* getTotalBalances(exchange_id_type exchangeId,
                                                            AccountId accountId) const override {
        return apiManager_.getTotalBalances(exchangeId, accountId);
    }

    const std::vector<RateLimitLoad>* getRateLimitLoads(exchange_id_type exchangeId,
                                                        AccountId accountId) const override {
        return apiManager_.getRateLimitLoads(exchangeId, accountId);
    }

    std::vector<instrument_id_type> getSubscribedInstruments(exchange_id_type exchangeId) const override {
        return apiManager_.getSubscribedInstruments(exchangeId);
    }

    bool isInstrumentSubscribed(exchange_id_type exchangeId,
                                instrument_id_type instrumentId) const override {
        return apiManager_.isInstrumentSubscribed(exchangeId, instrumentId);
    }

    std::optional<std::string> getBestBid(exchange_id_type exchangeId,
                                          instrument_id_type instrumentId) const override {
        return apiManager_.getBestBid(exchangeId, instrumentId);
    }

    std::optional<std::string> getBestAsk(exchange_id_type exchangeId,
                                          instrument_id_type instrumentId) const override {
        return apiManager_.getBestAsk(exchangeId, instrumentId);
    }

    bool isExchangeReady(exchange_id_type exchangeId, AccountId accountId) const override {
        return apiManager_.isExchangeReady(exchangeId, accountId);
    }

private:
    ApiManager& apiManager_;
};
