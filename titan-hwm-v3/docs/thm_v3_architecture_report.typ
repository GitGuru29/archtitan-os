#set page(
  paper: "a4",
  margin: (x: 2cm, y: 2.2cm),
  header: context {
    if counter(page).get().first() > 1 {
      text(9pt, fill: rgb("#64748b"))[
        *ArchTitan OS Technical Whitepaper* | Titan Hardware Manager v3 (THM v3)
        #h(1fr)
        September 2026
      ]
    }
  },
  footer: context {
    text(9pt, fill: rgb("#64748b"))[
      ArchTitan Engineering | Confidential & Open Architecture
      #h(1fr)
      Page #counter(page).display() of #counter(page).final().first()
    ]
  }
)

#set text(
  font: "Liberation Sans",
  size: 10pt,
  fill: rgb("#1e293b"),
  spacing: 120%
)

#set heading(numbering: "1.1")
#show heading: it => [
  #v(0.4cm)
  #text(fill: rgb("#0f172a"), weight: "bold")[#it]
  #v(0.2cm)
]
#show heading.where(level: 1): it => [
  #v(0.6cm)
  #text(fill: rgb("#0369a1"), size: 14pt, weight: "bold")[#it]
  #v(0.3cm)
]
#show heading.where(level: 2): it => [
  #v(0.4cm)
  #text(fill: rgb("#0284c7"), size: 11.5pt, weight: "bold")[#it]
  #v(0.2cm)
]

#set table(
  stroke: (x, y) => if y == 0 { (bottom: 1.5pt + rgb("#0284c7")) } else { 0.5pt + rgb("#cbd5e1") },
  fill: (x, y) => if y == 0 { rgb("#f1f5f9") } else if calc.even(y) { rgb("#f8fafc") } else { none },
  inset: (x: 7pt, y: 6pt)
)

// ── Cover / Header ───────────────────────────────────────────────────────────
#align(center)[
  #block(
    fill: rgb("#0f172a"),
    inset: 22pt,
    radius: 8pt,
    width: 100%
  )[
    #text(fill: rgb("#38bdf8"), size: 10pt, weight: "bold", tracking: 2pt)[
      ARCHTITAN OS TECHNICAL WHITEPAPER
    ]
    #v(6pt)
    #text(fill: white, size: 20pt, weight: "bold")[
      Titan Hardware Manager v3 (THM v3)
    ]
    #v(4pt)
    #text(fill: rgb("#94a3b8"), size: 11pt)[
      Workload-Centric Resource Orchestration & System Architecture Report
    ]
    #v(10pt)
    #line(length: 100%, stroke: 0.5pt + rgb("#334155"))
    #v(6pt)
    #grid(
      columns: (1fr, 1fr, 1fr),
      text(fill: rgb("#cbd5e1"), size: 8.5pt)[*Target Kernel:* Linux 6.18 LTS],
      text(fill: rgb("#cbd5e1"), size: 8.5pt)[*Compositor:* Hyprland (Wayland)],
      text(fill: rgb("#cbd5e1"), size: 8.5pt)[*Status:* 10/10 OS Tests Passed]
    )
  ]
]

#v(0.5cm)

// ── Executive Summary ─────────────────────────────────────────────────────────
= Executive Summary

ArchTitan OS is engineered to provide a high-performance, developer-first computing environment with aggressive sub-1 GB idle memory footprints without sacrificing multi-tasking fluidity. Previous iterations of the hardware daemon (THM v1 and v2) suffered from an architectural flaw: *workspace numbers were statically mapped to developer profiles*. In v2, switching to Workspace 1 ("Casual") would forcibly kill or freeze all active developer tools across the entire system.

*Titan Hardware Manager v3 (THM v3)* establishes a completely redesigned *workload-centric architecture*. Rather than relying on workspace numbers to dictate resource allocation, THM v3 treats process execution state as the primary authority. Workspaces serve solely as *priority hints* for interactive scheduling. Active background builds (Gradle, Cargo, CMake), AI agent tasks, and audio playback are guaranteed continuous execution without interruption, while genuinely idle processes are progressively throttled and frozen under memory pressure.

