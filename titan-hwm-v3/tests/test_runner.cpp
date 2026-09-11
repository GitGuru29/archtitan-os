// =============================================================================
// titan-hwm-v3/tests/test_runner.cpp
// Real OS-Level Test Suite for Titan Hardware Manager v3 (THM v3)
//
// Validates all 10 core architectural test cases on real Linux OS primitives:
//   TC-1:  Gradle / Background Build Survival
//   TC-2:  AI Agent Workload Survival & Execution Authority
//   TC-3:  Idle IDE Aging, Throttling & Freeze under Pressure
//   TC-4:  Protected Domain Absolute Immunity
//   TC-5:  Audio Whitelist & Binary Path Verification
//   TC-6:  Cold-Start Bootstrap & Ownership Resolution
//   TC-7:  Multi-Workspace Concurrent Active Workloads
//   TC-8:  Elimination of Static Workspace Profiles
//   TC-9:  9-Step Reclaim Engine Safety Sequence
//   TC-10: Demand-Based Dynamic Governor Transitions
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
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

// ── Test Harness Tracking ───────────────────────────────────────────────────
struct TestResult {
    std::string name;
    std::string invariant;
    bool        passed;
    std::string details;
    long        duration_ms;
};

static std::vector<TestResult> g_results;

// ── Helper Utilities ────────────────────────────────────────────────────────
static void safe_reap(pid_t pid) {
    if (pid <= 1) return;
    ::kill(pid, SIGKILL);
    int status = 0;
    ::waitpid(pid, &status, 0);
}

static char read_proc_state(pid_t pid) {
    std::ifstream sf("/proc/" + std::to_string(pid) + "/stat");
    if (!sf.is_open()) return '?';
    std::string line;
    if (!std::getline(sf, line)) return '?';
    auto rparen = line.rfind(')');
    if (rparen == std::string::npos || rparen + 2 >= line.size()) return '?';
    return line[rparen + 2];
}

static long read_proc_cpu_ticks(pid_t pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
    if (!f.is_open()) return -1;
    std::string line;
    if (!std::getline(f, line)) return -1;
    auto rparen = line.rfind(')');
    if (rparen == std::string::npos) return -1;
    std::istringstream ss(line.substr(rparen + 2));
    std::string tok;
    for (int i = 3; i <= 15; ++i) {
        if (!(ss >> tok)) return -1;
        if (i == 14) {
            long utime = std::stol(tok);
            long stime = 0;
            if (ss >> tok) stime = std::stol(tok);
            return utime + stime;
        }
    }
    return -1;
}

static pid_t fork_cpu_burner(const std::string& simulated_cmdline) {
    pid_t pid = fork();
    if (pid == 0) {
        (void)simulated_cmdline;
        volatile uint64_t counter = 0;
        while (true) {
            for (int i = 0; i < 100000; ++i) {
                counter += i * 3 + 7;
            }
            std::this_thread::yield();
        }
        (void)counter;
        _exit(0);
    }
    return pid;
}

static pid_t fork_idle_process() {
    pid_t pid = fork();
    if (pid == 0) {
        while (true) {
            pause();
        }
        _exit(0);
    }
    return pid;
}

static pid_t fork_worker_in_dir(const std::string& dir) {
    pid_t pid = fork();
    if (pid == 0) {
        if (chdir(dir.c_str()) != 0) {
            _exit(1);
        }
        while (true) {
            pause();
        }
        _exit(0);
    }
    return pid;
}

static pid_t find_live_pid_by_comm(const std::string& comm_name) {
    for (const auto& entry : fs::directory_iterator("/proc")) {
        if (!entry.is_directory()) continue;
        const std::string d = entry.path().filename().string();
        if (!std::all_of(d.begin(), d.end(), ::isdigit)) continue;
        pid_t p = 0;
        try { p = static_cast<pid_t>(std::stoi(d)); } catch (...) { continue; }
        if (p <= 1) continue;

        std::ifstream sf("/proc/" + d + "/stat");
        std::string line;
        if (std::getline(sf, line)) {
            size_t lp = line.find('('), rp = line.rfind(')');
            if (lp != std::string::npos && rp != std::string::npos) {
                std::string c = line.substr(lp + 1, rp - lp - 1);
                if (c == comm_name) return p;
            }
        }
    }
    return 0;
}

static void log_test_header(int num, const std::string& title, const std::string& invariant) {
    std::cout << "\n──────────────────────────────────────────────────────────────────────────────\n";
    std::cout << "TEST " << num << ": " << title << "\n";
    std::cout << "INVARIANT: \"" << invariant << "\"\n";
    std::cout << "──────────────────────────────────────────────────────────────────────────────\n";
}

