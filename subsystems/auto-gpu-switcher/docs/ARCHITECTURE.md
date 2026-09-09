# Auto GPU Switcher — Full Architecture Design

> **Subsystem:** Auto GPU Switcher  
> **Language:** C++17 (daemon + CLI), Bash (udev helpers)  
> **Integration:** THM daemon, Hyprland IPC, systemd  
> **Target:** Intel iGPU + NVIDIA dGPU hybrid (PRIME/Optimus) on Wayland  

---

## 1. Problem Statement

Every Intel+NVIDIA hybrid laptop requires manual `prime-run` prefixing or environment variable injection to offload rendering to the dGPU. There is no Linux distro that handles this transparently. The user must:

1. Know which GPU their app should use
2. Remember to prefix commands with `prime-run`
3. Manually manage dGPU power state for battery life
4. Have no visibility into which GPU is active

**Performance target:** < 2 seconds from trigger to effective GPU switch (per FYP report §10).

---

## 2. System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        Hyprland Compositor                       │
│  windowrulev2 → env tags    activewindowv2 IPC events           │
└──────────┬──────────────────────────┬───────────────────────────┘
           │                          │
           ▼                          ▼
┌──────────────────┐    ┌──────────────────────────────────────┐
│  titan-gpu-cli   │    │       titan-gpu-switcherd (daemon)    │
│  (manual override│    │                                       │
│   + status)      │    │  ┌─────────────┐  ┌───────────────┐  │
└──────────────────┘    │  │ GPUEnforcer │  │WorkloadClassifier│
                        │  └──────┬──────┘  └───────┬───────┘  │
                        │         │                  │          │
                        │  ┌──────▼──────────────────▼───────┐  │
                        │  │       PowerManager              │  │
                        │  │  (sysfs PCI power control)      │  │
                        │  └──────────────┬──────────────────┘  │
                        │                 │                      │
                        │  ┌──────────────▼──────────────────┐  │
                        │  │     HyprlandIPCBridge            │  │
                        │  │  (activewindowv2 listener)       │  │
                        │  └─────────────────────────────────┘  │
                        └──────────────────────────────────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
             ┌──────────┐  ┌──────────────┐  ┌────────────┐
             │ THM IPC  │  │  sysfs/DRM   │  │  nvidia-smi│
             │ (socket) │  │  /sys/bus/pci│  │  (fallback)│
             └──────────┘  └──────────────┘  └────────────┘
```

---

## 3. Component Design

### 3.1 GPU Detection (`GPUDetector`)

Runs once at daemon startup. Enumerates DRM devices and PCI GPUs.

```
/sys/class/drm/card*/device/uevent     → PCI_ID, PCI_SUBSYS_ID
/sys/class/drm/card*/device/vendor     → 8086=Intel, 10de=NVIDIA, 1002=AMD
/sys/class/drm/card*/device/power/     → runtime PM support
/dev/dri/card*                          → DRM device nodes
/dev/dri/renderD*                       → Render nodes (offloading targets)
```

**Output struct:**
```cpp
struct GPUInfo {
    int         card_num;           // DRM card number (card0, card1, ...)
    uint16_t    vendor_id;          // PCI vendor
    uint16_t    device_id;          // PCI device
    std::string drm_path;           // /sys/class/drm/card1/device
    std::string render_node;        // /dev/dri/renderD129
    bool        is_nvidia;
    bool        is_intel;
    bool        is_amd;
    bool        supports_drm_prime; // kernel parameter check
};
```

**Detection logic:**
1. Scan `/sys/class/drm/card*` for all DRM devices
2. For each, read `device/vendor` to identify GPU vendor
3. Identify which GPU is wired to the display connector (iGPU = connector "connected" status in `/sys/class/drm/card*/status`)
4. For NVIDIA: check if proprietary driver is loaded (`/proc/driver/nvidia/version`) or nouveau
5. Map render nodes: NVIDIA render node is typically `renderD129` when iGPU is `renderD128`
6. Store the PCI address for power management (`0000:01:00.0` format)

### 3.2 Workload Classifier (`WorkloadClassifier`)

Determines which GPU a window should use. Two-tier classification:

**Tier 1 — Rule-based (instant, 0ms):**
Matches window class/title against a rules database. Rules are loaded from config and can be hot-reloaded.

**Tier 2 — THM profile fallback (when no rule matches):**
Uses the current THM workload profile to make a default decision.

**GPU routing decision:**
```cpp
enum class GPUTarget { IGPU, DGPU, AUTO };

