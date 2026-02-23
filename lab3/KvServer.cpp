#include "KvServer.h"
#include "CallDataBase.h"
KvServer::KvServer(const std::string& addr, Raft& raft):addr_(addr),raft_(raft){
    
}

KvServer::~KvServer(){

}

void KvServer::start(){
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    
    // CQ 消费循环（通常独立线程跑）
    // 为每个 RPC 类型预创建一个 CallData

    cq_thread_ = std::thread([this]() { pool_cq_loop_(); });
}

void KvServer::stop(){
    if (server_) server_->Shutdown();
    if (cq_) cq_->Shutdown();
    if (cq_thread_.joinable()) cq_thread_.join();
}

void KvServer::pool_cq_loop_(){
    void* tag = nullptr;
    bool ok = false;
    while (cq_->Next(&tag, &ok)) {
      static_cast<CallDataBase*>(tag)->proceed(ok);
    }
}