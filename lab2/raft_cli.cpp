// raft_cli.cpp
#include <grpcpp/grpcpp.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// TODO: 改成你项目真实的生成头文件
#include "lab2/lab2.grpc.pb.h"

// TODO: 改成你 proto 里的包名
using lab2::RaftService;
using lab2::ProposeRequest;
using lab2::ProposeReply;

static void set_deadline_ms(grpc::ClientContext& ctx, int ms) {
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(ms));
}

struct Client {
    std::string addr;
    std::unique_ptr<RaftService::Stub> stub;

    explicit Client(std::string a)
        : addr(std::move(a)),
          stub(RaftService::NewStub(grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()))) {}

    bool propose(const std::string& cmd, bool wait, ProposeReply& rep, int timeout_ms = 3000) {
        ProposeRequest req;
        req.set_command(cmd);
        req.set_wait_for_commit(wait);

        grpc::ClientContext ctx;
        set_deadline_ms(ctx, timeout_ms);

        auto st = stub->propose(&ctx, req, &rep); // <-- 这里按你的proto可能需要改成 stub->Propose(...)
        return st.ok();
    }
};

static int find_leader(std::vector<Client>& cs) {
    for (int round = 0; round < 10; ++round) {
        for (int i = 0; i < (int)cs.size(); ++i) {
            ProposeReply rep;
            if (!cs[i].propose("probe", false, rep, 500)) continue;
            if (!rep.wrong_leader()) return i;
            if (rep.leader_id() >= 0) return rep.leader_id();
        }
    }
    return -1;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "usage: ./raft_cli addr0 [addr1 addr2 ...]\n";
        return 0;
    }

    std::vector<Client> clients;
    for (int i = 1; i < argc; ++i) clients.emplace_back(argv[i]);

    bool wait_for_commit = true;
    int leader = find_leader(clients);

    std::cout << "[cli] leader guess = " << leader << "\n";
    std::cout << "[cli] commands:\n";
    std::cout << "  wait on/off\n";
    std::cout << "  exit\n";
    std::cout << "  <any other line>  -> propose(line)\n";

    std::string line;
    while (true) {
        std::cout << (wait_for_commit ? "[wait] " : "[nowait] ") << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit") break;

        if (line == "wait on")  { wait_for_commit = true;  continue; }
        if (line == "wait off") { wait_for_commit = false; continue; }

        if (leader < 0 || leader >= (int)clients.size()) leader = find_leader(clients);

        ProposeReply rep;
        bool ok = (leader >= 0) && clients[leader].propose(line, wait_for_commit, rep, 5000);
        if (!ok) {
            std::cout << "[cli] rpc failed to leader=" << leader << ", try re-detect...\n";
            leader = find_leader(clients);
            if (leader >= 0) ok = clients[leader].propose(line, wait_for_commit, rep, 5000);
        }

        if (!ok) {
            std::cout << "[cli] rpc failed (no reachable node?)\n";
            continue;
        }

        if (rep.wrong_leader()) {
            std::cout << "[cli] wrong_leader. leader_id=" << rep.leader_id() << "\n";
            if (rep.leader_id() >= 0) leader = rep.leader_id();
            else leader = find_leader(clients);
            continue;
        }

        std::cout << "[cli] ok: term=" << rep.term()
                  << " leader=" << rep.leader_id()
                  << " index=" << rep.index()
                  << "\n";
    }
    return 0;
}