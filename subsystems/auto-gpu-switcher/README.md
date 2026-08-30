# Auto GPU Switcher

> **Owner:** [@GitGuru29](https://github.com/GitGuru29)  
> **Subsystem:** Auto GPU Switcher  
> **Language:** C++17 / Shell  
> **Status:** 🚀 Active Development / Hardware Verified

## Overview

**Auto GPU Switcher** is the intelligent, seamless hybrid-GPU switching subsystem for **ArchTitan OS** (and standalone Arch Linux with Hyprland). It automatically routes rendering between integrated GPU (Intel Iris Xe / AMD Radeon) and discrete GPU (NVIDIA GeForce / RTX) based on binary analysis, workload rules, window focus, power supply state, and thermal limits — eliminating the need for manual `prime-run` prefixes.

---

## Key Features

- 🔍 **Automatic GPU Detection** — Enumerates DRM devices and PCI bus at startup to map render nodes (`/dev/dri/renderD128`, `renderD129`).
- ⚡ **Binary & ELF Analyzer** — Automatically scans installed packages and binaries via ALPM/pacman hooks for 3D/Vulkan/OpenGL dependencies (`libGL`, `libvulkan`, `libcuda`) and registers them in `heavy_apps.list`.
- 🎮 **Environment Variable Injection** — Automatically injects `__NV_PRIME_RENDER_OFFLOAD=1`, `__GLX_VENDOR_LIBRARY_NAME=nvidia`, and `__VK_LAYER_NV_optimus=NVIDIA_only` for heavy applications.
- 🪟 **Hyprland IPC Bridge** — Connects directly to Hyprland's UNIX event socket (`.socket2.sock`) to track active and focused windows in real-time.
- 🔋 **Dynamic Runtime Power Management** — Uses Linux kernel PCI runtime power management (`auto` / `on`) and udev rules to power down the dGPU when idle (D3cold) and wake it instantly on demand.
- 🌡️ **Thermal & Fan Protection** — Integrates with `thermald` and Lenovo ACPI DYTC thermal profiles to prevent hardware thermal trips and overheating during intensive 3D gaming/benchmarks.
- 💻 **Unified CLI (`titan-gpu`)** — Provides instant status checks, manual GPU overrides, and power state control.
- 📊 **Waybar Widget Integration** — Writes atomic JSON state to `/tmp/titan_gpu_state` for dynamic status bar indicators.

---

## Subsystem Architecture

```
auto-gpu-switcher/
├── src/                    ← C++17 source code (daemon, detector, enforcer, CLI)
│   ├── gpu_detector.cpp    ← DRM/PCI device enumeration
│   ├── workload_classifier.cpp ← App rules & power state evaluation
│   ├── gpu_enforcer.cpp    ← Runtime client tracking & power enforcement
│   ├── power_manager.cpp   ← AC/battery detection & PCI power sysfs interface
│   ├── hyprland_ipc_bridge.cpp ← Wayland/Hyprland socket event listener
│   ├── state_writer.cpp    ← Atomic JSON state generator for UI/Waybar
│   ├── config.cpp          ← INI configuration parser
│   └── main.cpp            ← Daemon lifecycle & UNIX command socket
├── interceptor/            ← Binary wrapper & dynamic offload policy
├── analyzer/               ← ALPM hooks and package ELF inspection scripts
├── configs/                ← Default configuration template (/etc/titan-gpu/config)
├── launchers/              ← Pre-configured application launchers
├── systemd/                ← systemd unit files (titan-gpu-switcherd.service)
├── udev/                   ← 99-nvidia-power.rules for non-root PCI power management
├── waybar/                 ← Waybar custom module script and styling
└── docs/                   ← Detailed architecture and troubleshooting guides
```

---

## Build & Installation

### 1. Build from Source
```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 2. System Installation
```bash
# Install binary & daemon
sudo cmake --install .

# Install configuration template
sudo mkdir -p /etc/titan-gpu/launchers
sudo cp ../configs/titan-gpu.config.default /etc/titan-gpu/config

# Install udev power rules
sudo cp ../udev/99-nvidia-power.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger

# Enable & start systemd daemon
sudo cp ../systemd/titan-gpu-switcherd.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now titan-gpu-switcherd
```

---

## CLI Usage (`titan-gpu`)

| Command | Description |
| :--- | :--- |
| `titan-gpu status` | Display detected GPUs, active power source, current target, and dGPU power status |
| `titan-gpu set auto` | Enable automatic rule and window-based switching *(default)* |
| `titan-gpu set dgpu` | Force all rendering to discrete GPU |
| `titan-gpu set igpu` | Force integrated GPU rendering to maximize battery life |
| `titan-gpu power on` | Force PCI power rail `on` for dGPU |
| `titan-gpu power auto` | Allow dGPU to enter runtime suspend (D3cold) when idle |
| `titan-gpu reload` | Reload `/etc/titan-gpu/config` rules without restarting daemon |

---

## Thermal & Hardware Considerations

When running sustained 3D workloads (e.g. Unigine benchmarks, AAA gaming) on hybrid laptops:

1. **Active Thermal Daemon (`thermald`):**  
   Ensure `thermald` is active to balance CPU PL1/PL2 power budgets with the dGPU:
   ```bash
   sudo pacman -S thermald
   sudo systemctl enable --now thermald
   ```

2. **Power Profile:**  
   Set the power profile to `balanced` to prevent unnecessary CPU turbo voltage spikes:
   ```bash
   powerprofilesctl set balanced
   ```

3. **Lenovo Extreme Performance Fan Mode (Lenovo Laptops):**  
   Press `Fn + Q` or send the ACPI DYTC command to unlock maximum fan cooling:
   ```bash
   echo '\_SB.PCI0.LPCB.EC0.VPC0.DYTC 0x0012B001' | sudo tee /proc/acpi/call
   ```

---

## Status & Roadmap

- [x] Phase 1: DRM/PCI detection & ELF package analyzer hook
- [x] Phase 2: Hyprland IPC bridge & automatic runtime power management
- [x] Phase 3: Hardware validation on Intel TigerLake + NVIDIA GeForce MX350
- [ ] Phase 4: Native Waybar GUI popup & dynamic thermal kick integration
