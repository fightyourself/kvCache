#pragma once
#include <grpcpp/grpcpp.h>
#include "CallDataBase.h"
#include "lab2/lab2.grpc.pb.h"
#include "raft.h"
using grpc::ServerContext;
using grpc::Status;
using lab2::ProposeRequest;
using lab2::ProposeReply;
class ProposeCall final : public CallDataBase {
public:
    ProposeCall(lab2::RaftService::AsyncService* service,
                grpc::ServerCompletionQueue* cq,
                Raft* raft);

    void proceed(bool ok) override;
    void finish(const ProposeReply& rep);
private:
    enum class State { CREATE, PROCESS, FINISH };
    lab2::RaftService::AsyncService* service_;
    grpc::ServerCompletionQueue* cq_;
    grpc::ServerContext ctx_;

    lab2::ProposeRequest req_;
    lab2::ProposeReply rep_;
    grpc::ServerAsyncResponseWriter<lab2::ProposeReply> responder_;

    Raft* raft_;
    State state_;
};
