#include "PutAppendCall.h"
#include "KvServer.h"
PutAppendCall::PutAppendCall(kvService::KvService::AsyncService *service,
            grpc::ServerCompletionQueue *cq):
            service_(service),
            cq_(cq),
            responder_(&ctx_),
            state_(State::CREATE){
    proceed(true);
}

PutAppendCall::~PutAppendCall(){

}

void PutAppendCall::proceed(bool ok){
    if (!ok) {
        // ok=false 常见于：客户端取消、server shutdown
        delete this;
        return;
    }

    if (state_ == State::CREATE) {
        state_ = State::PROCESS;

        service_->Requestput_append(
            &ctx_, &req_, &responder_,
            cq_, cq_,
            this);
        return;
    }

    if (state_ == State::PROCESS) {
        // 关键：立刻创建下一个 handler，保证服务持续可接收新 RPC
        new PutAppendCall(service_, cq_);

        KvServer_->handle_put_append(req_);
        return;
    }

    // FINISH：Finish 完成事件回到 CQ，此时可以销毁
    if (state_ == State::FINISH) {
        delete this;
        return;
    }
}