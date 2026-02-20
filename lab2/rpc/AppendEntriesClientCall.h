#pragma once 
#include "CallDataBase.h"
#include "grpcpp/grpcpp.h"
#include "lab2/lab2.grpc.pb.h"
using grpc::ClientContext;
using grpc::Status;
using grpc::CompletionQueue;
using lab2::AppendEntriesRequest;
using lab2::AppendEntriesReply;
class Raft;
class AppendEntriesClientCall final : public CallDataBase{
public:
    AppendEntriesClientCall(lab2::RaftService::Stub *stub, CompletionQueue *cq, Raft *raft, int id, uint64_t last_index);
    virtual void proceed(bool ok) override;
    void start(const AppendEntriesRequest& req);


private:
    lab2::RaftService::Stub *stub_;
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