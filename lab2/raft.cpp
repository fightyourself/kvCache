#include "raft.h"
#include "grpcpp/grpcpp.h"
#include "RaftRpcClient.h"
#include "PeerClient.hpp"
#include <functional>
#include <thread>

// 为rpc请求设置超时
void rpc_set_timeout(ClientContext* context, int ms){
    std::chrono::time_point<std::chrono::system_clock> deadline =
        std::chrono::system_clock::now() + std::chrono::milliseconds(ms);
    context->set_deadline(deadline);
}

void Raft::handle_election_timeout(const boost::system::error_code & ec){
    if(ec == boost::asio::error::operation_aborted){
        std::cout << "Election timer cannelled" << std::endl;
        return;
    }
    // std::cout << "gained vote: " << gained_vote_ <<std::endl;
    ++current_term_;
    become_candidate();
    start_election();
}

void Raft::handle_heart_beat_timeout(const boost::system::error_code & ec){
    if(ec == boost::asio::error::operation_aborted) return;
    send_heart_beat();
    restart_heart_beat_timer();
}

void Raft::start_election(){
    std::cout << "start election term: " << current_term_ << std::endl;
    RequestVoteRequest req;
    req.set_term(current_term_);
    req.set_candidate_id(me_);
    req.set_last_log_index(0);
    req.set_last_log_term(0);
    rpc_->send_request_vote_to_all(std::move(req));
}

void Raft::send_heart_beat(){
    AppendEntriesRequest req;
    req.set_term(current_term_);
    req.set_leader_id(me_);
    req.set_prev_log_index(0);
    req.set_prev_log_term(0);
    req.set_leader_commit(0);
    rpc_->send_append_entries_to_all(std::move(req));
}



void Raft::become_follower(){
    heart_beat_timer_.cancel();
    role_ = RaftRole::FOLLOWER;
    restart_election_timer();
}

void Raft::become_candidate(){
    heart_beat_timer_.cancel();
    role_ = RaftRole::CANDIDATE;
    gained_vote_ = 1;
    voted_for_ = me_;
    restart_election_timer();
}

void Raft::become_leader(){
    election_timer_.cancel();
    std::cout<< "i am leader" <<std::endl;
    role_ = RaftRole::LEADER;
    send_heart_beat();
    restart_heart_beat_timer();
}


void Raft::restart_election_timer(){
    election_timer_.expires_from_now(std::chrono::milliseconds(distrib_(gen_)));
    election_timer_.async_wait(std::bind(&Raft::handle_election_timeout,this,std::placeholders::_1));
}

void Raft::restart_heart_beat_timer(){
    heart_beat_timer_.expires_from_now(std::chrono::milliseconds(300));
    heart_beat_timer_.async_wait(std::bind(&Raft::handle_heart_beat_timeout,this,std::placeholders::_1));
}


Raft::Raft(std::vector<std::string> peer_address, int me, boost::asio::io_context& io_context):
        me_(me),
        num_peers_(peer_address.size()),
        io_context_(io_context),
        election_timer_(io_context_),
        heart_beat_timer_(io_context_,std::chrono::milliseconds(300)),
        rd_(),gen_(rd_()),distrib_(1000,1100),
        rpc_(std::move(std::make_unique<RaftRpcClient>(peer_address,me,this)))
{
    rpc_->start();
    current_term_ = 0; // 测试初始情况，后续需要从持久化数据中读出来
    voted_for_ = -1;
    gained_vote_ = 0;
    role_ = RaftRole::FOLLOWER;
    restart_election_timer();
}

Raft::~Raft(){
    rpc_->stop();
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

AppendEntriesReply Raft::handle_append_entries(const AppendEntriesRequest& req){
    std::cout << "term " << current_term_ << ": handle append entries term " << req.term() <<std::endl;
    AppendEntriesReply rep;
    if(req.term()>current_term_){
        current_term_ = req.term();
        become_follower();
        rep.set_success(true);
    }else if(req.term() < current_term_){
        rep.set_success(false);
    }else{
        rep.set_success(true);
        if(role_ == RaftRole::CANDIDATE) become_follower();
        else restart_election_timer();
    }
    rep.set_term(current_term_);
    return rep;
}

RequestVoteReply Raft::handle_request_vote(const RequestVoteRequest& req){
    std::cout << "term " << current_term_ << ": handle request vote term " << req.term() <<std::endl;
    RequestVoteReply rep;
    if(req.term()>current_term_){
        current_term_ = req.term();
        become_follower();
        voted_for_ = req.candidate_id();
        rep.set_vote_granted(true);
    }else if(req.term() < current_term_){
        rep.set_vote_granted(false);
    }else if(voted_for_ == req.candidate_id() || voted_for_ == -1){
        rep.set_vote_granted(true);
    }
    rep.set_term(current_term_);
    return rep;
}

void Raft::on_append_entries_reply(const AppendEntriesReply& rep, int peer_id){
    std::cout << "term " << current_term_ << ": on append entries term " << rep.term() << " from " << peer_id <<std::endl;
    if(rep.term()>current_term_){
        current_term_ = rep.term();
        become_follower();
        return;
    }
    if(rep.term()<current_term_) return;
    if(rep.success()){
        // to do;
    }
    return;
}

void Raft::on_request_vote_reply(const RequestVoteReply& rep, int peer_id){
    std::cout << "term " << current_term_ << ": on request vote term " << rep.term() << " from " << peer_id <<std::endl;
    if(rep.term() > current_term_){
        current_term_ = rep.term();
        become_follower();
        return;
    }
    if(rep.term() < current_term_) return;
    if(rep.vote_granted()) ++gained_vote_;
    if(gained_vote_ > num_peers_/2) become_leader();
    return;
}
