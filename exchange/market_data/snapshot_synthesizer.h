#pragma once

#include <atomic>
#include <string>
#include <thread>

#include "common/logging.h"
#include "common/mem_pool.h"
#include "common/macros.h"
#include "common/mcast_socket.h"
#include "common/thread_utils.h"
#include "exchange/market_data/market_update.h"

namespace Exchange
{
    class SnapshotSynthesizer
    {
    private:
        MDPMarketUpdateLFQueue *snapshot_md_updates_ = nullptr;
        std::atomic<bool> run_{false};
        std::string time_str_;
        Common::Logger logger_;
        Common::McastSocket snapshot_socket_;
        std::array<std::array<MEMarketUpdate *, ME_MAX_ORDER_IDS>, ME_MAX_TICKERS> ticker_orders_;
        size_t last_inc_seq_num_ = 0;
        Nanos last_snapshot_time_ = 0;
        MemPool<MEMarketUpdate> order_pool_;

    public:
        SnapshotSynthesizer(MDPMarketUpdateLFQueue *market_updates, const std::string &iface,
                                                 const std::string &snapshot_ip, int snapshot_port)
            : snapshot_md_updates_(market_updates), logger_("exchange_snapshot_synthesizer.log"), snapshot_socket_(logger_), order_pool_(ME_MAX_ORDER_IDS)
        {
            ASSERT(snapshot_socket_.init(snapshot_ip, iface, snapshot_port, /*is_listening*/ false) >= 0,
                   "Unable to create snapshot mcast socket. error:" + std::string(std::strerror(errno)));
            for (auto &orders : ticker_orders_)
                orders.fill(nullptr);
        }

        ~SnapshotSynthesizer();

        SnapshotSynthesizer() = delete;
        SnapshotSynthesizer(const SnapshotSynthesizer &) = delete;
        SnapshotSynthesizer &operator=(const SnapshotSynthesizer &) = delete;

        auto start() -> void;

        auto stop() -> void;
        auto addToSnapshot(const MDPMarketUpdate *market_update);
        auto publishSnapshot();
        auto run() noexcept -> void;
    };
}
