#include "AppendEntriesClientCall.h"
#include "raft.h"
AppendEntriesClientCall::AppendEntriesClientCall(lab2::RaftService::Stub *stub, CompletionQueue *cq,Raft *raft, int id, uint64_t last_index):
        stub_(stub),
        cq_(cq),
        raft_(raft),
        id_(id),
        last_index_(last_index){

}


void AppendEntriesClientCall::start(const AppendEntriesRequest& req){
    req_ = req;
    ctx_.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(100));
    rpc_ = stub_->Asyncappend_entries(&ctx_, req_, cq_);
    rpc_->Finish(&rep_, &status_, this);
}

void AppendEntriesClientCall::proceed(bool ok){
    if(!ok){
        raft_->post([raft=raft_,id=id_](){
            raft->on_append_entries_fail(id);
        });
        delete this;
        return;
    }
    if(status_.ok()){
        raft_->post([raft=raft_,req=req_,rep=rep_,id=id_,last_index=last_index_](){
            raft->on_append_entries_success(std::move(req),std::move(rep), id,last_index);
        });
    }else if(status_.error_code() == grpc::DEADLINE_EXCEEDED){
        std::cout << "RPC timeout" << std::endl;
        raft_->post([raft=raft_,id=id_](){
            raft->on_append_entries_timeout(id);
        });
    }else {
        std::cout << "[AE done] peer=" << id_
                  << " code=" << status_.error_code() << std::endl;
        raft_->post([raft=raft_,id=id_](){
            raft->on_append_entries_fail(id);
        });
    }
    delete this;
}