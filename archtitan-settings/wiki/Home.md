# ArchTitan Settings — Wiki

Welcome to the **ArchTitan Settings** documentation wiki. This is the native settings application for **ArchTitan OS** — a custom Arch Linux distribution with a Hyprland desktop environment.

**Version:** 1.0.0 · **Platform:** Wayland / Hyprland / Qt 6.11+ · **Status:** Work in progress

---

## Wiki Structure

```
Home
├── Architecture
├── Backend Modules
├── Audio System
├── Power Management
├── UI Design
├── Build & Deployment
├── Known Issues
├── Roadmap
├── Developer Guide
└── Technical Report
```

| Page | Description |
|---|---|
| [Architecture](Architecture.md) | System design, data flow, tech stack, directory layout |
| [Backend Modules](Backend-Modules.md) | All six C++ backend classes — properties, methods, integrations |
| [Audio System](Audio-System.md) | PipeWire EQ, spatial audio, Cava visualizer, volume sync |
| [Power Management](Power-Management.md) | Power profiles, battery protection, keyboard shortcuts |
| [UI Design](UI-Design.md) | Window shell, design tokens, components, settings pages |
| [Build & Deployment](Build-and-Deployment.md) | Requirements, build steps, post-install setup |
| [Known Issues](Known-Issues.md) | Resolved bugs and open pending items |
| [Roadmap](Roadmap.md) | Short, medium, and long-term development plans |
| [Developer Guide](Developer-Guide.md) | Dev workflow, conventions, scripts, reference tables |
| [Technical Report](Technical-Report.md) | Comprehensive compilation and technical overview document |


---

## Overview

ArchTitan Settings replaces fragmented shell scripts and third-party tools with a single Qt6/QML control center tailored to the ArchTitan desktop stack. Six C++ backend singletons expose reactive `Q_PROPERTY` bindings to eight QML settings pages.

### Key Capabilities

| Page | Highlights |
|---|---|
| **Appearance** | Theme, accent color, glassmorphism, icon theme, font |
| **Display** | Brightness, Night Light, scale factor, resolution info |
| **Network** | Wi-Fi scan/connect, connection details |
| **Audio** | Native PipeWire EQ, spatial audio, Cava visualizer |
| **Power** | Profile cycling, battery protection, live battery meter |
| **Security** | Screen autolock, firewall status, quick lock |
| **System** | Live CPU/RAM/disk meters, hardware info |
| **About** | OS version, kernel, tech stack, project links |

### Design Philosophy

- **Native over web views** — Qt6/QML with Wayland transparency
- **Direct system integration** — `wpctl`, `nmcli`, `hyprctl`, D-Bus, sysfs
- **Reactive UI** — backend state as Qt properties, QML binds directly
- **Hardware adaptability** — battery UI adapts to ThinkPad, ASUS, IdeaPad, etc.
- **Minimal dependencies** — native PipeWire filter-chain replaces EasyEffects

---

## Quick Links

- **Repository:** [archtitan-settings](https://github.com/GitGuru29/archtitan-settings)
- **Parent OS:** [archtitan-os](https://github.com/GitGuru29/archtitan-os)
- **GitHub Wiki:** [archtitan-os/wiki](https://github.com/GitGuru29/archtitan-os/wiki)
- **README:** [Quick-start guide](../README.md)

---

## Project Status

> ⚠ **Work in progress** — not yet integrated into the main ISO build. This folder is excluded from the `custom-os-build` repo via `.gitignore`.

| Item | Value |
|---|---|
| Active branch | `feature/audio-eq-and-about-page` |
| Commits ahead of `main` | ~20 |
| Last wiki update | July 2026 |

---

*ArchTitan Settings — Final Year Project · © 2026 ArchTitan Project*
