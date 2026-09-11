// =============================================================================
// titan-hwm-v3/policy/policy_engine.cpp
// Phase 6 — Policy Engine implementation
// =============================================================================
#include "policy_engine.hpp"
#include <iostream>

namespace thm {

PolicyDecision PolicyEngine::evaluate(
        const Workload&          wl,
        const WorkspaceState&    ws,
        ActivityState            activity,
        PressureLevel            pressure,
        const ProtectedRegistry& registry) const {

    // ── Step 1: Protected domain — hard block, always KEEP_FULL ──────────────
    // Check the registry first (covers THM self, Titan ecosystem, audio stack).
    if (wl.is_protected) return PolicyDecision::KEEP_FULL;
    if (registry.is_protected_any(wl.pids)) return PolicyDecision::KEEP_FULL;

    // ── Step 1b (HC-07): SERVICE workloads — DB servers, containers ──────────
    // These headless services have no window and low CPU but must stay alive.
    // Minimum decision: KEEP_BACKGROUND. Never FREEZE or RECLAIM.
    if (wl.type == WorkloadType::SERVICE) {
        if (ws.is_focused || ws.is_visible || activity == ActivityState::EXECUTING)
            return PolicyDecision::KEEP_BACKGROUND;
        // Under CRITICAL pressure: throttle but never freeze or reclaim
        if (pressure >= PressureLevel::CRITICAL) return PolicyDecision::THROTTLE;
        return PolicyDecision::KEEP_BACKGROUND;
    }

    // ── Step 1c (HC-08): VM workloads — QEMU/KVM virtual machines ────────────
    // Guest kernel has its own timer wheel. Host SIGSTOP stalls the guest hard.
    // Minimum decision: KEEP_BACKGROUND. Never FREEZE.
    if (wl.type == WorkloadType::VM) {
        if (activity == ActivityState::EXECUTING || ws.is_focused || ws.is_visible)
            return PolicyDecision::KEEP_BACKGROUND;
        // Allow throttle under extreme pressure, but never freeze
        if (pressure >= PressureLevel::CRITICAL) return PolicyDecision::THROTTLE;
        return PolicyDecision::KEEP_BACKGROUND;
    }

    // ── Step 2: Execution is the primary authority ────────────────────────────
    // A workload doing real work gets resources, regardless of which workspace
    // is focused. Workspace visibility only determines the priority tier.
    if (activity == ActivityState::EXECUTING) {
        if (ws.is_focused || ws.is_visible)
            return PolicyDecision::KEEP_FULL;        // interactive dev context
        return PolicyDecision::KEEP_BACKGROUND;      // background build / AI task
    }

    // ── Step 3: User is actively in this workspace + workload is ACTIVE ───────
    // The workload itself isn't executing right now (e.g. IDE waiting for input),
    // but the user is present. Grant KEEP_FULL so UI remains responsive.
    if ((ws.is_focused || ws.is_visible) &&
         wl.state == WorkloadState::ACTIVE)
        return PolicyDecision::KEEP_FULL;

    // ── Step 3b (HC-09): Workspace hysteresis — protect recently-left workloads
    // If the workspace was left within the hysteresis window, do not downgrade
    // below KEEP_BACKGROUND. Prevents signal churn on rapid WS oscillation.
    if (wl.ws_hysteresis_ticks > 0)
        return PolicyDecision::KEEP_BACKGROUND;

    // ── Step 3c (HC-05/06): Browser with latency_sensitive — keep network alive
    // Browsers with active WebRTC/WebSocket sessions must not receive SIGSTOP.
    // Policy returns max THROTTLE; enforcement will skip SIGSTOP for browser roots.
    if (wl.is_browser_root && wl.latency_sensitive) {
        switch (pressure) {
        case PressureLevel::NORMAL:
        case PressureLevel::MODERATE:
            return PolicyDecision::KEEP_BACKGROUND;
        case PressureLevel::HIGH:
        case PressureLevel::CRITICAL:
            // Return FREEZE so cgroup throttle fires, but enforcement_plane
            // will skip SIGSTOP for is_browser_root workloads (HC-05 fix).
            return PolicyDecision::FREEZE;
        }
    }

    // ── Step 4: Idle workload — pressure determines the severity ─────────────
    // ai_owned raises the minimum action from RECLAIM to FREEZE for IDLE, but
    // it does NOT prevent throttling/freezing. A dormant AI process is not immune.
    if (wl.state == WorkloadState::IDLE) {
        switch (pressure) {
        case PressureLevel::NORMAL:
            return PolicyDecision::KEEP_BACKGROUND;  // no action needed yet
        case PressureLevel::MODERATE:
            return PolicyDecision::THROTTLE;
        case PressureLevel::HIGH:
        case PressureLevel::CRITICAL:
            return PolicyDecision::FREEZE;
        }
    }

    if (wl.state == WorkloadState::AGING) {
        if (pressure >= PressureLevel::HIGH)
            return PolicyDecision::FREEZE;
        return PolicyDecision::THROTTLE;
    }

    // ── Step 5: Reclaimable — termination sequence ───────────────────────────
    if (wl.state == WorkloadState::RECLAIMABLE) {
        // ai_owned: downgrade RECLAIM → FREEZE. Prevents silent reaping of a
        // dormant AI process that has no explicit COMPLETED signal yet.
        // ReclaimEngine will re-evaluate next tick with fresh safety checks.
        if (wl.ai_owned) {
            std::cout << "[Policy] ai_owned workload " << wl.id
                      << " is RECLAIMABLE but downgraded to FREEZE.\n";
            return PolicyDecision::FREEZE;
        }
        return PolicyDecision::RECLAIM;
    }

    // Default: keep in background without consuming unnecessary resources
    return PolicyDecision::KEEP_BACKGROUND;
}

} // namespace thm

