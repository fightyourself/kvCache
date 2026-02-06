#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <queue>
#include <mutex>
#include <vector>
#include <atomic>
#include "grpcpp/grpcpp.h"
#include "lab1/lab1.grpc.pb.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using Task = lab1::Task;
using Empty = lab1::Empty;
using GetNumReduceRep =lab1::GetNumReduceRep;

class Master;
class MasterServiceImpl final : public lab1::Master::Service{
public:
    MasterServiceImpl(Master *master);
    Status get_task(ServerContext *context, const Empty * req, Task * task) override;
    Status notify_finished_task(ServerContext *context, const Task *task, Empty * rep) override;
    Status get_numReduce(ServerContext *context, const Empty *req, GetNumReduceRep * rep) override;
private:
    Master *master_;
};

class Master{
private:
    std::vector<std::string> mapTasks_;
    std::mutex mapTaskMtx_;
    std::unordered_set<int> finishedMapTaskId_;
    std::mutex reduceTaskMtx_;
    std::unordered_set<int> finishedReduceTaskId_;
    // std::unordered_map<std::string,int> mapTasks_; // map任务的对应的文件名和对应的id，id是为了生成中间文件，比如：mr_x_y，x是mapid，y是reduceid
    std::unordered_map<std::string,int> reduceTask_; // reduce任务的对应的文件名和对应的id，id是为了生成结果文件，比如：mr_out_y, y是reduceid
    std::mutex mtx_;
    std::queue<Task> tasks_;
    int numMap_;
    int numReduce_;
    std::atomic<bool> done_;
public:
    Master(int argc, char *argv[], int numReduce);
    bool done();
    int numReduce() const;
    std::shared_ptr<Task> assign_task();
    void add_task(const Task& task);
    void handle_task_done(const Task &task);
    bool task_is_finished(const Task &task);
    ~Master();
};


void RunServer(const std::string& address);
