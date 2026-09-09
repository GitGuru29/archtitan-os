# 🚀 ArchTitan OS

[![Arch Linux](https://img.shields.io/badge/Base-Arch%20Linux-1793D1?logo=arch-linux&logoColor=white)](https://archlinux.org/)
[![Compositor](https://img.shields.io/badge/Compositor-Hyprland%20(Wayland)-00C8FF?logo=wayland&logoColor=white)](https://hyprland.org/)
[![UI Framework](https://img.shields.io/badge/GUI-Qt6%20%2F%20QML-41CD52?logo=qt&logoColor=white)](https://www.qt.io/)
[![Theme](https://img.shields.io/badge/Theme-Catppuccin%20Mocha-cba6f7)](https://github.com/catppuccin/catppuccin)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

ArchTitan is a custom, high-performance Linux distribution built on top of Arch Linux. Designed around the modern Wayland ecosystem, it offers a deeply integrated, aesthetically pleasing, and resource-efficient environment tailored for power users, developers, and enthusiasts.

---

## 🎯 Core Philosophy

ArchTitan embraces the Unix philosophy while providing a cohesive, pre-configured premium experience out of the box.

- **Wayland Native:** Powered by the **Hyprland** dynamic tiling window manager with fluid animations and responsive window rules.
- **Resource Aware:** Features custom daemons like the **Titan Hardware Manager (THM)** for deterministic system-level resource orchestration.
- **Aesthetic First:** Deeply integrated **Catppuccin Mocha** dark theme across all UI elements, terminals, browsers, overlays, and custom tools.
- **Privacy & Security Focused:** Process sandboxing via Linux namespaces & seccomp-bpf, plus native ad & tracker blocking in Titan Browser.
- **System Safety:** Automated BTRFS pre-transaction snapshots before `pacman` updates with bootable subvolume rollback entries in GRUB.
- **Developer Ready:** Ships with modern CLI utilities, an optimized `fish` + `starship` shell experience, PipeWire audio, and Qt6 development toolchains.

---

## 🏗️ System Architecture

ArchTitan bridges the gap between the modern Wayland compositor, custom middleware, and low-level Linux kernel subsystems:

```mermaid
graph TD
    subgraph "User Space (Wayland & UI)"
        H[Hyprland Compositor]
        W[Waybar Status Panel]
        MH[Titan Media HUD<br>Dynamic Island Overlay]
        R[Rofi App Launcher]
        K[Kitty Terminal]
    end

    subgraph "Core ArchTitan Applications & Tooling"
        TB[Titan Browser<br>Qt6 WebEngine + Adblock]
        TS_APP[ArchTitan Settings<br>Unified Qt6 Control Center]
        TF[TitanFetch<br>Glassmorphic Technical HUD]
        TSD[Titan Sandbox<br>Namespace Isolation]
        THM[Titan Hardware Manager<br>Privileged Daemon]
        BTRFS[BTRFS Snapshot Hook<br>Pacman Pre-Update Safety]
    end

    subgraph "Kernel & System Subsystems"
        CG[cgroups v2 & Scheduler]
        SEC[seccomp-bpf & Namespaces]
        PM[Power Management / ACPI]
        PW[PipeWire Audio / WirePlumber]
        NM[NetworkManager & Wi-Fi]
        DB[D-Bus IPC Bus]
    end

    %% User Space Connections
    H --- W
    H --- MH
    H --- R
    H --- K
    
    %% Tooling Interactions
    TB -.->|MPRIS Metadata| MH
    MH -.->|D-Bus Control| PW
    TS_APP -.->|Configures| NM
    TS_APP -.->|Manages Profiles| PM
    TSD ==>|Enforces Policies| SEC
    TF -.->|Reads Telemetry| CG
    THM ==>|Enforces Resource Limits| CG
    THM -.->|Coordinates| DB
    THM -.->|Controls Governor| PM
    BTRFS -.->|Creates Snapshots| CG
```

---

## 🧩 Core Components & Ecosystem

ArchTitan provides a suite of native C++ and Qt6 applications built specifically for the OS:

### 1. ⚙️ Titan Hardware Manager (`titan-hwm`)
* **Lead:** `@GitGuru29` | **Location:** `titan-hwm-source/` & `subsystems/titan-hwm/`
* Privileged systemd daemon orchestrating CPU, RAM, and background workloads using `cgroups v2`.
* Real-time thermal and power-state throttling with Wayland session awareness to guarantee frame rates for active desktop workflows.

### 2. 🎛️ ArchTitan Settings (`archtitan-settings`)
* **Lead:** `@GitGuru29` | **Location:** `archtitan-settings/`
* Unified C++/Qt system control center featuring:
  - **Appearance:** Theme toggle, Catppuccin Mocha presets, panel opacity, icon sets.
  - **Display:** Brightness, Night Light (`wlsunset`), resolution, and scaling.
  - **Audio:** Output/input volume, live PipeWire audio profile controls.
  - **Power & THM:** Universal stateful power profile cycling (**Casual** ➔ **Web Dev** ➔ **Android Dev** ➔ **System Dev** via `Super + P` / `Fn + P`).
  - **Network & Security:** NetworkManager Wi-Fi scanner, Titan Sandbox status, screen autolock, and firewall inspection.

### 3. 🌐 Titan Browser (`titanbrowser`)
* **Lead:** `@GitGuru29` | **Location:** `titan-browser-source/`
* Fast, lightweight first-party web browser built on **Qt6 WebEngine** with native Wayland rendering:
  - Glassmorphic UI matching the system-wide Catppuccin Mocha aesthetic.
  - Multi-tab management, smart URL/search bar, default system browser bound to `<kbd>Super</kbd> + <kbd>W</kbd>`.
  - Built-in network-level & DOM ad-blocker optimized for streaming without player stalls.
  - Sandboxed launch via `titan-exec-hook` under `/etc/titan-sandbox/policies/browser.toml`.

### 4. 🛡️ Titan Sandbox (`titan-sandboxd`)
* **Lead:** `@GitGuru29` | **Location:** `sandbox/` & `subsystems/titan-sandbox/`
* Fine-grained application isolation engine using Linux user/mount/PID/network namespaces, seccomp-bpf syscall filtering, and capability drops configured via declarative TOML policy files.

### 5. 🏝️ Titan Media HUD (`titan-media-hud`)
* **Lead:** `@GitGuru29` | **Location:** `subsystems/titan-media-hud/`
* Modern "Dynamic Island" style QML overlay and Waybar center module integration:
  - Interactive center capsule in Waybar with click-to-expand trigger.
  - Features MPRIS media playback controls, GPU telemetry (`titan-hud-gpu`), system context (`titan-hud-context`), and custom power menu (`titan-powermenu`).

### 6. 📊 TitanFetch (`titanfetch`)
* **Lead:** `@GitGuru29` | **Location:** `titanfetch-src/` & `subsystems/titan-fetch/`
* High-performance C++/Qt6 system information utility:
  - Terminal CLI mode (`titanfetch`) with sub-millisecond execution.
  - Interactive **Glassmorphic Technical HUD** (`titanfetch --gui`) featuring real-time per-core CPU load, live RAM/Swap gauges, active THM profile indicator, and quick system actions.

### 7. 🔄 BTRFS Automatic Snapshot Hook
* **Lead:** `@GitGuru29` | **Location:** `airootfs/etc/pacman.d/hooks/` & `airootfs/usr/local/bin/archtitan-btrfs-snapshot`
* Automated pre-transaction safety hook creating timestamped read-only BTRFS root subvolume snapshots (`/.snapshots/@pre-pacman-YYYYMMDD-HHMMSS`) before `pacman` updates, with automatic 10-snapshot retention rotation and bootable rollback menu entries in GRUB via `grub-btrfs`.

### 8. 🚧 Additional Subsystems (In Development)
* **Auto GPU Switcher** (`subsystems/auto-gpu-switcher/`): Intelligent iGPU/dGPU dynamic switching.
* **TITAN AI** (`subsystems/titan-ai/`): Context-aware developer assistant.
* **TITAN Task Manager** (`subsystems/titan-task-manager/`): Advanced scheduling and resource tracking.
* **TITAN Share** (`subsystems/titan-share/`): Peer-to-peer local network discovery and file exchange.
* **TITAN Mirror** (`subsystems/titan-mirror/`): Wayland-native screen mirroring for mobile devices.

---

## ⌨️ Desktop Environment & Keybindings

ArchTitan uses an optimized Hyprland desktop with smooth bezier curves, subtle drop shadows, and responsive window grouping.

```mermaid
mindmap
  root((ArchTitan Desktop))
    Compositor
      Hyprland (v0.53+ Block Syntax)
      Aquamarine VM Backend Support
      Overshot & Snappy Animations
    HUD & Bars
      Dark Pill Waybar Status Panel
      Titan Media HUD Dynamic Island
      Dunst Notification Center
    Launchers & Utilities
      Rofi App Launcher
      Cliphist (Clipboard Manager)
      Calamares (launch-installer)
    Core Apps
      Kitty (Fish + Starship)
      Titan Browser (WebEngine)
      ArchTitan Settings (Qt6)
      TitanFetch Glassmorphic HUD
```

### 🔑 Essential Keybindings

| Shortcut | Action | Description |
| :--- | :--- | :--- |
| <kbd>Super</kbd> / <kbd>Super</kbd> + <kbd>Space</kbd> | App Launcher | Launches Rofi application menu |
| <kbd>Super</kbd> + <kbd>Return</kbd> | Terminal | Opens Kitty terminal with Fish shell |
| <kbd>Super</kbd> + <kbd>W</kbd> | Titan Browser | Launches first-party TitanBrowser |
| <kbd>Super</kbd> + <kbd>,</kbd> | ArchTitan Settings | Opens unified System Settings panel |
| <kbd>Super</kbd> + <kbd>E</kbd> | File Manager | Opens Ranger terminal file manager |
| <kbd>Super</kbd> + <kbd>P</kbd> / <kbd>Fn</kbd> + <kbd>P</kbd> | Power Profile Toggle | Cycles THM workload profiles |
| <kbd>Super</kbd> + <kbd>V</kbd> | Clipboard History | Opens clipboard history picker |
| <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>S</kbd> | Screenshot | Area snip copied to clipboard (`grim` + `slurp`) |
| <kbd>Super</kbd> + <kbd>Q</kbd> | Close Window | Closes active window |
| <kbd>Super</kbd> + <kbd>F</kbd> | Fullscreen | Toggles fullscreen mode |
| <kbd>Super</kbd> + <kbd>I</kbd> | System Installer | Launches Calamares via `launch-installer` helper |
| <kbd>Super</kbd> + <kbd>Escape</kbd> | Power Menu | Opens Titan Power Menu / Rofi Power Menu |

---

## 🛠️ Building the Live ISO

ArchTitan is built using the official Arch Linux `archiso` suite.

### Prerequisites

An Arch Linux host system with `archiso`, `base-devel`, `cmake`, `qt6-base`, and `qt6-webengine` installed:
```bash
sudo pacman -S archiso git base-devel cmake qt6-base qt6-webengine
```

### Build Workflow

```mermaid
sequenceDiagram
    participant U as Developer / CI
    participant M as mkarchiso
    participant P as Pacstrap
    participant C as Build Hooks & Scripts

    U->>M: sudo mkarchiso -v -w tmp-work/ -o out/ ./
    M->>P: Pull packages defined in packages.x86_64
    P-->>M: Base rootfs generated
    M->>C: Execute profiledef.sh customizations
    C->>C: Compile Titan tools (Browser, Settings, THM, Fetch, Media HUD, Sandbox)
    C->>C: Install Hyprland configs, Waybar, fonts & themes
    C-->>M: airootfs assembled
    M->>M: SquashFS compression (zstd, 2 CPU limit) & EFI image packing
    M-->>U: Output: out/archtitan-YYYY.MM.DD-x86_64.iso
```

### Build Commands

```bash
# Clone the repository
git clone https://github.com/GitGuru29/archtitan-os.git
cd archtitan-os

# Clean previous build artifacts
sudo rm -rf tmp-work out

# Build the ISO
sudo mkarchiso -v -w tmp-work/ -o out/ ./
```

The generated `.iso` will be placed in the `out/` directory.

---

## 💻 Installation & Testing

### 1. Bare Metal Installation
1. Flash the generated ISO to a USB flash drive using `dd`, BalenaEtcher, or Rufus:
   ```bash
   sudo dd if=out/archtitan-*.iso of=/dev/sdX bs=4M status=progress oflag=sync
   ```
2. Boot the target machine in **UEFI mode** (disable Secure Boot).
3. On the live desktop, launch the **Calamares Graphical Installer** (<kbd>Super</kbd> + <kbd>I</kbd>).
4. Follow the step-by-step partition and user setup wizards to complete the installation.

### 2. QEMU / KVM Quick Runner
Use the bundled `run-vm.sh` script (with ISO auto-detection in `out/` or `Downloads/`) to test live or installed builds in QEMU:

```bash
# Boot the live ISO
./run-vm.sh

# Boot from installed qcow2 virtual disk
./run-vm.sh --disk

# Wipe disk and perform fresh reinstall
./run-vm.sh --fresh
```

### 3. VirtualBox Configuration
When testing inside VirtualBox, ensure the following VM settings are enabled to support Wayland:
- **System ➔ Motherboard:** Check `Enable EFI (special OSes only)`
- **Display ➔ Screen:** Video Memory = `128 MB`, Graphics Controller = `VMSVGA`
- **Display ➔ Screen:** Check `Enable 3D Acceleration`

---

## 👥 Team Structure & Subsystem Ownership

ArchTitan is developed as a modular group project with 4 members. The repository is organized into core OS components at the root and dedicated subsystem packages in `subsystems/`.

### Core OS Components vs. Subsystems

| Component | Category | Owner | Status | Directory |
| :--- | :--- | :--- | :--- | :--- |
| **Titan Hardware Manager** | Core Daemon | @GitGuru29 (Lead) | 🟢 Shipped | `titan-hwm-source/` & `subsystems/titan-hwm/` |
| **ArchTitan Settings** | Core App | @GitGuru29 (Lead) | 🟢 Shipped | `archtitan-settings/` |
| **Titan Browser** | Core App | @GitGuru29 (Lead) | 🟢 Shipped | `titan-browser-source/` |
| **Titan Sandbox** | Core Subsystem | @GitGuru29 (Lead) | 🟢 Shipped | `sandbox/` & `subsystems/titan-sandbox/` |
| **TitanFetch** | Core Utility | @GitGuru29 (Lead) | 🟢 Shipped | `titanfetch-src/` & `subsystems/titan-fetch/` |
| **Titan Media HUD** | Companion App | @GitGuru29 (Lead) | 🟢 Shipped | `subsystems/titan-media-hud/` |
| **BTRFS Snapshot Hook** | Safety Feature | @GitGuru29 (Lead) | 🟢 Shipped | `airootfs/etc/pacman.d/hooks/` |
| **Auto GPU Switcher** | Subsystem | @GitGuru29 (Lead) | 🟡 In Dev | `subsystems/auto-gpu-switcher/` |
| **TITAN AI** | Subsystem | Teammate | 🟡 In Dev | `subsystems/titan-ai/` |
| **TITAN Task Manager** | Subsystem | Teammate | 🟡 In Dev | `subsystems/titan-task-manager/` |
| **TITAN Share** | Subsystem | Teammate | 🟡 In Dev | `subsystems/titan-share/` |
| **TITAN Mirror** | Subsystem | Teammate | 🟡 In Dev | `subsystems/titan-mirror/` |

---

## 🚨 Repository Rules & Contribution Guidelines

> **⚠️ All team members must review and adhere to these guidelines before submitting changes.**

### 🔒 Branch Protection & Pull Requests

- The `main` branch is protected. Direct commits are restricted.
- All contributions must be submitted via a **Pull Request (PR)** targeting `main`.
- PRs require at least **1 review and approval** from `@GitGuru29` before merging.
- Contributors must work exclusively within their assigned `subsystems/<name>/` directories.

### 🛡️ Protected OS Core Files (`CODEOWNERS`)

Changes affecting base OS configurations and core tooling require explicit review by `@GitGuru29`:

| Protected Path | Purpose |
| :--- | :--- |
| `airootfs/` | Rootfs overlay (systemd units, dotfiles, desktop configs, binary overlays) |
| `archtitan-settings/` | ArchTitan Settings application source |
| `titan-browser-source/` | Titan Browser source and resources |
| `titan-hwm-source/` | Titan Hardware Manager daemon source |
| `titanfetch-src/` | TitanFetch utility source |
| `sandbox/` | Titan Sandbox daemon & policy parser source |
| `efiboot/` & `grub/` | EFI & GRUB bootloader configurations |
| `packages.x86_64` | Package manifest for live ISO generation |
| `pacman.conf` | Pacman package manager and custom repository configuration |
| `profiledef.sh` | Archiso build profile definitions and permissions |
| `install.sh` & `run-vm.sh` | Host installer and QEMU virtual test harness |
| `.github/` | CI/CD automation workflows and repository governance |

### 🏷️ Commit Message Format

```
<type>(<scope>): <short description>

Examples:
  feat(titan-browser): add native Qt6 directory picker for download location
  fix(settings): implement universal power profile cycling hotkey
  feat(titan-media-hud): add async album art caching and MPRIS seeker
  docs(readme): update system architecture and core components guide
```

---

## 📚 Documentation & Resources

Comprehensive guides and technical documentation are available on the [ArchTitan OS Wiki](https://github.com/GitGuru29/archtitan-os/wiki):

| Topic | Description | Link |
| :--- | :--- | :--- |
| **System Architecture** | Subsystem layers, D-Bus IPC, and kernel interfaces | [Architecture Guide](https://github.com/GitGuru29/archtitan-os/wiki/Architecture) |
| **Building the ISO** | Archiso compilation steps and dependency handling | [Build Documentation](https://github.com/GitGuru29/archtitan-os/wiki/Building-the-ISO) |
| **Installation Guide** | UEFI partitioning, live environment, and dual-booting | [Installation Manual](https://github.com/GitGuru29/archtitan-os/wiki/Installation-Guide) |
| **Hardware Manager** | cgroups v2 resource orchestration and power policies | [THM Documentation](https://github.com/GitGuru29/archtitan-os/wiki/Titan-Hardware-Manager) |
| **Titan Sandbox** | Namespaces, capability drops, and TOML policies | [Sandbox Manual](https://github.com/GitGuru29/archtitan-os/wiki/Titan-Sandbox) |
| **Titan Browser** | Qt6 WebEngine browser and sandboxing guide | [Titan Browser Guide](https://github.com/GitGuru29/archtitan-os/wiki/Titan-Browser) |
| **ArchTitan Settings** | System control center and THM profile manager | [Settings Manual](https://github.com/GitGuru29/archtitan-os/wiki/ArchTitan-Settings) |
| **Titan Media HUD** | Dynamic Island overlay and power menu | [Media HUD Guide](https://github.com/GitGuru29/archtitan-os/wiki/Titan-Media-HUD) |
| **Desktop Environment** | Hyprland config, Waybar widgets, and themes | [Desktop Configuration](https://github.com/GitGuru29/archtitan-os/wiki/Desktop-Environment) |
| **Developer Guide** | Subsystem development and contributor standards | [Developer Guide](https://github.com/GitGuru29/archtitan-os/wiki/Developer-Guide) |

---

## 📜 License

This project is licensed under the **[Apache License 2.0](LICENSE)**. Arch Linux and upstream package software remain subject to their respective licenses.

<br />
---

<div align="center">

<img src="https://raw.githubusercontent.com/GitGuru29/archtitan-os/main/assets/logo.png" alt="ArchTitan OS Logo" width="120" />

### **ArchTitan OS**
*High-Performance Wayland Linux Distribution*

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://raw.githubusercontent.com/GitGuru29/archtitan-os/main/LICENSE)
[![GitHub](https://img.shields.io/badge/GitHub-archtitan--os-181717?logo=github)](https://github.com/GitGuru29/archtitan-os)

</div>
