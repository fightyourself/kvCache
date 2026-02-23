#pragma once
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include "lab3/raftService.grpc.pb.h"
#include "AppendEntriesClientCall.h"
#include "grpcpp/grpcpp.h"
using grpc::CompletionQueue;
using raftService::AppendEntriesRequest;
using raftService::AppendEntriesReply;
using raftService::RequestVoteRequest;
using raftService::RequestVoteReply;
class PeerClient;
class Raft;
class RaftRpcClient{
public:
    RaftRpcClient(const std::vector<std::string>& peer_address, int me, Raft *raft);
    ~RaftRpcClient();
    void send_request_vote_to_all(const RequestVoteRequest& req);
    void send_append_entries_with_id(const AppendEntriesRequest& req, int id, uint64_t last_index);
    void start();
    void stop();
    
private:
    void pool_cp_loop();
private:
    std::vector<std::unique_ptr<PeerClient>> peers_;
    std::vector<std::unique_ptr<raftService::RaftService::Stub>> stub_;
    int me_;
    std::unique_ptr<CompletionQueue> cq_;
    std::thread cq_thread_;
    Raft *raft_;
};