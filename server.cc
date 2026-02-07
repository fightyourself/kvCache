#include <iostream>
#include <string>
#include "helloworld.grpc.pb.h"
#include "grpcpp/grpcpp.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class GreeterServiceImpl final : public Greeter::Service{
public:
    Status SayHello(ServerContext *context,const HelloRequest * req, HelloReply *rep) override {
        std::string prefix("Hello ");
        rep->set_message(prefix + req->name());
        return Status::OK;
    }
};

void RunServer(const std::string& address) {
    std::string server_address(address);
    GreeterServiceImpl service;
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}



int main(){
    RunServer("127.0.0.1:5005");
}