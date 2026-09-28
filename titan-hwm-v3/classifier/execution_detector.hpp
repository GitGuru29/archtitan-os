// =============================================================================
// titan-hwm-v3/classifier/execution_detector.hpp
// Phase 4 — Execution Detector
//
// Determines ActivityState for a process/workload from multi-signal observation:
//   1. CPU tick delta over a sample window
//   2. Child set membership change
//   3. Kernel process state (R=running, D=uninterruptible I/O)
//   4. Known build command in cmdline
//
// ActivityState::IDLE must be confirmed N consecutive samples (idle_confirm_count)
// before a workload transitions out of BACKGROUND_EXECUTING.
// =============================================================================
#pragma once

#include "../core/types.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <sys/types.h>

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// ExecutionConfig — tunable parameters (sourced from Config in Phase 12)
// ─────────────────────────────────────────────────────────────────────────────
struct ExecutionConfig {
    int   sample_ms          = 50;   // CPU tick sample window in milliseconds
    float idle_cpu_pct       = 0.5f; // below this % CPU → considered idle
    int   idle_confirm_count = 3;    // must be idle N consecutive scans to confirm
};

// ─────────────────────────────────────────────────────────────────────────────
// Known build command prefixes — a process running one of these is always
// considered EXECUTING, even if instantaneous CPU is temporarily low
// (e.g. waiting for disk I/O between compile steps).
// ─────────────────────────────────────────────────────────────────────────────
inline bool is_known_build_command(const std::string& cmdline) {
    static const std::vector<std::string> BUILD_CMDS = {
        // Compilers / linkers
        "gradle", "gradlew", "./gradlew",
        "cargo",  "cargo build", "cargo run", "cargo test",
        "cmake",  "cmake --build",
        "make",   "ninja",
        "cc1",    "cc1plus", "collect2",    // GCC internals
        "ld",     "ld.lld",  "ld.gold",
        "rustc",  "kotlinc", "javac",
        "tsc",    "esbuild", "vite build",
        "webpack","rollup",  "parcel",
        "bazel",  "buck2",   "meson",
        "aarch64-linux-gnu-ld",             // cross-compile linkers
        // HC-02: dev file-watchers (sleep on inotify — always treat as building)
        "vite",          "webpack-dev-server", "webpack serve",
        "tsc --watch",   "nodemon",
        "next dev",      "next",
        // HC-04: test runners that sleep between suites
        "pytest",  "py.test",
        "jest",    "vitest",
        "mocha",   "jasmine",
        "go test",
    };
    for (const auto& cmd : BUILD_CMDS)
        if (cmdline.find(cmd) != std::string::npos)
            return true;
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// HC-01, HC-04, HC-07, HC-08: Persistent service / infrastructure processes
// that must NEVER be classified as idle regardless of CPU delta.
// Includes headless DB servers, emulators, container runtimes, VM hosts.
// ─────────────────────────────────────────────────────────────────────────────
inline bool is_persistent_service(const std::string& cmdline) {
    static const std::vector<std::string> PERSISTENT_CMDS = {
        // HC-01: Android emulator host process
        "qemu-system-x86_64", "qemu-system-aarch64", "qemu-system-arm",
        "emulator",           "android-emulator",    "qemu-img",
        // HC-07: Database servers (headless, sleep between queries)
        "postgres", "mysqld", "mariadbd",
        "redis-server", "mongod", "sqlite3",
        "cassandra",  "elasticsearch",
        // HC-07: Container / orchestration runtimes
        "dockerd",    "docker",
        "containerd", "crun",  "runc",
        "podman",     "buildkitd",
        // HC-07: Dev-stack reverse proxies / API servers
        "nginx",  "caddy",  "traefik", "haproxy",
        // HC-08: VM hypervisor hosts
        "qemu-kvm", "kvm",
    };
    for (const auto& cmd : PERSISTENT_CMDS)
        if (cmdline.find(cmd) != std::string::npos)
            return true;
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// ExecutionDetector
// ─────────────────────────────────────────────────────────────────────────────
class ExecutionDetector {
public:
    explicit ExecutionDetector(ExecutionConfig cfg = {}) : cfg_(cfg) {}

    // Detect the ActivityState of a single process.
    // Updates node.activity and node.idle_count in-place.
    // prev_children: the child set from the previous scan (used to detect change).
    void detect(ProcessNode& node,
                const std::vector<pid_t>& prev_children);

    // Aggregate ActivityState across all PIDs of a workload.
    // A workload is EXECUTING if ANY member is EXECUTING.
    // A workload is COMPLETED if ALL members are COMPLETED (zombie or gone).
    // Otherwise IDLE.
    ActivityState aggregate(const Workload& wl,
                             const std::unordered_map<pid_t, ProcessNode>& graph) const;

    // Update the per-PID CPU tick baseline (call after each scan tick)
    void update_baseline(pid_t pid, long ticks);
    void remove_baseline(pid_t pid);

private:
    ExecutionConfig cfg_;

    // Previous CPU tick counts for delta calculation (pid → utime+stime)
    std::unordered_map<pid_t, long> prev_ticks_;

    // Read utime+stime from /proc/<pid>/stat
    static long read_cpu_ticks(pid_t pid);

    // Compute CPU percentage from tick delta over sample_ms
    static float cpu_pct(long t0, long t1, int sample_ms);
};

} // namespace thm
