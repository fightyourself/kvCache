#pragma once
#include<memory>
#include "grpcpp/grpcpp.h"
#include "lab1/lab1.grpc.pb.h"
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using Task = lab1::Task;
using Empty = lab1::Empty;
using GetNumReduceRep = lab1::GetNumReduceRep;
class Worker{
private:
    std::unique_ptr<lab1::Master::Stub> stub_;
    int numReduce_;
public:
    Worker(std::shared_ptr<Channel> channel);
    void run();
};
