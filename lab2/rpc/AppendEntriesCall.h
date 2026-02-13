#pragma once
#include "CallDataBase.h"
#include "grpcpp/grpcpp.h"
#include "lab2/lab2.grpc.pb.h"
using grpc::ServerContext;
using grpc::Status;
using lab2::AppendEntriesRequest;
using lab2::AppendEntriesReply;
class Raft;
class AppendEntriesCall final : public CallDataBase {
public:
    AppendEntriesCall(lab2::RaftService::AsyncService* service,
                    grpc::ServerCompletionQueue* cq,
                    Raft* raft);

    virtual void proceed(bool ok) override;
  

private:
	enum class State { CREATE, PROCESS, FINISH };

	lab2::RaftService::AsyncService *service_;
	grpc::ServerCompletionQueue *cq_;
	grpc::ServerContext ctx_;

	AppendEntriesRequest req_;
	AppendEntriesReply rep_;
	grpc::ServerAsyncResponseWriter<AppendEntriesReply> responder_;

	Raft *raft_;
	State state_;
};