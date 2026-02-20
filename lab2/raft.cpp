#include "raft.h"
#include "grpcpp/grpcpp.h"
#include "RaftRpcClient.h"
#include "PeerClient.hpp"
#include "ProposeCall.h"
#include <functional>


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
    set_current_term_(current_term_+1);
    become_candidate();
    start_election();
}

void Raft::handle_heart_beat_timeout(const boost::system::error_code & ec){
    if(ec == boost::asio::error::operation_aborted) return;
    send_append_entries();
    restart_heart_beat_timer();
}

void Raft::start_election(){
    std::cout << "start election term: " << current_term_ << std::endl;
    RequestVoteRequest req;
    req.set_term(current_term_);
    req.set_candidate_id(me_);
    req.set_last_log_index(last_log_index_());
    req.set_last_log_term(last_log_term_());
    rpc_->send_request_vote_to_all(std::move(req));
}

void Raft::send_append_entries(){
    for(int peer_id = 0; peer_id < num_peers_; ++peer_id){
        if(peer_id == me_) continue;
        if(in_flight_[peer_id]) continue;
        send_append_entries_with_id(peer_id);
        dirty_[peer_id] = false;
    }
}

void Raft::send_append_entries_with_id(int peer_id){
    AppendEntriesRequest req;
    req.set_term(current_term_);
    req.set_leader_id(leader_id_);
    auto prev_log_index = next_index_[peer_id]  - 1;
    req.set_prev_log_index(prev_log_index);
    req.set_prev_log_term(entries_[prev_log_index].term());
    for(int i = next_index_[peer_id]; i < entries_.size(); ++i){
        req.add_entries()->CopyFrom(entries_[i]);
    }
    req.set_leader_commit(commit_index_);
    in_flight_[peer_id] = true;
    rpc_->send_append_entries_with_id(std::move(req),peer_id,entries_.size()-1);
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
    set_voted_for_(me_);
    restart_election_timer();
}

