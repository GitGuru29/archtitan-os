# Titan Sandbox

> **Owner:** [@GitGuru29](https://github.com/GitGuru29)  
> **Subsystem:** Titan Sandbox  
> **Language:** C++  
> **Status:** ✅ Active Development

## Overview

Titan Sandbox is the secure container and isolation subsystem of ArchTitan OS. It leverages `systemd-nspawn` containers with policy enforcement to provide sandboxed execution environments for untrusted applications and processes.

## Features

- `systemd-nspawn` based container execution
- Policy-driven sandbox profiles
- Snapshot and restore support
- THM integration for resource-aware sandboxing
- Service test harness

## Folder Structure

```
titan-sandbox/
├── src/        ← Source code (link to /sandbox at repo root)
├── tests/
├── docs/
├── configs/    ← Sandbox policy configs
└── README.md
```

## Build

```bash
cd src/
cmake -B build && cmake --build build
```

## Related

- Source: [`/sandbox`](../../sandbox/)
- Wiki: [`/wiki/Titan-Sandbox.md`](../../wiki/Titan-Sandbox.md)