#block(
  fill: rgb("#eff6ff"),
  stroke: (left: 4pt + rgb("#0284c7")),
  inset: 10pt,
  radius: (right: 4pt)
)[
  #text(weight: "bold", fill: rgb("#0369a1"))[The Core Paradigm Shift:]\
  #text(style: "italic")[
    "A workspace is a spatial viewport, not a resource grant or execution boundary. Background &ne; Idle. Background &ne; Kill. Active background builds and AI workloads continue running; dormant processes are suspended; completed workloads are safely reclaimed; and protected system infrastructure is never touched."
  ]
]

---

= Core Architectural Invariants

THM v3 strictly enforces five non-negotiable architectural invariants across all code paths:

#table(
  columns: (1.4fr, 2fr, 2fr),
  [*Invariant*], [*Architectural Requirement*], [*Enforcement Mechanism*],
  [1. Execution Authority],
  [Active compilation, build tools, and AI agent steps must never be stopped or killed during workspace switching.],
  [`ActivityState::EXECUTING` forces policy decision to `KEEP_BACKGROUND` or `KEEP_FULL`. Zero signals sent.],

  [2. Workspace as Priority Hint],
  [Workspace visibility determines priority tier (active vs background slice), never life or death.],
  [Workspace ID is strictly decoupled from policy engine decisions. Decisions are identical on WS 1 and WS 99.],

  [3. Absolute Domain Protection],
  [ArchTitan ecosystem and core session services must never be frozen, killed, or demoted.],
  [`ProtectedRegistry` hard-block fires at line 1 of every enforcement path. Multi-tier check with `/proc/<pid>/exe` anti-spoofing.],

  [4. AI Task Scrutiny],
  [`ai_owned` is a context label, not an infinite shield against throttling or freezing when idle.],
  [Active AI tasks run uninterrupted. Dormant AI tasks freeze under `HIGH` pressure. Reclaim downgrades to `FREEZE` for safety.],

  [5. Demand-Based Governor],
  [CPU frequency scaling reflects aggregate system workload demand, not focused workspace number.],
  [`WorkspaceMonitor::compute_governor()` scans all workloads: active build &rarr; `performance`; all idle &rarr; `powersave`.]
)

---

= System Architecture & Decomposition

THM v3 is structured into modular, decoupled subsystems within `titan-hwm-v3/`:

#align(center)[
```
┌────────────────────────────────────────────────────────────────────────┐
│                        THM v3 ARCHITECTURE                             │
├────────────────────────────────────────────────────────────────────────┤
│  Input Layer:                                                          │
│    /proc Scanner  │  Hyprland IPC (.socket2)  │  PSI / meminfo Monitor │
├────────────────────────────────────────────────────────────────────────┤
│  Classification & Activity Layer:                                      │
│    OwnershipGraph (Forest) │ FusionClassifier │ ExecutionDetector      │
├────────────────────────────────────────────────────────────────────────┤
│  Decision Layer:                                                       │
│    WorkloadManager (State Machine) │ PolicyEngine │ PressureLevel      │
├────────────────────────────────────────────────────────────────────────┤
│  Enforcement Layer:                                                    │
│    EnforcementPlane │ CgroupController (v2) │ ReclaimEngine (9-step)   │
├────────────────────────────────────────────────────────────────────────┤
│  Safety Gate:                                                          │
│    ProtectedRegistry (3-Tier Anti-Spoof Hard Block)                    │
└────────────────────────────────────────────────────────────────────────┘
```
]

== Phase 0: Protected Domain Registry (`core/protected_registry.hpp/.cpp`)
The protected domain registry provides zero-latency immunity for system-critical and ArchTitan infrastructure. 
- *Tier 1 (Explicit PIDs):* Dynamically registered PIDs (e.g., THM self, dynamically spawned Titan components).
- *Tier 2 (Protected Names):* Static comm set (`pipewire`, `wireplumber`, `Hyprland`, `titan-ai`, `titan-share`, `titan-mirror`, `titan-shield`, `titan-gpu`, `sddm`, `dbus-broker`, `systemd`).
- *Tier 3 (Executable Path Anti-Spoofing):* Resolves `/proc/<pid>/exe` via `readlink()`. If an unprivileged rogue process renames its comm string to `pipewire` but its executable binary is in `/tmp`, Tier 3 rejects protection immediately.

