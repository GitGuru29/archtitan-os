# Titan Hardware Manager (THM)

The **Titan Hardware Manager (`titan-hwm`)** is ArchTitan OS's primary system resource orchestration service. It is a privileged C++ daemon running as a systemd service (`titan_hw_manager.service`) that dynamically optimizes system resource allocation based on user workflow state.

---

## Key Features

- **Workload Classification**: Automatically identifies developer activity (System Dev, Web Dev, Android Dev, Casual, Neutral) via a multi-signal fusion classifier.
- **cgroup v2 Slicing**: Controls process priority, CPU bandwidth, and memory allocation by dynamically organizing processes into `titan-active.slice`, `titan-background.slice`, and `titan-frozen.slice`.
- **PSI & Memory Escalation**: Monitors `/proc/pressure/memory` (Pressure Stall Information) and `/sys/class/thermal` to prevent OOM thrashing and thermal throttling.
- **Audio Protection Whitelist**: Ensures background media players and audio daemons (PipeWire, Spotify, mpv) are never frozen or SIGKILLed during memory pressure.
- **CLI & IPC**: Controlled and monitored via `/usr/local/bin/titan-hwm` interacting over a Unix domain socket at `/tmp/titan_hwm.sock`.

---

## Architecture & Control Loop

```mermaid
flowchart TD
    subgraph Inputs["Telemetry & Signals"]
        HYPR[Hyprland Window Focus]
        PROC[Process Tree / procfs]
        PSI[/proc/pressure/memory]
        THERM[/sys/class/thermal]
        SOCK[/tmp/titan_hwm.sock IPC]
    end

    subgraph Daemon["titan_hw_manager Daemon"]
        FUS[Fusion Classifier<br>3-Signal Engine]
        POL[Policy Manager & Governor]
        ESC[PSI Escalation Ladder]
        STATE[State Writer<br>/tmp/titan_hwm_state]
    end

    subgraph Control["cgroup v2 Control"]
        ACT[titan-active.slice]
        BG[titan-background.slice]
        FRZ[titan-frozen.slice]
        SYS[CPU Governor Hints]
    end

    HYPR --> FUS
    PROC --> FUS
    SOCK --> POL
    FUS --> POL
    PSI --> ESC
    THERM --> POL

    POL --> ACT
    POL --> BG
    ESC --> FRZ
    POL --> SYS
    POL --> STATE
```

---

## Workload Profiles

THM recognizes five primary workload profiles based on window focus, running binaries, and workspace assignments:

| Profile | Target Activity | Allocation Strategy |
| :--- | :--- | :--- |
| **System Dev** | C/C++, Rust, Linux Kernel, CMake (`clangd`, `cargo`, `gcc`) | Maximum CPU priority, protected LSP memory, active governor hints |
| **Web Dev** | Node.js, Vite, TypeScript, React (`node`, `npm`, `vite`, `tsserver`) | Elevated memory budget, high thread prioritization for build watchers |
| **Android Dev** | Android Studio, Gradle, ADB, QEMU Emulators (`java`, `gradle`, `adb`) | Reserved memory slab for emulator & Gradle daemon, high I/O priority |
| **Casual** | Web browsing, video streaming, text editing (`chromium`, `mpv`) | Balanced CPU scheduling, background compiler tasks throttled |
| **Neutral** | General shell usage, idle system | Default Linux CFS scheduling |

---

## cgroup v2 Slices & Resource Protection

THM creates and manages three cgroup v2 sub-trees under `/sys/fs/cgroup/`:

1. `titan-active.slice`:
   - Contains the foreground application, active IDE, active LSP servers, and current terminal shell.
   - Allocated `cpu.weight = 200` and high memory pressure protection (`memory.low`).

2. `titan-background.slice`:
   - Contains unfocused applications, idle browser tabs, and secondary tools.
   - Allocated `cpu.weight = 50` to guarantee foreground responsiveness.

3. `titan-frozen.slice`:
   - Managed during high PSI pressure (>50% memory pressure stall).
   - Suspends non-essential background processes using cgroup `cgroup.freeze = 1`.

### Architectural Comparison to `oomd`

THM adopts the proven pressure-response pattern pioneered by Meta's `oomd` (and subsequently `systemd-oomd`)—specifically the use of kernel PSI metrics to trigger cgroup-level throttling and kills before traditional kernel OOM lockups occur. However, while `oomd` evaluates memory statistics in isolation on headless servers, THM's core novelty is layering **workspace and compositor awareness** on top of this PSI escalation ladder. By integrating with Hyprland IPC and analyzing GUI context, THM intelligently routes processes into `active`, `background`, or `frozen` slices based on user focus. This allows THM to enforce cross-workspace exemptions (like shielding compile daemons while browsing) that a pure memory-stat-driven daemon fundamentally cannot achieve.

---

## Audio Whitelist Protection

To avoid disrupting audio playback when background apps are constrained, THM enforces an explicit whitelist for audio-producing processes:

```
spotify, spotifyd, mpd, mpdris2, mpv, vlc, rhythmbox, strawberry, deadbeef,
cmus, ncmpcpp, cantata, audacious, elisa, playerctld, pipewire, pipewire-pulse,
wireplumber, pulseaudio
```

Processes in the audio whitelist are strictly excluded from `titan-frozen.slice` and OOM score escalation.

---

## CLI Control Reference (`titan-hwm`)

The `titan-hwm` utility provides command-line control and live telemetry:

### Manual Profile Switching

```bash
# Force a specific profile
titan-hwm switch web
titan-hwm switch android
titan-hwm switch system
titan-hwm switch casual
```

### Daemon Status & State

```bash
# Display active profile, RAM usage, CPU temperature
titan-hwm status
```

### Live Telemetry Dashboard

```bash
# Launch real-time telemetry dashboard (PSI, cgroups memory consumption)
titan-hwm metrics
```

---

## Configuration & State Files

- **IPC Socket**: `/tmp/titan_hwm.sock` (UNIX domain socket for CLI requests)
- **State File**: `/tmp/titan_hwm_state` (Read by Waybar custom modules and scripts)
- **Service Unit**: `/etc/systemd/system/titan_hw_manager.service`
- **Binary Locations**:
  - Daemon: `/usr/local/bin/titan_hw_manager`
  - CLI: `/usr/local/bin/titan-hwm`
