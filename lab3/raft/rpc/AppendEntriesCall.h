#pragma once
#include "CallDataBase.h"
#include "grpcpp/grpcpp.h"
#include "lab3/raftService.grpc.pb.h"
using grpc::ServerContext;
using grpc::Status;
using raftService::AppendEntriesRequest;
using raftService::AppendEntriesReply;
class Raft;
class AppendEntriesCall final : public CallDataBase {
public:
    AppendEntriesCall(raftService::RaftService::AsyncService* service,
                    grpc::ServerCompletionQueue* cq,
                    Raft* raft);

    virtual void proceed(bool ok) override;
  

private:
	enum class State { CREATE, PROCESS, FINISH };

	raftService::RaftService::AsyncService *service_;
	grpc::ServerCompletionQueue *cq_;
	grpc::ServerContext ctx_;

	AppendEntriesRequest req_;
	AppendEntriesReply rep_;
	grpc::ServerAsyncResponseWriter<AppendEntriesReply> responder_;

	Raft *raft_;
	State state_;
};