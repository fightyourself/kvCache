#pragma once
#include <vector>
#include <string>
#include <utility>
#include <boost/asio.hpp>
#include <chrono>
#include <random>
#include <functional>
#include <unordered_map>
#include <deque>
#include "raftRole.hpp"
#include "lab2/lab2.grpc.pb.h"
#include "RaftRpcClient.h"
#include "Persister.h"
using Timer = boost::asio::steady_timer;
using lab2::AppendEntriesRequest;
using lab2::AppendEntriesReply;
using lab2::RequestVoteRequest;
using lab2::RequestVoteReply;
using lab2::ProposeRequest;
using lab2::ProposeReply;
class RaftRpcClient;
class Persister;
class ProposeCall;
using pair_int_propose_call =  std::pair<int, ProposeCall*>;
class Raft{
public:
    Raft(std::vector<std::string> peer_address, int me, boost::asio::io_context& io_context);
    ~Raft();

public:
    std::pair<int, bool> get_state();
    void persist(); // 持久化
    void read_persist(); // 读取持久化
    void post(std::function<void()>fn);
    
    AppendEntriesReply handle_append_entries(const AppendEntriesRequest& req);
    RequestVoteReply handle_request_vote(const RequestVoteRequest& req);
    void on_append_entries_success(const AppendEntriesRequest& req,const AppendEntriesReply& rep, int peer_id, uint64_t last_index);
    void on_append_entries_timeout(int peer_id);
    void on_append_entries_fail(int peer_id);
    void on_request_vote_reply(const RequestVoteReply& rep, int peer_id);
    // 客户端rpc请求
    void handle_propose(const ProposeRequest& req,ProposeCall *call);
private:
    void handle_election_timeout(const boost::system::error_code & ec);
    void handle_heart_beat_timeout(const boost::system::error_code & ec);
    void start_election();
    void send_append_entries();
    void send_append_entries_with_id(int peer_id);

    void become_follower();
    void become_candidate();
    void become_leader();

    void restart_election_timer();
    void restart_heart_beat_timer();

    void set_current_term_(uint64_t term);
    void set_voted_for_(int voted_for);

    // 回复客户端请求
    void apply_command_to_state_machine_(const std::string& command);

    // log entry
    void truncate_from_(int index);

    uint64_t last_log_index_() const noexcept;
    uint64_t last_log_term_()  const noexcept;


    void apply_committed_entries_();        // apply (last_applied_, commit_index_]
    void advance_commit_index_();           // leader 计算多数派提交点并推进 commit_index_
    void respond_committed_proposes_();     // 按 commit_index_ 响应等待提交的 propose
private:
    uint64_t current_term_;
    int voted_for_;
    int gained_vote_;
    int num_peers_;
    std::vector<std::string> logs_;
    uint64_t commit_index_;
    uint64_t last_applied_;
    RaftRole role_;
    int me_;
    int leader_id_;
    std::vector<uint64_t> next_index_;
    std::vector<uint64_t> match_index_;
    std::vector<bool> in_flight_;
    std::vector<bool> dirty_;
    boost::asio::io_context& io_context_;
    Timer election_timer_;
    Timer heart_beat_timer_;
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_int_distribution<int> distrib_;
    std::unique_ptr<RaftRpcClient> rpc_;
    std::unique_ptr<Persister> persister_;
    std::unordered_map<int,std::streamoff> offset_;
    std::vector<Entry> entries_;
    std::deque<pair_int_propose_call> propose_calls_;
};

