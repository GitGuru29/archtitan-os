// =============================================================================
// titan-hwm-v3/enforcement/enforcement_plane.cpp
// Phase 9 — implementation
// =============================================================================
#include "enforcement_plane.hpp"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <csignal>
#include <cstring>

namespace fs = std::filesystem;

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// signal_tree — send signal to all PIDs; skip protected PIDs (extra safety)
// ─────────────────────────────────────────────────────────────────────────────
void EnforcementPlane::signal_tree(const std::vector<pid_t>& pids, int sig) {
    for (pid_t pid : pids) {
        // Read comm for the protected check
        std::string comm;
        std::ifstream sf("/proc/" + std::to_string(pid) + "/stat");
        std::string tok;
        if (sf >> tok && sf >> tok &&
            tok.size() >= 2 && tok.front() == '(' && tok.back() == ')')
            comm = tok.substr(1, tok.size() - 2);

        // Hard block — never signal a protected process
        if (registry_.is_protected(pid, comm)) {
            std::cerr << "[Enforce] BLOCKED signal " << sig
                      << " to protected PID " << pid << " (" << comm << ")\n";
            continue;
        }

        if (!fs::exists("/proc/" + std::to_string(pid))) continue;
        if (::kill(pid, sig) != 0 && errno != ESRCH)
            std::cerr << "[Enforce] kill(" << pid << ", " << sig
                      << ") failed: " << std::strerror(errno) << "\n";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// sigcont_if_stopped — SIGCONT only PIDs in T (stopped) state
// ─────────────────────────────────────────────────────────────────────────────
void EnforcementPlane::sigcont_if_stopped(const std::vector<pid_t>& pids) {
    for (pid_t pid : pids) {
        std::ifstream sf("/proc/" + std::to_string(pid) + "/stat");
        std::string tok;
        char state = '?';
        if (sf >> tok && sf >> tok && sf >> state) {
            if (state == 'T' || state == 't')
                ::kill(pid, SIGCONT);
        }
    }
}

void EnforcementPlane::sigcont_tree(const std::vector<pid_t>& pids) {
    signal_tree(pids, SIGCONT);
}

// ─────────────────────────────────────────────────────────────────────────────
// apply — translate PolicyDecision into kernel actions
// ─────────────────────────────────────────────────────────────────────────────
bool EnforcementPlane::apply(const Workload& wl, PolicyDecision decision) {
    // ── Hard block ─────────────────────────────────────────────────────────
    if (wl.is_protected || registry_.is_protected_any(wl.pids)) {
        // Silently ensure protected workload is in active slice + SIGCONT
        cgroup_.move_to_slice(wl.pids, SLICE_ACTIVE);
        sigcont_if_stopped(wl.pids);
        return false; // indicates protected — caller can log if needed
    }

    switch (decision) {

    case PolicyDecision::KEEP_FULL:
        cgroup_.move_to_slice(wl.pids, SLICE_ACTIVE);
        cgroup_.set_cpu_weight(SLICE_ACTIVE, CPU_WEIGHT_ACTIVE);
        cgroup_.set_oom_score(wl.pids, OOM_PROTECT);
        sigcont_if_stopped(wl.pids);       // unfreeze if previously stopped
        cgroup_.set_freeze(SLICE_FROZEN, false); // ensure frozen slice is thawed
        break;

    case PolicyDecision::KEEP_BACKGROUND:
        cgroup_.move_to_slice(wl.pids, SLICE_BACKGROUND);
        cgroup_.set_cpu_weight(SLICE_BACKGROUND, CPU_WEIGHT_BACKGROUND);
        cgroup_.set_oom_score(wl.pids, OOM_NORMAL);
        sigcont_if_stopped(wl.pids);       // background ≠ stopped
        break;

    case PolicyDecision::THROTTLE:
        cgroup_.move_to_slice(wl.pids, SLICE_BACKGROUND);
        cgroup_.set_cpu_weight(SLICE_BACKGROUND, CPU_WEIGHT_THROTTLE);
        // Reduce memory.high to 75% of current RSS as a soft cap
        // (exact calculation deferred to daemon tick which has RSS info)
        cgroup_.set_oom_score(wl.pids, OOM_EXPOSE);
        break;

    case PolicyDecision::FREEZE:
        // SIGSTOP first — cgroup freeze alone doesn't stop kernel threads
        signal_tree(wl.pids, SIGSTOP);
        cgroup_.move_to_slice(wl.pids, SLICE_FROZEN);
        cgroup_.set_freeze(SLICE_FROZEN, true);
        cgroup_.set_oom_score(wl.pids, OOM_EXPOSE);
        std::cout << "[Enforce] Froze workload " << wl.id
                  << " (" << wl.pids.size() << " PIDs)\n";
        break;

    case PolicyDecision::RECLAIM:
        // ReclaimEngine handles the full 9-step safety sequence.
        // EnforcementPlane only handles the signal delivery part.
        // This case should not be called directly from EnforcementPlane;
        // the daemon calls ReclaimEngine::reclaim() which calls signal_tree.
        std::cerr << "[Enforce] RECLAIM reached enforcement plane directly "
                  << "— use ReclaimEngine::reclaim() instead\n";
        break;
    }

    return true;
}

} // namespace thm
