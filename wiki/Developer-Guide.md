# Developer Guide

Welcome to the **ArchTitan OS Developer Guide**. This document outlines the repository layout, build environments, local iteration workflows, and contribution practices for developers working on the distribution profile, custom daemons, first-party GUI apps, or helper tools.

---

## Repository Tree Overview

```
custom-os-build/
├── .github/                 # GitHub CI workflows & repo governance
│   ├── CODEOWNERS           # Automated review assignment for protected OS files
│   ├── pull_request_template.md # PR validation checklist
│   └── workflows/           # GitHub Actions (build-iso.yml, deploy-wiki.yml)
├── airootfs/                # Rootfs overlay copied into the ISO image
│   ├── etc/
│   │   ├── skel/.config/    # Default user dotfiles (Hyprland, Waybar, Kitty, Fish, etc.)
│   │   ├── systemd/system/  # Custom systemd units (titan_hw_manager.service)
│   │   └── titan-sandbox/   # Sandbox security policies
│   └── usr/
│       └── local/bin/       # Pre-installed helper scripts and binaries
├── archtitan-settings/      # ArchTitan Settings C++/Qt6 control center source
├── docs/                    # Architectural design documents & FYP reports
├── efiboot/                 # EFI boot image staging
├── grub/                    # GRUB bootloader configuration & unicode fonts
├── sandbox/                 # Titan Sandbox source code & C++ headers
├── subsystems/              # Group project subsystems (standardized layout)
│   ├── README.md            # Subsystem index & contribution rules
│   ├── auto-gpu-switcher/   # Auto GPU Switcher (Lead / GitGuru29)
│   ├── titan-ai/            # TITAN AI (Team Subsystem)
│   ├── titan-fetch/         # TitanFetch (Lead / GitGuru29)
│   ├── titan-hwm/           # Titan Hardware Manager (Lead / GitGuru29)
│   ├── titan-media-hud/     # Titan Media HUD Dynamic Island (Lead / GitGuru29)
│   ├── titan-mirror/        # TITAN Mirror (Team Subsystem)
│   ├── titan-sandbox/       # Titan Sandbox (Lead / GitGuru29)
│   ├── titan-settings/      # Titan Settings (Lead / GitGuru29)
│   ├── titan-share/         # TITAN Share (Team Subsystem)
│   └── titan-task-manager/  # TITAN Task Manager (Team Subsystem)
├── titan-browser-source/    # First-party TitanBrowser Qt6 WebEngine source
├── titan-hwm-source/        # Titan Hardware Manager source staging
├── titanfetch-src/          # TitanFetch C++/Qt6 source code
├── wiki/                    # Project Wiki documentation (Markdown)
├── install.sh               # Local host installation script for THM daemon
├── packages.x86_64          # List of official Arch packages for pacstrap
├── pacman.conf              # Pacman repository configuration for ISO build
├── profiledef.sh            # archiso profile metadata, compression (zstd), & permissions
└── run-vm.sh                # QEMU / KVM helper script with ISO auto-detection
```

---

## Team Ownership & Subsystem Division

The project is structured into dedicated subsystem directories under `subsystems/`. Each subsystem folder has an identical standard layout:

```
subsystems/<subsystem-name>/
├── src/        ← Source code
├── tests/      ← Unit & integration tests
├── docs/       ← Subsystem documentation
├── configs/    ← Default configs for OS integration
└── README.md   ← Subsystem overview & build instructions
```

### Assignment Matrix

