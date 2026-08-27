#pragma once
#include <functional>
#include <atomic>
#include "common/thread_utils.h"
#include "common/macros.h"
#include "common/tcp_server.h"
#include "exchange/order_server/client_request.h"
#include "exchange/order_server/client_response.h"
#include "exchange/order_server/fifo_sequencer.h"

namespace Exchange{
class OrderServer{
private:
    const std::string iface_;
    const int port_ =0;
    ClientResponseLFQueue *outgoing_responses_ = nullptr;
    std::atomic<bool> run_=false;
    std::string time_str_;
    Logger logger_;
    std::array<size_t,ME_MAX_NUM_CLIENTS> cid_next_outgoing_seq_num_;
    std::array<size_t,ME_MAX_NUM_CLIENTS> cid_next_exp_seq_num_;
    std::array<Common::TCPSocket *,ME_MAX_NUM_CLIENTS> cid_tcp_socket_;
    Common::TCPServer tcp_server_;
    FIFOSequencer fifo_sequencer_;
public:
    OrderServer(ClientRequestLFQueue *client_requests,
        ClientResponseLFQueue *client_responses,const std::string &iface,int port);
    ~OrderServer();
    auto start()->void;
    auto stop()->void;
    auto recvCallback(TCPSocket *socket,Nanos rx_time) noexcept{
        logger_.log("%:% %() % Received socket:% len:% rx:%\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                  socket->socket_fd_, socket->next_rcv_valid_index_, rx_time);
        if(socket->next_rcv_valid_index_>=sizeof(OMClientRequest)){
            size_t i=0;
            for(;i+sizeof(OMClientRequest)<=socket->next_rcv_valid_index_;i+=sizeof(OMClientRequest)){
                const auto* request = reinterpret_cast<const OMClientRequest*>
                                    (socket->inbound_data_.data()+i);
                logger_.log("%:% %() % Received %\n",__FILE__,__LINE__,__FUNCTION__,Common::getCurrentTimeStr(&time_str_),request->toString());
                if(UNLIKELY(cid_tcp_socket_[request->me_client_request_.client_id_]==nullptr)){
                    cid_tcp_socket_[request->me_client_request_.client_id_]=socket;
                }
                if(cid_tcp_socket_[request->me_client_request_.client_id_]!=socket){
                    logger_.log("%:% %() % Received %\n",__FILE__,__LINE__,__FUNCTION__,Common::getCurrentTimeStr(&time_str_),request->me_client_request_.client_id_,socket->socket_fd_,cid_tcp_socket_[request->me_client_request_.client_id_]->socket_fd_);
                    continue;
                }
                auto &next_exp_seq_num=cid_next_exp_seq_num_[request->me_client_request_.client_id_];
                if(request->seq_num_!=next_exp_seq_num){
                    logger_.log("%:% %() % Received %\n",__FILE__,__LINE__,__FUNCTION__,Common::getCurrentTimeStr(&time_str_),request->me_client_request_.client_id_,next_exp_seq_num,request->seq_num_);
                    continue;
                }
                ++next_exp_seq_num;
                fifo_sequencer_.addClientRequest(rx_time,request->me_client_request_);
                memcpy(socket->inbound_data_.data(),socket->inbound_data_.data()+i,socket->next_rcv_valid_index_-i);
                socket->next_rcv_valid_index_-=i;
            }
        }
    }
    auto recvFinishedCallback() noexcept{
        fifo_sequencer_.sequenceAndPublish();
    }
    auto run()->void;
};
}