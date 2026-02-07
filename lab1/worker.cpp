#include "worker.h"
#include <thread>
#include <fstream>
#include <algorithm>

using KV = std::pair<std::string, std::string>;

static inline uint32_t fnv1a_32(const std::string& s) {
    const uint32_t FNV_OFFSET = 2166136261u;
    const uint32_t FNV_PRIME  = 16777619u;
    uint32_t h = FNV_OFFSET;
    for (unsigned char c : s) {
        h ^= static_cast<uint32_t>(c);
        h *= FNV_PRIME;
    }
    return h;
}

static inline int ihash(const std::string& key) {
    return static_cast<int>(fnv1a_32(key) & 0x7fffffff);
}

// 在当前目录生成临时文件名（避免跨盘 rename 失败）
static std::string make_tmp_name(const std::string& prefix) {
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::steady_clock::now().time_since_epoch())
                  .count();
    return prefix + "-" + std::to_string(us) + ".tmp";
}

static void safe_rename_replace(const std::string& from, const std::string& to) {
    std::error_code ec;
    std::filesystem::rename(from, to, ec);
    if (!ec) return;

    // 某些平台目标存在会失败，先删再改名
    std::filesystem::remove(to, ec);
    ec.clear();
    std::filesystem::rename(from, to, ec);
    if (ec) {
        throw std::runtime_error("rename failed: " + from + " -> " + to + " : " + ec.message());
    }
}


std::string read_file(const std::string& path) {
    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("cannot open file: " + path);
    }

    ifs.seekg(0, std::ios::end);
    std::size_t size = ifs.tellg();
    ifs.seekg(0);

    std::string content(size, '\0');
    ifs.read(&content[0], size);

    return content;
}

Worker::Worker(std::shared_ptr<Channel> channel):stub_(lab1::Master::NewStub(channel)){}

void Worker::execute_map_task(const std::string& filename, int taskId) {
    // 1) read file
    const std::string content = read_file(filename);

    // 2) map
    std::vector<KV> kvs = map_func(filename, content);

    // 3) sort by key only (align Go ByKey)
    std::sort(kvs.begin(), kvs.end(),
              [](const KV& a, const KV& b) { return a.first < b.first; });

    // 4) create temp output files (one per reduce partition)
    std::vector<std::string> tmp_names(numReduce_);
    std::vector<std::ofstream> outs(numReduce_);

    for (int r = 0; r < numReduce_; r++) {
        tmp_names[r] = make_tmp_name("mr-map-" + std::to_string(taskId) + "-r" + std::to_string(r));
        outs[r].open(tmp_names[r], std::ios::out | std::ios::trunc);
        if (!outs[r]) {
            throw std::runtime_error("cannot create temp file: " + tmp_names[r]);
        }
    }

    // 5) write JSON lines, bucket by ihash(key)%numReduce_
    //    (keep Go's "group by key" loop style)
    for (size_t i = 0; i < kvs.size();) {
        size_t j = i + 1;
        while (j < kvs.size() && kvs[j].first == kvs[i].first) j++;

        const int reduceIdx = ihash(kvs[i].first) % numReduce_;
        auto& out = outs[reduceIdx];

        for (size_t k = i; k < j; k++) {
            nlohmann::json js;
            js["Key"] = kvs[k].first;
            js["Value"] = kvs[k].second;
            out << js.dump() << "\n";
            if (!out) {
                throw std::runtime_error("write failed: " + tmp_names[reduceIdx]);
            }
        }

        i = j;
    }

    // 6) close and rename to final mr-<mapId>-<reduceId>
    for (int r = 0; r < numReduce_; r++) {
        outs[r].close();
        const std::string final_name = "mr-" + std::to_string(taskId) + "-" + std::to_string(r);
        safe_rename_replace(tmp_names[r], final_name);
    }
}

void Worker::execute_reduce_task(const std::vector<std::string>& files, int taskId) {
    std::vector<KV> kva;
    kva.reserve(1 << 20);

    // 1) read all intermediate files (JSON per line)
    for (const auto& fname : files) {
        std::ifstream in(fname, std::ios::in);
        if (!in) {
            throw std::runtime_error("cannot open intermediate file: " + fname);
        }

        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            auto js = nlohmann::json::parse(line);
            std::string k = js.at("Key").get<std::string>();
            std::string v = js.at("Value").get<std::string>();
            kva.emplace_back(std::move(k), std::move(v));
        }
    }

    // 2) sort by key
    std::sort(kva.begin(), kva.end(),
              [](const KV& a, const KV& b) { return a.first < b.first; });

    // 3) write output mr-out-<reduceId>
    const std::string oname = "mr-out-" + std::to_string(taskId);
    std::ofstream out(oname, std::ios::out | std::ios::trunc);
    if (!out) throw std::runtime_error("cannot create output file: " + oname);

    // 4) group and reduce
    for (size_t i = 0; i < kva.size();) {
        size_t j = i + 1;
        while (j < kva.size() && kva[j].first == kva[i].first) j++;

        std::vector<std::string> values;
        values.reserve(j - i);
        for (size_t k = i; k < j; k++) {
            values.push_back(kva[k].second);
        }

        const std::string result = reduce_func(kva[i].first, values);
        out << kva[i].first << " " << result << "\n";

        i = j;
    }

    out.close();
}

void Worker::execute_task(const Task &task){
    if(task.tasktype() == lab1::Task_TaskType::Task_TaskType_MAP){
        execute_map_task(task.files(0),task.taskid());
    }else {
        const auto & files = task.files();
        execute_reduce_task(std::vector(files.begin(),files.end()), task.taskid());
    }
}

bool Worker::notify_finished_task(const Task &task){
    grpc::ClientContext context;
    Empty rep;
    Status status = stub_->notify_finished_task(&context,task,&rep);
    if(status.ok()) return true;
    return false;
}

void Worker::run() {
    // 1) get_numReduce：单独一个 context
    {
        grpc::ClientContext context;
        grpc::Status status;
        Empty req;
        GetNumReduceRep rep;

        status = stub_->get_numReduce(&context, req, &rep);
        if (!status.ok()) {
            std::cerr << "get_numReduce failed: "
                      << status.error_code() << ": " << status.error_message() << "\n";
            return;
        }
        numReduce_ = rep.numreduce();
    }

    // 2) get_task：每次循环都新建 context
    while (true) {
        grpc::ClientContext context;
        grpc::Status status;
        Empty req;
        Task task;
        status = stub_->get_task(&context, req, &task);
        if (status.ok()) {
            std::cout << task.files_size() << std::endl;
            this->execute_task(task);
            std::cout << "task completed" << std::endl;
            if(!this->notify_finished_task(task)) return;
        } else if(status.error_code() == 14){
            return;
        }else {
            std::cerr << "get_task failed: "
                      << status.error_code() << ": " << status.error_message() << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        // std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}



int main(){
    Worker w(grpc::CreateChannel("127.0.0.1:5005",
        grpc::InsecureChannelCredentials()
    ));
    w.run();
    return 0;
}