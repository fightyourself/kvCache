#include <vector>
#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include "raft.h"
#include "RaftRpcServer.h"
int main(int argc, char *argv[]){
    if(argc!=2){
        std::cout<< "usage: ./main [id]" <<std::endl;
        return 0;
    }
    boost::asio::io_context io_context;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_Guard_(boost::asio::make_work_guard(io_context));
    std::vector<std::string> addrs{"127.0.0.1:5000","127.0.0.1:5001","127.0.0.1:5002"};
    int me = atoi(argv[1]);
    Raft raft(addrs,me,io_context);
    RaftRpcServer server(&raft);
    server.start(addrs[me]);
    
    io_context.run();
    return 0;
}