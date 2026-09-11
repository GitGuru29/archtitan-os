// =============================================================================
// titan-hwm-v3/policy/reclaim_engine.cpp
// Phase 10 — 9-step safety-checked termination sequence
// =============================================================================
#include "reclaim_engine.hpp"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <csignal>
#include <cstring>
#include <cstdlib>

namespace fs = std::filesystem;

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// Safety checks
// ─────────────────────────────────────────────────────────────────────────────

bool ReclaimEngine::check_not_protected(const Workload& wl) const {
    if (wl.is_protected) return false;
    if (registry_.is_protected_any(wl.pids)) return false;
    return true;
}

bool ReclaimEngine::check_not_executing(
        const Workload& wl,
        const std::unordered_map<pid_t, ProcessNode>& graph) const {
    ActivityState act = detector_.aggregate(wl, graph);
    return act != ActivityState::EXECUTING;
}

bool ReclaimEngine::check_no_active_dependent(
        const Workload& wl,
        const std::unordered_map<uint32_t, Workload>& all) const {
    // A dependent is another workload whose root_pid is a child of this workload's pids
    for (const auto& [id, other] : all) {
        if (id == wl.id) continue;
        if (other.state != WorkloadState::ACTIVE &&
            other.state != WorkloadState::BACKGROUND_EXECUTING) continue;
        // Check if the other workload's root_pid is in this workload's pid set
        for (pid_t our_pid : wl.pids) {
            if (other.root_pid == our_pid) return false; // dependency found
        }
    }
    return true;
}

bool ReclaimEngine::check_no_recent_focus(const Workload& wl) const {
    long secs = std::chrono::duration_cast<std::chrono::seconds>(
        ms_clock::now() - wl.last_active).count();
    return secs >= cfg_.recent_focus_s;
}

// ─────────────────────────────────────────────────────────────────────────────
// notify — desktop notification (non-blocking via notify-send)
// ─────────────────────────────────────────────────────────────────────────────
void ReclaimEngine::notify(const std::string& name) {
    std::string cmd = "notify-send -u normal -t 4000 "
                      "'THM: Reclaiming idle workload' "
                      "'\"" + name + "\" is idle and will be removed.' &";
    if (std::system(cmd.c_str()) != 0) {
        // notify-send may not be available; non-fatal
        std::cout << "[Reclaim] notify-send unavailable — skipping notification\n";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// kill_tree — send signal to all PIDs in workload; skip protected PIDs
// ─────────────────────────────────────────────────────────────────────────────
void ReclaimEngine::kill_tree(const std::vector<pid_t>& pids, int sig) const {
    for (pid_t pid : pids) {
        if (!fs::exists("/proc/" + std::to_string(pid))) continue;
        if (registry_.is_protected_any({pid})) {
            std::cerr << "[Reclaim] BLOCKED signal " << sig
                      << " to protected PID " << pid << "\n";
            continue;
        }
        if (::kill(pid, sig) != 0 && errno != ESRCH)
            std::cerr << "[Reclaim] kill(" << pid << ", " << sig
                      << "): " << std::strerror(errno) << "\n";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// reclaim — the full 9-step safety sequence (from Phase 10 spec)
// ─────────────────────────────────────────────────────────────────────────────
bool ReclaimEngine::reclaim(
        const Workload&                               wl,
        const std::unordered_map<pid_t, ProcessNode>& graph,
        const std::unordered_map<uint32_t, Workload>& all_workloads)
{
    // ── Step 1: Protected check ───────────────────────────────────────────
    if (!check_not_protected(wl)) {
        std::cout << "[Reclaim] Abort: workload " << wl.id << " is protected.\n";
        return false;
    }

    // ── Step 2: Execution check ────────────────────────────────────────────
    if (!check_not_executing(wl, graph)) {
        std::cout << "[Reclaim] Abort: workload " << wl.id << " is executing.\n";
        return false;
    }

    // ── Step 3: Active dependent check ────────────────────────────────────
    if (!check_no_active_dependent(wl, all_workloads)) {
        std::cout << "[Reclaim] Abort: workload " << wl.id
                  << " has an active dependent.\n";
        return false;
    }

    // ── Step 4: Recent user interaction check ─────────────────────────────
    if (!check_no_recent_focus(wl)) {
        std::cout << "[Reclaim] Abort: workload " << wl.id
                  << " had user focus within " << cfg_.recent_focus_s << "s.\n";
        return false;
    }

    // ── Step 5: Grace notification ────────────────────────────────────────
    std::string name = wl.pids.empty() ? "workload-" + std::to_string(wl.id)
                                       : std::to_string(wl.root_pid);
    notify(name);
    std::cout << "[Reclaim] Notified. Waiting " << cfg_.grace_period_ms
              << "ms before SIGTERM...\n";

    // ── Step 6: Wait grace period ─────────────────────────────────────────
    std::this_thread::sleep_for(std::chrono::milliseconds(cfg_.grace_period_ms));

    // ── Step 7: Re-check all conditions ───────────────────────────────────
    // Conditions may have changed during the grace period
    if (!check_not_protected(wl)          ||
        !check_not_executing(wl, graph)   ||
        !check_no_active_dependent(wl, all_workloads) ||
        !check_no_recent_focus(wl)) {
        std::cout << "[Reclaim] Conditions changed during grace period — aborting.\n";
        return false;
    }

    // ── Step 8: SIGTERM ────────────────────────────────────────────────────
    std::cout << "[Reclaim] Sending SIGTERM to workload " << wl.id
              << " (" << wl.pids.size() << " PIDs)\n";
    kill_tree(wl.pids, SIGTERM);

    // ── Step 9: Wait SIGTERM grace, then SIGKILL survivors ────────────────
    std::this_thread::sleep_for(std::chrono::milliseconds(cfg_.sigterm_grace_ms));

    for (pid_t pid : wl.pids) {
        if (fs::exists("/proc/" + std::to_string(pid))) {
            std::cout << "[Reclaim] PID " << pid
                      << " survived SIGTERM — sending SIGKILL\n";
            ::kill(pid, SIGKILL);
        }
    }

    std::cout << "[Reclaim] Workload " << wl.id << " reclaimed.\n";
    return true;
}

} // namespace thm