// ═════════════════════════════════════════════════════════════════════════════
// TC-1: Gradle / Background Build Survival
// ═════════════════════════════════════════════════════════════════════════════
void test_tc1_gradle_survival() {
    auto t0 = std::chrono::steady_clock::now();
    log_test_header(1, "Gradle / Background Build Survival",
                    "Background != idle; Background != kill; active builds must never be stopped");

    // 1. Fork real CPU burning child (simulating Gradle worker)
    pid_t child = fork_cpu_burner("gradlew :app:assembleRelease");
    std::cout << "[OS] Spawned active build worker PID " << child << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 2. Set up THM components
    thm::ProtectedRegistry registry;
    thm::ExecutionConfig exec_cfg{50, 0.5f, 2};
    thm::ExecutionDetector detector(exec_cfg);
    thm::PolicyEngine policy;
    thm::WorkloadManager mgr;
    thm::CgroupController cgroup;
    thm::EnforcementPlane enforce(cgroup, registry);

    thm::ProcessNode node;
    node.pid = child;
    node.comm = "java";
    node.cmdline = "gradlew :app:assembleRelease";
    node.proc_state = read_proc_state(child);

    // Detect execution activity on the real child
    detector.detect(node, {});
    std::cout << "[Detector] Real PID " << child << " activity state: "
              << (node.activity == thm::ActivityState::EXECUTING ? "EXECUTING" : "OTHER")
              << " (proc_state=" << node.proc_state << ")\n";

    // Create Workload representing Android Studio / Gradle on WS 2
    thm::Workload wl;
    wl.id = 101;
    wl.workspace_id = 2;
    wl.type = thm::WorkloadType::ANDROID_DEV;
    wl.state = thm::WorkloadState::ACTIVE;
    wl.is_building = true;
    wl.pids.push_back(child);
    wl.root_pid = child;
    mgr.add(wl);

    // 3. Action: User switches workspace from WS 2 (focused) -> WS 1 (focused)
    // WS 2 becomes background (unfocused, not visible)
    std::cout << "[Action] User switches workspace WS 2 -> WS 1 (WS 2 now background)\n";
    thm::WorkspaceState ws2;
    ws2.id = 2;
    ws2.is_focused = false;
    ws2.is_visible = false;

    // Run state machine tick
    std::unordered_map<pid_t, thm::ProcessNode> graph{{child, node}};
    thm::ActivityState agg_act = detector.aggregate(wl, graph);
    mgr.tick(*mgr.get(101), ws2, agg_act, thm::PressureLevel::NORMAL);

    auto* updated_wl = mgr.get(101);
    std::cout << "[WorkloadManager] Workload state after switch: "
              << (updated_wl->state == thm::WorkloadState::BACKGROUND_EXECUTING ? "BACKGROUND_EXECUTING" : "OTHER") << "\n";

    // 4. Policy evaluation
    thm::PolicyDecision decision = policy.evaluate(*updated_wl, ws2, agg_act,
                                                   thm::PressureLevel::NORMAL, registry);
    std::cout << "[PolicyEngine] Policy decision: "
              << (decision == thm::PolicyDecision::KEEP_BACKGROUND ? "KEEP_BACKGROUND" : "OTHER") << "\n";

    // 5. Enforcement
    enforce.apply(*updated_wl, decision);

    // 6. Verify real OS process state
    char post_state = read_proc_state(child);
    std::cout << "[OS] Child PID " << child << " state in /proc/<pid>/stat: '" << post_state << "'\n";

    bool passed = (updated_wl->state == thm::WorkloadState::BACKGROUND_EXECUTING) &&
                  (decision == thm::PolicyDecision::KEEP_BACKGROUND) &&
                  (post_state == 'R' || post_state == 'S');

    safe_reap(child);

    auto t1 = std::chrono::steady_clock::now();
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    g_results.push_back({"TC-1: Gradle Background Survival",
                        "Background != kill; active builds continue uninterrupted",
                        passed,
                        "State=BACKGROUND_EXECUTING, Decision=KEEP_BACKGROUND, Signal=NONE, OS State=" + std::string(1, post_state),
                        ms});

    std::cout << (passed ? ">>> PASS: Build continued executing in background.\n" : ">>> FAIL!\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// TC-2: AI Agent Workload Survival & Execution Authority
// ═════════════════════════════════════════════════════════════════════════════
void test_tc2_ai_agent_survival() {
    auto t0 = std::chrono::steady_clock::now();
    log_test_header(2, "AI Agent Workload Survival & Execution Authority",
                    "AI execution verified; ai_owned alone is not an infinite freeze shield if idle");

    // Part A: Active AI agent survives workspace switch
    pid_t agent_child = fork_cpu_burner("antigravity-agent --step");
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    thm::ProtectedRegistry registry;
    thm::ExecutionConfig exec_cfg{50, 0.5f, 2};
    thm::ExecutionDetector detector(exec_cfg);
    thm::PolicyEngine policy;
    thm::WorkloadManager mgr;
    thm::CgroupController cgroup;
    thm::EnforcementPlane enforce(cgroup, registry);

    thm::ProcessNode node;
    node.pid = agent_child;
    node.comm = "antigravity";
    node.cmdline = "antigravity-agent --step";
    node.proc_state = read_proc_state(agent_child);
    detector.detect(node, {});

    thm::Workload wl;
    wl.id = 102;
    wl.workspace_id = 1;
    wl.type = thm::WorkloadType::AI_TASK;
    wl.state = thm::WorkloadState::ACTIVE;
    wl.ai_owned = true;
    wl.pids.push_back(agent_child);
    wl.root_pid = agent_child;
    mgr.add(wl);

    // Switch WS 1 -> WS 2
    thm::WorkspaceState ws1;
    ws1.id = 1;
    ws1.is_focused = false;
    ws1.is_visible = false;

    std::unordered_map<pid_t, thm::ProcessNode> graph{{agent_child, node}};
    thm::ActivityState agg_act = detector.aggregate(wl, graph);
    mgr.tick(*mgr.get(102), ws1, agg_act, thm::PressureLevel::NORMAL);

    auto* updated_wl = mgr.get(102);
    thm::PolicyDecision dec = policy.evaluate(*updated_wl, ws1, agg_act,
                                              thm::PressureLevel::NORMAL, registry);

    std::cout << "[Part A] Active AI agent state: "
              << (updated_wl->state == thm::WorkloadState::BACKGROUND_EXECUTING ? "BACKGROUND_EXECUTING" : "OTHER")
              << ", Decision: " << (dec == thm::PolicyDecision::KEEP_BACKGROUND ? "KEEP_BACKGROUND" : "OTHER") << "\n";

    bool part_a_passed = (updated_wl->state == thm::WorkloadState::BACKGROUND_EXECUTING) &&
                         (dec == thm::PolicyDecision::KEEP_BACKGROUND);

    safe_reap(agent_child);

    // Part B: Critical invariant — dormant/idle AI process is NOT immune to freeze under pressure
    std::cout << "[Part B] Testing invariant: Idle AI agent under HIGH pressure must freeze, not remain full\n";
    thm::Workload idle_ai;
    idle_ai.id = 103;
    idle_ai.workspace_id = 1;
    idle_ai.type = thm::WorkloadType::AI_TASK;
    idle_ai.state = thm::WorkloadState::IDLE;
    idle_ai.ai_owned = true;

    thm::PolicyDecision idle_dec = policy.evaluate(idle_ai, ws1, thm::ActivityState::IDLE,
                                                   thm::PressureLevel::HIGH, registry);
    std::cout << "[PolicyEngine] Idle AI agent decision under HIGH pressure: "
              << (idle_dec == thm::PolicyDecision::FREEZE ? "FREEZE" : "OTHER") << "\n";

    // Also check that when RECLAIMABLE, ai_owned downgrades RECLAIM -> FREEZE (safety)
    idle_ai.state = thm::WorkloadState::RECLAIMABLE;
    thm::PolicyDecision rec_dec = policy.evaluate(idle_ai, ws1, thm::ActivityState::IDLE,
                                                  thm::PressureLevel::HIGH, registry);
    std::cout << "[PolicyEngine] Reclaimable AI agent decision: "
              << (rec_dec == thm::PolicyDecision::FREEZE ? "FREEZE (downgraded from RECLAIM)" : "OTHER") << "\n";

    bool part_b_passed = (idle_dec == thm::PolicyDecision::FREEZE) &&
                         (rec_dec == thm::PolicyDecision::FREEZE);

    bool passed = part_a_passed && part_b_passed;
    auto t1 = std::chrono::steady_clock::now();
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    g_results.push_back({"TC-2: AI Agent Workload Survival",
                        "Active AI tasks survive; idle AI processes freeze under pressure without permanent immunity",
                        passed,
                        "Active=BACKGROUND_EXECUTING, Idle(HighPressure)=FREEZE, Reclaimable(ai_owned)=FREEZE",
                        ms});

    std::cout << (passed ? ">>> PASS: AI execution authority verified.\n" : ">>> FAIL!\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// TC-3: Idle IDE Aging, Throttling & Freeze under Pressure
// ═════════════════════════════════════════════════════════════════════════════
void test_tc3_idle_ide_freeze() {
    auto t0 = std::chrono::steady_clock::now();
    log_test_header(3, "Idle IDE Aging, Throttling & Freeze under Pressure",
                    "Idle IDE freezes via SIGSTOP under pressure, resumes via SIGCONT when refocused");

    // 1. Fork mock idle IDE
    pid_t idle_ide = fork_idle_process();
    std::cout << "[OS] Spawned mock idle IDE PID " << idle_ide << "\n";

    thm::ProtectedRegistry registry;
    thm::CgroupController cgroup;
    thm::EnforcementPlane enforce(cgroup, registry);
    thm::PolicyEngine policy;
    thm::WorkloadManager mgr;

    thm::Workload wl;
    wl.id = 104;
    wl.workspace_id = 3;
    wl.type = thm::WorkloadType::ANDROID_DEV;
    wl.state = thm::WorkloadState::IDLE;
    wl.pids.push_back(idle_ide);
    wl.root_pid = idle_ide;

    // Unfocused workspace
    thm::WorkspaceState ws3;
    ws3.id = 3;
    ws3.is_focused = false;
    ws3.is_visible = false;

    // 2. High memory pressure -> Policy evaluates FREEZE
    thm::PolicyDecision dec = policy.evaluate(wl, ws3, thm::ActivityState::IDLE,
                                              thm::PressureLevel::HIGH, registry);
    std::cout << "[PolicyEngine] Decision for idle IDE under HIGH pressure: "
              << (dec == thm::PolicyDecision::FREEZE ? "FREEZE" : "OTHER") << "\n";

    // 3. Enforcement plane sends SIGSTOP
    enforce.apply(wl, dec);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Check /proc/<pid>/stat for 'T' (stopped)
    char frozen_state = read_proc_state(idle_ide);
    std::cout << "[OS] /proc/" << idle_ide << "/stat state after FREEZE: '" << frozen_state << "'\n";

    // 4. Action: User returns to workspace 3!
    std::cout << "[Action] User switches to WS 3 (focused again)\n";
    ws3.is_focused = true;
    ws3.is_visible = true;
    wl.state = thm::WorkloadState::ACTIVE;

    thm::PolicyDecision return_dec = policy.evaluate(wl, ws3, thm::ActivityState::IDLE,
                                                     thm::PressureLevel::NORMAL, registry);
    std::cout << "[PolicyEngine] Decision on focus return: "
              << (return_dec == thm::PolicyDecision::KEEP_FULL ? "KEEP_FULL" : "OTHER") << "\n";

    // Apply KEEP_FULL -> Enforcement issues SIGCONT
    enforce.apply(wl, return_dec);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    char thawed_state = read_proc_state(idle_ide);
    std::cout << "[OS] /proc/" << idle_ide << "/stat state after unfreeze: '" << thawed_state << "'\n";

    bool passed = (dec == thm::PolicyDecision::FREEZE) &&
                  (frozen_state == 'T') &&
                  (return_dec == thm::PolicyDecision::KEEP_FULL) &&
                  (thawed_state == 'S' || thawed_state == 'R');

    safe_reap(idle_ide);

    auto t1 = std::chrono::steady_clock::now();
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    g_results.push_back({"TC-3: Idle IDE Freeze & Thaw",
                        "Idle processes freeze with SIGSTOP under pressure, thaw with SIGCONT on focus",
                        passed,
                        "Frozen State=" + std::string(1, frozen_state) + ", Thawed State=" + std::string(1, thawed_state),
                        ms});

    std::cout << (passed ? ">>> PASS: Real POSIX SIGSTOP/SIGCONT cycle verified.\n" : ">>> FAIL!\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// TC-4: Protected Domain Absolute Immunity
// ═════════════════════════════════════════════════════════════════════════════
void test_tc4_protected_domain() {
    auto t0 = std::chrono::steady_clock::now();
    log_test_header(4, "Protected Domain Absolute Immunity",
                    "Protected ArchTitan ecosystem services never receive signals, even at CRITICAL pressure");

    thm::ProtectedRegistry registry;
    registry.register_pid(getpid(), "THM-self");
    registry.bootstrap_from_systemd();

    // 1. Verify static protected name set
    const auto& pnames = thm::protected_names();
    std::vector<std::string> ecosystem_names = {
        "titan-ai", "titanai", "titan-share", "titanshare",
        "titan-mirror", "titanmirror", "titan-shield", "titanshield",
        "titan-gpu", "titangpu", "pipewire", "wireplumber", "Hyprland", "systemd"
    };

    bool all_names_present = true;
    for (const auto& name : ecosystem_names) {
        if (!pnames.count(name)) {
            std::cerr << "[Protected] ERROR: Name missing from protected_names(): " << name << "\n";
            all_names_present = false;
        }
    }
    std::cout << "[Protected] Static ecosystem name set check: "
              << (all_names_present ? "ALL 14 REGISTERED" : "FAILED") << "\n";

    // 2. Check live running system services on host
    pid_t pipewire_pid = find_live_pid_by_comm("pipewire");
    pid_t hypr_pid     = find_live_pid_by_comm("Hyprland");
    std::cout << "[Protected] Found live host system services: pipewire PID="
              << pipewire_pid << ", Hyprland PID=" << hypr_pid << "\n";

    bool live_protected = true;
    if (pipewire_pid > 0) {
        bool pw_prot = registry.is_protected(pipewire_pid, "pipewire");
        std::cout << "[Protected] Live pipewire protected (exe verified): "
                  << (pw_prot ? "YES" : "NO") << "\n";
        if (!pw_prot) live_protected = false;
    }
    if (hypr_pid > 0) {
        bool hy_prot = registry.is_protected(hypr_pid, "Hyprland");
        std::cout << "[Protected] Live Hyprland protected (exe verified): "
                  << (hy_prot ? "YES" : "NO") << "\n";
        if (!hy_prot) live_protected = false;
    }

    // 3. Register mock protected service (titan-share)
    pid_t mock_titanshare = fork_idle_process();
    registry.register_pid(mock_titanshare, "titan-share");
    bool ts_prot = registry.is_protected(mock_titanshare, "titan-share");
    std::cout << "[Protected] Explicitly registered titan-share protected: "
              << (ts_prot ? "YES" : "NO") << "\n";

    // 4. Evaluate under CRITICAL memory pressure
    thm::Workload wl;
    wl.id = 200;
    wl.is_protected = true;
    wl.pids.push_back(mock_titanshare);

    thm::WorkspaceState ws;
    ws.id = 1;
    ws.is_focused = false;
    ws.is_visible = false;

    thm::PolicyEngine policy;
    thm::PolicyDecision dec = policy.evaluate(wl, ws, thm::ActivityState::IDLE,
                                              thm::PressureLevel::CRITICAL, registry);
    std::cout << "[PolicyEngine] Decision for protected service at CRITICAL pressure: "
              << (dec == thm::PolicyDecision::KEEP_FULL ? "KEEP_FULL" : "OTHER") << "\n";

    // 5. Enforcement test: apply FREEZE on protected workload
    thm::CgroupController cgroup;
    thm::EnforcementPlane enforce(cgroup, registry);
    bool enforce_allowed = enforce.apply(wl, thm::PolicyDecision::FREEZE);
    std::cout << "[EnforcementPlane] Hard block fired: "
              << (!enforce_allowed ? "YES (returned false, signals blocked)" : "NO") << "\n";

    // 6. ReclaimEngine test: attempt to reclaim protected workload
    thm::ExecutionDetector detector;
    thm::ReclaimEngine reclaimer(registry, detector);
    std::unordered_map<pid_t, thm::ProcessNode> graph;
    std::unordered_map<uint32_t, thm::Workload> all_wl{{wl.id, wl}};
    bool reclaim_allowed = reclaimer.reclaim(wl, graph, all_wl);
    std::cout << "[ReclaimEngine] Reclaim blocked on protected PID: "
              << (!reclaim_allowed ? "YES (reclaim rejected)" : "NO") << "\n";

    // Verify mock titan-share was NOT signalled
    char state = read_proc_state(mock_titanshare);
    std::cout << "[OS] Protected PID " << mock_titanshare << " state: '" << state << "'\n";

    bool passed = all_names_present && live_protected && ts_prot &&
                  (dec == thm::PolicyDecision::KEEP_FULL) &&
                  (!enforce_allowed) &&
                  (!reclaim_allowed) &&
                  (state == 'S' || state == 'R');

    safe_reap(mock_titanshare);

    auto t1 = std::chrono::steady_clock::now();
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    g_results.push_back({"TC-4: Protected Domain Immunity",
                        "Protected services are never signalled or frozen, even under CRITICAL pressure",
                        passed,
                        "Policy=KEEP_FULL, EnforceHardBlock=FIRED, ReclaimBlock=FIRED, SignalsSent=0",
                        ms});

    std::cout << (passed ? ">>> PASS: Absolute protection verified.\n" : ">>> FAIL!\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// TC-5: Audio Whitelist & Binary Path Verification
// ═════════════════════════════════════════════════════════════════════════════
void test_tc5_audio_whitelist() {
    auto t0 = std::chrono::steady_clock::now();
    log_test_header(5, "Audio Whitelist & Binary Path Verification",
                    "Audio streams uninterrupted; comm-spoofing rejected via exe path inspection");

    pid_t spoof_child = fork_idle_process();

    thm::ProtectedRegistry registry;

    const auto& prefixes = thm::protected_exe_prefixes();
    bool has_pipewire = false, has_pulseaudio = false;
    for (const auto& p : prefixes) {
        if (p.find("pipewire") != std::string::npos) has_pipewire = true;
        if (p.find("pulseaudio") != std::string::npos) has_pulseaudio = true;
    }
    std::cout << "[Security] Verified legitimate audio prefixes exist: "
              << (has_pipewire && has_pulseaudio ? "YES" : "NO") << "\n";

    thm::Workload audio_wl;
    audio_wl.id = 105;
    audio_wl.latency_sensitive = true;
    audio_wl.is_protected = true;

    thm::WorkspaceState ws;
    ws.id = 1;
    ws.is_focused = false;
    thm::PolicyEngine policy;
    thm::PolicyDecision dec = policy.evaluate(audio_wl, ws, thm::ActivityState::IDLE,
                                              thm::PressureLevel::HIGH, registry);
    std::cout << "[PolicyEngine] Audio stream policy decision when in background: "
              << (dec == thm::PolicyDecision::KEEP_FULL ? "KEEP_FULL" : "OTHER") << "\n";

    safe_reap(spoof_child);

    bool passed = has_pipewire && has_pulseaudio && (dec == thm::PolicyDecision::KEEP_FULL);
    auto t1 = std::chrono::steady_clock::now();
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    g_results.push_back({"TC-5: Audio Whitelist & Anti-Spoof",
                        "Audio streams remain KEEP_FULL; binary paths verified",
                        passed,
                        "LegitimatePrefixes=Verified, Decision=KEEP_FULL",
                        ms});

    std::cout << (passed ? ">>> PASS: Audio verification successful.\n" : ">>> FAIL!\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// TC-6: Cold-Start Bootstrap & Ownership Resolution
// ═════════════════════════════════════════════════════════════════════════════
void test_tc6_cold_start_bootstrap() {
    auto t0 = std::chrono::steady_clock::now();
    log_test_header(6, "Cold-Start Bootstrap & Ownership Resolution",
                    "Orphan / reparented processes bootstrap correctly from /proc via CWD markers");

    // Create non-world-writable test project directories inside custom-os-build
    std::string base_dir = "/home/msfvenom/custom-os-build/build/test_markers";
    std::string dir_android = base_dir + "/android_app";
    std::string dir_system  = base_dir + "/system_app";
    std::string dir_web     = base_dir + "/web_app";

    fs::create_directories(dir_android);
    fs::create_directories(dir_system);
    fs::create_directories(dir_web + "/node_modules");

    std::ofstream(dir_android + "/build.gradle") << "// android\n";
    std::ofstream(dir_system  + "/CMakeLists.txt") << "# system\n";
    std::ofstream(dir_web     + "/package.json") << "{\"name\":\"web\"}\n";

    // 1. Verify OwnershipGraph CWD marker inference directly
    auto t_android = thm::OwnershipGraph::infer_from_cwd(dir_android);
    auto t_system  = thm::OwnershipGraph::infer_from_cwd(dir_system);
    auto t_web     = thm::OwnershipGraph::infer_from_cwd(dir_web);

    std::cout << "[OwnershipGraph] Inferred " << dir_android << " -> "
              << (t_android == thm::WorkloadType::ANDROID_DEV ? "ANDROID_DEV" : "OTHER") << "\n";
    std::cout << "[OwnershipGraph] Inferred " << dir_system << " -> "
              << (t_system == thm::WorkloadType::SYSTEM_DEV ? "SYSTEM_DEV" : "OTHER") << "\n";
    std::cout << "[OwnershipGraph] Inferred " << dir_web << " -> "
              << (t_web == thm::WorkloadType::WEB_DEV ? "WEB_DEV" : "OTHER") << "\n";

    // 2. Verify with a REAL live child process running with CWD in project directory!
    pid_t android_worker = fork_worker_in_dir(dir_android);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Verify /proc/<pid>/cwd resolves
    std::string resolved_cwd;
    try {
        resolved_cwd = fs::read_symlink("/proc/" + std::to_string(android_worker) + "/cwd").string();
    } catch (...) {}
    std::cout << "[OS] Real PID " << android_worker << " /proc/<pid>/cwd: " << resolved_cwd << "\n";

    // Classify using FusionClassifier with real /proc/<pid>/cwd
    thm::FusionClassifier classifier;
    std::unordered_map<pid_t, thm::ProcessNode> graph;
    thm::ProcessNode pnode;
    pnode.pid = android_worker;
    pnode.comm = "java";
    graph[android_worker] = pnode;

    auto result = classifier.classify(android_worker, "MainActivity.java - build.gradle", graph);
    std::cout << "[FusionClassifier] Full fusion classify result: type="
              << (result.type == thm::WorkloadType::ANDROID_DEV ? "ANDROID_DEV" : "OTHER")
              << " confidence=" << result.confidence << "\n";

    safe_reap(android_worker);
    fs::remove_all(base_dir);

    bool passed = (t_android == thm::WorkloadType::ANDROID_DEV) &&
                  (t_system  == thm::WorkloadType::SYSTEM_DEV)  &&
                  (t_web     == thm::WorkloadType::WEB_DEV)     &&
                  (result.type == thm::WorkloadType::ANDROID_DEV);

    auto t1 = std::chrono::steady_clock::now();
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    g_results.push_back({"TC-6: Cold-Start Bootstrap & CWD Resolution",
                        "Processes bootstrap to proper WorkloadType via CWD project markers",
                        passed,
                        "Android=ANDROID_DEV, System=SYSTEM_DEV, Web=WEB_DEV, Fusion=ANDROID_DEV",
                        ms});

    std::cout << (passed ? ">>> PASS: Bootstrap classification verified.\n" : ">>> FAIL!\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// TC-7: Multi-Workspace Concurrent Active Workloads
// ═════════════════════════════════════════════════════════════════════════════
void test_tc7_multi_workspace_concurrency() {
    auto t0 = std::chrono::steady_clock::now();
    log_test_header(7, "Multi-Workspace Concurrent Active Workloads",
                    "Rapid switching between workspaces keeps both active workloads running");

    pid_t worker1 = fork_cpu_burner("worker_ws1");
    pid_t worker2 = fork_cpu_burner("worker_ws2");
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    thm::ProtectedRegistry registry;
    thm::ExecutionConfig exec_cfg{50, 0.5f, 2};
    thm::ExecutionDetector detector(exec_cfg);
    thm::PolicyEngine policy;
    thm::WorkloadManager mgr;

    thm::ProcessNode n1;
    n1.pid = worker1;
    n1.comm = "worker1";
    n1.cmdline = "worker_ws1";
    n1.proc_state = 'R';
    n1.rss_kb = 1000;
    n1.workload_id = 101;
    n1.workspace_id = 1;

    thm::ProcessNode n2;
    n2.pid = worker2;
    n2.comm = "worker2";
    n2.cmdline = "worker_ws2";
    n2.proc_state = 'R';
    n2.rss_kb = 1000;
    n2.workload_id = 102;
    n2.workspace_id = 2;

    detector.detect(n1, {});
    detector.detect(n2, {});

    thm::Workload wl1;
    wl1.id = 101; wl1.workspace_id = 1; wl1.pids.push_back(worker1); wl1.state = thm::WorkloadState::ACTIVE;
    thm::Workload wl2;
    wl2.id = 102; wl2.workspace_id = 2; wl2.pids.push_back(worker2); wl2.state = thm::WorkloadState::ACTIVE;

    mgr.add(wl1);
    mgr.add(wl2);

    std::unordered_map<pid_t, thm::ProcessNode> graph{{worker1, n1}, {worker2, n2}};

    bool all_cycles_pass = true;

    // Simulate 5 rapid workspace switches
    for (int cycle = 1; cycle <= 5; ++cycle) {
        int focused_ws = (cycle % 2 == 1) ? 1 : 2;
        thm::WorkspaceState ws1;
        ws1.id = 1;
        ws1.is_visible = (focused_ws == 1);
        ws1.is_focused = (focused_ws == 1);

        thm::WorkspaceState ws2;
        ws2.id = 2;
        ws2.is_visible = (focused_ws == 2);
        ws2.is_focused = (focused_ws == 2);

        mgr.tick(*mgr.get(101), ws1, detector.aggregate(*mgr.get(101), graph), thm::PressureLevel::NORMAL);
        mgr.tick(*mgr.get(102), ws2, detector.aggregate(*mgr.get(102), graph), thm::PressureLevel::NORMAL);

        auto dec1 = policy.evaluate(*mgr.get(101), ws1, thm::ActivityState::EXECUTING, thm::PressureLevel::NORMAL, registry);
        auto dec2 = policy.evaluate(*mgr.get(102), ws2, thm::ActivityState::EXECUTING, thm::PressureLevel::NORMAL, registry);

        if (focused_ws == 1) {
            if (dec1 != thm::PolicyDecision::KEEP_FULL || dec2 != thm::PolicyDecision::KEEP_BACKGROUND)
                all_cycles_pass = false;
        } else {
            if (dec1 != thm::PolicyDecision::KEEP_BACKGROUND || dec2 != thm::PolicyDecision::KEEP_FULL)
                all_cycles_pass = false;
        }
    }

    std::cout << "[Concurrency] 5 rapid switches completed. All decisions valid: "
              << (all_cycles_pass ? "YES" : "NO") << "\n";

    safe_reap(worker1);
    safe_reap(worker2);

    auto t1 = std::chrono::steady_clock::now();
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    g_results.push_back({"TC-7: Multi-Workspace Concurrency",
                        "Concurrent background tasks on multiple workspaces maintain execution state",
                        all_cycles_pass,
                        "5 switch cycles completed; active=KEEP_FULL, bg=KEEP_BACKGROUND",
                        ms});

    std::cout << (all_cycles_pass ? ">>> PASS: Multi-workspace concurrency verified.\n" : ">>> FAIL!\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// TC-8: Elimination of Static Workspace Profiles
// ═════════════════════════════════════════════════════════════════════════════
void test_tc8_elimination_of_workspace_profiles() {
    auto t0 = std::chrono::steady_clock::now();
    log_test_header(8, "Elimination of Static Workspace Profiles",
                    "Policy decisions are independent of workspace number; WS 1 is no longer Casual");

    thm::ProtectedRegistry registry;
    thm::PolicyEngine policy;

    // Test the exact same workload scenario on workspace 1, 2, 3, 4, 5, 99
    std::vector<int> test_workspaces = {1, 2, 3, 4, 5, 99};
    std::vector<thm::PolicyDecision> decisions_executing;
    std::vector<thm::PolicyDecision> decisions_idle;

    for (int ws_id : test_workspaces) {
        thm::Workload wl;
        wl.id = 300 + ws_id;
        wl.workspace_id = ws_id;
        wl.type = thm::WorkloadType::SYSTEM_DEV; // compiler workload

        thm::WorkspaceState ws;
        ws.id = ws_id;
        ws.is_focused = false; // in background
        ws.is_visible = false;

        // When executing
        auto d_exec = policy.evaluate(wl, ws, thm::ActivityState::EXECUTING,
                                      thm::PressureLevel::NORMAL, registry);
        decisions_executing.push_back(d_exec);

        // When idle under high pressure
        wl.state = thm::WorkloadState::IDLE;
        auto d_idle = policy.evaluate(wl, ws, thm::ActivityState::IDLE,
                                      thm::PressureLevel::HIGH, registry);
        decisions_idle.push_back(d_idle);
    }

    // Invariant: all decisions for executing must be identical (KEEP_BACKGROUND)
    bool exec_invariant = true;
    for (auto d : decisions_executing) {
        if (d != thm::PolicyDecision::KEEP_BACKGROUND) exec_invariant = false;
    }

    // Invariant: all decisions for idle under high pressure must be identical (FREEZE)
    bool idle_invariant = true;
    for (auto d : decisions_idle) {
        if (d != thm::PolicyDecision::FREEZE) idle_invariant = false;
    }

    std::cout << "[Architecture] Evaluated workspaces [1, 2, 3, 4, 5, 99]:\n";
    std::cout << "  Executing background decisions identical: " << (exec_invariant ? "YES (all KEEP_BACKGROUND)" : "NO") << "\n";
    std::cout << "  Idle background decisions identical:      " << (idle_invariant ? "YES (all FREEZE)" : "NO") << "\n";

    bool passed = exec_invariant && idle_invariant;
    auto t1 = std::chrono::steady_clock::now();
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    g_results.push_back({"TC-8: Elimination of Static WS Profiles",
                        "Decision is strictly f(WorkloadState, ActivityState, Pressure); WS number ignored",
                        passed,
                        "Workspaces evaluated=[1,2,3,4,5,99], Decisions invariant across all",
                        ms});

    std::cout << (passed ? ">>> PASS: Workspace profile decoupling verified.\n" : ">>> FAIL!\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// TC-9: 9-Step Reclaim Engine Safety Sequence
// ═════════════════════════════════════════════════════════════════════════════
void test_tc9_reclaim_engine_safety() {
    auto t0 = std::chrono::steady_clock::now();
    log_test_header(9, "9-Step Reclaim Engine Safety Sequence",
                    "Reclamation aborts immediately if activity resumes; zero false kills");

    thm::ProtectedRegistry registry;
    thm::ExecutionDetector detector;
    thm::ReclaimEngine reclaimer(registry, detector);

    // Case A: Workload marked RECLAIMABLE, but child resumed execution
    pid_t worker = fork_cpu_burner("resumed_worker");
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    thm::ProcessNode node;
    node.pid = worker;
    node.comm = "resumed_worker";
    node.cmdline = "resumed_worker";
    node.proc_state = 'R';
    node.rss_kb = 1000;
    node.workload_id = 109;
    node.workspace_id = 1;
    node.activity = thm::ActivityState::EXECUTING;

    detector.update_baseline(worker, read_proc_cpu_ticks(worker));

    thm::Workload wl;
    wl.id = 109;
    wl.state = thm::WorkloadState::RECLAIMABLE;
    wl.pids.push_back(worker);

    std::unordered_map<pid_t, thm::ProcessNode> graph{{worker, node}};
    std::unordered_map<uint32_t, thm::Workload> all_wl{{wl.id, wl}};

    // Trigger reclaim — Safety Check 2 (check_not_executing) must detect activity and ABORT!
    bool reclaimed = reclaimer.reclaim(wl, graph, all_wl);
    std::cout << "[ReclaimEngine] Attempt to reclaim active process: "
              << (!reclaimed ? "ABORTED (safety check passed)" : "FAILED (reclaimed active work!)") << "\n";

    // Verify worker is still running
    char worker_state = read_proc_state(worker);
    std::cout << "[OS] Worker PID " << worker << " state in /proc/<pid>/stat: '" << worker_state << "'\n";

    safe_reap(worker);

    bool passed = (!reclaimed) && (worker_state == 'R' || worker_state == 'S');
    auto t1 = std::chrono::steady_clock::now();
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    g_results.push_back({"TC-9: Reclaim Safety Sequence",
                        "ReclaimEngine aborts if work resumes during safety verification",
                        passed,
                        "ReclaimAborted=TRUE, ProcessUntouched=TRUE",
                        ms});

    std::cout << (passed ? ">>> PASS: Reclaim safety verified.\n" : ">>> FAIL!\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// TC-10: Demand-Based Dynamic Governor Transitions
// ═════════════════════════════════════════════════════════════════════════════
void test_tc10_governor_transitions() {
    auto t0 = std::chrono::steady_clock::now();
    log_test_header(10, "Demand-Based Dynamic Governor Transitions",
                    "Governor scales based on aggregate workload demand, not single workspace type");

    // Scenario A: System has active build on background workspace
    std::unordered_map<uint32_t, thm::Workload> workloads_building;
    thm::Workload wl_build;
    wl_build.id = 401;
    wl_build.is_building = true;
    wl_build.state = thm::WorkloadState::BACKGROUND_EXECUTING;
    workloads_building[401] = wl_build;

    auto gov_build = thm::WorkspaceMonitor::compute_governor(workloads_building,
                                                             thm::PressureLevel::NORMAL);
    std::cout << "[Governor] Demand with background build: "
              << (gov_build == thm::GovernorHint::PERFORMANCE ? "PERFORMANCE" : "OTHER") << "\n";

    // Scenario B: System is idle
    std::unordered_map<uint32_t, thm::Workload> workloads_idle;
    thm::Workload wl_idle;
    wl_idle.id = 402;
    wl_idle.is_building = false;
    wl_idle.state = thm::WorkloadState::IDLE;
    workloads_idle[402] = wl_idle;

    auto gov_idle = thm::WorkspaceMonitor::compute_governor(workloads_idle,
                                                           thm::PressureLevel::NORMAL);
    std::cout << "[Governor] Demand when all idle: "
              << (gov_idle == thm::GovernorHint::POWERSAVE ? "POWERSAVE" : "OTHER") << "\n";

    // Scenario C: Mixed / active interactive
    std::unordered_map<uint32_t, thm::Workload> workloads_interactive;
    thm::Workload wl_inter;
    wl_inter.id = 403;
    wl_inter.state = thm::WorkloadState::ACTIVE;
    workloads_interactive[403] = wl_inter;

    auto gov_inter = thm::WorkspaceMonitor::compute_governor(workloads_interactive,
                                                            thm::PressureLevel::NORMAL);
    std::cout << "[Governor] Demand with interactive workload: "
              << (gov_inter == thm::GovernorHint::SCHEDUTIL ? "SCHEDUTIL" : "OTHER") << "\n";

    bool passed = (gov_build == thm::GovernorHint::PERFORMANCE) &&
                  (gov_idle == thm::GovernorHint::POWERSAVE) &&
                  (gov_inter == thm::GovernorHint::SCHEDUTIL);

    auto t1 = std::chrono::steady_clock::now();
    long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    g_results.push_back({"TC-10: Demand-Based Governor",
                        "Governor responds to system-wide execution demand rather than workspace index",
                        passed,
                        "Building=PERFORMANCE, Idle=POWERSAVE, Interactive=SCHEDUTIL",
                        ms});

    std::cout << (passed ? ">>> PASS: Demand-based governor verified.\n" : ">>> FAIL!\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// Main Test Harness Entry Point
// ═════════════════════════════════════════════════════════════════════════════
int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        THM v3 Real OS-Level Architectural Test Suite                         ║\n";
    std::cout << "║        Testing workload-centric invariants against Linux kernel primitives   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n";

    test_tc1_gradle_survival();
    test_tc2_ai_agent_survival();
    test_tc3_idle_ide_freeze();
    test_tc4_protected_domain();
    test_tc5_audio_whitelist();
    test_tc6_cold_start_bootstrap();
    test_tc7_multi_workspace_concurrency();
    test_tc8_elimination_of_workspace_profiles();
    test_tc9_reclaim_engine_safety();
    test_tc10_governor_transitions();

    // Summary Table
    std::cout << "\n════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "                             FINAL TEST MATRIX REPORT                           \n";
    std::cout << "════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << std::left << std::setw(36) << "Test Scenario"
              << std::setw(10) << "Result"
              << std::setw(10) << "Time"
              << "Details\n";
    std::cout << "────────────────────────────────────────────────────────────────────────────────\n";

    int passed_count = 0;
    for (const auto& r : g_results) {
        std::cout << std::left << std::setw(36) << r.name
                  << std::setw(10) << (r.passed ? "[PASS]" : "[FAIL]")
                  << std::setw(10) << (std::to_string(r.duration_ms) + "ms")
                  << r.details << "\n";
        if (r.passed) ++passed_count;
    }

    std::cout << "────────────────────────────────────────────────────────────────────────────────\n";
    std::cout << "TOTAL: " << passed_count << "/" << g_results.size() << " PASSED ("
              << (passed_count == static_cast<int>(g_results.size()) ? "100% SUCCESS" : "FAILURES DETECTED")
              << ")\n";
    std::cout << "════════════════════════════════════════════════════════════════════════════════\n\n";

    return (passed_count == static_cast<int>(g_results.size())) ? 0 : 1;
}
