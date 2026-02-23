#pragma once 
#include "CallDataBase.h"
#include "grpcpp/grpcpp.h"
#include "lab3/raftService.grpc.pb.h"
using grpc::ClientContext;
using grpc::Status;
using grpc::CompletionQueue;
using raftService::AppendEntriesRequest;
using raftService::AppendEntriesReply;
class Raft;
class AppendEntriesClientCall final : public CallDataBase{
public:
    AppendEntriesClientCall(raftService::RaftService::Stub *stub, CompletionQueue *cq, Raft *raft, int id, uint64_t last_index);
    virtual void proceed(bool ok) override;
    void start(const AppendEntriesRequest& req);


private:
    raftService::RaftService::Stub *stub_;
    grpc::CompletionQueue *cq_;
    Raft *raft_;
    
    int id_;
    uint64_t last_index_;
    AppendEntriesReply rep_;
    AppendEntriesRequest req_;

    std::unique_ptr<grpc::ClientAsyncResponseReader<AppendEntriesReply>> rpc_;
    ClientContext ctx_;
    Status status_;
};