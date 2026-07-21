# Developer Guide

Welcome to the **ArchTitan OS Developer Guide**. This document outlines the repository layout, build environments, local iteration workflows, and contribution practices for developers working on the distribution profile, custom daemons, or helper tools.

---

## Repository Tree Overview

```
custom-os-build/
├── airootfs/                # Rootfs overlay copied into the ISO image
│   ├── etc/
│   │   ├── skel/.config/    # Default user dotfiles (Hyprland, Waybar, Kitty, Fish, etc.)
│   │   ├── systemd/system/  # Custom systemd units (titan_hw_manager.service)
│   │   └── titan-sandbox/   # Sandbox security policies
│   └── usr/
│       └── local/bin/       # Pre-installed helper scripts and binaries
├── docs/                    # Architectural design documents & FYP reports
├── efiboot/                 # EFI boot image staging
├── grub/                    # GRUB bootloader configuration for live ISO
├── sandbox/                 # Titan Sandbox source code & C++ headers
│   ├── titan-sandboxd.cpp
│   └── thm_sandbox_integration.h
├── titan-hwm-source/        # Titan Hardware Manager source staging
├── titanfetch-src/          # TitanFetch C++/Qt6 source code
│   ├── sysinfo.cpp / sysinfo.h
│   ├── cli.cpp / gui.cpp
│   └── CMakeLists.txt
├── wiki/                    # Project Wiki documentation (Markdown)
├── install.sh               # Local host installation script for THM daemon
├── packages.x86_64          # List of official Arch packages for pacstrap
├── pacman.conf              # Pacman repository configuration for ISO build
├── profiledef.sh            # archiso profile metadata, compression, & permissions
└── run-vm.sh                # QEMU / KVM helper script for live ISO testing
```

---

## Development Environment Setup

### Required Packages (Arch Host)

```bash
sudo pacman -S --needed \
    archiso git base-devel cmake qt6-base \
    qemu-desktop edk2-ovmf virt-viewer
```

---

## Local Component Iteration Workflows

Building a full ISO image with `mkarchiso` takes several minutes. To test and iterate on custom daemons quickly, develop them locally on your Arch host system:

### 1. Titan Hardware Manager (`titan-hwm`)

To compile and install `titan-hwm` onto your current host machine:

```bash
# From repository root
./install.sh
```

This compiles `titan_hw_manager.cpp` and `titan_hwm_cli.cpp`, installs binaries to `/usr/local/bin/`, and starts `titan_hw_manager.service`.

To test manually:

```bash
# Check service status
systemctl status titan_hw_manager

# Trigger profile switch via socket CLI
titan-hwm switch system

# View live telemetry
titan-hwm metrics
```

---

### 2. TitanFetch (`titanfetch`)

To compile TitanFetch locally:

```bash
cd titanfetch-src
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run CLI mode
./build/titanfetch

# Run Qt6 GUI mode
./build/titanfetch --gui
```

---

### 3. Titan Sandbox (`titan-sandboxd`)

To build the sandbox daemon:

```bash
cd sandbox
g++ -std=c++20 -O2 titan-sandboxd.cpp -o titan-sandboxd

# Test executing a binary under sandbox policy
./titan-sandboxd --policy policies/system-dev.toml -- /usr/bin/whoami
```

---

### 4. Live ISO Testing via QEMU

Once ISO changes are ready, generate the ISO and test inside a local QEMU virtual machine:

```bash
# Clean previous build artifacts
sudo rm -rf tmp-work out

# Build the ISO
sudo mkarchiso -v -w tmp-work/ -o out/ ./

# Launch in QEMU
./run-vm.sh
```

---

## Coding Standards & Conventions

- **C++**: C++17 or C++20 standard. Use standard library algorithms and `std::filesystem`. Minimize heavy external third-party dependencies to keep ISO sizes small.
- **IPC & Sockets**: All daemons must handle socket write errors gracefully and clean up socket files on `SIGINT`/`SIGTERM`.
- **Systemd**: System services must use `Type=simple` or `Type=exec` and redirect stdout/stderr to `journald`.
- **Shell Scripts**: Bash scripts must pass `shellcheck` with zero warnings, use explicit error checking, and set `set -euo pipefail` where applicable.

---

## Submitting Pull Requests

1. Create a feature branch: `git checkout -b feature/your-feature-name`.
2. Ensure changes build cleanly without compiler warnings (`-Wall -Wextra`).
3. Verify that `profiledef.sh` permissions cover any newly added scripts or binaries in `airootfs/`.
4. Run `run-vm.sh` to confirm the ISO boots to the Hyprland desktop without systemd errors.
