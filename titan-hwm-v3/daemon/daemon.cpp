// =============================================================================
// titan-hwm-v3/daemon/daemon.cpp
// Phase 12 — Main daemon entry point
//
// Wires all phases together into a single event-driven tick loop:
//   1. Bootstrap protected registry + ownership graph
//   2. Start Hyprland IPC listener
//   3. Every tick_ms:
//      a. Refresh /proc snapshot
//      b. Classify new processes (FusionClassifier)
//      c. Detect execution state per workload
//      d. Drive state machine (WorkloadManager)
//      e. Evaluate policy decisions (PolicyEngine)
//      f. Enforce decisions (EnforcementPlane / ReclaimEngine)
//      g. Update governor from aggregate demand
// =============================================================================
#include "../core/types.hpp"
#include "../core/protected_registry.hpp"
#include "../workload/ownership_graph.hpp"
#include "../workload/workload_manager.hpp"
#include "../classifier/fusion_classifier.hpp"
#include "../classifier/execution_detector.hpp"
#include "../policy/policy_engine.hpp"
#include "../policy/reclaim_engine.hpp"
#include "../enforcement/cgroup_controller.hpp"
#include "../enforcement/enforcement_plane.hpp"
#include "../resources/pressure_level.hpp"
#include "../ipc/workspace_monitor.hpp"

#include <iostream>
#include <fstream>
#include <csignal>
#include <chrono>
#include <thread>
#include <filesystem>
#include <algorithm>
#include <unistd.h>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Global shutdown flag (SIGTERM / SIGINT handler)
// ─────────────────────────────────────────────────────────────────────────────
static volatile sig_atomic_t g_shutdown = 0;
static void handle_signal(int) { g_shutdown = 1; }

