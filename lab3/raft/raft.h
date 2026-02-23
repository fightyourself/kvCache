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
#include "Persister.h"
#include "lab3/raftService.grpc.pb.h"
using Timer = boost::asio::steady_timer;
using raftService::AppendEntriesRequest;
using raftService::AppendEntriesReply;
using raftService::RequestVoteRequest;
using raftService::RequestVoteReply;
class RaftRpcServer;
class RaftRpcClient;
class Persister;
class ProposeCall;
using pair_int_propose_call =  std::pair<int, ProposeCall*>;
class Raft{
public:
    Raft(std::vector<std::string> peer_address, int me);
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
    void start(const std::string command);

    void run();
private:
    void handle_election_timeout_(const boost::system::error_code & ec);
    void handle_heart_beat_timeout_(const boost::system::error_code & ec);
    void start_election_();
    void send_append_entries_();
    void send_append_entries_with_id_(int peer_id);

    void become_follower_();
    void become_candidate_();
    void become_leader_();

    void restart_election_timer_();
    void restart_heart_beat_timer_();

    void set_current_term_(uint64_t term);
    void set_voted_for_(int voted_for);

    // log entry
    void truncate_from_(int index);
    uint64_t last_log_index_() const noexcept;
    uint64_t last_log_term_()  const noexcept;


    void apply_committed_entries_();        // apply (last_applied_, commit_index_]
    void advance_commit_index_();           // leader 计算多数派提交点并推进 commit_index_
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
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_Guard_;
    Timer election_timer_;
    Timer heart_beat_timer_;
    std::unique_ptr<RaftRpcServer> server_;
    std::unique_ptr<RaftRpcClient> client_;
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_int_distribution<int> distrib_;
    std::unique_ptr<Persister> persister_;
    std::unordered_map<int,std::streamoff> offset_;
    std::vector<Entry> entries_;
};

