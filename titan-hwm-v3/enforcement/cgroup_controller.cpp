// =============================================================================
// titan-hwm-v3/enforcement/cgroup_controller.cpp
// Phase 7 — implementation
// =============================================================================
#include "cgroup_controller.hpp"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
bool CgroupController::write_file(const std::string& path, const std::string& value) {
    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[cgroup] Cannot write " << path << "\n";
        return false;
    }
    f << value;
    return f.good();
}

bool CgroupController::mkdir_p(const std::string& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    if (ec) {
        std::cerr << "[cgroup] mkdir_p failed for " << path
                  << ": " << ec.message() << "\n";
        return false;
    }
    return true;
}

std::string CgroupController::slice_path(const std::string& slice_name) {
    return std::string(SLICE_ROOT) + "/" + slice_name;
}

// ─────────────────────────────────────────────────────────────────────────────
// setup_slices — create the 4 sub-slices under archtitan.slice (idempotent)
// Requires Delegate=yes in /etc/systemd/system/archtitan.slice
// ─────────────────────────────────────────────────────────────────────────────
bool CgroupController::setup_slices() {
    // Verify the root slice exists (systemd must have created it)
    if (!fs::exists(SLICE_ROOT)) {
        std::cerr << "[cgroup] archtitan.slice not found at " << SLICE_ROOT
                  << " — is systemd unit loaded?\n";
        return false;
    }

    bool ok = true;
    for (const char* name : {SLICE_PROTECTED, SLICE_ACTIVE,
                              SLICE_BACKGROUND, SLICE_FROZEN}) {
        ok &= mkdir_p(slice_path(name));
    }

    // Enable memory accounting in the slices we manage
    for (const char* name : {SLICE_ACTIVE, SLICE_BACKGROUND, SLICE_FROZEN}) {
        write_file(slice_path(name) + "/memory.oom.group", "1");
    }

    if (ok) std::cout << "[cgroup] archtitan.slice hierarchy ready.\n";
    return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// move_to_slice — write PIDs to cgroup.procs of target slice
// ─────────────────────────────────────────────────────────────────────────────
bool CgroupController::move_to_slice(const std::vector<pid_t>& pids,
                                      const std::string& slice_name) {
    std::string procs_path = slice_path(slice_name) + "/cgroup.procs";
    bool ok = true;
    for (pid_t pid : pids) {
        // Skip if /proc/<pid> no longer exists
        if (!fs::exists("/proc/" + std::to_string(pid))) continue;
        ok &= write_file(procs_path, std::to_string(pid));
    }
    return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// set_cpu_weight — write cpu.weight (1–10000)
// ─────────────────────────────────────────────────────────────────────────────
bool CgroupController::set_cpu_weight(const std::string& slice_name, int weight) {
    weight = std::max(1, std::min(10000, weight));
    return write_file(slice_path(slice_name) + "/cpu.weight",
                      std::to_string(weight));
}

// ─────────────────────────────────────────────────────────────────────────────
// set_memory_high — limit memory.high (bytes; 0 = max / remove limit)
// ─────────────────────────────────────────────────────────────────────────────
bool CgroupController::set_memory_high(const std::string& slice_name, long bytes) {
    std::string val = (bytes <= 0) ? "max" : std::to_string(bytes);
    return write_file(slice_path(slice_name) + "/memory.high", val);
}

// ─────────────────────────────────────────────────────────────────────────────
// set_freeze — write cgroup.freeze (1 = frozen, 0 = thawed)
// ─────────────────────────────────────────────────────────────────────────────
bool CgroupController::set_freeze(const std::string& slice_name, bool freeze) {
    return write_file(slice_path(slice_name) + "/cgroup.freeze",
                      freeze ? "1" : "0");
}

// ─────────────────────────────────────────────────────────────────────────────
// set_oom_score — write /proc/<pid>/oom_score_adj (-1000 to +1000)
// ─────────────────────────────────────────────────────────────────────────────
void CgroupController::set_oom_score(const std::vector<pid_t>& pids, int score) {
    score = std::max(-1000, std::min(1000, score));
    for (pid_t pid : pids) {
        std::string path = "/proc/" + std::to_string(pid) + "/oom_score_adj";
        write_file(path, std::to_string(score));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// hierarchy_healthy — verify cgroup.controllers has "cpu memory" delegated
// ─────────────────────────────────────────────────────────────────────────────
bool CgroupController::hierarchy_healthy() const {
    std::ifstream f(std::string(SLICE_ROOT) + "/cgroup.controllers");
    if (!f.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return content.find("cpu") != std::string::npos &&
           content.find("memory") != std::string::npos;
}

} // namespace thm
