// =============================================================================
// titan-hwm-v3/workload/workload_manager.hpp
// Phase 5 — WorkloadState machine
//
// Owns the Workload registry and drives state transitions each tick.
// Does NOT touch processes or cgroups — that's EnforcementPlane's job.
// =============================================================================
#pragma once

#include "../core/types.hpp"
#include "../classifier/execution_detector.hpp"

#include <unordered_map>
#include <mutex>
#include <vector>

namespace thm {

// Tunable decay thresholds (sourced from Config in Phase 12)
struct DecayConfig {
    int age_soft_decay_min = 5;   // IDLE → AGING
    int age_hard_decay_min = 15;  // AGING → RECLAIMABLE
    int hysteresis_ticks   = 10;  // HC-09: ticks before downgrade after WS leave (10×200ms = 2s)
};

class WorkloadManager {
public:
    explicit WorkloadManager(DecayConfig cfg = {}) : cfg_(cfg) {}

    // ── Registry ─────────────────────────────────────────────────────────────
    void add(Workload wl);
    void remove(uint32_t workload_id);
    Workload* get(uint32_t workload_id);
    const std::vector<uint32_t>& all_ids() const;

    // ── State machine tick ───────────────────────────────────────────────────
    // Drive one state transition for a workload.
    // Called every policy tick for every registered workload.
    void tick(Workload&           wl,
              const WorkspaceState& ws,
              ActivityState         activity,
              PressureLevel         pressure);

    // Convenience: tick all workloads at once
    void tick_all(const std::unordered_map<int, WorkspaceState>& workspaces,
                  const std::unordered_map<pid_t, ProcessNode>&  graph,
                  const ExecutionDetector&                        detector,
                  PressureLevel                                   pressure);

private:
    DecayConfig cfg_;
    mutable std::mutex mtx_;

    std::unordered_map<uint32_t, Workload> registry_;
    std::vector<uint32_t> id_list_; // insertion-order list for iteration
};

} // namespace thm