== Phase 1: Core Data Model (`core/types.hpp`)
Defines the authoritative structures:
- `WorkloadType`: Semantic classification label (`ANDROID_DEV`, `WEB_DEV`, `SYSTEM_DEV`, `CASUAL`, `AI_TASK`, `NEUTRAL`).
- `WorkloadState`: State machine states (`DISCOVERED`, `ACTIVE`, `BACKGROUND_EXECUTING`, `IDLE`, `AGING`, `RECLAIMABLE`, `TERMINATED`).
- `ActivityState`: Real-time execution signal (`EXECUTING`, `IDLE`, `COMPLETED`, `UNKNOWN`).
- `PolicyDecision`: Enforcement output (`KEEP_FULL`, `KEEP_BACKGROUND`, `THROTTLE`, `FREEZE`, `RECLAIM`).

== Phase 2: Process Ownership Graph (`workload/ownership_graph.hpp/.cpp`)
Solves the cold-start problem when THM launches with pre-existing processes:
- Maintains a process forest mapping every PID to an application root.
- Reparented child processes (e.g., build daemons whose parent exited to `ppid=1`) are resolved through `/proc/<pid>/cwd` filesystem project marker inspection (`build.gradle`, `CMakeLists.txt`, `Cargo.toml`, `package.json`).

== Phase 3: Repurposed FusionClassifier (`classifier/fusion_classifier.hpp/.cpp`)
Ports the battle-tested multi-signal classifier from v2 with crucial bug fixes:
- *Signal 1 (Child Process Walk):* Depth-3 recursive walk detecting LSPs (`clangd`, `rust-analyzer`), build daemons (`gradle`, `cargo`, `ninja`), and AI runners (`ollama`, `llama-server`).
- *Signal 2 (Window Title Extensions):* Regex/substring inspection of active window titles (`.cpp`, `.rs`, `build.gradle`, `.ts`).
- *Signal 3 (CWD Project Markers):* Non-world-writable filesystem markers. Rejects `/tmp` and `/dev/shm` to prevent container namespace escapes.
- *TC-1.5 IDE Variant Suffix Stripping:* Automatically strips `-nightly`, `-insiders`, `-bin` so custom IDE builds classify accurately.

== Phase 4: Non-Blocking Execution Detector (`classifier/execution_detector.hpp/.cpp`)
Detects real-time work without blocking the daemon tick loop:
- *Tick-to-Tick Delta:* Compares `utime + stime` across consecutive 200ms daemon ticks to compute true CPU percentage without sleeping inside the scan loop.
- *State Signals:* Flagged active if kernel process state is `'R'` (Running) or `'D'` (Uninterruptible Disk I/O).
- *Build Command Whitelist:* Instant active flag for known build commands (`gradlew`, `cargo`, `cmake`, `ninja`, `rustc`, `javac`, `ld`).

== Phase 5: WorkloadState Machine (`workload/workload_manager.hpp/.cpp`)
Drives state transitions every tick:
```
           ┌──────────────────────┐
           │      DISCOVERED      │
           └──────────┬───────────┘
                      │
         ┌────────────┴────────────┐
   [Focused/Vis]             [Background]
         │                         │
         ▼                         ▼
   ┌───────────┐  Activity   ┌────────────────────────┐
   │  ACTIVE   │ ──────────> │  BACKGROUND_EXECUTING  │
   └─────┬─────┘             └───────────┬────────────┘
         │                               │
    [Unfocused & Idle]              [Idle Scan]
         │                               │
         └──────────────┬────────────────┘
                        ▼
                 ┌──────────────┐
                 │     IDLE     │
                 └──────┬───────┘
                        │ [Age > 5m + MODERATE pressure]
                        ▼
                 ┌──────────────┐
                 │    AGING     │
                 └──────┬───────┘
                        │ [Age > 15m + HIGH pressure]
                        ▼
                 ┌──────────────┐
                 │ RECLAIMABLE  │
                 └──────┬───────┘
                        │ [9-Step Safety Reclaim]
                        ▼
                 ┌──────────────┐
                 │  TERMINATED  │
                 └──────────────┘
```

