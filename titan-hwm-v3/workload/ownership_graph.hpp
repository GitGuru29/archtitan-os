// =============================================================================
// titan-hwm-v3/workload/ownership_graph.hpp
// Phase 2 — Process Ownership Graph + Cold-Start Bootstrap
//
// Maps every observed PID to a workload ID. The graph is a forest: each
// process tree has a root PID, and all descendants inherit that root's workload.
// =============================================================================
#pragma once

#include "../core/types.hpp"

#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <vector>
#include <string>
#include <sys/types.h>

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// WorkloadRegistry — forward-declared reference the graph uses for inference
// ─────────────────────────────────────────────────────────────────────────────
class WorkloadManager;

// ─────────────────────────────────────────────────────────────────────────────
// CwdMarker — filesystem signals used to infer workload type from CWD
// ─────────────────────────────────────────────────────────────────────────────
struct CwdMarker {
    std::string  filename;
    WorkloadType type;
};

inline const std::vector<CwdMarker>& cwd_markers() {
    static const std::vector<CwdMarker> markers = {
        // Android — checked before generic build files
        { "build.gradle",        WorkloadType::ANDROID_DEV },
        { "build.gradle.kts",    WorkloadType::ANDROID_DEV },
        { "settings.gradle",     WorkloadType::ANDROID_DEV },
        { "settings.gradle.kts", WorkloadType::ANDROID_DEV },
        { "gradlew",             WorkloadType::ANDROID_DEV },
        // Web
        { "package.json",        WorkloadType::WEB_DEV     },
        { "bun.lockb",           WorkloadType::WEB_DEV     },
        { "pnpm-lock.yaml",      WorkloadType::WEB_DEV     },
        { "vite.config.ts",      WorkloadType::WEB_DEV     },
        { "next.config.js",      WorkloadType::WEB_DEV     },
        { "next.config.ts",      WorkloadType::WEB_DEV     },
        // System / kernel dev
        { "Cargo.toml",          WorkloadType::SYSTEM_DEV  },
        { "CMakeLists.txt",      WorkloadType::SYSTEM_DEV  },
        { "Makefile",            WorkloadType::SYSTEM_DEV  },
        { "meson.build",         WorkloadType::SYSTEM_DEV  },
        { "PKGBUILD",            WorkloadType::SYSTEM_DEV  },
        { ".SRCINFO",            WorkloadType::SYSTEM_DEV  },
    };
    return markers;
}

// Known IDE/editor comm names → workload type hint (used in bootstrap)
inline WorkloadType infer_type_from_comm(const std::string& comm) {
    // Android-specific IDEs
    if (comm == "studio"    || comm == "studio64"  ||
        comm == "java"      || comm == "kotlin"    ||
        comm == "android-studio") return WorkloadType::ANDROID_DEV;
    // Web IDEs
    if (comm == "code"      || comm == "Code"      ||
        comm == "codium"    || comm == "cursor"    ||
        comm == "webstorm"  || comm == "node")      return WorkloadType::WEB_DEV;
    // AI tools
    if (comm == "ollama"    || comm == "llama-server" ||
        comm == "llamacpp"  || comm == "aider"     ||
        comm == "sgpt")                             return WorkloadType::AI_TASK;
    // System dev
    if (comm == "clion"     || comm == "clangd"    ||
        comm == "rust-analyzer" || comm == "cargo" ||
        comm == "cmake"     || comm == "make"      ||
        comm == "ninja"     || comm == "cc1plus"   ||
        comm == "ld"        || comm == "as")        return WorkloadType::SYSTEM_DEV;
    return WorkloadType::NEUTRAL;
}

// ─────────────────────────────────────────────────────────────────────────────
// OwnershipGraph
// ─────────────────────────────────────────────────────────────────────────────
class OwnershipGraph {
public:
    // ── Query ────────────────────────────────────────────────────────────────

    // Returns the workload_id this PID belongs to (0 = unassigned)
    uint32_t workload_of(pid_t pid) const;

    // True if any PID in the workload still has living children in /proc
    bool has_live_children(uint32_t workload_id) const;

    // ── Mutation ─────────────────────────────────────────────────────────────

    // Assign a pid to a workload (parent assignment propagates to children)
    void assign(pid_t pid, uint32_t workload_id);

    // Remove a pid from the graph (called when /proc/<pid> disappears)
    void remove(pid_t pid);

    // ── Cold-start bootstrap ─────────────────────────────────────────────────

    // Called once at startup. Scans all /proc/<pid> entries, sorts by PPID,
    // and assigns each PID to a workload via ancestry or CWD inference.
    // Returns the set of newly created Workload stubs (type+pids filled in,
    // remaining fields to be completed by WorkloadManager).
    std::vector<Workload> bootstrap(uint32_t& next_id);

private:
    mutable std::mutex mtx_;

    // pid → workload_id
    std::unordered_map<pid_t, uint32_t> pid_to_wl_;

    // workload_id → set of member pids
    std::unordered_map<uint32_t, std::unordered_set<pid_t>> wl_to_pids_;

    // ── Helpers ───────────────────────────────────────────────────────────────
    static WorkloadType infer_from_cwd(const std::string& cwd);
    static std::string  read_cwd(pid_t pid);
    static std::string  read_comm(pid_t pid);
    static pid_t        read_ppid(pid_t pid);
};

} // namespace thm
