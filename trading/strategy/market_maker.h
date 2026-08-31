#pragma once
#include "common/macros.h"
#include "common/logging.h"
#include "trading/strategy/order_manager.h"
#include "feature_engine.h"
using namespace Common;

namespace Trading
{
    class MarketMaker
    {
    private:
        const FeatureEngine *feature_engine_ = nullptr;
        OrderManager *order_manager_ = nullptr;
        std::string time_str_;
        Common::Logger *logger_ = nullptr;
        const TradeEngineCfgHashMap ticker_cfg_;

    public:
        MarketMaker(Common::Logger *logger,
                    TradeEngine *trade_engine, const FeatureEngine *feature_engine, OrderManager *order_manager, const TradeEngineCfgHashMap &ticker_cfg);

        auto onOrderBookUpdate(TickerId ticker_id, Price price, Side side, const MarketOrderBook *book) noexcept -> void;

        /// Process trade events, which for the market making algorithm is none.
        auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update, MarketOrderBook * /* book */) noexcept -> void
        {
            logger_->log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                         market_update->toString().c_str());
        }

        /// Process client responses for the strategy's orders.
        auto onOrderUpdate(const Exchange::MEClientResponse *client_response) noexcept -> void
        {
            logger_->log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                         client_response->toString().c_str());

            order_manager_->onOrderUpdate(client_response);
        }

        /// Deleted default, copy & move constructors and assignment-operators.
        MarketMaker() = delete;

        MarketMaker(const MarketMaker &) = delete;

        MarketMaker(const MarketMaker &&) = delete;

        MarketMaker &operator=(const MarketMaker &) = delete;

        MarketMaker &operator=(const MarketMaker &&) = delete;
    };
}