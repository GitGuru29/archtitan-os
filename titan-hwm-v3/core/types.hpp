// =============================================================================
// titan-hwm-v3/core/types.hpp
// Core data model for THM v3 — workload-centric, no static WS profiles
// =============================================================================
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <sys/types.h>

namespace thm {

using ms_clock   = std::chrono::steady_clock;
using time_point = std::chrono::time_point<ms_clock>;

// ─────────────────────────────────────────────────────────────────────────────
// WorkloadType — classification label ONLY, never used as a kill authority
// ─────────────────────────────────────────────────────────────────────────────
enum class WorkloadType {
    ANDROID_DEV,
    WEB_DEV,
    SYSTEM_DEV,
    CASUAL,
    AI_TASK,
    // ── v3.1 additions ────────────────────────────────────────────────────
    SERVICE,    // HC-07: headless Docker/Podman containers, DBs, daemons
    VM,         // HC-08: QEMU/KVM virtual machines (host process)
    NEUTRAL     // unclassified / unknown
};

// ─────────────────────────────────────────────────────────────────────────────
// WorkloadState — the central state machine
//
// DISCOVERED          → just seen, being classified
// ACTIVE              → workspace focused/visible, user interacting
// BACKGROUND_EXECUTING→ workspace background, useful work detected
// IDLE                → no useful work, no user interaction
// AGING               → idle for age_soft_decay_min minutes
// RECLAIMABLE         → idle for age_hard_decay_min minutes + pressure >= HIGH
// TERMINATED          → removed from registry
// ─────────────────────────────────────────────────────────────────────────────
enum class WorkloadState {
    DISCOVERED,
    ACTIVE,
    BACKGROUND_EXECUTING,
    IDLE,
    AGING,
    RECLAIMABLE,
    TERMINATED
};

// ─────────────────────────────────────────────────────────────────────────────
// ActivityState — per-process / per-workload execution signal
//
// EXECUTING   → CPU delta, child changes, I/O, D-state, or known build command
// IDLE        → confirmed idle across N consecutive samples
// COMPLETED   → process became zombie (Z-state) = work is done
// UNKNOWN     → not yet sampled (cold start)
// ─────────────────────────────────────────────────────────────────────────────
enum class ActivityState {
    EXECUTING,
    IDLE,
    COMPLETED,
    UNKNOWN
};

// ─────────────────────────────────────────────────────────────────────────────
// PressureLevel — aggregated from RAM + PSI
// ─────────────────────────────────────────────────────────────────────────────
enum class PressureLevel {
    NORMAL,     // system healthy
    MODERATE,   // start throttling idle workloads
    HIGH,       // freeze idle workloads
    CRITICAL    // aggressive: drop governor, raise swappiness, reclaim candidates
};

// ─────────────────────────────────────────────────────────────────────────────
// PolicyDecision — output of PolicyEngine, consumed by EnforcementPlane
// ─────────────────────────────────────────────────────────────────────────────
enum class PolicyDecision {
    KEEP_FULL,          // active slice, full CPU weight, OOM protected
    KEEP_BACKGROUND,    // background slice, reduced CPU weight
    THROTTLE,           // background slice + memory.high reduced
    FREEZE,             // SIGSTOP + cgroup freeze
    RECLAIM             // safety-checked termination sequence
};

// ─────────────────────────────────────────────────────────────────────────────
// ProcessNode — single process observed in /proc
// ─────────────────────────────────────────────────────────────────────────────
struct ProcessNode {
    pid_t       pid        = 0;
    pid_t       ppid       = 0;
    std::string comm;           // /proc/pid/stat field 2
    std::string exe;            // /proc/pid/exe readlink result
    std::string cmdline;        // argv[0] from /proc/pid/cmdline
    std::string cwd;            // /proc/pid/cwd readlink result
    char        proc_state = '?'; // R, S, D, T, Z from /proc/pid/stat
    long        rss_kb     = 0;
    long        cpu_ticks  = 0;  // utime+stime snapshot for delta
    std::vector<pid_t> children;

    // Ownership — assigned by OwnershipGraph
    uint32_t    workload_id  = 0;   // 0 = unassigned
    int         workspace_id = -1;  // -1 = unknown

    // Activity — updated by ExecutionDetector
    ActivityState activity      = ActivityState::UNKNOWN;
    int           idle_count    = 0; // consecutive idle samples
    time_point    last_active;
};

// ─────────────────────────────────────────────────────────────────────────────
// Workload — group of related processes forming one logical unit of work
// ─────────────────────────────────────────────────────────────────────────────
struct Workload {
    uint32_t      id;
    int           workspace_id  = -1;
    WorkloadType  type          = WorkloadType::NEUTRAL;
    WorkloadState state         = WorkloadState::DISCOVERED;

    // Classification metadata (from FusionClassifier)
    float         confidence    = 0.0f;
    bool          has_lsp       = false;   // clangd/tsserver/rust-analyzer child
    bool          is_building   = false;   // gradle/cargo/make/cmake child active