struct ClassificationResult {
    GPUTarget   target;
    std::string matched_rule;    // which rule triggered (for debug/logging)
    float       confidence;      // 0.0-1.0
    bool        power_transition; // does this require dGPU power-on?
};
```

**Classification sources (in priority order):**
1. **Manual override** (`titan-gpu set igpu/dgpu/auto` — user explicitly chose)
2. **Per-app rule** (config file: `steam → DGPU`, `kitty → IGPU`)
3. **Class pattern match** (wildcard: `*.game → DGPU`, `*.browser → AUTO`)
4. **File extension in title** (`.mp4 → DGPU` for hardware decode, `.c → IGPU`)
5. **THM profile fallback** (`CASUAL → IGPU`, `SYSTEM_DEV → IGPU`, `WEB_DEV → IGPU`)
6. **Power state heuristic** (on battery → prefer IGPU, on AC → allow DGPU)

### 3.3 GPU Enforcer (`GPUEnforcer`)

The core switching engine. Applies the classification decision.

**Approach: Environment Variable Injection via Hyprland Launch Scripts**

On Wayland/Hyprland, you cannot change a running window's GPU assignment. The solution is to intercept app launches and inject the correct environment variables *before* the app starts.

**Mechanism:**
1. Maintain a directory `/etc/titan-gpu/launchers/` containing wrapper scripts
2. Each wrapper sets the appropriate env vars and exec's the real binary
3. Hyprland `exec` commands in `hyprland.conf` use these wrappers
4. For apps not in the launcher directory, the daemon monitors Hyprland's `activewindowv2` IPC and logs the wm_class for the user to create a rule

**Environment variables per GPU target:**

| Target | Variables |
|--------|-----------|
| NVIDIA dGPU (OpenGL) | `__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia` |
| NVIDIA dGPU (Vulkan) | `__NV_PRIME_RENDER_OFFLOAD=1` (Vulkan ICD auto-selects) |
| Intel iGPU (OpenGL) | `__GLX_VENDOR_LIBRARY_NAME=intel` (or unset everything) |
| Intel iGPU (Vulkan) | `VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/intel_icd.x86_64.json` |
| DRI_PRIME (open-source) | `DRI_PRIME=1` (for nouveau/AMD reverse PRIME) |

**Power state control (for DGPU off when not needed):**
```bash
# Power off NVIDIA dGPU
echo "auto" > /sys/bus/pci/devices/0000:01:00.0/power/control

# Force power on (before launching dGPU app)
echo "on" > /sys/bus/pci/devices/0000:01:00.0/power/control

# NVIDIA-specific power management
nvidia-smi -pm 1          # enable persistence mode
nvidia-smi -pl 75         # set power limit (watts)
```

**Power transition sequence:**
```
I GPU active, dGPU off
    ↓ (app needs dGPU)
Step 1: echo "on" > /sys/bus/pci/.../power/control     (50-200ms)
Step 2: Wait for NVIDIA driver to resume                  (200-500ms)
Step 3: Verify render node exists: ls /dev/dri/renderD129
Step 4: Launch app with dGPU env vars
    ↓ (no dGPU app for 30s)
