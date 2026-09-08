// =============================================================================
// titan-hwm-v3/enforcement/enforcement_plane.hpp
// Phase 9 — Enforcement Plane
//
// Consumes PolicyDecision + Workload and translates it into concrete kernel
// actions: cgroup moves, SIGSTOP/SIGCONT, OOM scores, cpu.weight.
// Contains the protected-domain hard-block at the top of every code path.
// =============================================================================
#pragma once

#include "../core/types.hpp"
#include "../core/protected_registry.hpp"
#include "cgroup_controller.hpp"

#include <csignal>
#include <unordered_map>

namespace thm {

// OOM scores used by THM
static constexpr int OOM_PROTECT  = -500; // active workloads — harder to kill
static constexpr int OOM_NORMAL   =    0; // background workloads
static constexpr int OOM_EXPOSE   =  500; // aging/reclaimable — OOM prefers these

// CPU weights per slice
static constexpr int CPU_WEIGHT_ACTIVE     = 500;
static constexpr int CPU_WEIGHT_BACKGROUND =  50;
static constexpr int CPU_WEIGHT_THROTTLE   =  20;

class EnforcementPlane {
public:
    explicit EnforcementPlane(CgroupController& cgroup,
                               const ProtectedRegistry& registry)
        : cgroup_(cgroup), registry_(registry) {}

    // Apply a PolicyDecision to a workload.
    // Returns false if the hard-block fires (workload is protected).
    bool apply(const Workload& wl, PolicyDecision decision);

    // Thaw all stopped PIDs (used during startup recovery / crash thaw)
    void sigcont_tree(const std::vector<pid_t>& pids);

private:
    CgroupController&        cgroup_;
    const ProtectedRegistry& registry_;

    // Send signal to all PIDs in list; skip protected PIDs (extra safety)
    void signal_tree(const std::vector<pid_t>& pids, int sig);

    // SIGCONT any PIDs currently in T (stopped) state
    void sigcont_if_stopped(const std::vector<pid_t>& pids);
};

} // namespace thm
