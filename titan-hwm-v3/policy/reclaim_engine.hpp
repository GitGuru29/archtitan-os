// =============================================================================
// titan-hwm-v3/policy/reclaim_engine.hpp
// Phase 10 — Reclaim Engine (safety-checked termination sequence)
//
// 9-step safety check before any SIGTERM/SIGKILL is issued.
// Workloads that fail any check are NOT reclaimed on this tick.
// =============================================================================
#pragma once

#include "../core/types.hpp"
#include "../core/protected_registry.hpp"
#include "../classifier/execution_detector.hpp"

#include <unordered_map>
#include <chrono>
#include <string>
#include <vector>

namespace thm {

struct ReclaimConfig {
    int   grace_period_ms   = 2000; // notify → SIGTERM gap
    int   sigterm_grace_ms  = 3000; // SIGTERM → SIGKILL gap
    int   recent_focus_s    = 30;   // user interaction window (seconds)

    // ISSUE-09/14: reclaim fires once per reclaimable workload, which under load
    // meant one notify-send per reclaim (5,184 in a 10 h run). Desktop
    // notifications are coalesced to at most one per this interval. The grace
    // period is enforced by the sleep in step 6, not by the notification, so
    // rate-limiting the notification does not weaken the safety sequence.
    int   notify_min_interval_s = 60;

    // Set false to suppress desktop notifications entirely (headless servers).
    bool  notify_enabled = true;
};

// Whether a grace notification was actually dispatched, so callers can report
// the real outcome instead of assuming every reclaim notified the user.
enum class NotifyOutcome {
    Dispatched,   // notify-send was spawned
    Coalesced,    // suppressed by the burst rate limit
    Unavailable   // notify-send not installed or could not be spawned
};

class ReclaimEngine {
public:
    ReclaimEngine(const ProtectedRegistry&  registry,
                  const ExecutionDetector&  detector,
                  ReclaimConfig             cfg = {})
        : registry_(registry), detector_(detector), cfg_(cfg) {}

    // Attempt to reclaim a workload. Returns true if reclaim was initiated.
    // Performs all 9 safety checks before sending any signal.
    bool reclaim(const Workload&                               wl,
                 const std::unordered_map<pid_t, ProcessNode>& graph,
                 const std::unordered_map<uint32_t, Workload>& all_workloads);

    // Signal every PID in `pids`, refusing any PID the registry protects.
    //
    // Public so the protected-PID guard can be tested directly: the escalation
    // path (step 9) is only reachable when the workload passed the step-1 and
    // step-7 aggregate checks, so a regression there cannot be provoked from
    // reclaim() alone. TC-12 exercises both the SIGTERM and SIGKILL stages.
    void kill_tree(const std::vector<pid_t>& pids, int sig,
                   const char* stage) const;

private:
    const ProtectedRegistry&  registry_;
    const ExecutionDetector&  detector_;
    ReclaimConfig             cfg_;

    // Notification coalescing state (ISSUE-09/14)
    std::chrono::steady_clock::time_point last_notify_{};
    std::vector<pid_t>                   notify_children_;

    // Individual safety checks (all must pass for reclaim to proceed)
    bool check_not_protected(const Workload& wl) const;
    bool check_not_executing(const Workload& wl,
                              const std::unordered_map<pid_t, ProcessNode>& graph) const;
    bool check_no_active_dependent(const Workload& wl,
                                    const std::unordered_map<uint32_t, Workload>& all) const;
    bool check_no_recent_focus(const Workload& wl) const;

    // Send desktop notification. Spawns notify-send directly via posix_spawn —
    // no shell, no string interpolation — and coalesces bursts.
    NotifyOutcome notify(const std::string& name);

    // Reap finished notify-send children so they do not accumulate as zombies.
    void reap_notifications();
};

} // namespace thm
