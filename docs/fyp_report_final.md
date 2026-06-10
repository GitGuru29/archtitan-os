# TitanArch — Developer-Focused Custom Linux Distribution
**Final Year Project Report**
*Architecture, Design, and Implementation*

---

## 1. Introduction
Modern software development demands computing environments that are performant, minimal, and purpose-built for development workflows. General-purpose operating systems such as Windows 11, macOS, and even mainstream Linux distributions carry significant overhead from background services, telemetry, and features irrelevant to developers — all of which translate into wasted CPU cycles, memory pressure, and context-switching friction.

TitanArch is a custom Arch Linux-based distribution built from the ground up as a developer-first operating system. Unlike distros that patch or configure an existing environment, TitanArch is constructed using `archiso` with deliberate component selection at every layer — from the base system through the compositor, system services, and companion tooling. The result is an OS that is lightweight by design, intelligent in resource allocation, and equipped with developer-centric utilities that do not exist in any other distribution out of the box.

The project encompasses a full custom ISO that boots successfully on real hardware and virtual machines, a TUI installer written in Bash with full ANSI rendering, a context-aware performance analyzer and resource allocator, an AI-driven project analyzer, an automated hybrid GPU switching mechanism, the Neon Monitor hardware dashboard, and TitanShare — a cross-platform peer-to-peer file transfer system comprising a C++ Linux daemon and a native Android application.

## 2. Problem Statement
Developers working on Linux face a recurring set of friction points that are largely unsolved by existing distributions:

* **High Idle Resource Consumption:** Desktop environments like GNOME and KDE consume 800MB–1.5GB RAM at idle before a single development tool is launched.
* **Static Resource Management:** No built-in workflow-awareness. The OS treats a compile job the same as watching a video. CPU governor, I/O scheduler, and memory pressure parameters are static regardless of the developer's activity.
* **Manual GPU Switching:** Hybrid GPU setups (Intel iGPU + NVIDIA/AMD dGPU) require manual configuration, third-party tools, or proprietary utilities. There is no seamless user-space switching.
* **Fragmented Connectivity:** Cross-device file transfer between Linux and Android remains fragmented (MTP is unreliable, KDE Connect is heavy, cloud-sync has latency/privacy concerns).
* **Setup Barrier:** Arch Linux's renowned minimalism comes at the cost of a steep setup barrier. There is no streamlined path from ISO to productive developer environment.

TitanArch is designed to close these gaps with purpose-built solutions at the OS level rather than patching them with external tools.

## 3. Objectives
* Design and build a bootable, installable Arch-based Linux distribution using `archiso`.
* Implement a full TUI installer in Bash with ANSI rendering that guides users from live ISO to configured system.
* Develop a context-based performance analyzer that monitors active workloads and adjusts CPU governor, I/O scheduler, and system parameters dynamically.
* Build a resource allocator that enforces performance profiles (development, performance, battery).
* Implement an AI-driven project analyzer that introspects development project structures and provides contextual tooling recommendations.
* Create an automated hybrid GPU switcher that manages iGPU/dGPU routing at the userspace level.
* Deliver Neon Monitor — a real-time hardware monitoring dashboard surfacing CPU, GPU, memory, thermals, and I/O metrics.
* Build TitanShare — a local-network peer-to-peer file transfer system with a native C++ Linux daemon (mDNS discovery) and a Kotlin/Jetpack Compose Android application.
* Establish a BTRFS-based system snapshot and rollback mechanism for safe system updates.

## 4. Analysis of Existing Systems

### 4.1 Windows 11
* Strong hardware compatibility and driver ecosystem.
* Background services (Windows Update, Defender, telemetry) consume significant resources.
* Linux development requires WSL2, adding virtualization and filesystem translation overhead.
* No native package management comparable to `pacman`.

### 4.2 macOS
* Unix-based, stable, and well-suited for development with Homebrew.
* Tightly controlled hardware ecosystem (no custom hybrid GPU management).
* No user-level access to kernel parameters, CPU governors, or I/O schedulers.

