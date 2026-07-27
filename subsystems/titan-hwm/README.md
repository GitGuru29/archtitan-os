# Titan Hardware Manager (THM)

> **Owner:** [@GitGuru29](https://github.com/GitGuru29)  
> **Subsystem:** Titan Hardware Manager  
> **Language:** C++  
> **Status:** ✅ Active Development

## Overview

Titan Hardware Manager (THM) is the hardware abstraction and resource management subsystem of ArchTitan OS. It monitors CPU, GPU, RAM, thermals, and power states — exposing a CLI and Waybar integration for real-time feedback and automated resource tiering.

## Features

- CPU/GPU/RAM monitoring and telemetry
- Thermal tier classification (Cool / Warm / Hot / Critical)
- Fusion classifier with workspace-aware scoring
- Age decay resource management
- Waybar integration (`titan-hwm-waybar`)
- Session guard (`archtitan-session-guard`)

## Folder Structure

```
titan-hwm/
├── src/        ← Source code (link to /titan-hwm-source at repo root)
├── tests/      ← Unit and integration tests
├── docs/       ← Subsystem-specific documentation
├── configs/    ← Default config files shipped into the OS
└── README.md
```

## Build

```bash
cd src/
g++ -O2 -o titan_hw_manager titan_hw_manager.cpp
g++ -O2 -o titan_hwm_cli titan_hwm_cli.cpp
```

## Install

```bash
sudo ./src/install.sh
```

## Related

- Source: [`/titan-hwm-source`](../../titan-hwm-source/)
- Wiki: [`/wiki/Titan-Hardware-Manager.md`](../../wiki/Titan-Hardware-Manager.md)