Step 5: Verify no processes using renderD129
Step 6: echo "auto" > /sys/bus/pci/.../power/control    (saves battery)
```

### 3.4 Power Manager (`PowerManager`)

Controls dGPU power state and monitors power supply.

**Sources:**
```
/sys/class/power_supply/AC/online         → 1=plugged in, 0=battery
/sys/class/power_supply/BAT*/status       → Charging/Discharging/Full
/sys/bus/pci/devices/0000:01:00.0/power/ → runtime PM states
```

**Power profiles:**
```cpp
enum class PowerProfile {
    BALANCED,      // dGPU on when needed, off after 30s idle
    POWER_SAVER,   // dGPU off always, force IGPU-only
    PERFORMANCE,   // dGPU always on (gaming session)
    AUTO           // based on AC/battery state
};
```

**AC/Battery auto-switching:**
- On AC power → `BALANCED` (dGPU available, auto power-on when needed)
- On battery → `POWER_SAVER` (dGPU off, IGPU-only unless user overrides)
- User can lock to `PERFORMANCE` for gaming sessions regardless of power

### 3.5 Hyprland IPC Bridge (`HyprlandIPCBridge`)

Reuses the same IPC pattern as THM — connect to Hyprland's `.socket2.sock` for event stream.

**Events consumed:**
```
activewindowv2>>addr,pid,wm_class,title   → trigger classification
workspace>>id                              → track active workspace
monitoradded>>name / monitorremoved>>name  → re-evaluate multi-GPU routing
```

**Integration with THM:**
The daemon reads THM's state file `/tmp/titan_hwm_state` to get the current workload profile. It can also register itself as a THM plugin via the UNIX socket at `/tmp/titan_hwm.sock` to receive profile change notifications.

### 3.6 CLI Tool (`titan-gpu`)

```bash
titan-gpu status              # show GPU state, power, current target
titan-gpu set igpu            # force IGPU for all apps
titan-gpu set dgpu            # force DGPU for all apps
titan-gpu set auto            # return to automatic switching
titan-gps rule add "steam" dgpu    # add per-app rule
titan-gpu rule remove "steam"
titan-gpu rule list
titan-gpu power on            # force dGPU on
titan-gpu power off           # force dGPU off
titan-gpu power status        # show power state
titan-gpu profile set balanced|saver|performance|auto
titan-gpu metrics             # live dashboard (like titan-hwm metrics)
```

### 3.7 Waybar Widget

JSON module reading from `/tmp/titan_gpu_state`:
```json
{
    "igpu": "Intel UHD 630",
    "dgpu": "NVIDIA RTX 3060",
    "active": "igpu",
    "power": "off",
    "profile": "balanced",
    "battery_pct": 87
}
```

---

## 4. Configuration Format

File: `/etc/titan-gpu/config`

```ini
# Titan GPU Switcher Configuration

[power]
default_profile = balanced
battery_profile = saver
dgpu_idle_timeout_sec = 30
power_transition_timeout_ms = 1000

[detector]
nvidia_driver = proprietary    # proprietary | nouveau | auto
render_node_igpu = /dev/dri/renderD128
render_node_dgpu = /dev/dri/renderD129

[classifier]
# THM profile → GPU default (when no per-app rule matches)
thm_casual_gpu = igpu
thm_web_dev_gpu = igpu
thm_system_dev_gpu = igpu
thm_android_dev_gpu = igpu
thm_neutral_gpu = igpu

[apps]
# Per-app GPU assignment rules
# Format: wm_class = dgpu | igpu | auto
# wm_class comes from Hyprland's window rule system (lowercase)
steam = dgpu
lutris = dgpu
blender = dgpu
obs-studio = dgpu
mpv = dgpu
kitty = igpu
falkon = igpu
code = igpu
neovim = igpu

[patterns]
# Wildcard patterns for class matching
# *game* matches any class containing "game"
*game* = dgpu
*gpu-intensive* = dgpu
*browser* = igpu
*editor* = igpu
*terminal* = igpu
```

---

## 5. Data Flow

### 5.1 App Launch → GPU Assignment

```
User launches "steam"
    │
    ▼
Hyprland exec rule: exec = /etc/titan-gpu/launchers/steam %U
    │
    ▼
Launcher script:
    #!/bin/sh
    export __NV_PRIME_RENDER_OFFLOAD=1
    export __GLX_VENDOR_LIBRARY_NAME=nvidia
    exec /usr/bin/steam "$@"
    │
    ▼
Steam starts on NVIDIA dGPU
    │
    ▼
titan-gpu-switcherd detects via activewindowv2>>...,pid,steam,...
    │
    ▼
Logs: "[GPU] steam (PID 1234) → DGPU (rule: steam=dgpu)"
    │
    ▼