| Subsystem / Application | Assigned Owner | Role | Location |
| :--- | :--- | :--- | :--- |
| **Titan Hardware Manager** | @GitGuru29 (Lead) | Lead Subsystem | `subsystems/titan-hwm/` & `titan-hwm-source/` |
| **TitanFetch** | @GitGuru29 (Lead) | Lead Subsystem | `subsystems/titan-fetch/` & `titanfetch-src/` |
| **Titan Sandbox** | @GitGuru29 (Lead) | Lead Subsystem | `subsystems/titan-sandbox/` & `sandbox/` |
| **Titan Media HUD** | @GitGuru29 (Lead) | Lead Subsystem | `subsystems/titan-media-hud/` |
| **TitanBrowser** | @GitGuru29 (Lead) | Core Application | `titan-browser-source/` |
| **ArchTitan Settings** | @GitGuru29 (Lead) | Core Application | `archtitan-settings/` & `subsystems/titan-settings/` |
| **Auto GPU Switcher** | @GitGuru29 (Lead) | Lead Subsystem | `subsystems/auto-gpu-switcher/` |
| **TITAN AI** | Teammate | Group Subsystem | `subsystems/titan-ai/` |
| **TITAN Task Manager** | Teammate | Group Subsystem | `subsystems/titan-task-manager/` |
| **TITAN Share** | Teammate | Group Subsystem | `subsystems/titan-share/` |
| **TITAN Mirror** | Teammate | Group Subsystem | `subsystems/titan-mirror/` |

---

## Development Environment Setup

### Required Packages (Arch Host)

```bash
sudo pacman -S --needed \
    archiso git base-devel cmake qt6-base qt6-webengine \
    qemu-desktop edk2-ovmf virt-viewer
```

---

## Local Component Iteration Workflows

Building a full ISO image with `mkarchiso` takes several minutes. To test and iterate on custom daemons and applications quickly, develop them locally on your Arch host system:

### 1. Titan Hardware Manager (`titan-hwm`)

To compile and install `titan-hwm` onto your current host machine:

```bash
# From repository root
./install.sh
```

To test manually:

```bash
systemctl status titan_hw_manager
titan-hwm switch system
titan-hwm metrics
```

---

### 2. TitanBrowser (`titanbrowser`)

To build TitanBrowser locally:

```bash
cd titan-browser-source
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/titanbrowser
```

---

### 3. ArchTitan Settings (`archtitan-settings`)

To build ArchTitan Settings locally:

```bash
cd archtitan-settings
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/archtitan-settings
```

---

### 4. Titan Media HUD (`titan-media-hud`)

To build Titan Media HUD locally:

```bash
cd subsystems/titan-media-hud
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/titan-media-hud
```

---

### 5. Live ISO Testing via QEMU

Once ISO changes are ready, generate the ISO and test inside a local QEMU virtual machine:

```bash
# Clean previous build artifacts
sudo rm -rf tmp-work out

# Build the ISO
sudo mkarchiso -v -w tmp-work/ -o out/ ./

# Launch in QEMU (auto-detects ISO in out/)
./run-vm.sh
```

---

## Coding Standards & Conventions

- **C++**: C++17 or C++20 standard. Use standard library algorithms and `std::filesystem`. Minimize heavy external third-party dependencies to keep ISO sizes small.
- **IPC & Sockets**: All daemons must handle socket write errors gracefully and clean up socket files on `SIGINT`/`SIGTERM`.
- **Systemd**: System services must use `Type=simple` or `Type=exec` and redirect stdout/stderr to `journald`.
- **Shell Scripts**: Bash scripts must pass `shellcheck` with zero warnings, use explicit error checking, and set `set -u` or proper error handling.

---

## Submitting Pull Requests & Governance

1. **Branching**: Create a feature branch off `main`: `git checkout -b feature/subsystem-name`.
2. **Scoping**: Work **exclusively** inside your assigned `subsystems/<name>/` directory.
3. **OS Core Protection**: Core ISO build files (`airootfs/`, `efiboot/`, `grub/`, `profiledef.sh`, `packages.x86_64`, `install.sh`) are protected via `.github/CODEOWNERS`. Changes to core OS files require approval from @GitGuru29.
4. **PR Checklist**: Complete the pull request checklist in `.github/pull_request_template.md` confirming that no core OS files were modified without authorization.
5. **Validation**: Run `run-vm.sh` or local unit tests to ensure your subsystem builds cleanly before submitting your PR.
