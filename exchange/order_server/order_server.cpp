#include "order_server.h"

namespace Exchange {
  OrderServer::OrderServer(ClientRequestLFQueue *client_requests, ClientResponseLFQueue *client_responses, const std::string &iface, int port)
      : iface_(iface), port_(port), outgoing_responses_(client_responses), logger_("exchange_order_server.log"),
        tcp_server_(logger_), fifo_sequencer_(client_requests, &logger_) {
    cid_next_outgoing_seq_num_.fill(1);
    cid_next_exp_seq_num_.fill(1);
    cid_tcp_socket_.fill(nullptr);

    tcp_server_.recv_callback_ = [this](auto socket, auto rx_time) { recvCallback(socket, rx_time); };
    tcp_server_.recv_finished_callback_ = [this]() { recvFinishedCallback(); };
  }

  OrderServer::~OrderServer() {
    stop();
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(1s);
  }

  auto OrderServer::start() -> void {
    run_ = true;
    tcp_server_.listen(iface_, port_);

    ASSERT(Common::createAndStartThread(-1, "Exchange/OrderServer", [this]() { run(); }) != nullptr, "Failed to start OrderServer thread.");
  }

  auto OrderServer::stop() -> void {
    run_ = false;
  }

  auto OrderServer::run() -> void {
    logger_.log("%:% %() %\n",__FILE__,__LINE__,__FUNCTION__,Common::getCurrentTimeStr(&time_str_));
    while (run_) {
      tcp_server_.poll();
      tcp_server_.sendAndRecv();
      for(auto client_response=outgoing_responses_->getNextToRead();outgoing_responses_->size()&&client_response;client_response=outgoing_responses_->getNextToRead()){
        auto &next_outgoing_seq_num=cid_next_outgoing_seq_num_[client_response->client_id_];
        logger_.log("%:% %() % Processing cid:% seq:% %\n",__FILE__,__LINE__,__FUNCTION__,
                    Common::getCurrentTimeStr(&time_str_),client_response->client_id_,next_outgoing_seq_num,client_response->toString());
        ASSERT(cid_tcp_socket_[client_response->client_id_]!=nullptr,"Cont have a TCPSocket for ClientId:"+std::to_string(client_response->client_id_));
        cid_tcp_socket_[client_response->client_id_]->send(&next_outgoing_seq_num,sizeof(next_outgoing_seq_num));
        cid_tcp_socket_[client_response->client_id_]->send(client_response,sizeof(client_response));

        outgoing_responses_->updateReadIndex();
        ++next_outgoing_seq_num;
      }
    }
  }
}
