#pragma once
#include "grpcpp/grpcpp.h"
#include "lab2/lab2.grpc.pb.h"
#include <memory>
#include <thread>
class Raft;

class RaftRpcServer {
public:
  RaftRpcServer(Raft* raft);

  void start(const std::string& addr);

  void stop();

private:
  void pool_cp_loop();

private:
  Raft *raft_;

  lab2::RaftService::AsyncService service_;
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  std::unique_ptr<grpc::Server> server_;
  std::thread cq_thread_;
};
