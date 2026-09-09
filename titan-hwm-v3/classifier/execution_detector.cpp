// =============================================================================
// titan-hwm-v3/classifier/execution_detector.cpp
// Phase 4 — Execution Detector implementation
// =============================================================================
#include "execution_detector.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// Read utime + stime from /proc/<pid>/stat (fields 14 + 15)
// Returns -1 on failure (process gone)
// ─────────────────────────────────────────────────────────────────────────────
long ExecutionDetector::read_cpu_ticks(pid_t pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
    if (!f.is_open()) return -1;

    std::string line;
    if (!std::getline(f, line)) return -1;

    // Find end of comm field (handles spaces in comm)
    auto rparen = line.rfind(')');
    if (rparen == std::string::npos) return -1;

    std::istringstream ss(line.substr(rparen + 2));
    std::string tok;
    // After comm+state: field 3=state 4=ppid 5=pgrp 6=session 7=tty 8=tpgid
    // 9=flags 10=minflt 11=cminflt 12=majflt 13=cmajflt 14=utime 15=stime
    for (int i = 3; i <= 15; ++i) {
        if (!(ss >> tok)) return -1;
        if (i == 14) {
            long utime = 0;
            try { utime = std::stol(tok); } catch (...) { return -1; }
            long stime = 0;
            if (!(ss >> tok)) return -1;
            try { stime = std::stol(tok); } catch (...) { return -1; }
            return utime + stime;
        }
    }
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// Compute % CPU from tick delta over sample_ms
// Kernel clock ticks = sysconf(_SC_CLK_TCK), typically 100
// ─────────────────────────────────────────────────────────────────────────────
float ExecutionDetector::cpu_pct(long t0, long t1, int sample_ms) {
    if (t0 < 0 || t1 < 0 || sample_ms <= 0) return 0.0f;
    long delta = t1 - t0;
    if (delta < 0) return 0.0f;
    // ticks per second = 100 (standard HZ); sample_ms/1000 seconds elapsed
    // pct = (delta / 100) / (sample_ms / 1000) * 100
    return static_cast<float>(delta * 1000) / static_cast<float>(sample_ms);
}

// ─────────────────────────────────────────────────────────────────────────────
// detect — update node.activity and node.idle_count
// ─────────────────────────────────────────────────────────────────────────────
void ExecutionDetector::detect(ProcessNode& node,
                                const std::vector<pid_t>& prev_children) {
    const pid_t pid = node.pid;

    // ── Signal 1: CPU tick delta (tick-to-tick non-blocking observation) ─
    long t0 = prev_ticks_.count(pid) ? prev_ticks_[pid] : -1;
    long t1 = read_cpu_ticks(pid);

    bool cpu_active = false;
    if (t0 >= 0 && t1 >= 0) {
        cpu_active = cpu_pct(t0, t1, cfg_.sample_ms) >= cfg_.idle_cpu_pct;
    } else if (t1 >= 0) {
        // First observation: initialize baseline; if kernel state is R or D, flag active
        if (node.proc_state == 'R' || node.proc_state == 'D')
            cpu_active = true;
    }

    // Update baseline for next call
    if (t1 >= 0) prev_ticks_[pid] = t1;
    node.cpu_ticks = (t1 >= 0) ? t1 : node.cpu_ticks;

    // ── Signal 2: Child set changed ────────────────────────────────────────
    bool children_changed = (node.children != prev_children);

    // ── Signal 3: Kernel proc state ───────────────────────────────────────
    // R = running, D = uninterruptible I/O (e.g. waiting for disk during compile)
    bool state_active = (node.proc_state == 'R' || node.proc_state == 'D');

    // ── Signal 4: Known build command ─────────────────────────────────────
    bool known_build = is_known_build_command(node.cmdline);

    // ── Signal 5 (HC-01/07/08): Persistent service — never idle ───────────
    // DB servers, container runtimes, emulators, VMs sleep between requests
    // but must never be frozen or reclaimed by THM.
    bool persistent = is_persistent_service(node.cmdline);

    // ── Signal 6: Zombie = completed ──────────────────────────────────────
    if (node.proc_state == 'Z') {
        node.activity   = ActivityState::COMPLETED;
        node.idle_count = 0;
        return;
    }

    // ── Aggregate ─────────────────────────────────────────────────────────
    bool is_active = cpu_active || children_changed || state_active || known_build || persistent;

    if (is_active) {
        node.activity   = ActivityState::EXECUTING;
        node.idle_count = 0;
        node.last_active = ms_clock::now();
    } else {
        // Increment idle counter; only confirm IDLE after N consecutive samples
        ++node.idle_count;
        if (node.idle_count >= cfg_.idle_confirm_count)
            node.activity = ActivityState::IDLE;
        // else: leave previous state (EXECUTING/UNKNOWN) to avoid premature idle flip
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// aggregate — workload-level ActivityState from all member PIDs
// EXECUTING if any member is EXECUTING
// COMPLETED if all members are COMPLETED or missing from graph
// IDLE otherwise
// ─────────────────────────────────────────────────────────────────────────────
ActivityState ExecutionDetector::aggregate(
        const Workload& wl,
        const std::unordered_map<pid_t, ProcessNode>& graph) const {

    bool any_executing = false;
    bool any_live      = false;

    for (pid_t pid : wl.pids) {
        auto it = graph.find(pid);
        if (it == graph.end()) continue; // process gone
        any_live = true;
        if (it->second.activity == ActivityState::EXECUTING) {
            any_executing = true;
            break;
        }
    }

    if (any_executing)  return ActivityState::EXECUTING;
    if (!any_live)      return ActivityState::COMPLETED;
    return ActivityState::IDLE;
}

// ─────────────────────────────────────────────────────────────────────────────
// Baseline management
// ─────────────────────────────────────────────────────────────────────────────
void ExecutionDetector::update_baseline(pid_t pid, long ticks) {
    prev_ticks_[pid] = ticks;
}

void ExecutionDetector::remove_baseline(pid_t pid) {
    prev_ticks_.erase(pid);
}

} // namespace thm
