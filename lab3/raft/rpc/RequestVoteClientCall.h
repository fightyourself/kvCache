#pragma once
#include "CallDataBase.h"
#include "grpcpp/grpcpp.h"
#include "lab3/raftService.grpc.pb.h"
using grpc::ClientContext;
using grpc::Status;
using grpc::CompletionQueue;
using raftService::RequestVoteRequest;
using raftService::RequestVoteReply;
class Raft;
class RequestVoteClientCall final : public CallDataBase{
public:
    RequestVoteClientCall(raftService::RaftService::Stub *stub, CompletionQueue *cq,Raft *raft, int id);
    virtual void proceed(bool ok) override;
    void start(const RequestVoteRequest& req);


private:
    raftService::RaftService::Stub *stub_;
    grpc::CompletionQueue *cq_;
    Raft *raft_;
    
    int id_;
    RequestVoteRequest req_;
    RequestVoteReply rep_;

    std::unique_ptr<grpc::ClientAsyncResponseReader<RequestVoteReply>> rpc_;
    ClientContext ctx_;
    Status status_;
};