#pragma once

#include <functional>
#include <atomic>

#include "common/thread_utils.h"
#include "common/time_utils.h"
#include "common/lf_queue.h"
#include "common/macros.h"
#include "common/logging.h"

#include "exchange/order_server/client_request.h"
#include "exchange/order_server/client_response.h"
#include "exchange/market_data/market_update.h"

#include "market_order_book.h"

#include "feature_engine.h"
#include "position_keeper.h"
#include "order_manager.h"
#include "risk_manager.h"

#include "market_maker.h"
#include "liquidity_taker.h"

namespace Trading
{
    class TradeEngine
    {
    public:
        using OrderBookUpdateHandler =
            std::function<void(Common::TickerId, Common::Price, Common::Side, MarketOrderBook *)>;
        using TradeUpdateHandler =
            std::function<void(const Exchange::MEMarketUpdate *, MarketOrderBook *)>;
        using OrderUpdateHandler =
            std::function<void(const Exchange::MEClientResponse *)>;
        OrderBookUpdateHandler algoOnOrderBookUpdate_;
        TradeUpdateHandler algoOnTradeUpdate_;
        OrderUpdateHandler algoOnOrderUpdate_;

        TradeEngine(Common::ClientId client_id,
                    AlgoType algo_type,
                    const TradeEngineCfgHashMap &ticker_cfg,
                    Exchange::ClientRequestLFQueue *client_requests,
                    Exchange::ClientResponseLFQueue *client_responses,
                    Exchange::MEMarketUpdateLFQueue *market_updates);

        auto start() -> void;
        auto stop() -> void;
        auto sendClientRequest(const Exchange::MEClientRequest *client_request) noexcept -> void;
        ~TradeEngine();
        auto run() noexcept -> void;
        auto onOrderBookUpdate(Common::TickerId ticker_id, Common::Price price,
                               Common::Side side, MarketOrderBook *book) noexcept -> void;
        auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update,
                           MarketOrderBook *book) noexcept -> void;
        auto onOrderUpdate(const Exchange::MEClientResponse *client_response);
        /// Deleted default, copy & move constructors and assignment-operators.
        TradeEngine() = delete;

        TradeEngine(const TradeEngine &) = delete;

        TradeEngine(const TradeEngine &&) = delete;

        TradeEngine &operator=(const TradeEngine &) = delete;

        TradeEngine &operator=(const TradeEngine &&) = delete;

        auto initLastEventTime()
        {
            last_event_time_ = Common::getCurrentNanos();
        }

        auto silentSeconds()
        {
            return (Common::getCurrentNanos() - last_event_time_) / NANOS_TO_SECS;
        }

        auto clientId() const
        {
            return client_id_;
        }

    private:
        const ClientId client_id_;
        MarketOrderBookHashMap ticker_order_book_;
        Exchange::ClientRequestLFQueue *outgoing_ogw_requests_ = nullptr;
        Exchange::ClientResponseLFQueue *incoming_ogw_responses_ = nullptr;
        Exchange::MEMarketUpdateLFQueue *incoming_md_updates_ = nullptr;
        Nanos last_event_time_ = 0;
        std::atomic<bool> run_ = false;
        std::string time_str_;
        Logger logger_;

        FeatureEngine feature_engine_;
        PositionKeeper position_keeper_;
        RiskManager risk_manager_;
        OrderManager order_manager_;
        MarketMaker *mm_algo_ = nullptr;
        LiquidityTaker *taker_algo_ = nullptr;

        /// Default methods to initialize the function wrappers.
        auto defaultAlgoOnOrderBookUpdate(TickerId ticker_id, Price price, Side side, MarketOrderBook *) noexcept -> void
        {
            logger_.log("%:% %() % ticker:% price:% side:%\n", __FILE__, __LINE__, __FUNCTION__,
                        Common::getCurrentTimeStr(&time_str_), ticker_id, Common::priceToString(price).c_str(),
                        Common::sideToString(side).c_str());
        }

        auto defaultAlgoOnTradeUpdate(const Exchange::MEMarketUpdate *market_update, MarketOrderBook *) noexcept -> void
        {
            logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                        market_update->toString().c_str());
        }

        auto defaultAlgoOnOrderUpdate(const Exchange::MEClientResponse *client_response) noexcept -> void
        {
            logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                        client_response->toString().c_str());
        }
    };
}
