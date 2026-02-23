#pragma once
#include <fstream>
#include <unordered_map>
#include "lab3/raftService.grpc.pb.h"
using raftService::Entry;

class Persister final {
public:
    explicit Persister(int me);
    ~Persister();

    // log
    bool append_entry(const Entry& entry);
    bool append_entry(const Entry& entry, std::streamoff *record_pos);
    bool read_entry_at(std::streamoff record_pos, Entry* out) const;

    void truncate_log_to(std::streamoff new_size);

    // config: layout = [term:uint64_t][voted_for:int]
    bool persist_current_term(uint64_t term);
    bool persist_voted_for(int voted_for);
    bool read_current_term(uint64_t* term) const;
    bool read_voted_for(int* voted_for) const;

    // scan log to build index->offset map; if tail is corrupted/partial, truncate it.
    void init(std::unordered_map<int, std::streamoff>& offset, std::vector<Entry>& entries);

private:
    void open_or_create_();
    void open_rw_(std::fstream& fs, const std::string& path);
    void ensure_exists_(const std::string& path);
    void init_config_if_needed_();

    void truncate_log_to_(std::streamoff new_size);
    static constexpr int kMaxEntryBytes = 64 * 1024 * 1024; // 64MB 防御性上限

private:
    std::string config_path_;
    std::string log_path_;
    std::fstream config_fstream_;
    std::fstream log_fstream_;
};