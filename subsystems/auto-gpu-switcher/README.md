# Auto GPU Switcher

> **Owner:** [@GitGuru29](https://github.com/GitGuru29)  
> **Subsystem:** Auto GPU Switcher  
> **Language:** C++17  
> **Status:** 📐 Architecture Designed

## Overview

Auto GPU Switcher is the intelligent GPU switching subsystem for ArchTitan OS. It automatically routes rendering between Intel iGPU and NVIDIA dGPU based on workload rules, power state, and THM profile — eliminating the need for manual `prime-run` prefixing.

## Architecture

Full design: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)

## Features

- **Automatic GPU detection** — DRM/PCI enumeration at startup
- **Rule-based classification** — per-app and pattern-matching GPU routing
- **Environment variable injection** — `__NV_PRIME_RENDER_OFFLOAD`, `__GLX_VENDOR_LIBRARY_NAME`
- **Dynamic power management** — dGPU powers down when idle, spins up on demand
- **AC/battery awareness** — auto-switches power profile based on power supply
- **THM integration** — reads workload profiles for default GPU decisions
- **CLI tool** — `titan-gpu status|set|rule|power|profile`
- **Waybar widget** — live GPU status with power indicator
- **Manual override** — user can force igpu/dgpu/auto at any time

## Components

| Component | Purpose |
|-----------|---------|
| `GPUDetector` | Enumerates DRM devices, identifies Intel/NVIDIA |
| `WorkloadClassifier` | Rules engine → determines GPU target per window |
| `GPUEnforcer` | Applies env vars + controls dGPU power state |
| `PowerManager` | AC/battery detection, PCI power control |
| `HyprlandIPCBridge` | Listens for window events via Hyprland IPC |
| `titan-gpu` | CLI tool (UNIX socket client) |
| Waybar widget | Reads `/tmp/titan_gpu_state` for display |

## Folder Structure

```
auto-gpu-switcher/
├── src/            ← Source code (daemon + CLI)
├── tests/          ← Unit + integration tests
├── docs/           ← Architecture, user guide, troubleshooting
├── configs/        ← Default config template
├── launchers/      ← Template launcher scripts for dGPU apps
├── systemd/        ← Service unit files
├── udev/           ← Power management udev rules
├── waybar/         ← Waybar module script + styles
└── README.md
```

## Status

> Architecture fully designed. Implementation begins at Milestone 1.1.
