#include "RequestVoteCall.h"
#include "raft.h"
RequestVoteCall::RequestVoteCall(lab2::RaftService::AsyncService *service,
                grpc::ServerCompletionQueue *cq,
                Raft *raft):
                service_(service),
                cq_(cq),
                responder_(&ctx_),
                raft_(raft),
                state_(State::CREATE){
    Proceed(true);
}


void RequestVoteCall::Proceed(bool ok){
    if (!ok) {
        // ok=false 常见于：客户端取消、server shutdown
        delete this;
        return;
    }

    if (state_ == State::CREATE) {
        state_ = State::PROCESS;

        service_->Requestrequest_vote(
            &ctx_, &req_, &responder_,
            cq_, cq_,
            this);
        return;
    }

    if (state_ == State::PROCESS) {
        // 关键：立刻创建下一个 handler，保证服务持续可接收新 RPC
        new RequestVoteCall(service_, cq_, raft_);

        // 把处理投递到 Raft(Asio)线程，避免锁
        
        raft_->post([this]() {
            rep_ = raft_->handle_request_vote(req_);
            state_ = State::FINISH;
            responder_.Finish(rep_, grpc::Status::OK, this);
        });
        return;
    }

    // FINISH：Finish 完成事件回到 CQ，此时可以销毁
    if (state_ == State::FINISH) {
        delete this;
        return;
    }
}


