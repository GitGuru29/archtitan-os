// =============================================================================
// titan-hwm-v3/core/protected_registry.cpp
// Phase 0 — Protected Domain Registry implementation
// =============================================================================
#include "protected_registry.hpp"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <climits>
#include <unistd.h>

namespace fs = std::filesystem;

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// Private: resolve /proc/<pid>/exe and check against protected prefixes
// ─────────────────────────────────────────────────────────────────────────────
bool ProtectedRegistry::exe_is_protected(pid_t pid) const {
    char buf[PATH_MAX];
    std::string link = "/proc/" + std::to_string(pid) + "/exe";
    ssize_t len = readlink(link.c_str(), buf, sizeof(buf) - 1);
    if (len <= 0) return false; // process gone or no permission
    buf[len] = '\0';
    std::string exe(buf);

    // Strip " (deleted)" suffix the kernel appends for replaced-on-disk binaries
    static const std::string del_sfx = " (deleted)";
    if (exe.size() > del_sfx.size() &&
        exe.compare(exe.size() - del_sfx.size(), del_sfx.size(), del_sfx) == 0)
        exe.erase(exe.size() - del_sfx.size());

    for (const auto& prefix : protected_exe_prefixes())
        if (exe.rfind(prefix, 0) == 0) return true;

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// is_protected — primary query: pid + comm
// Check order:
//   1. Explicit PID registry (THM self, dynamically registered Titan PIDs)
//   2. Protected name set (comm match)
//   3. Protected exe prefix (anti-spoof cross-check)
// ─────────────────────────────────────────────────────────────────────────────
bool ProtectedRegistry::is_protected(pid_t pid, const std::string& comm) const {
    std::lock_guard<std::mutex> lk(mtx_);

    // Step 1: explicitly registered PIDs (highest confidence)
    if (explicit_pids_.count(pid)) return true;

    // Step 2: name-based check (fast path)
    if (protected_names().count(comm)) {
        // Cross-verify exe path to prevent comm spoofing
        // If exe check fails but name matched, log and deny protection
        if (exe_is_protected(pid)) return true;
        // Name match but exe mismatch → potential spoof attempt
        std::cerr << "[Protected] WARNING: comm='" << comm
                  << "' pid=" << pid
                  << " matched protected name but exe path is NOT in protected set — DENIED\n";
        return false;
    }

    // Step 3: exe-only match (catches binaries renamed via prctl but still
    // running from a protected path — e.g. titanshare updating its comm string)
    return exe_is_protected(pid);
}

// ─────────────────────────────────────────────────────────────────────────────
// is_protected_any — returns true if ANY pid in the list is protected
// Used by enforcement paths that operate on whole workloads
// ─────────────────────────────────────────────────────────────────────────────
bool ProtectedRegistry::is_protected_any(const std::vector<pid_t>& pids) const {
    for (pid_t pid : pids) {
        // Read comm from /proc for the name check — avoids requiring caller to pass it
        std::string comm;
        std::ifstream stat_f("/proc/" + std::to_string(pid) + "/stat");
        std::string token;
        if (stat_f >> token) { // field 1: pid
            if (stat_f >> token) { // field 2: (comm)
                if (token.size() >= 2 && token.front() == '(' && token.back() == ')')
                    comm = token.substr(1, token.size() - 2);
            }
        }
        if (is_protected(pid, comm)) return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// register_pid — explicit protection (THM self-registers at startup)
// ─────────────────────────────────────────────────────────────────────────────
void ProtectedRegistry::register_pid(pid_t pid, const std::string& reason) {
    std::lock_guard<std::mutex> lk(mtx_);
    explicit_pids_[pid] = reason;
    std::cout << "[Protected] Registered PID " << pid << " (" << reason << ")\n";
}

void ProtectedRegistry::unregister_pid(pid_t pid) {
    std::lock_guard<std::mutex> lk(mtx_);
    explicit_pids_.erase(pid);
}

// ─────────────────────────────────────────────────────────────────────────────
// bootstrap_from_systemd
// Scans /proc for known Titan service unit executables and pre-populates the
// explicit PID registry so that the first policy tick after startup doesn't
// accidentally touch a Titan service before its comm name is recognised.
// ─────────────────────────────────────────────────────────────────────────────
void ProtectedRegistry::bootstrap_from_systemd() {
    std::cout << "[Protected] Bootstrapping protected PID registry from /proc...\n";
    int count = 0;

    try {
        for (const auto& entry : fs::directory_iterator("/proc")) {
            if (!entry.is_directory()) continue;
            const std::string d = entry.path().filename().string();
            if (!std::all_of(d.begin(), d.end(), ::isdigit)) continue;

            pid_t pid = 0;
            try { pid = static_cast<pid_t>(std::stoi(d)); } catch (...) { continue; }
            if (pid <= 1) continue;

            // Read comm
            std::string comm;
            {
                std::ifstream stat_f("/proc/" + d + "/stat");
                std::string token;
                if (stat_f >> token && stat_f >> token &&
                    token.size() >= 2 && token.front() == '(' && token.back() == ')')
                    comm = token.substr(1, token.size() - 2);
            }

            // If name OR exe matches protected sets, register explicitly
            bool name_match = protected_names().count(comm) > 0;
            bool exe_match  = exe_is_protected(pid);

            if (name_match || exe_match) {
                std::string reason = name_match
                    ? "bootstrap:name=" + comm
                    : "bootstrap:exe";
                // Bypass the mutex since we're called before other threads start
                explicit_pids_[pid] = reason;
                std::cout << "[Protected] Bootstrap: PID " << pid
                          << " (" << comm << ") — " << reason << "\n";
                ++count;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Protected] Bootstrap scan error: " << e.what() << "\n";
    }

    std::cout << "[Protected] Bootstrap complete: " << count
              << " protected process(es) registered.\n";
}

} // namespace thm
