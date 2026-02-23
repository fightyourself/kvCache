#pragma once
#include "grpcpp/grpcpp.h"
#include  "lab3/kvService.grpc.pb.h"
#include "CallDataBase.h"
using kvService::GetRequest;
using kvService::GetReply;
class KvServer;
class GetCall final : public CallDataBase{
public:
    GetCall(kvService::KvService::AsyncService *service,
            grpc::ServerCompletionQueue *cq);
    ~GetCall();
    virtual void proceed(bool ok) override;
private:
    enum class State { CREATE, PROCESS, FINISH };

    kvService::KvService::AsyncService *service_;
    grpc::ServerCompletionQueue *cq_;

    grpc::ServerContext ctx_;
    GetRequest req_;
    GetReply rep_;
    grpc::ServerAsyncResponseWriter<GetReply> responder_;

    State state_;
    KvServer *KvServer_;
};