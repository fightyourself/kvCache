#include "master.h"
#include <thread>
#include <chrono>
MasterServiceImpl::MasterServiceImpl(Master *master):master_(master){}

Status MasterServiceImpl::get_task(ServerContext *context, const Empty * req, Task * task){
    auto temp = master_->assign_task();
    if(temp==nullptr) return Status(grpc::StatusCode::NOT_FOUND, "No task available");
    task->CopyFrom(*temp);
    std::thread([=](){
        std::this_thread::sleep_for(std::chrono::seconds(10)); //10s内worker没有完成任务的话，可以认为worker断开了或是worker比较慢
        if(!master_->task_is_finished(*temp)) master_->add_task(*temp); // 把任务分配给其他worker
    }).detach();
    return Status::OK;
}

Status MasterServiceImpl::notify_finished_task(ServerContext *context, const Task *task, Empty * rep){
    master_->handle_task_done(*task);
    return Status::OK;
}

Status MasterServiceImpl::get_numReduce(ServerContext *context, const Empty *req, GetNumReduceRep * rep){
    rep->set_numreduce(master_->numReduce());
    return Status::OK;
}

Master::Master(int argc,char *argv[], int numReduce):numMap_(argc-1),numReduce_(numReduce),done_(false){
    for(int i=1;i<argc;++i){
        mapTasks_.push_back(std::string(argv[i]));
        Task task;
        task.set_taskid(i);
        task.set_tasktype(Task::TaskType::Task_TaskType_MAP);
        task.add_files(std::string(argv[i]));
        // std::cout<<task.files_size()<<std::endl;
        tasks_.push(task);
    }
}

bool Master::done() {
    return done_;
}

int Master::numReduce() const{
    return numReduce_;
}

std::shared_ptr<Task> Master::assign_task() {
    std::lock_guard lock(mtx_);
    if(tasks_.empty()) return nullptr;
    std::shared_ptr<Task> task(new Task(std::move(tasks_.front())));
    tasks_.pop();
    return task;
}

void Master::add_task(const Task& task){
    std::lock_guard lock(mtx_);
    tasks_.push(task);
}


void Master::handle_task_done(const Task& task){
    if(task.tasktype() == lab1::Task_TaskType::Task_TaskType_MAP){
        std::lock_guard lock(mapTaskMtx_);
        finishedMapTaskId_.insert(task.taskid());
        if(finishedMapTaskId_.size()==numMap_){
            std::lock_guard qlock(mtx_);
            for(int i=0;i<numReduce_;++i){
                Task tmep;
                for(int j=0;j<numMap_;++j){
                    std::string file = "mr_" + std::to_string(j) + "_" + std::to_string(i);
                    tmep.add_files(file);    
                }
                tmep.set_taskid(i);
                tmep.set_tasktype(lab1::Task_TaskType::Task_TaskType_MAP);
                tasks_.push(std::move(task));
            }
        }    
    }else{
        std::lock_guard lock(reduceTaskMtx_);
        finishedReduceTaskId_.insert(task.taskid());
        if(reduceTask_.size()==numReduce_) done_ = true;
    }
}

bool Master::task_is_finished(const Task &task){
    if(task.tasktype() == lab1::Task_TaskType::Task_TaskType_MAP){
        std::lock_guard lock(mapTaskMtx_);
        if(finishedMapTaskId_.find(task.taskid()) == finishedMapTaskId_.end()) return false;
    }else {
        std::lock_guard lock(reduceTaskMtx_);
        if(finishedReduceTaskId_.find(task.taskid()) == finishedReduceTaskId_.end()) return false;
    }
    return true;
}



Master::~Master(){

}

void RunServer(const std::string& address, Master &master) {
    std::string server_address(address);
    MasterServiceImpl service(&master);
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char *argv[]){
    Master master(argc, argv, 8);
    RunServer("127.0.0.1:5005",master);
}