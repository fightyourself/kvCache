#pragma once
#include "grpcpp/grpcpp.h"
#include "lab3/raftService.grpc.pb.h"
#include <memory>
#include <thread>
class Raft;

class RaftRpcServer {
public:
  RaftRpcServer(const std::string& addr, Raft* raft);

  void start();

  void stop();

private:
  void pool_cp_loop();

private:
  Raft *raft_;
  std::string addr_;
  raftService::RaftService::AsyncService service_;
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  std::unique_ptr<grpc::Server> server_;
  std::thread cq_thread_;
};
