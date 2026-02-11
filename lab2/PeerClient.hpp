#pragma once
#include <string>
#include <memory>
#include "grpcpp/grpcpp.h"
#include "raft.h"
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
struct PeerClient {
	std::string addr_;
  	std::shared_ptr<grpc::Channel> channel_;
	std::unique_ptr<lab2::RaftService::Stub> stub_;
	PeerClient(const std::string& addr, std::shared_ptr<grpc::Channel> channel, 
    	std::unique_ptr<lab2::RaftService::Stub> stub):
	addr_(addr),channel_(channel),stub_(std::move(stub)){}
};