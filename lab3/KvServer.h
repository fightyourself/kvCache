#pragma once
#include <thread>
#include <unordered_map>
#include <string>
#include "grpcpp/grpcpp.h"
#include "lab3/kvService.grpc.pb.h"
using kvService::GetRequest;
using kvService::GetReply;
using kvService::PutAppendRequest;
using kvService::PutAppendReply;
class Raft;
class KvServer{
public:
    KvServer(const std::string& addr, Raft &raft);
    ~KvServer();

public:
    void start();
    void stop();

    // 处理请求
    void handle_get(const GetRequest& req);
    void handle_put_append(const PutAppendRequest& req);
private:
    void pool_cq_loop_();


private:
    kvService::KvService::AsyncService service_;
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    std::unique_ptr<grpc::Server> server_;
    std::thread cq_thread_;
    std::string addr_;

    Raft& raft_;
    std::unordered_map<std::string, std::string> kv_;
    std::unordered_map<int, int> last_seq_;
};