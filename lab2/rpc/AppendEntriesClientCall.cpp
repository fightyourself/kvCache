#include "AppendEntriesClientCall.h"
#include "raft.h"
AppendEntriesClientCall::AppendEntriesClientCall(lab2::RaftService::Stub *stub, CompletionQueue *cq,Raft *raft, int id):
        stub_(stub),
        cq_(cq),
        raft_(raft),
        id_(id){

}


void AppendEntriesClientCall::start(const AppendEntriesRequest& req){
    req_ = req;
    ctx_.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(100));
    rpc_ = stub_->Asyncappend_entries(&ctx_, req_, cq_);
    rpc_->Finish(&rep_, &status_, this);
}

void AppendEntriesClientCall::proceed(bool ok){
    if(!ok){
        delete this;
        return;
    }
    if(status_.ok()){
        raft_->post([raft=raft_,rep=rep_,id=id_](){
            raft->on_append_entries_reply(std::move(rep), id);
        });
    }else if(status_.error_code() == grpc::DEADLINE_EXCEEDED){
        std::cout << "RPC timeout" << std::endl;
    }else {
        std::cout << "[AE done] peer=" << id_
                  << " code=" << status_.error_code() << std::endl;
    }
    delete this;
}