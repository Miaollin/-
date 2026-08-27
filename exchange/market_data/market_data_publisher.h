#pragma once
#include <functional>
#include <atomic>
#include "common/logging.h"
#include "common/mcast_socket.h"
#include "exchange/market_data/snapshot_synthesizer.h"
#include "exchange/market_data/market_update.h"

namespace Exchange{
class MarketDataPublisher{
private:
    size_t next_inc_seq_num_=1;
    MEMarketUpdateLFQueue *outgoing_md_updates_=nullptr;
    MDPMarketUpdateLFQueue snapshot_md_updates_;
    std::atomic<bool> run_=false;
    std::string time_str_;
    Logger logger_;
    Common::McastSocket incremental_socket_;
    SnapshotSynthesizer *snapshot_synthesizer_=nullptr;
    std::thread *thread_=nullptr;
public:
    MarketDataPublisher(MEMarketUpdateLFQueue *market_updates, const std::string &iface,
                                           const std::string &snapshot_ip, int snapshot_port,
                                           const std::string &incremental_ip, int incremental_port);
    auto start()->void;
    auto run() noexcept -> void;
    auto stop() -> void;
    ~MarketDataPublisher();
};
}