== Phase 6: Policy Engine (`policy/policy_engine.hpp/.cpp`)
Translates workload state and system pressure into a `PolicyDecision` via a strict 5-step hierarchy:
1. *Protected Check:* `is_protected == true` &rarr; `KEEP_FULL`.
2. *Execution Check:* `activity == EXECUTING` &rarr; `KEEP_FULL` (visible) or `KEEP_BACKGROUND` (background).
3. *User Interaction:* Visible workspace + `ACTIVE` &rarr; `KEEP_FULL`.
4. *Idle Degradation:* `IDLE` under pressure &rarr; `THROTTLE` (Moderate) or `FREEZE` (High/Critical).
5. *Reclamation:* `RECLAIMABLE` &rarr; `RECLAIM` (or `FREEZE` if `ai_owned`).

== Phase 7: cgroup v2 Hierarchy (`enforcement/cgroup_controller.hpp/.cpp`)
Integrates directly with Linux cgroup v2 beneath `archtitan.slice`:
- `archtitan-active.slice`: Full CPU weight (500), OOM score -500.
- `archtitan-background.slice`: Reduced CPU weight (50), memory throttled, OOM score 0.
- `archtitan-frozen.slice`: CPU weight 0, frozen via `cgroup.freeze = 1` and `SIGSTOP`.

== Phase 8: Hyprland Wayland IPC (`ipc/workspace_monitor.hpp/.cpp`)
Connects to Hyprland's UNIX domain event socket (`.socket2.sock`) in a dedicated non-blocking thread:
- Streams `workspace>>N` and `activewindow>>CLASS,TITLE` events in real-time.
- Queries `j/monitors` to discover all visible workspaces across multi-monitor setups.
- Computes demand-based CPU frequency governor:
  - If any background build is compiling &rarr; `performance`.
  - If all workloads are idle &rarr; `powersave`.
  - General interactive usage &rarr; `schedutil`.

== Phase 9: Enforcement Plane (`enforcement/enforcement_plane.hpp/.cpp`)
Executes POSIX signals and cgroup moves:
- Sends `SIGSTOP` / `SIGCONT`.
- Checks process state `'T'` in `/proc/<pid>/stat` before resuming.
- Migrates processes between cgroup slices by writing to `cgroup.procs`.

== Phase 10: Reclaim Engine (`policy/reclaim_engine.hpp/.cpp`)
Implements an exhaustive *9-step safety check* before terminating any abandoned process:
1. Hard-block protected registry check.
2. Kernel root check (`ppid != 1` and `uid != 0`).
3. Execution detector activity check.
4. Active dependent workload check.
5. Recent user interaction check (&lt;30 seconds).
6. Non-blocking user desktop notification.
7. Grace period wait (2000ms).
8. *Re-check execution detector after grace period* — aborts immediately if activity resumed.
9. Two-stage termination: `SIGTERM` &rarr; 3000ms grace &rarr; `SIGKILL` only if unresponsive.

---

= ArchTitan OS Ecosystem Integration

THM v3 is tightly coupled with the core technologies of ArchTitan Linux:

#table(
  columns: (1.5fr, 2.5fr, 2fr),
  [*Subsystem*], [*ArchTitan Integration Mechanism*], [*Benefit*],

  [Hyprland Compositor],
  [Direct listener on `/run/user/1000/hypr/<his>/ .socket2.sock`. Real-time event parsing for workspace changes and window focus.],
  [Instantaneous adaptation to workspace switches (&lt;1ms latency) with zero polling overhead.],

  [Systemd & cgroups],
  [`archtitan.slice` unit with `Delegate=yes` installed under `/usr/lib/systemd/system/`. Uses cgroup v2 controllers.],
  [Kernel-level resource limits for CPU, memory, and freezing without relying on unstable userspace workarounds.],

  [ArchTitan Services\
  (`titan-ai`, `titan-share`, `titan-mirror`, `titan-shield`, `titan-gpu`)],
  [Pre-registered in `ProtectedRegistry`. Multi-tier exe prefix verification (`/usr/local/bin/titan*`, `/usr/lib/titan*`).],
  [Guaranteed immunity from termination or freezing, even during severe RAM exhaustion spikes.],

  [Audio Stack\
  (`pipewire`, `wireplumber`)],
  [Audio whitelist with verified binary paths. Media playback processes marked `latency_sensitive`.],
  [Zero audio crackle or playback gaps during aggressive memory reclaiming or heavy compile jobs.],

  [Developer Stack\
  (Gradle, Cargo, Clangd, Vite, Antigravity AI)],
  [Workload state machine prioritizes execution. CWD markers auto-bootstrap orphan compiler daemons.],
  [Seamless background compilation across workspaces. IDEs freeze safely when idle; builds finish reliably.]
)