    // Behaviour modifiers
    bool          is_protected       = false; // hard block — never touch
    bool          ai_owned           = false; // AI agent owns this workload
    bool          latency_sensitive  = false; // TitanMirror, audio, real-time
    // ── v3.1 additions ────────────────────────────────────────────────────
    bool          is_browser_root    = false; // HC-05/06: browser root — skip SIGSTOP on FREEZE
    bool          is_reclassifiable  = false; // HC-10: allow WorkloadType re-evaluation
    int           ws_hysteresis_ticks = 0;   // HC-09: countdown ticks before downgrade on WS leave
    std::string   root_cmdline;              // HC-10: snapshot of root PID cmdline for drift detection

    // Lifecycle timestamps
    time_point    created_at;
    time_point    last_active;       // last user interaction or focus
    time_point    last_executing;    // last time ActivityState::EXECUTING confirmed

    // Member processes
    std::vector<pid_t> pids;         // all member PIDs (root + children)
    pid_t              root_pid = 0; // the application root PID
};

// ─────────────────────────────────────────────────────────────────────────────
// WorkspaceState — topology only, no profile or kill authority
// ─────────────────────────────────────────────────────────────────────────────
struct WorkspaceState {
    int           id           = -1;
    bool          is_visible   = false; // on a physical monitor right now
    bool          is_focused   = false; // receiving keyboard input
    time_point    last_user_activity;
    std::string   active_window_class;  // Hyprland window class of focused window
    std::string   active_window_title;  // Hyprland window title of focused window
    std::vector<uint32_t> workload_ids; // workloads associated with this WS
};

// ─────────────────────────────────────────────────────────────────────────────
// HardwarePolicy — result of compute_hardware_policy() in Phase 8
// ─────────────────────────────────────────────────────────────────────────────
struct HardwarePolicy {
    std::string governor     = "schedutil";
    int         swappiness   = 60;
    std::string io_scheduler = "bfq";
};

// ─────────────────────────────────────────────────────────────────────────────
// ClassificationResult — output of FusionClassifier (Phase 3)
// NOTE: this is descriptive metadata only. It NEVER selects processes to kill.
// ─────────────────────────────────────────────────────────────────────────────
struct ClassificationResult {
    WorkloadType type        = WorkloadType::NEUTRAL;
    float        confidence  = 0.0f;
    bool         is_building = false;  // build daemon child detected
    bool         has_lsp     = false;  // LSP server child detected
    bool         ai_modifier = false;  // AI agent child detected
    bool         latency_sensitive = false; // streaming / media detected
    bool         is_browser_root   = false; // HC-05/06: root is a browser process
    // Full per-type score map for polyglot / multi-context detection
    std::unordered_map<int, float> score_map;
};

// ─────────────────────────────────────────────────────────────────────────────
// Helper: human-readable labels
// ─────────────────────────────────────────────────────────────────────────────
inline const char* to_string(WorkloadType t) {
    switch (t) {
        case WorkloadType::ANDROID_DEV: return "ANDROID_DEV";
        case WorkloadType::WEB_DEV:     return "WEB_DEV";
        case WorkloadType::SYSTEM_DEV:  return "SYSTEM_DEV";
        case WorkloadType::CASUAL:      return "CASUAL";
        case WorkloadType::AI_TASK:     return "AI_TASK";
        case WorkloadType::SERVICE:     return "SERVICE";    // v3.1
        case WorkloadType::VM:          return "VM";         // v3.1
        default:                        return "NEUTRAL";
    }
}

inline const char* to_string(WorkloadState s) {
    switch (s) {
        case WorkloadState::DISCOVERED:           return "DISCOVERED";
        case WorkloadState::ACTIVE:               return "ACTIVE";
        case WorkloadState::BACKGROUND_EXECUTING: return "BACKGROUND_EXECUTING";
        case WorkloadState::IDLE:                 return "IDLE";
        case WorkloadState::AGING:                return "AGING";
        case WorkloadState::RECLAIMABLE:          return "RECLAIMABLE";
        case WorkloadState::TERMINATED:           return "TERMINATED";
        default:                                  return "UNKNOWN_STATE";
    }
}

inline const char* to_string(ActivityState a) {
    switch (a) {
        case ActivityState::EXECUTING: return "EXECUTING";
        case ActivityState::IDLE:      return "IDLE";
        case ActivityState::COMPLETED: return "COMPLETED";
        default:                       return "UNKNOWN";
    }
}

inline const char* to_string(PressureLevel p) {
    switch (p) {
        case PressureLevel::NORMAL:   return "NORMAL";
        case PressureLevel::MODERATE: return "MODERATE";
        case PressureLevel::HIGH:     return "HIGH";
        case PressureLevel::CRITICAL: return "CRITICAL";
        default:                      return "UNKNOWN";
    }
}

inline const char* to_string(PolicyDecision d) {
    switch (d) {
        case PolicyDecision::KEEP_FULL:       return "KEEP_FULL";
        case PolicyDecision::KEEP_BACKGROUND: return "KEEP_BACKGROUND";
        case PolicyDecision::THROTTLE:        return "THROTTLE";
        case PolicyDecision::FREEZE:          return "FREEZE";
        case PolicyDecision::RECLAIM:         return "RECLAIM";
        default:                              return "UNKNOWN_DECISION";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Convenience: minutes elapsed since a time_point
// ─────────────────────────────────────────────────────────────────────────────
inline long minutes_since(const time_point& tp) {
    return std::chrono::duration_cast<std::chrono::minutes>(
        ms_clock::now() - tp).count();
}

} // namespace thm
