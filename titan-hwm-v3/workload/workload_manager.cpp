// =============================================================================
// titan-hwm-v3/workload/workload_manager.cpp
// Phase 5 — WorkloadState machine implementation
// =============================================================================
#include "workload_manager.hpp"
#include <iostream>
#include <algorithm>

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// Registry
// ─────────────────────────────────────────────────────────────────────────────
void WorkloadManager::add(Workload wl) {
    std::lock_guard<std::mutex> lk(mtx_);
    uint32_t id = wl.id;
    if (!registry_.count(id)) id_list_.push_back(id);
    registry_[id] = std::move(wl);
}

void WorkloadManager::remove(uint32_t workload_id) {
    std::lock_guard<std::mutex> lk(mtx_);
    registry_.erase(workload_id);
    id_list_.erase(std::remove(id_list_.begin(), id_list_.end(), workload_id),
                   id_list_.end());
}

Workload* WorkloadManager::get(uint32_t workload_id) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = registry_.find(workload_id);
    return (it != registry_.end()) ? &it->second : nullptr;
}

const std::vector<uint32_t>& WorkloadManager::all_ids() const {
    return id_list_;
}

// ─────────────────────────────────────────────────────────────────────────────
// State machine tick
//
// The invariants encoded here match the approved plan exactly:
//   - Execution signal is the authority, not workspace membership
//   - ai_owned does NOT substitute for an EXECUTING signal
//   - Workspace visibility only upgrades to ACTIVE (never blocks IDLE/FREEZE)
//   - IDLE in a visible WS is still IDLE — policy engine handles KEEP_BACKGROUND
// ─────────────────────────────────────────────────────────────────────────────
void WorkloadManager::tick(Workload&           wl,
                            const WorkspaceState& ws,
                            ActivityState         activity,
                            PressureLevel         pressure) {
    // Protected workloads stay ACTIVE — never degrade
    if (wl.is_protected) {
        wl.state = WorkloadState::ACTIVE;
        return;
    }

    switch (wl.state) {

    case WorkloadState::DISCOVERED:
        // Classification runs separately; move to ACTIVE or IDLE on first tick
        if (ws.is_focused || ws.is_visible)
            wl.state = WorkloadState::ACTIVE;
        else if (activity == ActivityState::EXECUTING)
            wl.state = WorkloadState::BACKGROUND_EXECUTING;
        else
            wl.state = WorkloadState::IDLE;
        break;

    case WorkloadState::ACTIVE:
        if (!(ws.is_focused || ws.is_visible)) {
            // Workspace left focus/visibility — activity is the authority
            // ai_owned does NOT keep a workload BACKGROUND_EXECUTING if idle
            if (activity == ActivityState::EXECUTING)
                wl.state = WorkloadState::BACKGROUND_EXECUTING;
            else
                wl.state = WorkloadState::IDLE;
        }
        // else: still visible/focused → stay ACTIVE
        break;

    case WorkloadState::BACKGROUND_EXECUTING:
        // Workspace return always upgrades to ACTIVE
        if (ws.is_focused || ws.is_visible) {
            wl.state = WorkloadState::ACTIVE;
            break;
        }
        // Activity signal is the sole authority — ai_owned does NOT prevent IDLE
        if (activity == ActivityState::IDLE)
            wl.state = WorkloadState::IDLE;
        // EXECUTING → stay in BACKGROUND_EXECUTING
        break;

    case WorkloadState::IDLE: {
        // Re-executing (build resumed, AI agent woke up) → upgrade immediately
        if (activity == ActivityState::EXECUTING) {
            wl.state = WorkloadState::BACKGROUND_EXECUTING;
            wl.last_executing = ms_clock::now();
            break;
        }
        // Workspace return → ACTIVE
        if (ws.is_focused || ws.is_visible) {
            wl.state = WorkloadState::ACTIVE;
            break;
        }
        // Age + moderate pressure → AGING
        long age = minutes_since(wl.last_active);
        if (age >= cfg_.age_soft_decay_min && pressure >= PressureLevel::MODERATE)
            wl.state = WorkloadState::AGING;
        break;
    }

    case WorkloadState::AGING: {
        // Workspace return → ACTIVE
        if (ws.is_focused || ws.is_visible) {
            wl.state = WorkloadState::ACTIVE;
            break;
        }
        // Execution resumed (AI agent or build) → upgrade
        if (activity == ActivityState::EXECUTING) {
            wl.state = WorkloadState::BACKGROUND_EXECUTING;
            wl.last_executing = ms_clock::now();
            break;
        }
        // Age + high pressure → RECLAIMABLE
        long age = minutes_since(wl.last_active);
        if (age >= cfg_.age_hard_decay_min && pressure >= PressureLevel::HIGH)
            wl.state = WorkloadState::RECLAIMABLE;
        break;
    }

    case WorkloadState::RECLAIMABLE:
        // Workspace return → rescue from reclaim
        if (ws.is_focused || ws.is_visible) {
            wl.state = WorkloadState::ACTIVE;
            break;
        }
        // Execution resumed → rescue
        if (activity == ActivityState::EXECUTING) {
            wl.state = WorkloadState::BACKGROUND_EXECUTING;
            break;
        }
        // PolicyEngine + ReclaimEngine handle actual reclamation
        break;

    case WorkloadState::TERMINATED:
        break; // no transitions out of TERMINATED
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// tick_all — convenience: tick every registered workload
// ─────────────────────────────────────────────────────────────────────────────
void WorkloadManager::tick_all(
        const std::unordered_map<int, WorkspaceState>& workspaces,
        const std::unordered_map<pid_t, ProcessNode>&  graph,
        const ExecutionDetector&                        detector,
        PressureLevel                                   pressure) {

    // Snapshot IDs to avoid holding lock during tick (tick may call back into registry)
    std::vector<uint32_t> ids;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        ids = id_list_;
    }

    for (uint32_t id : ids) {
        Workload* wl = get(id);
        if (!wl) continue;

        // Find the workspace state for this workload
        static const WorkspaceState default_ws{};
        const WorkspaceState* ws = &default_ws;
        auto ws_it = workspaces.find(wl->workspace_id);
        if (ws_it != workspaces.end()) ws = &ws_it->second;

        // Aggregate activity from all member PIDs
        ActivityState activity = detector.aggregate(*wl, graph);

        tick(*wl, *ws, activity, pressure);
    }
}

} // namespace thm
