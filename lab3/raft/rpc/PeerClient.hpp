#pragma once
#include <string>
#include <memory>
#include "grpcpp/grpcpp.h"
#include "lab3/raftService.grpc.pb.h"
using grpc::Channel;
struct PeerClient {
	std::string addr_;
  	std::shared_ptr<grpc::Channel> channel_;
	std::unique_ptr<raftService::RaftService::Stub> stub_;
	PeerClient(const std::string& addr, std::shared_ptr<grpc::Channel> channel, 
    	std::unique_ptr<raftService::RaftService::Stub> stub):
	addr_(addr),channel_(channel),stub_(std::move(stub)){}
};