// =============================================================================
// titan-hwm-v3/ipc/workspace_monitor.hpp
// Phase 8 — Workspace Transition Engine + Hardware Policy
//
// Monitors Hyprland IPC for workspace/window events.
// Feeds WorkspaceState changes into the policy tick loop.
// Also computes the CPU governor policy from aggregate system demand.
// =============================================================================
#pragma once

#include "../core/types.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// GovernorPolicy — what the CPU governor should be set to
// Derived from aggregate demand across ALL workspaces + latency requirements,
// NOT from the type of the focused workspace (user-approved correction).
// ─────────────────────────────────────────────────────────────────────────────
enum class GovernorHint {
    POWERSAVE,      // all idle, no builds, no active user input
    SCHEDUTIL,      // default — let kernel autoscale
    PERFORMANCE,    // any build executing OR latency-sensitive workload active
};

// Callback type for workspace events
using WorkspaceCallback = std::function<void(int workspace_id, bool is_focused)>;
using WindowCallback    = std::function<void(int workspace_id,
                                              const std::string& window_class,
                                              const std::string& window_title)>;

// ─────────────────────────────────────────────────────────────────────────────
// WorkspaceMonitor — Hyprland IPC socket listener
// ─────────────────────────────────────────────────────────────────────────────
class WorkspaceMonitor {
public:
    // Set callbacks before calling start()
    void on_workspace_change(WorkspaceCallback cb)  { ws_cb_  = std::move(cb); }
    void on_window_change(WindowCallback cb)        { win_cb_ = std::move(cb); }

    // Start the IPC listener thread. Non-blocking; runs in background.
    bool start();

    // Stop the listener thread (call from destructor or shutdown).
    void stop();

    // ── State queries ────────────────────────────────────────────────────────
    int  focused_workspace() const { return focused_ws_.load(); }

    // Get current WorkspaceState map (thread-safe snapshot)
    std::unordered_map<int, WorkspaceState> snapshot_workspaces() const;

    // Update visibility info from hyprctl workspaces (called during tick)
    void refresh_visibility();

    // ── Governor hint ────────────────────────────────────────────────────────
    // Compute from aggregate system demand. Called each tick.
    static GovernorHint compute_governor(
        const std::unordered_map<uint32_t, Workload>& workloads,
        PressureLevel pressure);

    // Apply governor hint to /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
    static bool apply_governor(GovernorHint hint);

    ~WorkspaceMonitor() { stop(); }

private:
    WorkspaceCallback ws_cb_;
    WindowCallback    win_cb_;

    std::atomic<int>  focused_ws_{1};
    std::atomic<bool> running_{false};
    std::thread       listener_thread_;

    mutable std::mutex ws_mtx_;
    std::unordered_map<int, WorkspaceState> workspaces_;

    // IPC socket path discovery
    static std::string hyprland_socket_path();

    // Main listener loop (runs on listener_thread_)
    void listen_loop();

    // Parse Hyprland IPC event lines
    void handle_event(const std::string& line);
};

} // namespace thm