### 4.3 Ubuntu / GNOME-based Linux
* Accessible and well-documented.
* GNOME introduces significant idle RAM usage (800MB+) and background service overhead.
* Snap packages introduce startup latency and sandboxing conflicts.
* No built-in workload adaptation.

### 4.4 Arch Linux (Vanilla)
* Minimal base with full user control (closest to TitanArch's foundation).
* No installer, no default desktop, no developer tooling out of the box.
* No built-in GPU management, performance profiling, or cross-device communication.

### 4.5 Comparative Summary
| Feature | Windows 11 | macOS | Ubuntu | Arch Linux | **TitanArch** |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Idle RAM usage** | High | Medium | High | Low | **Very Low** |
| **Workload-aware CPU management** | No | No | No | No | **Yes** |
| **Hybrid GPU auto-switching** | Partial | N/A | No | Manual | **Yes (automated)** |
| **Built-in developer TUI installer** | N/A | N/A | GUI only | None | **Yes** |
| **Local P2P file transfer (Linux+Android)** | No | No | No | No | **Yes (TitanShare)** |
| **AI-driven project analyzer** | No | No | No | No | **Yes** |
| **System snapshot & rollback** | Limited | Time Machine | No | Manual | **Yes (BTRFS)** |
| **Hardware monitoring dashboard** | Third-party | Third-party | Third-party | Third-party | **Yes (Neon Monitor)** |
| **Android screen mirroring (native)** | No | No | No | No | **Yes (TitanMirror)** |

## 5. Research Gap Analysis
1. **No Developer-Optimized Distro:** No distribution combines Arch's minimal base with purpose-built developer workflow tooling and cross-device communication at the OS level.
2. **Static Resource Management:** Linux kernel schedulers operate without application-context awareness. TitanArch's context-based performance analyzer fills this gap.
3. **Unsolved Hybrid GPU Management:** TitanArch's GPU switcher automates PRIME configurations at the user-space level, providing seamless switching.
4. **Lack of Native File Transfer:** TitanArch addresses Linux-to-Android transfers with TitanShare, a purpose-built system using mDNS.
5. **No OS-Level AI Assistance:** TitanArch's AI-driven project analyzer operates at the system level to recommend tooling configuration independently of specific editors.
6. **No Native Wayland Android Mirroring:** TitanMirror integrates natively with a Wayland compositor, reuses mDNS pairing infrastructure, and operates without ADB.

## 6. System Architecture
TitanArch is structured into four horizontal layers:

1. **Base System Layer:** Arch Linux core, pacman, custom packages, BTRFS filesystem (built with archiso, Bash).
2. **Desktop Environment Layer:** Hyprland compositor, Waybar, custom keybindings, theme configuration (Wayland, C).
3. **System Services Layer:** Performance analyzer, resource allocator, GPU switcher, Neon Monitor (C++, Python, systemd).
4. **Application Layer:** TUI installer, TitanShare (Linux daemon + Android app), AI project analyzer (Bash, C++, Kotlin).

## 7. Implemented Features

### 7.1 Custom TUI Installer
Written in Bash using ANSI escape sequences. Provides disk detection (auto/manual), BTRFS default selection, network/user setup, package selection, and bootloader installation (UEFI/BIOS).

### 7.2 Context-Based Performance Analyzer
A system service monitoring running processes to classify active workload contexts:
* **Compile mode:** gcc, clang, rustc, make, cargo.
* **IDE/editor mode:** nvim, code, jetbrains.
* **Gaming mode:** GPU thresholds and known games.
* **Idle mode:** Baseline.

### 7.3 Resource Allocator and Profile Management
Receives workload signals and applies system configurations:
* **Development:** schedutil governor, mq-deadline scheduler, standard memory.
* **Performance:** performance governor, direct I/O, aggressive memory.
* **Gaming:** performance governor, bfq scheduler.
* **Battery:** powersave governor, bfq scheduler, conservative memory.

### 7.4 AI-Driven Project Analyzer
A tool that introspects project directories (detecting languages, lock files, build systems) and outputs structured recommendations for compiler flags, env vars, and IDE settings.

### 7.5 Automated Hybrid GPU Switcher
Manages iGPU/dGPU routing at user-space level via DRM device enumeration and environment variable injection (`DRI_PRIME`, `__NV_PRIME_RENDER_OFFLOAD`). Automates switches based on THM profiles (e.g., Gaming routes to dGPU).

### 7.6 Neon Monitor
A lightweight terminal dashboard implemented in C++ with direct sysfs/procfs reads for CPU, GPU, RAM, Thermals, and Disk I/O, avoiding daemon overhead.

### 7.7 TitanShare — Cross-Platform Local File Transfer
* **Linux Daemon:** C++ systemd user service utilizing mDNS and secure Unix sockets.
* **Android Application:** Native Kotlin/Jetpack Compose app. Operates entirely over the local network (no cloud).

### 7.8 TitanMirror — Android Screen Mirroring
Real-time Wayland-native Android screen mirroring utilizing MediaProjection API, H.264 video encoding, and mDNS pairing (no ADB required).

### 7.9 BTRFS Snapshot and System Recovery
Automated pre-update snapshots via pacman hooks allowing recovery from the bootloader menu.

## 8. Development Methodology
Component-driven incremental development methodology.

| Component | Technology | Rationale |
| :--- | :--- | :--- |
| OS build system | archiso (Bash) | Official Arch ISO construction tooling |
| TUI installer | Bash + ANSI escape sequences | No runtime dependency; ships in base ISO |
| System services | C++17 / C++20 | Performance-critical; direct syscall/sysfs access |
| AI project analyzer | Python | Rich library ecosystem for file/AST analysis |
| GPU switcher | C++ + DRM/KMS APIs | Kernel-level GPU enumeration |
| Neon Monitor | C++ + sysfs/procfs | Zero-overhead hardware reads |
| TitanShare daemon | C++, mDNS, Unix sockets | High-throughput local transfer |
| TitanShare Android app | Kotlin + Jetpack Compose | Native Android UI |
| TitanMirror Android component | Kotlin + MediaProjection + MediaCodec | OS-level screen capture and H.264 encoding |
| TitanMirror Linux client | C++ + Wayland + VA-API/NVDEC | Native Wayland rendering with hardware decode |
| Desktop environment | Hyprland + Wayland | Minimal compositor with scripting support |

## 9. Team Responsibilities

* **Siluna Dangalla:** archiso build pipeline, TUI installer, performance analyzer, resource allocator, GPU switcher, Neon Monitor, TitanShare Linux daemon, BTRFS snapshot integration, AI project analyzer.
* **Kaveesha:** Hyprland desktop environment configuration, installer UX design, Waybar theme, TitanShare Android app UI (Jetpack Compose).
* **Team Member 3:** TitanShare mDNS protocol design, local network transfer protocol, profile switching support.
* **Team Member 4:** VM-based system testing, stability validation, integration testing, benchmark documentation.

## 10. Evaluation Metrics
| Metric | Measurement Method | Target |
| :--- | :--- | :--- |
| Idle RAM usage | `free -m` after boot, no apps open | < 1GB |
| Boot time | `systemd-analyze blame` | < 10 seconds to usable desktop |
| Compile benchmark | Linux kernel `make -j$(nproc)` | >= parity or improvement |
| GPU switching latency | Time from trigger to DRI_PRIME effective | < 2 seconds |
| TitanShare transfer speed | Large file transfer throughput on LAN | >= 50 MB/s on 1Gbps |
| Profile switch latency | Time for governor/scheduler change | < 500ms |
| Neon Monitor CPU overhead | top/perf measurement | < 0.5% CPU at idle |
| System stability | Uptime under simulated workload | No crashes or service failures |

## 11. Conclusion
TitanArch demonstrates that a small, focused team can produce a complete, opinionated, and technically sophisticated Linux distribution targeting a specific domain. The system provides genuine advantages over general-purpose alternatives for software engineers through its context-aware performance management, cross-device ecosystem, and automated tooling integrations.
