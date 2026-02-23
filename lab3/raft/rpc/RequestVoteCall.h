#pragma once
#include "CallDataBase.h"
#include "grpcpp/grpcpp.h"
#include "lab3/raftService.grpc.pb.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using raftService::RequestVoteRequest;
using raftService::RequestVoteReply;
class Raft;
class RequestVoteCall final : public CallDataBase{
public:
    RequestVoteCall(raftService::RaftService::AsyncService *service,
                grpc::ServerCompletionQueue *cq,
                Raft *raft);

    virtual void proceed(bool ok) override;

private:
    enum class State { CREATE, PROCESS, FINISH };

    raftService::RaftService::AsyncService *service_;
	grpc::ServerCompletionQueue *cq_;
	grpc::ServerContext ctx_;

	RequestVoteRequest req_;
	RequestVoteReply rep_;
	grpc::ServerAsyncResponseWriter<RequestVoteReply> responder_;

    Raft *raft_;
    State state_;
};