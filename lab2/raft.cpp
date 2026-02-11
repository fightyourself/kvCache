#include "raft.h"
#include "grpcpp/grpcpp.h"
#include "PeerClient.hpp"
// #include "peerClient.hpp"
#include <functional>
#include <thread>
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ClientContext;
using grpc::Status;
using grpc::CompletionQueue;
using grpc::ClientAsyncResponseReader;
using lab2::AppendEntriesRequest;
using lab2::AppendEntriesReply;
using lab2::RequestVoteRequest;
using lab2::RequestVoteReply;

// 为rpc请求设置超时
void rpc_set_timeout(ClientContext* context, int ms){
    std::chrono::time_point<std::chrono::system_clock> deadline =
        std::chrono::system_clock::now() + std::chrono::milliseconds(ms);
    context->set_deadline(deadline);
}

void Raft::handle_heart_beat_timeout(const boost::system::error_code & ec){
    if(ec!=boost::asio::error::operation_aborted){
        ++current_term_;
        role_ = RaftRole::CANDIDATE;
        restart_election_timer();
        start_election();
    }
}

void Raft::start_election(){
    std::cout << "start election. term: " << current_term_ << std::endl;
    auto cq = std::make_unique<CompletionQueue>();
    struct AsyncClientCall {
        RequestVoteReply reply;
        ClientContext context;
        grpc::Status status;
        int id; // 用于标识请求
    };
    std::vector<std::unique_ptr<AsyncClientCall>> calls;
    for(int i = 0; i< peers_.size(); ++i){
        if(me_!=i){
            auto call = std::make_unique<AsyncClientCall>();
            call->id = i;
            rpc_set_timeout(&call->context,100); // 为每个rpc设置超时，以免cq.next永远阻塞
            RequestVoteRequest request;
            request.set_term(current_term_);
            request.set_candidate_id(me_);
            request.set_last_log_index(0);
            request.set_last_log_term(0);
            auto stub = peers_[i]->stub_.get();
            auto rpc = stub->Asyncrequest_vote(&call->context,request,cq.get());
            rpc->Finish(&call->reply,&call->status,reinterpret_cast<void*>(call.get()));
            calls.push_back(std::move(call));
        }
    }

    std::thread([calls = std::move(calls), cq = std::move(cq), this](){ //calls = std::move(calls)将AsyncClientCall生命周期延长到该线程
        void* tag;
        bool ok;
        int completed = 0;
        int gained_vote = 0;
        while(completed<calls.size()){
            cq->Next(&tag,&ok);
            ++completed;
            if(!ok){
                std::cout<<"RPC failed!"<<std::endl;
                continue;    
            }
            AsyncClientCall* call = reinterpret_cast<AsyncClientCall*>(tag);
            if(call->status.ok()){
                if(call->reply.vote_granted()){
                    ++gained_vote;
                }else if(call->reply.term()>current_term_){
                    become_follower();
                    restart_election_timer();
                    return;
                }
            }else{
                std::cout << "call " << call->id << " failed\n";
            }
            if(gained_vote>=peers_.size()/2){
                become_leader();
                return;
            }
        }
        // std::cout<< "thread exit" <<std::endl;
    }).detach();
    
}

void Raft::become_follower(){
    role_ = RaftRole::FOLLOWER;
    // to do;
    
}

void Raft::become_leader(){
    election_timer_.cancel();
    role_ = RaftRole::LEADER;
    struct AsyncClientCall {
        AppendEntriesReply reply;
        ClientContext context;
        grpc::Status status;
        int id; // 用于标识请求
    };
    auto cq = std::make_unique<CompletionQueue>();
    std::vector<std::unique_ptr<AsyncClientCall>> calls;
    for(int i = 0; i<peers_.size(); ++i){
        if(i != me_){
            auto call = std::make_unique<AsyncClientCall>();
            call->id = i;
            rpc_set_timeout(&call->context,100);
            AppendEntriesRequest request;
            request.set_term(current_term_);
            request.set_leader_id(me_);
            request.set_prev_log_index(0);
            request.set_prev_log_term(0);
            request.set_leader_commit(0);
            auto stub = peers_[i]->stub_.get();
            auto rpc = stub->Asyncappend_entries(&call->context,request,cq.get());
            rpc->Finish(&call->reply, &call->status, reinterpret_cast<void*>(call.get()));
        }
    }

    std::thread([calls = std::move(calls), cq = std::move(cq), this](){
        void *tag;
        bool ok;
        int completed = 0;
        while(completed<calls.size()){
            cq->Next(&tag, &ok);
            ++completed;
            if(!ok){
                std::cout<<"RPC failed!"<<std::endl;
                continue;
            }
            AsyncClientCall *call = reinterpret_cast<AsyncClientCall*>(tag);
            if(call->status.ok()){
                if(call->reply.term()>current_term_){
                    become_follower();
                    return;
                }
            }else{
                std::cout<<"RPC failed" << std::endl;
            }
        }

    }).detach();
}


void Raft::restart_election_timer(){
    election_timer_.expires_from_now(std::chrono::milliseconds(distrib_(gen_)));
    election_timer_.async_wait(std::bind(&Raft::handle_heart_beat_timeout,this,std::placeholders::_1));
}


Raft::Raft(std::vector<std::string> peer_address, int me, boost::asio::io_context& io_context):
me_(me),io_context_(io_context),
election_timer_(io_context_),heart_beat_timer(io_context_),rd_(),gen_(rd_()),distrib_(150,200)
{
    for(auto &addr:peer_address){
        auto channel = grpc::CreateChannel(addr,
             grpc::InsecureChannelCredentials());
        auto stub = lab2::RaftService::NewStub(channel);
        peers_.push_back(std::move(std::make_unique<PeerClient>(addr,channel,std::move(stub))));
    }
    current_term_ = 0; // 测试初始情况，后续需要从持久化数据中读出来
    role_ = RaftRole::FOLLOWER;
    election_timer_.expires_from_now(std::chrono::milliseconds(distrib_(gen_)));
    election_timer_.async_wait(std::bind(&Raft::handle_heart_beat_timeout,this,
        std::placeholders::_1));
}

Raft::~Raft(){

}

std::pair<int,bool> Raft::get_state(){
    // to do
    return {0,false};
}

void Raft::persist() {
    // to do
}

void Raft::read_persist() {
    // to do

}

void Raft::post(std::function<void()>fn){
    io_context_.post(std::move(fn));
}



