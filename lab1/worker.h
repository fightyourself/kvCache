#pragma once
#include <memory>
#include <utility>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>
#include <nlohmann/json.hpp>
#include "grpcpp/grpcpp.h"
#include "lab1/lab1.grpc.pb.h"
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using Task = lab1::Task;
using Empty = lab1::Empty;
using GetNumReduceRep = lab1::GetNumReduceRep;
using namespace std;
extern vector<pair<string,string>> map_func(const string &filename,const string &content);
extern string reduce_func(const string &key,const vector<string> &values);

class Worker{
private:
    std::unique_ptr<lab1::Master::Stub> stub_;
    int numReduce_;
    void execute_map_task(const std::string &filename, int taskId);
    void execute_reduce_task(const std::vector<std::string> &filenames, int taskId);
    void execute_task(const Task &task);
    bool notify_finished_task(const Task &task);
public:
    Worker(std::shared_ptr<Channel> channel);
    
    void run();
};
