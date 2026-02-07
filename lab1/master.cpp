#include "master.h"
#include <thread>
#include <chrono>
MasterServiceImpl::MasterServiceImpl(Master *master):master_(master){}

Status MasterServiceImpl::get_task(ServerContext *context, const Empty * req, Task * task){
    auto temp = master_->assign_task();
    if(temp==nullptr) return Status(grpc::StatusCode::NOT_FOUND, "No task available");
    task->CopyFrom(*temp);
    master_->schedule_task_timeout(temp);
    // std::thread([=](){
    //     std::this_thread::sleep_for(std::chrono::seconds(10)); //10s内worker没有完成任务的话，可以认为worker断开了或是worker比较慢
    //     if(!master_->task_is_finished(*temp)) master_->add_task(*temp); // 把任务分配给其他worker
    // }).detach();
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

Master::Master(int argc,char *argv[], int numReduce):numMap_(argc-1),numReduce_(numReduce),done_(false),
ioContext_(),workGuard_(boost::asio::make_work_guard(ioContext_)),
ioContextThread_([this](){ ioContext_.run(); }){
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

int32_t Master::numReduce() const{
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
    {
        std::lock_guard<std::mutex> lock(timersMutex_);
        auto it = timers_.find(get_timer_id(task));
        if (it != timers_.end()) {
            it->second->cancel();
            timers_.erase(it);
        }
    }
    if(task.tasktype() == lab1::Task_TaskType::Task_TaskType_MAP){
        std::lock_guard lock(mapTaskMtx_);
        if(finishedMapTaskId_.find(task.taskid()) == finishedMapTaskId_.end()){
            finishedMapTaskId_.insert(task.taskid());
            if(finishedMapTaskId_.size()==numMap_){
                std::lock_guard qlock(mtx_);
                std::cout<<"map task finished"<<std::endl;
                for(int i=0;i<numReduce_;++i){
                    Task temp;
                    for(int j=1;j<=numMap_;++j){
                        std::string file = "mr-" + std::to_string(j) + "-" + std::to_string(i);
                        temp.add_files(file);    
                    }
                    temp.set_taskid(i);
                    temp.set_tasktype(lab1::Task_TaskType::Task_TaskType_REDUCE);
                    tasks_.push(std::move(temp));
                }
            }   
        }
    }else{
        std::lock_guard lock(reduceTaskMtx_);
        finishedReduceTaskId_.insert(task.taskid());
        for(auto it : finishedReduceTaskId_){
                std::cout<< it <<' ';
            }
        std::cout<<std::endl;
        if(finishedReduceTaskId_.size()==numReduce_) {
            done_ = true;
            std::cout<< "all done"<<std::endl;
        }
    }
}

void Master::schedule_task_timeout(std::shared_ptr<Task> task){
    auto timer = std::make_unique<boost::asio::steady_timer>(
        ioContext_, boost::asio::chrono::seconds(10)
    );
    std::lock_guard lock(timersMutex_);
    auto timerId = get_timer_id(*task);
    timers_[timerId] = std::move(timer); // 将定时器存储起来，以便可以取消
    timers_[timerId]->async_wait([this, task](const boost::system::error_code & ec) {
        if(ec != boost::asio::error::operation_aborted) {
            if(!task_is_finished(*task)) add_task(*task);
            std::lock_guard lock(timersMutex_);
            timers_.erase(get_timer_id(*task));
        }
    });
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
    std::cout<<"master destruction"<<std::endl;
    ioContext_.stop();
    if(ioContextThread_.joinable()){
        ioContextThread_.join();
    }
}

void RunServer(const std::string& address, Master &master) {
    std::string server_address(address);
    MasterServiceImpl service(&master);
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::thread([&](){
        while(!master.done()){
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        server->Shutdown();
    }).detach();
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

std::string get_timer_id(const Task &task){
    if(task.tasktype()==lab1::Task_TaskType::Task_TaskType_MAP){
        return "map" + std::to_string(task.taskid());
    }else return "reduce" + std::to_string(task.taskid());
}

int main(int argc, char *argv[]){
    Master master(argc, argv, 10);
    RunServer("127.0.0.1:5005",master);
}