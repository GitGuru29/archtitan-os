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

namespace thm {

struct ReclaimConfig {
    int   grace_period_ms   = 2000; // notify → SIGTERM gap
    int   sigterm_grace_ms  = 3000; // SIGTERM → SIGKILL gap
    int   recent_focus_s    = 30;   // user interaction window (seconds)
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

private:
    const ProtectedRegistry&  registry_;
    const ExecutionDetector&  detector_;
    ReclaimConfig             cfg_;

    // Individual safety checks (all must pass for reclaim to proceed)
    bool check_not_protected(const Workload& wl) const;
    bool check_not_executing(const Workload& wl,
                              const std::unordered_map<pid_t, ProcessNode>& graph) const;
    bool check_no_active_dependent(const Workload& wl,
                                    const std::unordered_map<uint32_t, Workload>& all) const;
    bool check_no_recent_focus(const Workload& wl) const;

    // Send desktop notification (non-blocking)
    static void notify(const std::string& name);

    // Signal helpers
    void kill_tree(const std::vector<pid_t>& pids, int sig) const;
};

} // namespace thm