// ─────────────────────────────────────────────────────────────────────────────
// Build a fresh /proc snapshot (like v2 MemoryGraph::build())
// ─────────────────────────────────────────────────────────────────────────────
static void refresh_proc_graph(std::unordered_map<pid_t, thm::ProcessNode>& graph) {
    graph.clear();
    for (const auto& entry : fs::directory_iterator("/proc")) {
        if (!entry.is_directory()) continue;
        const std::string d = entry.path().filename().string();
        if (!std::all_of(d.begin(), d.end(), ::isdigit)) continue;
        pid_t pid = 0;
        try { pid = static_cast<pid_t>(std::stoi(d)); } catch (...) { continue; }
        if (pid <= 1) continue;

        thm::ProcessNode n;
        n.pid = pid;

        // Read comm, state, ppid from /proc/<pid>/stat
        std::ifstream sf("/proc/" + d + "/stat");
        std::string line;
        if (std::getline(sf, line)) {
            size_t lp = line.find('('), rp = line.rfind(')');
            if (lp != std::string::npos && rp != std::string::npos) {
                n.comm = line.substr(lp + 1, rp - lp - 1);
                std::istringstream iss(line.substr(rp + 2));
                iss >> n.proc_state >> n.ppid;
            }
        }

        // Read cmdline
        {
            std::ifstream cf("/proc/" + d + "/cmdline", std::ios::binary);
            std::string raw;
            std::getline(cf, raw, '\0');
            auto slash = raw.rfind('/');
            n.cmdline = (slash != std::string::npos) ? raw.substr(slash + 1) : raw;
        }

        // Read RSS from /proc/<pid>/statm (field 2, pages)
        {
            std::ifstream mf("/proc/" + d + "/statm");
            long total_pages = 0, rss_pages = 0;
            if (mf >> total_pages >> rss_pages)
                n.rss_kb = rss_pages * 4; // 4 KB pages
        }

        graph[pid] = std::move(n);
    }

    // Build child lists
    for (auto& [pid, node] : graph) {
        if (node.ppid > 0 && graph.count(node.ppid))
            graph[node.ppid].children.push_back(pid);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    std::cout << "╔══════════════════════════════════════════╗\n"
              << "║  Titan Hardware Manager v3               ║\n"
              << "║  Workload-centric resource management    ║\n"
              << "╚══════════════════════════════════════════╝\n";

    // ── Signal handlers ─────────────────────────────────────────────────────
    signal(SIGTERM, handle_signal);
    signal(SIGINT,  handle_signal);

    // ── Configuration ────────────────────────────────────────────────────────
    const int tick_ms = 200; // 5 Hz tick rate

    // ── Phase 0: Protected Domain Registry ──────────────────────────────────
    thm::ProtectedRegistry registry;
    registry.register_pid(getpid(), "THM-self");
    registry.bootstrap_from_systemd();

    // ── Phase 7: cgroup hierarchy ───────────────────────────────────────────
    thm::CgroupController cgroup;
    if (!cgroup.setup_slices()) {
        std::cerr << "[THM] WARNING: cgroup hierarchy setup failed — "
                  << "enforcement will use SIGSTOP/SIGCONT only.\n";
    }

    // ── Phase 9/10: Enforcement + Reclaim ───────────────────────────────────
    thm::EnforcementPlane enforcement(cgroup, registry);
    thm::ExecutionDetector detector;
    thm::ReclaimEngine reclaimer(registry, detector);

    // ── Phase 5: Workload registry ──────────────────────────────────────────
    thm::WorkloadManager workload_mgr;
    thm::OwnershipGraph  ownership;
    thm::FusionClassifier classifier;
    thm::PolicyEngine     policy;

    // ── Phase 2: Cold-start bootstrap ───────────────────────────────────────
    uint32_t next_workload_id = 1;
    auto bootstrap = ownership.bootstrap(next_workload_id);
    for (auto& wl : bootstrap) {
        workload_mgr.add(std::move(wl));
    }
    std::cout << "[THM] Bootstrap: " << bootstrap.size()
              << " workload(s) discovered.\n";

    // ── Phase 8: Workspace monitor ──────────────────────────────────────────
    thm::WorkspaceMonitor ws_monitor;
    ws_monitor.on_workspace_change([](int id, bool focused) {
        std::cout << "[WS] Switch → workspace " << id << "\n";
    });
    ws_monitor.on_window_change([](int ws, const std::string& cls,
                                    const std::string& title) {
        std::cout << "[WS] Window: " << cls << " — " << title << "\n";
    });
    if (!ws_monitor.start()) {
        std::cerr << "[THM] Hyprland IPC unavailable — running in headless mode.\n";
    }

    // ── Proc graph (reused each tick) ───────────────────────────────────────
    std::unordered_map<pid_t, thm::ProcessNode> proc_graph;

    // ── Previous child sets for execution detection ─────────────────────────
    std::unordered_map<pid_t, std::vector<pid_t>> prev_children;

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // MAIN TICK LOOP
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    std::cout << "[THM] Entering main tick loop (" << tick_ms << "ms interval)\n";

    while (!g_shutdown) {
        auto tick_start = std::chrono::steady_clock::now();

        // ── (a) Refresh /proc snapshot ──────────────────────────────────────
        refresh_proc_graph(proc_graph);

        // ── (b) Refresh workspace visibility ────────────────────────────────
        ws_monitor.refresh_visibility();
        auto workspaces = ws_monitor.snapshot_workspaces();

        // ── (c) Read pressure ───────────────────────────────────────────────
        thm::MemInfo  mem = thm::read_meminfo();
        thm::PSIStats psi = thm::read_memory_psi();
        thm::PressureLevel pressure = thm::compute_pressure(mem, psi);

        // ── (d) Classify + assign new processes ─────────────────────────────
        for (auto& [pid, node] : proc_graph) {
            if (ownership.workload_of(pid) != 0) continue; // already assigned
            if (registry.is_protected(pid, node.comm)) continue; // skip protected

            // Get window title for this workspace
            int ws_id = ws_monitor.focused_workspace();
            std::string title;
            auto ws_it = workspaces.find(ws_id);
            if (ws_it != workspaces.end())
                title = ws_it->second.active_window_title;

            auto result = classifier.classify(pid, title, proc_graph);
            if (result.type != thm::WorkloadType::NEUTRAL) {
                thm::Workload wl;
                wl.id               = next_workload_id++;
                wl.type             = result.type;
                wl.confidence       = result.confidence;
                wl.ai_owned         = result.ai_modifier;
                wl.is_building      = result.is_building;
                wl.latency_sensitive = result.latency_sensitive;
                wl.root_pid         = pid;
                wl.state            = thm::WorkloadState::DISCOVERED;
                wl.workspace_id     = ws_id;
                wl.created_at       = thm::ms_clock::now();
                wl.last_active      = thm::ms_clock::now();
                wl.pids.push_back(pid);
                ownership.assign(pid, wl.id);
                workload_mgr.add(std::move(wl));
            }
        }

        // ── (e) Detect execution state per process + aggregate ──────────────
        for (auto& [pid, node] : proc_graph) {
            auto& prev = prev_children[pid];
            detector.detect(node, prev);
            prev = node.children; // snapshot for next tick
        }

        // ── (f) Drive state machine + evaluate policy + enforce ─────────────
        // Build temporary registry snapshot for reclaim dependency check
        std::unordered_map<uint32_t, thm::Workload> all_workloads;
        for (uint32_t id : workload_mgr.all_ids()) {
            auto* wl = workload_mgr.get(id);
            if (wl) all_workloads[id] = *wl;
        }

        for (uint32_t id : workload_mgr.all_ids()) {
            auto* wl = workload_mgr.get(id);
            if (!wl || wl->state == thm::WorkloadState::TERMINATED) continue;

            // Find workspace state
            static const thm::WorkspaceState default_ws{};
            const thm::WorkspaceState* ws = &default_ws;
            auto ws_it = workspaces.find(wl->workspace_id);
            if (ws_it != workspaces.end()) ws = &ws_it->second;

            // Aggregate activity
            thm::ActivityState activity = detector.aggregate(*wl, proc_graph);

            // Drive state machine
            workload_mgr.tick(*wl, *ws, activity, pressure);

            // Evaluate policy
            thm::PolicyDecision decision = policy.evaluate(*wl, *ws, activity,
                                                            pressure, registry);

            // Enforce
            if (decision == thm::PolicyDecision::RECLAIM) {
                if (reclaimer.reclaim(*wl, proc_graph, all_workloads))
                    wl->state = thm::WorkloadState::TERMINATED;
            } else {
                enforcement.apply(*wl, decision);
            }
        }

        // ── (g) Governor update ─────────────────────────────────────────────
        auto gov = thm::WorkspaceMonitor::compute_governor(all_workloads, pressure);
        thm::WorkspaceMonitor::apply_governor(gov);

        // ── (h) Clean up dead processes ─────────────────────────────────────
        std::vector<pid_t> dead;
        for (const auto& [pid, _] : prev_children) {
            if (!proc_graph.count(pid)) dead.push_back(pid);
        }
        for (pid_t pid : dead) {
            ownership.remove(pid);
            detector.remove_baseline(pid);
            prev_children.erase(pid);
        }

        // ── Sleep remaining tick budget ─────────────────────────────────────
        auto elapsed = std::chrono::steady_clock::now() - tick_start;
        auto sleep_for = std::chrono::milliseconds(tick_ms) - elapsed;
        if (sleep_for.count() > 0)
            std::this_thread::sleep_for(sleep_for);
    }

    // ── Shutdown ────────────────────────────────────────────────────────────
    std::cout << "\n[THM] Shutdown initiated.\n";

    // Thaw all frozen processes before exiting
    for (uint32_t id : workload_mgr.all_ids()) {
        auto* wl = workload_mgr.get(id);
        if (wl) enforcement.sigcont_tree(wl->pids);
    }
    cgroup.set_freeze(thm::SLICE_FROZEN, false);

    ws_monitor.stop();

    std::cout << "[THM] Clean shutdown complete.\n";
    return 0;
}