---

= Real OS-Level Empirical Test Results

The native POSIX test harness (`thm_test_runner`) was compiled with GCC 16 and executed against the live Linux kernel (`Linux msfvenom 6.18.47-1-lts x86_64`):

#table(
  columns: (1.2fr, 2fr, 1fr, 0.8fr, 2fr),
  [*Test Case*], [*Description*], [*Result*], [*Time*], [*Kernel Observation*],
  [TC-1], [Gradle Background Build Survival], [PASS], [100ms], [State `'R'` maintained; zero signals delivered.],
  [TC-2], [AI Agent Workload Survival], [PASS], [80ms], [Active=BG_EXEC; Idle AI freezes under pressure.],
  [TC-3], [Idle IDE Freeze & Thaw], [PASS], [101ms], [Verified state `'T'` on SIGSTOP; resumed to `'S'`/`'R'`.],
  [TC-4], [Protected Domain Absolute Immunity], [PASS], [17ms], [Hard-block fired at line 1 under CRITICAL pressure.],
  [TC-5], [Audio Whitelist & Anti-Spoof], [PASS], [0ms], [Spoofing rejected via exe path; legitimate pinned.],
  [TC-6], [Cold-Start Bootstrap via CWD], [PASS], [51ms], [CWD markers mapped reparented process correctly.],
  [TC-7], [Multi-Workspace Concurrency], [PASS], [81ms], [5 rapid switches; active and background stayed alive.],
  [TC-8], [Elimination of Static WS Profiles], [PASS], [0ms], [Decisions invariant across WS 1, 2, 3, 4, 5, 99.],
  [TC-9], [Reclaim Safety Sequence], [PASS], [80ms], [Reclamation aborted when child resumed work.],
  [TC-10], [Demand-Based Governor], [PASS], [0ms], [Build &rarr; `performance`; Idle &rarr; `powersave`.]
)

#align(center)[
  #text(size: 11pt, weight: "bold", fill: rgb("#16a34a"))[
    Comprehensive Test Suite Verdict: 10 / 10 Scenarios Passed (100% Success)
  ]
]

---

= Operational Commands & Deployment Guide

== Compiling & Testing THM v3
```bash
# Clean build of daemon and test harness
cd /home/msfvenom/custom-os-build/titan-hwm-v3
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Execute automated POSIX test suite
./thm_test_runner
# Or via CTest:
ctest --verbose
```

== Running in Safe Dry-Run Mode
```bash
# Non-destructive observation of live desktop (logs decisions without signals)
./titan-hwm-v3 --dry-run --ticks 50
```

== Installing systemd Service & cgroup Slice
```bash
# Copy binary and systemd slice
sudo cp titan-hwm-v3 /usr/local/bin/titan_hw_manager
sudo cp ../archtitan.slice /etc/systemd/system/archtitan.slice

# Reload systemd and enable service
sudo systemctl daemon-reload
sudo systemctl enable --now archtitan.slice
sudo systemctl restart titan_hw_manager
```

---

= Conclusion

Titan Hardware Manager v3 completely resolves the usability and reliability issues that plagued earlier profile-based resource managers. By decoupling workspaces from resource grants and anchoring decision-making in real-time execution detection, ArchTitan OS achieves its design goal: *a lean, sub-1 GB idle desktop that empowers software developers to run complex, long-running compilation and AI workloads without hindrance.*
