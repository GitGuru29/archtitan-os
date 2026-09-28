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
#include <unordered_set>
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

    // Returns a pointer to the stored workload, or nullptr if absent.
    //
    // Pointer lifetime contract — this is narrower than it may look, and was
    // previously mischaracterised as vulnerable to rehashing:
    //   * std::unordered_map is node-based, so pointers to elements stay valid
    //     across insert() and across rehash. Growth does NOT invalidate.
    //   * The ONLY invalidating operation is remove()/prune_dead() of *this*
    //     workload id, or destruction of the manager.
    //   * WorkloadManager is not internally synchronised for the returned
    //     pointer: the mutex guards the map operations, not the pointee after
    //     return. The daemon drives this from a single thread, so the window
    //     does not currently exist; a future concurrent mutator must not hold
    //     the result across an add()/remove() of the same id.
    Workload* get(uint32_t workload_id);

    // Copy-out accessor for callers that only need to read, or that need a
    // snapshot immune to a concurrent remove(). Returns false if absent.
    bool get_copy(uint32_t workload_id, Workload& out) const;

    // Note: also returns a reference to internal storage without locking.
    // Same single-threaded assumption as get().
    const std::vector<uint32_t>& all_ids() const;

    // Remove every workload in a dead state (TERMINATED — i.e. reclaimed) from the
    // registry. The daemon calls this once per policy tick so the registry stays
    // bounded to live worklets — without it, reclaimed workloads accumulate for
    // the lifetime of the daemon (unbounded memory growth).
    void prune_dead();

    // Drop member PIDs no longer present in `live`. Without this, pids retains
    // every PID a workload ever had until the whole workload dies, so the
    // per-workload is_protected_any() scan costs one /proc open per historical
    // PID, per tick.
    void prune_dead_pids(uint32_t workload_id,
                         const std::unordered_set<pid_t>& live);

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