Writes /tmp/titan_gpu_state (Waybar updates)
```

### 5.2 Automatic Power Management

```
No dGPU apps running for 30s
    │
    ▼
PowerManager checks: any process using /dev/dri/renderD129?
    │ (no)
    ▼
echo "auto" > /sys/bus/pci/0000:01:00.0/power/control
    │
    ▼
nvidia-smi --gpu-reset (optional, full power-off)
    │
    ▼
Notify: "[GPU] dGPU powered down (battery saving)"
    │
    ▼
New dGPU app launched
    │
    ▼
PowerManager: echo "on" > /sys/bus/pci/.../power/control
    │ (wait 500ms for driver resume)
    ▼
Verify render node ready
    │
    ▼
Launch app with dGPU env vars
```

### 5.3 Daemon ↔ THM Communication

```
titan-hwm detects workload change
    │
    ▼
Writes profile to /tmp/titan_hwm_state
    │
    ▼
titan-gpu-switcherd reads state file (inotify watch)
    │
    ▼
Re-evaluates default GPU target for all visible windows
    │ (if profile changed from CASUAL → no change needed)
    │ (if user opens a game while in CASUAL → per-app rule takes priority)
    ▼
Applies any needed changes
```

---

## 6. File Layout

```
subsystems/auto-gpu-switcher/
├── src/
│   ├── main.cpp                    # Daemon entry point, signal handling, main loop
│   ├── gpu_detector.cpp/.h         # DRM/PCI GPU enumeration
│   ├── gpu_detector_test.cpp       # Unit tests for detection
│   ├── workload_classifier.cpp/.h  # Rule engine + THM profile fallback
│   ├── gpu_enforcer.cpp/.h         # Env var injection + power transitions
│   ├── power_manager.cpp/.h        # AC/battery detection, PCI power control
│   ├── hyprland_ipc_bridge.cpp/.h  # Hyprland event listener (reuse THM pattern)
│   ├── config.cpp/.h               # Config loader (same key=value format as THM)
│   ├── state_writer.cpp/.h         # Writes /tmp/titan_gpu_state for Waybar
│   └── cli.cpp                     # titan-gpu CLI tool (UNIX socket client)
├── tests/
│   ├── test_detector.cpp
│   ├── test_classifier.cpp
│   └── test_power.cpp
├── configs/
│   └── titan-gpu.config.default    # Default configuration template
├── launchers/                      # Template launcher scripts
│   ├── steam.sh
│   ├── blender.sh
│   ├── mpv.sh
│   └── generic-dgpu.sh             # Template for user-created launchers
├── systemd/
│   ├── titan-gpu-switcherd.service # System daemon (runs as root)
│   └── titan-gpu-switcherd.conf    # systemd override config
├── udev/
│   ├── 99-nvidia-power.rules       # udev rules for NVIDIA GPU hotplug
│   └── 10-titan-gpu-power.rules    # Power supply change triggers
├── waybar/
│   ├── titan-gpu.sh                # Waybar module script
│   └── titan-gpu.css               # Waybar widget styles
├── docs/
│   ├── ARCHITECTURE.md             # This file
│   ├── USER-GUIDE.md               # End-user documentation
│   └── TROUBLESHOOTING.md          # Debug guide
├── install.sh                      # Build + install script
└── README.md
```

---

## 7. Integration Points

### 7.1 THM Integration
- **State file:** Reads `/tmp/titan_hwm_state` for current workload profile
- **Config hot-reload:** Watches `/etc/titan-gpu/config` via inotify (same pattern as THM)
- **IPC socket:** Listens on `/tmp/titan_gpu.sock` for CLI commands (same pattern as THM)
- **Profile-aware defaults:** When THM switches profiles, GPU switcher re-evaluates defaults
- **Future:** Could register as a THM plugin via the daemon socket for push notifications

### 7.2 Hyprland Integration
- **IPC listener:** Connects to `.socket2.sock` for window events (same pattern as THM)
- **Exec rules:** Template launcher scripts installed to `/etc/titan-gpu/launchers/`
- **Window rules:** Hyprland `windowrulev2` can be used for additional class-based routing

### 7.3 Systemd Integration
- **Service type:** `Type=simple` (long-running daemon)
- **Dependencies:** `After=graphical-session.target`, `PartOf=graphical-session.target`
- **OOM score:** `-1000` (immune to OOM killer, same as THM)
- **Restart:** `Restart=on-failure`, `RestartSec=3`

### 7.4 Waybar Integration
- **Module type:** Custom script module
- **Update interval:** 2 seconds (reads `/tmp/titan_gpu_state`)
- **Style:** Catppuccin Mocha theme, GPU icon with power indicator

### 7.5 Package Dependencies
```
# Required
nvidia-utils          # NVIDIA userspace (proprietary driver)
# OR
mesa                  # For nouveau (open-source)

