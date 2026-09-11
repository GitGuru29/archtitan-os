// =============================================================================
// titan-hwm-v3/policy/policy_engine.hpp
// Phase 6 — Policy Engine
//
// Translates (Workload, WorkspaceState, ActivityState, PressureLevel) into a
// PolicyDecision. Contains ZERO cgroup/signal/process-targeting logic.
//
// Decision hierarchy (strict order):
//   1. PROTECTED?         → KEEP_FULL
//   2. EXECUTING?         → KEEP_FULL (interactive) or KEEP_BACKGROUND (BG)
//   3. USER INTERACTING?  → KEEP_FULL (visible WS + ACTIVE state)
//   4. IDLE?              → KEEP_BACKGROUND / THROTTLE / FREEZE (pressure)
//   5. RECLAIMABLE?       → RECLAIM (or FREEZE if ai_owned)
// =============================================================================
#pragma once

#include "../core/types.hpp"
#include "../core/protected_registry.hpp"

namespace thm {

class PolicyEngine {
public:
    // Evaluate the policy decision for one workload.
    // activity: aggregated ActivityState across all workload member PIDs.
    PolicyDecision evaluate(const Workload&          wl,
                             const WorkspaceState&    ws,
                             ActivityState            activity,
                             PressureLevel            pressure,
                             const ProtectedRegistry& registry) const;
};

} // namespace thm
