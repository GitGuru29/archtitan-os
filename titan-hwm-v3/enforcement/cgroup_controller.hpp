// =============================================================================
// titan-hwm-v3/enforcement/cgroup_controller.hpp
// Phase 7 — cgroup v2 Controller (systemd-delegation-aware)
//
// Operates entirely within archtitan.slice, which is declared in
// /etc/systemd/system/archtitan.slice with Delegate=yes.
// Never creates cgroups outside that slice.
// =============================================================================
#pragma once

#include "../core/types.hpp"
#include <string>
#include <vector>
#include <sys/types.h>

namespace thm {

// Named slices within archtitan.slice
static constexpr const char* SLICE_ROOT       = "/sys/fs/cgroup/archtitan.slice";
static constexpr const char* SLICE_PROTECTED  = "archtitan-protected.slice";
static constexpr const char* SLICE_ACTIVE     = "archtitan-active.slice";
static constexpr const char* SLICE_BACKGROUND = "archtitan-background.slice";
static constexpr const char* SLICE_FROZEN     = "archtitan-frozen.slice";

// Map PolicyDecision to the target slice name
inline const char* slice_for_decision(PolicyDecision d) {
    switch (d) {
        case PolicyDecision::KEEP_FULL:       return SLICE_ACTIVE;
        case PolicyDecision::KEEP_BACKGROUND: return SLICE_BACKGROUND;
        case PolicyDecision::THROTTLE:        return SLICE_BACKGROUND;
        case PolicyDecision::FREEZE:          return SLICE_FROZEN;
        case PolicyDecision::RECLAIM:         return SLICE_BACKGROUND; // transient until killed
        default:                              return SLICE_BACKGROUND;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CgroupController
// ─────────────────────────────────────────────────────────────────────────────
class CgroupController {
public:
    // One-time setup — create the 4 named slices under archtitan.slice.
    // Safe to call multiple times (idempotent).
    bool setup_slices();

    // Move a list of PIDs into a named slice (cgroup.procs write)
    bool move_to_slice(const std::vector<pid_t>& pids,
                       const std::string& slice_name);

    // Set cpu.weight inside archtitan.slice/<slice> (1–10000, default 100)
    bool set_cpu_weight(const std::string& slice_name, int weight);

    // Set memory.high limit (bytes; 0 = remove limit / restore to max)
    bool set_memory_high(const std::string& slice_name, long bytes);

    // Set cgroup.freeze (1 = frozen, 0 = thawed)
    bool set_freeze(const std::string& slice_name, bool freeze);

    // Set oom_score_adj for each PID in the list
    void set_oom_score(const std::vector<pid_t>& pids, int score);

    // Per-workload scope path (optional fine-grained isolation)
    static std::string workload_scope(uint32_t workload_id) {
        return std::string(SLICE_ROOT) + "/archtitan-workload-" +
               std::to_string(workload_id) + ".scope";
    }

    // True if the cgroup hierarchy is properly delegated to THM
    bool hierarchy_healthy() const;

private:
    static bool write_file(const std::string& path, const std::string& value);
    static bool mkdir_p(const std::string& path);
    static std::string slice_path(const std::string& slice_name);
};

} // namespace thm
