#pragma once
#pragma once 
#include "CallDataBase.h"
#include "grpcpp/grpcpp.h"
#include "lab2/lab2.grpc.pb.h"
using grpc::ClientContext;
using grpc::Status;
using grpc::CompletionQueue;
using lab2::RequestVoteRequest;
using lab2::RequestVoteReply;
class Raft;
class RequestVoteClientCall final : public CallDataBase{
public:
    RequestVoteClientCall(lab2::RaftService::Stub *stub, CompletionQueue *cq,Raft *raft, int id);
    virtual void proceed(bool ok) override;
    void start(const RequestVoteRequest& req);


private:
    lab2::RaftService::Stub *stub_;
    grpc::CompletionQueue *cq_;
    Raft *raft_;
    
    int id_;
    RequestVoteRequest req_;
    RequestVoteReply rep_;

    std::unique_ptr<grpc::ClientAsyncResponseReader<RequestVoteReply>> rpc_;
    ClientContext ctx_;
    Status status_;
};