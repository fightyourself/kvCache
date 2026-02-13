#pragma once
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include "lab2/lab2.grpc.pb.h"
#include "AppendEntriesClientCall.h"
#include "grpcpp/grpcpp.h"
using grpc::CompletionQueue;
using lab2::AppendEntriesRequest;
using lab2::AppendEntriesReply;
using lab2::RequestVoteRequest;
using lab2::RequestVoteReply;
class PeerClient;
class Raft;
class RaftRpcClient{
public:
    RaftRpcClient(const std::vector<std::string>& peer_address, int me, Raft *raft);
    ~RaftRpcClient();
    void send_append_entries_to_all(const AppendEntriesRequest& req);
    void send_request_vote_to_all(const RequestVoteRequest& req);
    void start();
    void stop();
    
private:
    void pool_cp_loop();
private:
    std::vector<std::unique_ptr<PeerClient>> peers_;
    std::vector<std::unique_ptr<lab2::RaftService::Stub>> stub_;
    int me_;
    std::unique_ptr<CompletionQueue> cq_;
    std::thread cq_thread_;
    Raft *raft_;
};