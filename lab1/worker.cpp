#include "worker.h"

Worker::Worker(std::shared_ptr<Channel> channel):stub_(lab1::Master::NewStub(channel)){}

void Worker::run() {
    ClientContext context;
    Status status;
    Empty req;
    {
        GetNumReduceRep rep;
        status = stub_->get_numReduce(&context,req,&rep);
        if(status.ok()){
            this->numReduce_ = rep.numreduce();
        }else return;
    }
    Task task;
    while(true){
        status = stub_->get_task(&context,req,&task);
        if(status.ok()){
        std::cout<<task.files_size()  <<std::endl;
        }else {
            std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
        }
    }
}


int main(){
    Worker w(grpc::CreateChannel("127.0.0.1:5005",
        grpc::InsecureChannelCredentials()
    ));
    w.run();
    return 0;
}