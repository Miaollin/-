#pragma once
#include <functional>
#include <map>
#include <atomic>
#include "common/thread_utils.h"
#include "common/lf_queue.h"
#include "common/logging.h"
#include "common/macros.h"
#include "common/mcast_socket.h"
#include "exchange/market_data/market_data_publisher.h"

namespace Trading
{
    class MarketDataConsumer
    {
    private:
        size_t next_exp_inc_seq_num_ = 1;
        Exchange::MEMarketUpdateLFQueue *incoming_md_updates_ = nullptr;
        std::atomic<bool> run_ = false;
        std::string time_str_;
        Common::Logger logger_;
        Common::McastSocket incremental_mcast_socket_, snapshot_mcast_socket_;
        bool in_recovery_ = false;
        const std::string iface_, snapshot_ip_;
        const int snapshot_port_;
        typedef std::map<size_t, Exchange::MEMarketUpdate> QueueMarketUpdates;
        QueueMarketUpdates snapshot_queued_msgs_, incremental_queued_msgs_;

    public:
        MarketDataConsumer(Common::ClientId client_id, Exchange::MEMarketUpdateLFQueue *market_updates,
                           const std::string &iface,
                           const std::string &snapshot_ip, int snapshot_port,
                           const std::string &incremental_ip, int incremental_port);

        auto start() ->void;
        ~MarketDataConsumer();
        auto stop() ->void;
        auto run() ->void;
        auto recvCallback(McastSocket *socket) noexcept ->void;
        auto startSnapshotSync()->void;
        auto queueMessage(bool is_snapshot,const Exchange::MDPMarketUpdate *request) -> void;
        auto checkSnapshotSync()->void;
    };
}