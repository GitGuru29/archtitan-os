// =============================================================================
// titan-hwm-v3/workload/ownership_graph.cpp
// Phase 2 — implementation
// =============================================================================
#include "ownership_graph.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <unistd.h>

namespace fs = std::filesystem;

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// Static /proc helpers
// ─────────────────────────────────────────────────────────────────────────────
std::string OwnershipGraph::read_comm(pid_t pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
    std::string tok;
    if (!(f >> tok)) return {};           // field 1: pid number
    if (!(f >> tok)) return {};           // field 2: (comm)
    if (tok.size() >= 2 && tok.front() == '(' && tok.back() == ')')
        return tok.substr(1, tok.size() - 2);
    return tok;
}

pid_t OwnershipGraph::read_ppid(pid_t pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
    std::string tok;
    // fields: pid comm state ppid ...
    int field = 0;
    while (f >> tok) {
        ++field;
        if (field == 4) {
            try { return static_cast<pid_t>(std::stoi(tok)); }
            catch (...) { return 0; }
        }
    }
    return 0;
}

std::string OwnershipGraph::read_cwd(pid_t pid) {
    char buf[4096];
    std::string link = "/proc/" + std::to_string(pid) + "/cwd";
    ssize_t len = readlink(link.c_str(), buf, sizeof(buf) - 1);
    if (len <= 0) return {};
    buf[len] = '\0';
    return std::string(buf);
}

WorkloadType OwnershipGraph::infer_from_cwd(const std::string& cwd) {
    if (cwd.empty()) return WorkloadType::NEUTRAL;
    for (const auto& marker : cwd_markers()) {
        std::string path = cwd + "/" + marker.filename;
        if (fs::exists(path)) return marker.type;
    }
    return WorkloadType::NEUTRAL;
}

// ─────────────────────────────────────────────────────────────────────────────
// Query
// ─────────────────────────────────────────────────────────────────────────────
uint32_t OwnershipGraph::workload_of(pid_t pid) const {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = pid_to_wl_.find(pid);
    return (it != pid_to_wl_.end()) ? it->second : 0;
}

bool OwnershipGraph::has_live_children(uint32_t workload_id) const {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = wl_to_pids_.find(workload_id);
    if (it == wl_to_pids_.end()) return false;
    for (pid_t pid : it->second) {
        if (fs::exists("/proc/" + std::to_string(pid))) return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mutation
// ─────────────────────────────────────────────────────────────────────────────
void OwnershipGraph::assign(pid_t pid, uint32_t workload_id) {
    std::lock_guard<std::mutex> lk(mtx_);
    // Remove from old workload if already assigned
    auto old_it = pid_to_wl_.find(pid);
    if (old_it != pid_to_wl_.end()) {
        auto& old_set = wl_to_pids_[old_it->second];
        old_set.erase(pid);
        if (old_set.empty()) wl_to_pids_.erase(old_it->second);
    }
    pid_to_wl_[pid] = workload_id;
    wl_to_pids_[workload_id].insert(pid);
}

void OwnershipGraph::remove(pid_t pid) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = pid_to_wl_.find(pid);
    if (it == pid_to_wl_.end()) return;
    auto& pids = wl_to_pids_[it->second];
    pids.erase(pid);
    if (pids.empty()) wl_to_pids_.erase(it->second);
    pid_to_wl_.erase(it);
}

// ─────────────────────────────────────────────────────────────────────────────
// Cold-start bootstrap
//
// Algorithm:
//   1. Snapshot all PIDs and their PPIDs from /proc (sorted PPID ascending
//      so parents are processed before children)
//   2. For each PID:
//      a. If PPID already has a workload → inherit it
//      b. Else if CWD contains a known marker → create new workload for type
//      c. Else if comm matches known IDE/build tool → create workload for type
//      d. Else → leave UNASSIGNED (workload_id = 0)
//   3. Return stub Workload objects for all created workload IDs
// ─────────────────────────────────────────────────────────────────────────────
std::vector<Workload> OwnershipGraph::bootstrap(uint32_t& next_id) {
    std::cout << "[OwnershipGraph] Cold-start bootstrap starting...\n";

    // ── Step 1: Collect all (ppid, pid) pairs from /proc ──────────────────
    struct ProcEntry {
        pid_t pid;
        pid_t ppid;
        std::string comm;
        std::string cwd;
    };
    std::vector<ProcEntry> entries;

    try {
        for (const auto& dir : fs::directory_iterator("/proc")) {
            if (!dir.is_directory()) continue;
            const std::string name = dir.path().filename().string();
            if (!std::all_of(name.begin(), name.end(), ::isdigit)) continue;
            pid_t pid = 0;
            try { pid = static_cast<pid_t>(std::stoi(name)); } catch (...) { continue; }
            if (pid <= 1) continue;

            ProcEntry e;
            e.pid  = pid;
            e.ppid = read_ppid(pid);
            e.comm = read_comm(pid);
            e.cwd  = read_cwd(pid);
            entries.push_back(std::move(e));
        }
    } catch (const std::exception& ex) {
        std::cerr << "[OwnershipGraph] Bootstrap scan error: " << ex.what() << "\n";
    }

    // Sort by PPID ascending: parents before children guarantees ancestry lookup works
    std::sort(entries.begin(), entries.end(),
              [](const ProcEntry& a, const ProcEntry& b){ return a.ppid < b.ppid; });

    // ── Step 2: Assign workload IDs ────────────────────────────────────────
    // workload_id → Workload stub
    std::unordered_map<uint32_t, Workload> stubs;

    auto make_workload = [&](WorkloadType type, pid_t root_pid) -> uint32_t {
        uint32_t id = next_id++;
        Workload wl;
        wl.id           = id;
        wl.type         = type;
        wl.state        = WorkloadState::DISCOVERED;
        wl.root_pid     = root_pid;
        wl.created_at   = ms_clock::now();
        wl.last_active  = ms_clock::now();
        stubs[id]       = std::move(wl);
        return id;
    };

    for (const auto& e : entries) {
        // a. Parent already assigned → inherit
        uint32_t parent_wl = 0;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            auto it = pid_to_wl_.find(e.ppid);
            if (it != pid_to_wl_.end()) parent_wl = it->second;
        }

        if (parent_wl != 0) {
            assign(e.pid, parent_wl);
            stubs[parent_wl].pids.push_back(e.pid);
            continue;
        }

        // b. CWD marker → new workload
        WorkloadType cwd_type = infer_from_cwd(e.cwd);
        if (cwd_type != WorkloadType::NEUTRAL) {
            uint32_t id = make_workload(cwd_type, e.pid);
            assign(e.pid, id);
            stubs[id].pids.push_back(e.pid);
            continue;
        }

        // c. comm matches known tool → new workload
        WorkloadType comm_type = infer_type_from_comm(e.comm);
        if (comm_type != WorkloadType::NEUTRAL) {
            uint32_t id = make_workload(comm_type, e.pid);
            assign(e.pid, id);
            stubs[id].pids.push_back(e.pid);
            continue;
        }

        // d. UNASSIGNED — leave workload_id = 0 (observed, not managed)
    }

    // ── Step 3: Return stubs as vector ────────────────────────────────────
    std::vector<Workload> result;
    result.reserve(stubs.size());
    for (auto& [id, wl] : stubs)
        result.push_back(std::move(wl));

    std::cout << "[OwnershipGraph] Bootstrap complete: "
              << result.size() << " workload(s) discovered from "
              << entries.size() << " process(es).\n";
    return result;
}

} // namespace thm
