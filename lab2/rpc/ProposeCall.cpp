#include "ProposeCall.h"

ProposeCall::ProposeCall(lab2::RaftService::AsyncService* service,
                         grpc::ServerCompletionQueue* cq,
                         Raft* raft)
    : service_(service),
      cq_(cq),
      responder_(&ctx_),
      raft_(raft),
      state_(State::CREATE) {
    proceed(true);
}

void ProposeCall::proceed(bool ok) {
    if (!ok) {
        delete this;
        return;
    }

    if (state_ == State::CREATE) {
        state_ = State::PROCESS;

        service_->Requestpropose(
            &ctx_, &req_, &responder_,
            cq_, cq_,
            this);
        return;
    }

    if (state_ == State::PROCESS) {
        // 先续命：保证持续接收新 RPC
        new ProposeCall(service_, cq_, raft_);

        
        // 投递到 Raft(Asio)线程串行处理，避免锁
        raft_->post([this]() {
            raft_->handle_propose(req_, this);
        });
        return;
    }

    if (state_ == State::FINISH) {
        delete this;
        return;
    }
}

void ProposeCall::finish(const ProposeReply &rep){
    state_ = State::FINISH;
    responder_.Finish(rep, grpc::Status::OK, this);
}