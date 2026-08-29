#pragma once
#include "common/macros.h"
#include "common/logging.h"
#include "exchange/order_server/client_response.h"
#include "exchange/order_server/client_request.h"
#include "trading/strategy/om_order.h"
#include "trading/strategy/risk_manager.h"

using namespace Common;
namespace Trading
{
    class TradeEngine;

    class OrderManager
    {
    private:
        TradeEngine *trade_engine_ = nullptr;
        const RiskManager &risk_manager_;
        std::string time_str_;
        Common::Logger *logger_ = nullptr;
        OMOrderTickerSideHashMap ticker_side_order_;
        OrderId next_order_id_ = 1;

    public:
        OrderManager(Common::Logger *logger, TradeEngine *trade_engine, RiskManager &risk_manager)
            : trade_engine_(trade_engine), risk_manager_(risk_manager), logger_(logger) {}

        auto getOrderSideHashMap(TickerId ticker_id) const
        {
            return &(ticker_side_order_.at(ticker_id));
        }
        auto newOrder(OMOrder *order, TickerId ticker_id, Price price, Side side, Qty qty) noexcept;
        auto cancelOrder(OMOrder *order) noexcept -> void;

        auto onOrderUpdate(const Exchange::MEClientResponse *client_response) noexcept -> void
        {
            logger_->log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                         client_response->toString().c_str());
            auto order = &(ticker_side_order_.at(client_response->ticker_id_).at(sideToIndex(client_response->side_)));
            logger_->log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                         order->toString().c_str());

            switch (client_response->type_)
            {
            case Exchange::ClientResponseType::ACCEPTED:
            {
                order->order_state_ = OMOrderState::LIVE;
            }
            break;
            case Exchange::ClientResponseType::CANCELED:
            {
                order->order_state_ = OMOrderState::DEAD;
            }
            break;
            case Exchange::ClientResponseType::FILLED:
            {
                order->qty_ = client_response->leaves_qty_;
                if (!order->qty_)
                    order->order_state_ = OMOrderState::DEAD;
            }
            break;
            case Exchange::ClientResponseType::CANCEL_REJECTED:
            case Exchange::ClientResponseType::INVALID:
            {
            }
            break;
            }
        }
        auto moveOrder(OMOrder *order, TickerId ticker_id, Price price, Side side, Qty qty) noexcept ->void;
        auto moveOrders(TickerId ticker_id,Price bid_price,Price ask_price,Qty clip) noexcept{
            auto bid_order=&(ticker_side_order_.at(ticker_id).at(sideToIndex(Side::BUY)));
            moveOrder(bid_order,ticker_id,bid_price,Side::BUY,clip);
            auto ask_order=&(ticker_side_order_.at(ticker_id).at(sideToIndex(Side::SELL)));
            moveOrder(ask_order,ticker_id,ask_price,Side::SELL,clip);
        }
    };
}