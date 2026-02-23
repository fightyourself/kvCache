#pragma once
#include "grpcpp/grpcpp.h"
#include "lab3/kvService.grpc.pb.h"
#include "CallDataBase.h"
using kvService::PutAppendRequest;
using kvService::PutAppendReply;
class KvServer;
class PutAppendCall final : public CallDataBase{
public:
    PutAppendCall(kvService::KvService::AsyncService *service,
            grpc::ServerCompletionQueue *cq);
    ~PutAppendCall();
    virtual void proceed(bool ok) override;
private:
    enum class State { CREATE, PROCESS, FINISH };

    kvService::KvService::AsyncService *service_;
    grpc::ServerCompletionQueue *cq_;

    grpc::ServerContext ctx_;
    PutAppendRequest req_;
    PutAppendReply rep_;
    grpc::ServerAsyncResponseWriter<PutAppendReply> responder_;

    State state_;
    KvServer *KvServer_;
};