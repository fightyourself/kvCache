#include "Persister.h"

#include <filesystem>
#include <iostream>
#include <limits>

namespace fs = std::filesystem;

Persister::Persister(int me) {
    config_path_ = "config_" + std::to_string(me);
    log_path_    = "log_" + std::to_string(me);
    open_or_create_();
}

Persister::~Persister() {
    if (config_fstream_.is_open()) config_fstream_.close();
    if (log_fstream_.is_open()) log_fstream_.close();
}

void Persister::ensure_exists_(const std::string& path) {
    if (fs::exists(path)) return;
    std::ofstream ofs(path, std::ios::binary);
    ofs.close();
}

void Persister::open_rw_(std::fstream& fsx, const std::string& path) {
    fsx.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!fsx.is_open()) {
        std::cerr << path << " can't open\n";
        // 这里你也可以 exit(0)，但建议不要在库里直接退出
        throw std::runtime_error("failed to open " + path);
    }
}

void Persister::init_config_if_needed_() {
    config_fstream_.clear();
    config_fstream_.seekg(0, std::ios::end);
    std::streamoff sz = config_fstream_.tellg();
    if (sz < 0) sz = 0;

    constexpr std::streamoff kNeed = (std::streamoff)sizeof(uint64_t) + (std::streamoff)sizeof(int);
    if (sz >= kNeed) {
        // 已初始化
        config_fstream_.clear();
        return;
    }

    // 初始化默认值
    uint64_t term = 0;
    int voted_for = -1;

    config_fstream_.clear();
    config_fstream_.seekp(0, std::ios::beg);
    config_fstream_.write(reinterpret_cast<const char*>(&term), sizeof(term));
    config_fstream_.write(reinterpret_cast<const char*>(&voted_for), sizeof(voted_for));
    config_fstream_.flush();

    // 确保文件至少 kNeed 大小（有些实现可能不自动扩展到这个大小）
    config_fstream_.clear();
}

void Persister::open_or_create_() {
    ensure_exists_(config_path_);
    ensure_exists_(log_path_);

    open_rw_(config_fstream_, config_path_);
    open_rw_(log_fstream_, log_path_);

    init_config_if_needed_();
}

bool Persister::append_entry(const Entry& entry) {
    return append_entry(entry, nullptr);
}

bool Persister::append_entry(const Entry& entry, std::streamoff *record_pos){
    std::string serialized;
    if(!entry.SerializeToString(&serialized)){
        std::cerr << "serialize fail\n";
        return false;
    }
    if(serialized.size() > static_cast<size_t>(std::numeric_limits<int>::max())){
        std::cerr << "entry too large\n";
        return false;
    }

    int len = static_cast<int>(serialized.size());

    log_fstream_.clear();
    log_fstream_.seekp(0, std::ios::end);

    std::streamoff pos = log_fstream_.tellp();
    if(record_pos) *record_pos = pos;

    log_fstream_.write(reinterpret_cast<char *>(&len), sizeof(len));
    log_fstream_.write(serialized.data(), serialized.size());
    log_fstream_.flush();
    return static_cast<bool>(log_fstream_);
}

bool Persister::persist_current_term(uint64_t term) {
    config_fstream_.clear();
    config_fstream_.seekp(0, std::ios::beg);
    config_fstream_.write(reinterpret_cast<const char*>(&term), sizeof(term));
    config_fstream_.flush();
    return static_cast<bool>(config_fstream_);
}

bool Persister::persist_voted_for(int voted_for) {
    config_fstream_.clear();
    config_fstream_.seekp((std::streamoff)sizeof(uint64_t), std::ios::beg);
    config_fstream_.write(reinterpret_cast<const char*>(&voted_for), sizeof(voted_for));
    config_fstream_.flush();
    return static_cast<bool>(config_fstream_);
}

bool Persister::read_current_term(uint64_t* term) const {
    if (!term) return false;
    auto& fsx = const_cast<std::fstream&>(config_fstream_);
    fsx.clear();
    fsx.seekg(0, std::ios::beg);
    fsx.read(reinterpret_cast<char*>(term), sizeof(*term));
    return static_cast<bool>(fsx);
}

bool Persister::read_voted_for(int* voted_for) const {
    if (!voted_for) return false;
    auto& fsx = const_cast<std::fstream&>(config_fstream_);
    fsx.clear();
    fsx.seekg((std::streamoff)sizeof(uint64_t), std::ios::beg);
    fsx.read(reinterpret_cast<char*>(voted_for), sizeof(*voted_for));
    return static_cast<bool>(fsx);
}

bool Persister::read_entry_at(std::streamoff record_pos, Entry* out) const {
    if (!out) return false;

    auto& fsx = const_cast<std::fstream&>(log_fstream_);
    fsx.clear();
    fsx.seekg(record_pos, std::ios::beg);

    int len = 0;
    fsx.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (!fsx || len < 0 || len > kMaxEntryBytes) return false;

    std::string buf;
    buf.resize((size_t)len);
    fsx.read(buf.data(), len);
    if (!fsx) return false;

    return out->ParseFromString(buf);
}


void Persister::truncate_log_to(std::streamoff new_size){
    truncate_log_to_(new_size);
}


void Persister::truncate_log_to_(std::streamoff new_size) {
    // 强一致：先关再截再开，避免“truncate 后 fstream 缓冲/指针状态不确定”
    if (log_fstream_.is_open()) {
        log_fstream_.flush();
        log_fstream_.close();
    }

    if (new_size < 0) new_size = 0;
    fs::resize_file(log_path_, (uintmax_t)new_size);

    open_rw_(log_fstream_, log_path_);
    log_fstream_.clear();
    log_fstream_.seekg(0, std::ios::beg);
    log_fstream_.seekp(0, std::ios::end);
}

void Persister::init(std::unordered_map<int, std::streamoff>& offset, std::vector<Entry>& entris) {
    offset.clear();
    entris.clear();
    entris.resize(1);
    // 取文件末尾
    log_fstream_.clear();
    log_fstream_.seekg(0, std::ios::end);
    const std::streamoff file_end = log_fstream_.tellg();
    log_fstream_.seekg(0, std::ios::beg);

    if (file_end <= 0) {
        log_fstream_.clear();
        log_fstream_.seekp(0, std::ios::end);
        return;
    }

    int idx = 1;
    std::streamoff last_good_end = 0;

    while (true) {
        std::streamoff record_pos = log_fstream_.tellg();
        if (record_pos < 0 || record_pos >= file_end) break;

        int len = 0;
        log_fstream_.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (!log_fstream_) break;

        // 防御性校验：长度非法 or 超出文件末尾 => 认为尾部损坏/半条记录
        if (len < 0 || len > kMaxEntryBytes) break;

        const std::streamoff payload_end =
            record_pos + (std::streamoff)sizeof(len) + (std::streamoff)len;

        if (payload_end > file_end) {
            // 半条记录（崩溃截断）或损坏
            break;
        }

        offset[idx++] = record_pos;
        std::string serilized;
        serilized.resize(len);
        log_fstream_.read(reinterpret_cast<char*>(serilized.data()),len);
        Entry entry;
        entry.ParseFromString(serilized);
        entris.push_back(std::move(entry));
        if (!log_fstream_) break;

        last_good_end = log_fstream_.tellg();
        if (last_good_end < 0) break;
    }

    // 如果尾部存在坏数据：truncate 到最后一条完整记录末尾
    if (last_good_end < file_end) {
        truncate_log_to_(last_good_end);
    } else {
        // 确保后续 append 正常写到 EOF
        log_fstream_.clear();
        log_fstream_.seekp(0, std::ios::end);
    }
}