void Raft::become_leader(){
    election_timer_.cancel();
    std::cout<< "i am leader" <<std::endl;
    role_ = RaftRole::LEADER;
    leader_id_ = me_;
    const uint64_t next = last_log_index_() + 1;
    next_index_.assign(num_peers_, next);
    match_index_.assign(num_peers_, 0);
    in_flight_.assign(num_peers_, false);
    dirty_.assign(num_peers_, false);
    send_append_entries();
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

void Raft::set_current_term_(uint64_t term){
    current_term_ = term;
    persister_->persist_current_term(current_term_);
}

void Raft::set_voted_for_(int voted_for){
    voted_for_ = voted_for;
    persister_->persist_voted_for(voted_for_);
}

void Raft::apply_command_to_state_machine_(const std::string& command){
    // to do
}


void Raft::truncate_from_(int index){
    int last_index = static_cast<int>(entries_.size()) -1;
    if(index <= 0) index = 1;
    if(index > last_index) return;

    std::streamoff new_size = 0;
    if(index == 1){
        new_size = 0;
    }else{
        auto it = offset_.find(index);
        if(it == offset_.end()) return;
        new_size = it->second;
    }

    persister_->truncate_log_to(new_size);
    entries_.resize(index);
    for(auto it = offset_.begin(); it!=offset_.end();){
        if(it->first >= index) it = offset_.erase(it);
        else ++it;
    }
}

uint64_t Raft::last_log_index_() const noexcept{
    return entries_.empty()? 0ULL : static_cast<uint64_t>(entries_.size() - 1);
}

uint64_t Raft::last_log_term_() const noexcept{
    return entries_.empty() ? 0ULL : static_cast<uint64_t>(entries_.back().term());
}

void Raft::apply_committed_entries_() {
    while (last_applied_ < commit_index_) {
        ++last_applied_;
        apply_command_to_state_machine_(entries_[last_applied_].command());
    }
}

// leader 推进 commit_index_：取 matchIndex 的“多数派位置”，并且只提交 current_term 的日志
void Raft::advance_commit_index_() {
    if (role_ != RaftRole::LEADER) return;

    std::vector<uint64_t> m = match_index_;
    m[me_] = last_log_index_();

    const size_t n = m.size();
    const size_t majority_cnt = n / 2 + 1;
    const size_t pos = n - majority_cnt; // 第 majority_cnt 大的元素 = ascending 的 index (n-majority_cnt)

    std::nth_element(m.begin(), m.begin() + pos, m.end());
    const uint64_t cand = m[pos];

    if (cand > commit_index_ && entries_[cand].term() == current_term_) {
        commit_index_ = cand;
        apply_committed_entries_();
        respond_committed_proposes_();
    }
}

void Raft::respond_committed_proposes_() {
    while (!propose_calls_.empty() && propose_calls_.front().first <= commit_index_) {
        auto [idx, call] = propose_calls_.front();
        propose_calls_.pop_front();

        ProposeReply rep;
        rep.set_term(current_term_);
        rep.set_wrong_leader(false);
        rep.set_leader_id(me_);
        rep.set_index(static_cast<uint64_t>(idx));
        call->finish(rep);
    }
}

Raft::Raft(std::vector<std::string> peer_address, int me, boost::asio::io_context& io_context):
        me_(me),
        num_peers_(peer_address.size()),
        io_context_(io_context),
        election_timer_(io_context_),
        heart_beat_timer_(io_context_,std::chrono::milliseconds(300)),
        rd_(),gen_(rd_()),distrib_(1000,1100),
        rpc_(std::make_unique<RaftRpcClient>(peer_address,me,this)),
        persister_(std::make_unique<Persister>(me))
{
    rpc_->start();
    persister_->init(offset_, entries_);
    persister_->read_current_term(&current_term_);
    persister_->read_voted_for(&voted_for_);
    gained_vote_ = 0;
    leader_id_ = -1;
    commit_index_ = 0;
    last_applied_ = 0;
    role_ = RaftRole::FOLLOWER;
    restart_election_timer();
}

Raft::~Raft(){
    rpc_->stop();
}

std::pair<int,bool> Raft::get_state(){
    return {static_cast<int>(current_term_), role_ == RaftRole::LEADER};
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
    rep.set_term(current_term_);
    rep.set_success(false);
    if(req.term() < current_term_) return rep;

    if(req.term()>current_term_){
        set_current_term_(req.term());
        become_follower();
    }else if(role_ == RaftRole::CANDIDATE){
        become_follower();
    }else{
        restart_election_timer();
    }

    leader_id_ = req.leader_id();

    // prev log 一致性检查
    int last_index = static_cast<int>(entries_.size()) - 1;
    int prev_log_index = req.prev_log_index();
    if(prev_log_index > last_index) return rep;
    
    
    if(prev_log_index!=0 && entries_[prev_log_index].term() != req.prev_log_term())
        return rep;

    const int local_start = prev_log_index + 1;
    const int n = req.entries_size();

    int k = 0;

    while(k < n){
        int local_idx = local_start + k;
        if(local_idx > last_index) break;
        if(entries_[local_idx].term() != req.entries(k).term()) break;
        ++k;
    }

    int conflict_index =local_start + k;
    if(k < n && conflict_index <= last_index){
        truncate_from_(conflict_index);
        last_index = static_cast<int>(entries_.size()) -1;
    }

    for(int j = k; j < n; ++j){
        std::streamoff pos = 0;
        if(!persister_->append_entry(req.entries(j),&pos)){
            return rep;
        }
        int new_index = static_cast<int>(entries_.size());
        offset_[new_index] = pos;
        entries_.push_back(req.entries(j));
    }

    uint64_t leader_commit = req.leader_commit();
    uint64_t my_last = last_log_index_();
    if(leader_commit > commit_index_){
        commit_index_ = std::min(leader_commit, my_last);
        apply_committed_entries_();
    }
    rep.set_success(true);
    return rep;
}

RequestVoteReply Raft::handle_request_vote(const RequestVoteRequest& req){
    std::cout << "term " << current_term_ << ": handle request vote term " << req.term() <<std::endl;
    RequestVoteReply rep;
    if(req.term()>current_term_){
        set_current_term_(req.term());
        become_follower();
        set_voted_for_(req.candidate_id());
        rep.set_term(current_term_);
        rep.set_vote_granted(true);
        return rep;
    }
    if(req.term() < current_term_){
        rep.set_term(current_term_);
        rep.set_vote_granted(false);
        return rep;
    }
    if((voted_for_ == req.candidate_id() || voted_for_ == -1) && (req.last_log_term()>last_log_term_() 
        || (req.last_log_term() == last_log_term_() && req.last_log_index() >= last_log_index_()))){
        set_voted_for_(req.candidate_id());
        rep.set_vote_granted(true);
    }else{
        rep.set_vote_granted(false);
    }
    rep.set_term(current_term_);
    return rep;
}

void Raft::on_append_entries_success(const AppendEntriesRequest& req, const AppendEntriesReply& rep, int peer_id, uint64_t last_index){
    std::cout << "term " << current_term_ << ": on append entries term " << rep.term() << " from " << peer_id <<std::endl;
    if(rep.term()>current_term_){
        set_current_term_(rep.term());
        become_follower();
        return;
    }
    if(rep.term()<current_term_) return;
    if(rep.success()){
        next_index_[peer_id] = last_index + 1;
        match_index_[peer_id] = last_index;
        advance_commit_index_();
        in_flight_[peer_id] = false;
    }else{
        next_index_[peer_id] = std::max<uint64_t>(1, next_index_[peer_id] - 1);
        dirty_[peer_id] = true;
    }
    return;
}

void Raft::on_append_entries_timeout(int peer_id){
    in_flight_[peer_id] = false;
    dirty_[peer_id] = true;
    send_append_entries_with_id(peer_id);
}

void Raft::on_append_entries_fail(int peer_id){
    in_flight_[peer_id] = false;
    dirty_[peer_id] = true;
}

void Raft::on_request_vote_reply(const RequestVoteReply& rep, int peer_id){
    std::cout << "term " << current_term_ << ": on request vote term " << rep.term() << " from " << peer_id <<std::endl;
    if(rep.term() > current_term_){
        set_current_term_(rep.term());
        become_follower();
        return;
    }
    if(role_ != RaftRole::CANDIDATE) return;
    if(rep.term() < current_term_) return;
    if(rep.vote_granted()) ++gained_vote_;
    if(gained_vote_ > num_peers_/2) become_leader();
    return;
}

void Raft::handle_propose(const ProposeRequest& req, ProposeCall *call){
    if(role_ != RaftRole::LEADER){
        ProposeReply rep;
        rep.set_wrong_leader(true);
        rep.set_leader_id(leader_id_);
        call->finish(rep);
        return;
    }
    if(!req.wait_for_commit()){
        ProposeReply rep;
        rep.set_wrong_leader(false);
        rep.set_leader_id(leader_id_);
        rep.set_term(current_term_);
        call->finish(rep);
        return;
    }
    Entry entry;
    entry.set_term(current_term_);
    entry.set_command(req.command());

    std::streamoff pos = 0;
    persister_->append_entry(entry, &pos);
    const uint64_t new_index = static_cast<uint64_t>(entries_.size());
    offset_[static_cast<int>(new_index)] = pos;

    match_index_[me_] = last_log_index_();
    entries_.push_back(std::move(entry));
    send_append_entries();

    propose_calls_.push_back({new_index,call});
}