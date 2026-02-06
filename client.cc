#include <iostream>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "helloworld.grpc.pb.h"
#include <boost/asio.hpp>
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class GreeterClient {
private:
    std::unique_ptr<Greeter::Stub> stub_;
public:
    GreeterClient(std::shared_ptr<Channel> channel)
        : stub_(Greeter::NewStub(channel)) {}
    std::string SayHello(const std::string &user){
        HelloRequest req;
        req.set_name(user);
        HelloReply rep;
        ClientContext context;
        Status status = stub_->SayHello(&context,req,&rep);
        if(status.ok()) {
            return rep.message();
        }else {
            std::cout << status.error_code() << ": " << status.error_message()
            << std::endl;
            return "RPC failed";
        }
    }
};

int main(){
    boost::asio::io_context context;
    // GreeterClient greeter(grpc::CreateChannel("127.0.0.1:5005",
    //     grpc::InsecureChannelCredentials()
    // ));
    // std::string user = "xuzhiwei";
    // std::string rep = greeter.SayHello(user);
    // std::cout << "Greeter received: " << rep << std::endl;
    
    return 0;
}