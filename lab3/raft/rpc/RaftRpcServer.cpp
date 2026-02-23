#include "RaftRpcServer.h"
#include "AppendEntriesCall.h"
#include "RequestVoteCall.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::CompletionQueue;
RaftRpcServer::RaftRpcServer(const std::string& addr, Raft* raft):
    addr_(addr),    
    raft_(raft){

}

void RaftRpcServer::start(){
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    
    // CQ 消费循环（通常独立线程跑）
    // 为每个 RPC 类型预创建一个 CallData
    new AppendEntriesCall(&service_, cq_.get(), raft_);
    new RequestVoteCall(&service_,cq_.get(),raft_);
    cq_thread_ = std::thread([this]() { pool_cp_loop(); });
}

void RaftRpcServer::stop(){
    if (server_) server_->Shutdown();
    if (cq_) cq_->Shutdown();
    if (cq_thread_.joinable()) cq_thread_.join();
}

void RaftRpcServer::pool_cp_loop(){
    void* tag = nullptr;
    bool ok = false;
    while (cq_->Next(&tag, &ok)) {
      static_cast<CallDataBase*>(tag)->proceed(ok);
    }
}