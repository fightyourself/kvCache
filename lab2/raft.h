#pragma once
#include <vector>
#include <string>
#include <utility>
#include <boost/asio.hpp>
#include <chrono>
#include <random>
#include <functional>
#include "raftRole.hpp"
#include "lab2/lab2.grpc.pb.h"
#include "RaftRpcClient.h"
using Timer = boost::asio::steady_timer;
using lab2::AppendEntriesRequest;
using lab2::AppendEntriesReply;
using lab2::RequestVoteRequest;
using lab2::RequestVoteReply;
class RaftRpcClient;
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
    void on_append_entries_reply(const AppendEntriesReply& rep, int peer_id);
    void on_request_vote_reply(const RequestVoteReply& rep, int peer_id);

private:
    void handle_election_timeout(const boost::system::error_code & ec);
    void handle_heart_beat_timeout(const boost::system::error_code & ec);
    void start_election();
    void send_heart_beat();

    void become_follower();
    void become_candidate();
    void become_leader();

    void restart_election_timer();
    void restart_heart_beat_timer();

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
    std::vector<uint64_t> next_index_;
    std::vector<uint64_t> match_index_;
    boost::asio::io_context& io_context_;
    Timer election_timer_;
    Timer heart_beat_timer_;
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_int_distribution<int> distrib_;
    std::unique_ptr<RaftRpcClient> rpc_;
};