# Optional
libdrm                # DRM device enumeration (already in base)
pciutils              # lspci (already in base)

# Build
cmake                 # Build system
gcc/g++               # Compiler
```

---

## 8. Edge Cases & Safety

### 8.1 No NVIDIA GPU Present
- `GPUDetector` finds only Intel → daemon becomes a no-op (logs "iGPU only, switching disabled")
- No power management, no launcher injection
- CLI shows: "No NVIDIA GPU detected"

### 8.2 NVIDIA Driver Not Loaded
- Check `/proc/driver/nvidia/version` at startup
- If missing: log warning, offer to load `nvidia` module via `modprobe`
- If nouveau only: use `DRI_PRIME=1` instead of `__NV_PRIME_RENDER_OFFLOAD`

### 8.3 dGPU Power-On Failure
- Timeout after `power_transition_timeout_ms` (default 1000ms)
- Log error, notify user: "dGPU power-on failed — falling back to iGPU"
- App launches on iGPU with degraded performance

### 8.4 App Already Running When Rule Changes
- Cannot retroactively switch a running window's GPU
- Log: "Rule changed for 'steam' — takes effect on next launch"
- User must close and reopen the app

### 8.5 Multiple NVIDIA GPUs (SLI/laptop+eGPU)
- Enumerate all NVIDIA devices
- User specifies preferred dGPU in config: `preferred_dgpu_pci = 0000:01:00.0`
- Default: use the first NVIDIA GPU found

### 8.6 Flatpak/Snap Apps
- Flatpak apps run in a sandbox with their own GPU access
- Flatpak GPU override: `flatpak override --user --device=dri com.app.Name`
- The daemon can auto-detect Flatpak apps and apply overrides

---

## 9. Implementation Priority

| Phase | Components | Effort |
|-------|-----------|--------|
| **Phase 1: Core** | GPUDetector + PowerManager + CLI (status/set/power) | ~800 lines |
| **Phase 2: Switching** | WorkloadClassifier + GPUEnforcer + launcher scripts | ~600 lines |
| **Phase 3: Integration** | HyprlandIPCBridge + StateWriter + systemd service | ~400 lines |
| **Phase 4: UI** | Waybar widget + Titan Settings integration | ~300 lines |
| **Phase 5: Polish** | Flatpak support, hot-plug, edge cases, tests | ~400 lines |

**Total estimated:** ~2500 lines C++17

---

## 10. Testing Strategy

| Test | Method |
|------|--------|
| GPU detection | Mock sysfs paths, verify correct enumeration |
| Power transitions | Mock sysfs power states, verify timing |
| Classification rules | Unit test rule matching with sample wm_class/title |
| Launcher scripts | Integration test: verify env vars in child process |
| End-to-end | QEMU VM with virtual GPU (current `run-vm.sh` pattern) |
| Battery drain | Measure dGPU power consumption with `powertop` |
| Switch latency | Time from rule match to render node availability |

---

## 11. Key Technical References

- NVIDIA PRIME Offloading: https://us.download.nvidia.com/XFree86/Linux-x86_64/535.104.05/README/primerenderoffload.html
- DRM PRIME: https://www.kernel.org/doc/html/latest/gpu/drm-uapi.html
- PCI Power Management: https://www.kernel.org/doc/Documentation/power/pci.txt
- Hyprland IPC: https://wiki.hyprland.org/IPC/
- THM source: `titan-hwm-source/titan_hw_manager.cpp` (reference for IPC, config, inotify patterns)
