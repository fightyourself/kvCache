#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <queue>
#include <mutex>
#include <vector>
#include <atomic>
#include <boost/asio.hpp>
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
    boost::asio::io_context ioContext_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard_;
    std::thread ioContextThread_;
    std::mutex timersMutex_; // 保护 timers
    std::unordered_map<std::string, std::unique_ptr<boost::asio::steady_timer>> timers_;
    std::vector<std::string> mapTasks_;
    std::mutex mapTaskMtx_;
    std::unordered_set<int> finishedMapTaskId_;
    std::mutex reduceTaskMtx_;
    std::unordered_set<int> finishedReduceTaskId_;
    std::mutex mtx_;
    std::queue<Task> tasks_;
    int numMap_;
    int numReduce_;
    std::atomic<bool> done_;
public:
    Master(int argc, char *argv[], int numReduce);
    bool done();
    int32_t numReduce() const;
    std::shared_ptr<Task> assign_task();
    void add_task(const Task& task);
    void handle_task_done(const Task &task);
    void schedule_task_timeout(std::shared_ptr<Task> task);
    bool task_is_finished(const Task &task);
    ~Master();
};


void RunServer(const std::string& address, Master &master);
std::string get_timer_id(const Task &task);