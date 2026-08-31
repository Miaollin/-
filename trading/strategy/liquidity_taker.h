#pragma once
#include "common/macros.h"
#include "common/logging.h"
#include "trading/strategy/order_manager.h"
#include "trading/strategy/feature_engine.h"
using namespace Common;

namespace Trading
{
    class LiquidityTaker
    {
    private:
        const FeatureEngine *feature_engine_ = nullptr;
        OrderManager *order_manager_ = nullptr;
        std::string time_str_;
        Common::Logger *logger_ = nullptr;
        const TradeEngineCfgHashMap ticker_cfg_;

    public:
        LiquidityTaker(Common::Logger *logger,
                       TradeEngine *trade_engine, FeatureEngine *feature_engine, OrderManager *order_manager,
                       const TradeEngineCfgHashMap &ticker_cfg);

        auto onOrderBookUpdate(TickerId ticker_id, Price price, Side side, MarketOrderBook *) noexcept -> void
        {
            logger_->log("%:% %() % ticker:% price:% side:%\n", __FILE__, __LINE__, __FUNCTION__,
                         Common::getCurrentTimeStr(&time_str_), ticker_id, Common::priceToString(price).c_str(),
                         Common::sideToString(side).c_str());
        }
        auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update, MarketOrderBook *book) noexcept -> void;

        auto onOrderUpdate(const Exchange::MEClientResponse *client_response) noexcept -> void
        {
            logger_->log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                         client_response->toString().c_str());
            order_manager_->onOrderUpdate(client_response);
        }

        /// Deleted default, copy & move constructors and assignment-operators.
        LiquidityTaker() = delete;

        LiquidityTaker(const LiquidityTaker &) = delete;

        LiquidityTaker(const LiquidityTaker &&) = delete;

        LiquidityTaker &operator=(const LiquidityTaker &) = delete;

        LiquidityTaker &operator=(const LiquidityTaker &&) = delete;
    };
}
