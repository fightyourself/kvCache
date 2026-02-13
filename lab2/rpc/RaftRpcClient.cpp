#include "RaftRpcClient.h"
#include "PeerClient.hpp"
#include "AppendEntriesClientCall.h"
#include "RequestVoteClientCall.h"
#include "raft.h"
RaftRpcClient::RaftRpcClient(const std::vector<std::string>& peer_address,int me, Raft *raft):
        me_(me),
        raft_(raft),
        cq_(std::move(std::make_unique<CompletionQueue>())){
    for(auto &addr:peer_address){
        auto channel = grpc::CreateChannel(addr,
             grpc::InsecureChannelCredentials());
        auto stub = lab2::RaftService::NewStub(channel);
        peers_.push_back(std::move(std::make_unique<PeerClient>(addr,channel,std::move(stub))));
    }
}

RaftRpcClient::~RaftRpcClient(){
    stop();
}


void RaftRpcClient::send_append_entries_to_all(const AppendEntriesRequest& req){
    for(int i = 0; i<peers_.size(); ++i){
        if(i == me_) continue;
        AppendEntriesRequest temp = req;
        auto call = new AppendEntriesClientCall(peers_[i]->stub_.get(),cq_.get(), raft_, i);
        call->start(std::move(temp));
    }
}

void RaftRpcClient::send_request_vote_to_all(const RequestVoteRequest& req){
    for(int i = 0; i<peers_.size(); ++i){
        if(i == me_) continue;
        RequestVoteRequest temp = req;
        auto call = new RequestVoteClientCall(peers_[i]->stub_.get(),cq_.get(),raft_, i);
        call->start(std::move(temp));
    }
}

void RaftRpcClient::start(){
    cq_thread_ = std::thread([this]() { pool_cp_loop(); });
}

void RaftRpcClient::stop(){
    if (cq_) cq_->Shutdown();
    if (cq_thread_.joinable()) cq_thread_.join();
}

void RaftRpcClient::pool_cp_loop(){
    void* tag = nullptr;
    bool ok = false;
    while (cq_->Next(&tag, &ok)) {
      static_cast<CallDataBase*>(tag)->proceed(ok);
    }
}

