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

## Canonical Source

THM is implemented in [`/titan-hwm-v3`](../../titan-hwm-v3/) at the repo root.
The v1/v2 monolith that used to live in `/titan-hwm-source` was **removed** —
it was never installed into the ISO and v3 superseded every behaviour in it.
Recover the old source from git history if ever needed:
`git show 9c7184e:titan-hwm-source/titan_hw_manager.cpp`

## Folder Structure

```
titan-hwm/                    ← this directory: docs/config stubs only
├── src/                      ← no longer used, see above
├── tests/                    ← no longer used; tests live in titan-hwm-v3/tests
├── docs/                     ← Subsystem-specific documentation
├── configs/                  ← Default config files shipped into the OS
└── README.md
```

## Build

```bash
cd titan-hwm-v3
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## Test

```bash
cd titan-hwm-v3/build && ctest --output-on-failure
```

## Install

```bash
sudo cmake --install titan-hwm-v3/build   # or copy via profiledef.sh
```

## Related

- Source: [`/titan-hwm-v3`](../../titan-hwm-v3/)
- Architecture: [`/titan-hwm-v3/docs`](../../titan-hwm-v3/docs/)
- Wiki: [`/wiki/Titan-Hardware-Manager.md`](../../wiki/Titan-Hardware-Manager.md)
