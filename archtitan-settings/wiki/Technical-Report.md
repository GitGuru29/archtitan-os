# ArchTitan Settings — Full Technical Report

**Document version:** 1.0  
**Last updated:** July 2026  
**Project type:** Final Year Project — ArchTitan OS  
**Application version:** 1.0.0

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Project Context](#2-project-context)
3. [Architecture Overview](#3-architecture-overview)
4. [Technology Stack](#4-technology-stack)
5. [Backend Modules](#5-backend-modules)
6. [User Interface](#6-user-interface)
7. [Audio Subsystem](#7-audio-subsystem)
8. [Power & Battery Management](#8-power--battery-management)
9. [System Integration](#9-system-integration)
10. [Build & Deployment](#10-build--deployment)
11. [Scripts & Utilities](#11-scripts--utilities)
12. [Known Issues & Resolutions](#12-known-issues--resolutions)
13. [Pending Work & Roadmap](#13-pending-work--roadmap)
14. [Dependencies](#14-dependencies)
15. [Appendix](#15-appendix)

---

## 1. Executive Summary

**ArchTitan Settings** is a native desktop settings application built for **ArchTitan OS**, a custom Arch Linux distribution running the **Hyprland** compositor on **Wayland**. It replaces fragmented shell scripts, third-party tools, and desktop-environment-specific panels with a single, cohesive control center tailored to the ArchTitan desktop stack.

The application is implemented in **C++17** with a **Qt6 / QML** frontend. Six C++ backend singletons expose reactive `Q_PROPERTY` bindings to QML pages, communicating with the host system through subprocess calls (`wpctl`, `nmcli`, `hyprctl`), D-Bus (`power-profiles-daemon`), and direct sysfs reads (`/proc`, `/sys/class/power_supply`).

### Key Capabilities

| Area | Highlights |
|---|---|
| **Appearance** | Theme, accent color, glassmorphism, icon theme, font — persisted via QSettings and applied through `gsettings` / `hyprctl` |
| **Display** | Brightness (`brightnessctl`), Night Light (`wlsunset`), scale factor (`hyprctl`), resolution info |
| **Network** | Wi-Fi scan/connect via `nmcli`, connection details, signal strength |
| **Audio** | Native PipeWire EQ (6 presets + custom), spatial stereo widening (Haas effect), real-time Cava visualizer, volume/mute sync |
| **Power** | Three-tier profile cycling (`Super+P` / `Fn+P`), screen/suspend timeouts, live battery meter, hardware-aware charge protection |
| **Security** | Screen autolock via `swaylock`/`swayidle`, firewall status, Titan Sandbox status |
| **System** | Live CPU/RAM/disk meters from `/proc`, hardware info, uptime |
| **About** | OS version, kernel, tech stack, project links |

---

## 2. Project Context

### 2.1 Purpose

ArchTitan OS is a minimal, performance-oriented Linux desktop built around Hyprland. Unlike GNOME or KDE, it has no built-in settings panel. ArchTitan Settings fills that gap with a purpose-built application that understands the exact tools and conventions of the ArchTitan stack (PipeWire, swww, swayidle, power-profiles-daemon, etc.).

### 2.2 Design Philosophy

- **Native over embedded web views** — Qt6/QML for GPU-accelerated UI with Wayland transparency support.
- **Direct system integration** — no abstraction layers where a shell command or D-Bus call suffices.
- **Reactive UI** — all backend state is exposed as Qt properties; QML binds directly without manual polling in the view layer.
- **Hardware adaptability** — battery charge protection UI adapts to ThinkPad, ASUS ROG, Lenovo IdeaPad, and other vendor-specific sysfs interfaces.
- **Minimal external dependencies** — the audio equalizer was migrated from EasyEffects to native PipeWire filter-chain modules, eliminating a heavy runtime dependency.

### 2.3 Repository Status

| Item | Status |
|---|---|
| ISO integration | Not yet included in `custom-os-build` |
| Git exclusion | Listed in parent repo `.gitignore` |
| Active development branch | `feature/audio-eq-and-about-page` |
| Commits ahead of `main` | ~20 (audio EQ, spatial audio, power refinements, About page) |

---

## 3. Architecture Overview

### 3.1 High-Level Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        QML Frontend                              │
│  Main.qml ── Sidebar ── 8 Settings Pages ── Reusable Components │
└──────────────────────────┬──────────────────────────────────────┘
                           │ Q_PROPERTY bindings (context properties)
┌──────────────────────────▼──────────────────────────────────────┐
│                     C++ Backend Layer                            │
│  SettingsBackend │ SystemInfo │ NetworkManager │ DisplayManager │
│  AudioBackend    │ WallpaperManager                              │
└──────┬───────────┬────────────┬───────────────┬─────────────────┘
       │           │            │               │
       ▼           ▼            ▼               ▼
  QSettings    /proc,       nmcli          brightnessctl
  gsettings    /sys         NetworkMgr     wlsunset, hyprctl
  hyprctl      power_supply  D-Bus          swww
  D-Bus        D-Bus        wpctl/pactl    PipeWire filter-chain
               swayidle     cava           playerctl (MPRIS)
```

### 3.2 Application Entry Point

`src/main.cpp` performs the following initialization sequence:

1. **Wayland alpha buffer** — sets `QSurfaceFormat` alpha buffer size to 8 for transparent window compositing.
2. **Font loading** — attempts to load Inter font weights from common Arch Linux font paths.
3. **Backend instantiation** — creates six singleton backend objects.
4. **QML context registration** — exposes backends via `engine.rootContext()->setContextProperty()`.
5. **QML loading** — supports dev-mode override via `QML_SOURCE_DIR` environment variable; otherwise loads embedded `qrc:/` resources.

### 3.3 Data Flow Pattern

All backends follow a consistent pattern:

```
Hardware / OS ──► C++ read/write ──► Q_PROPERTY ──► QML binding ──► UI update
                      ▲                                    │
                      └──────── user interaction ──────────┘
```

Settings that must survive reboots are persisted in **QSettings** (`~/.config/ArchTitan/archtitan-settings.conf`) and applied to the system on save via `SettingsBackend::applyAndSave()`.

### 3.4 Directory Structure

```
archtitan-settings/
├── CMakeLists.txt              # Qt6 build configuration
├── build.sh / rebuild.sh       # Build scripts
├── src/
│   ├── main.cpp                # Entry point
│   ├── settingsbackend.*       # Persistent settings + system apply
│   ├── systeminfo.*            # /proc + battery sysfs reader
│   ├── networkmanager.*        # nmcli Wi-Fi wrapper
│   ├── displaymanager.*        # Brightness, night light, scale
│   ├── audiobackend.*          # PipeWire audio + EQ + spatial
│   └── wallpapermanager.*      # swww wallpaper switcher
├── qml/
│   ├── Main.qml                # Root window (frameless, dark)
│   ├── components/             # Reusable UI primitives
│   └── pages/                  # 8 settings pages
├── assets/icons/               # SVG icons + branding PNGs
├── scripts/
│   ├── cycle_power.sh          # Power profile keyboard shortcut
│   └── install-battery-rules.sh# udev rules for battery sysfs
└── wiki/                       # This documentation
```

---

## 4. Technology Stack

| Layer | Technology | Version / Notes |
|---|---|---|
| Language | C++ | C++17 |
| UI Framework | Qt6 (Core, Quick, QuickControls2) | 6.4+ (tested on 6.11+) |
| UI Markup | QML / Qt Quick Controls 2 | Basic style |
| Build System | CMake + Ninja/Make | CMake 3.22+ |
| Compositor | Hyprland | Wayland |
| Audio Server | PipeWire + WirePlumber | wpctl, pactl, pw-cli |
| Audio Visualizer | Cava | Raw ASCII output, 24 bars @ 60 FPS |
| Network | NetworkManager | nmcli CLI |
| Power Profiles | power-profiles-daemon | D-Bus (`net.hadess.PowerProfiles`) |
| Display | brightnessctl, wlsunset | Backlight + color temperature |
| Wallpaper | swww | Image daemon |
| Idle/Lock | swayidle + swaylock | Screen off, suspend, autolock |
| Settings Store | QSettings + gsettings | App-local + GTK desktop settings |
| Font | Inter | Regular, Medium, SemiBold, Bold |

---

## 5. Backend Modules

### 5.1 SettingsBackend

**File:** `src/settingsbackend.cpp` / `src/settingsbackend.h`  
**QML binding:** `SettingsBackend`

Central persistence and system-apply layer for appearance, power, and security settings.

#### Properties

| Property | Type | Default | System Apply Target |
|---|---|---|---|
| `colorTheme` | QString | `"Dark"` | QSettings only |
| `accentColor` | QString | `"#7AA2F7"` | `hyprctl` border colors |
| `glassmorphism` | bool | `true` | QSettings only |
| `panelOpacity` | double | `0.88` | QSettings only |
| `iconTheme` | QString | `"Papirus-Dark"` | `gsettings` icon-theme |
| `fontFamily` | QString | `"Inter"` | `gsettings` font-name |
| `fontSize` | int | `13` | `gsettings` font-name |
| `screenTimeout` | int | `300` | `swayidle` config |
| `suspendTimeout` | int | `600` | `swayidle` config |
| `powerProfile` | QString | `"Balanced"` | D-Bus power-profiles-daemon |
| `autolockEnabled` | bool | `true` | `swayidle` lifecycle |
| `autolockDelay` | int | `300` | QSettings (swayidle integration) |

#### Key Methods

- **`applyAndSave()`** — persists all settings to QSettings, then applies each to the live system (Hyprland, gsettings, swayidle, power profile).
- **`applyPowerProfileNow(profile)`** — immediate D-Bus profile switch without full save cycle.
- **`resetToDefaults()`** — restores factory defaults.

#### D-Bus Integration

Listens for external power profile changes on both:
- `net.hadess.PowerProfiles` (power-profiles-daemon ≥ 0.10)
- `org.freedesktop.UPower.PowerProfiles` (legacy fallback)

This keeps the UI in sync when the user cycles profiles via `Super+P` or `scripts/cycle_power.sh`.

---

### 5.2 SystemInfo

**File:** `src/systeminfo.cpp` / `src/systeminfo.h`  
**QML binding:** `SystemInfo`

Real-time system and battery monitor. Refreshes on a 2-second `QTimer`.

#### System Metrics

| Property | Source |
|---|---|
| `hostname` | `gethostname()` |
| `kernelVersion` | `uname -r` |
| `osVersion` | `/etc/os-release` |
| `cpuModel` | `/proc/cpuinfo` (parsed once at init) |
| `gpuModel` | `lspci` or `/sys/class/drm` |
| `totalRam` / `usedRam` | `/proc/meminfo` |
| `cpuUsage` | `/proc/stat` delta calculation |
| `diskUsedGb` / `diskTotalGb` | `statvfs("/")` |
| `uptime` | `/proc/uptime` |

#### Battery Metrics

| Property | Source |
|---|---|
| `batteryLevel` | `BAT*/capacity` |
| `batteryCharging` | `BAT*/status` |
| `acConnected` | `AC*/online` (independent of charging state) |
| `batteryCycles` | `BAT*/cycle_count` |
| `batteryHealth` | Calculated from `energy_full_design` vs `energy_full` |
| `powerNow` | `BAT*/power_now` (µW) |
| `estimatedRuntime` | Derived from capacity and discharge rate |

#### Hardware-Adaptive Charge Protection

At startup, `initBatteryStatics()` probes sysfs to detect the vendor mechanism:

| Mode | Hardware | UI Control | Sysfs Path |
|---|---|---|---|
| `threshold` | ThinkPad, ASUS, Samsung, Framework, Huawei, MSI | Percentage slider (60–100%) | `charge_control_end_threshold` |
| `conservation` | Lenovo IdeaPad | Binary toggle (~60% cap) | `conservation_mode` |
| `asus_mode` | ASUS ROG/TUF | 3-way selector (Balanced / Full / Gaming) | `charge_mode` |
| `none` | Unsupported hardware | Hidden | — |

Rapid charge support is detected separately on IdeaPad (`rapid_charge` sysfs or `acpi_call` fallback).

---

### 5.3 NetworkManager

**File:** `src/networkmanager.cpp` / `src/networkmanager.h`  
**QML binding:** `NetworkManager`

Wraps `nmcli` for Wi-Fi management.

| Property / Method | Description |
|---|---|
| `wifiEnabled` | Radio on/off via `nmcli radio wifi` |
| `connectedSsid` | Currently connected network name |
| `signalStrength` | Signal percentage |
| `ipAddress` / `ipv6Address` / `macAddress` | Connection details |
| `linkSpeed` | Negotiated link speed |
| `availableNetworks` | SSID list from scan |
| `scanNetworks()` | Triggers rescan + refresh |
| `connectToNetwork(ssid, password)` | Connect with credentials |
| `disconnectNetwork()` | Drop active connection |

---

### 5.4 DisplayManager

**File:** `src/displaymanager.cpp` / `src/displaymanager.h`  
**QML binding:** `DisplayManager`

| Property | System Tool |
|---|---|
| `brightness` (0–100) | `brightnessctl set` |
| `nightLightEnabled` | Starts/stops `wlsunset` process |
| `nightLightTemp` (2700–6500 K) | `wlsunset -t` color temperature |
| `resolution` | `hyprctl monitors` (read-only) |
| `refreshRate` | `hyprctl monitors` (read-only) |
| `scaleFactor` | `hyprctl keyword monitor scale` |

An `m_initialized` guard prevents startup property bindings from triggering unintended side effects before the UI is ready.

---

### 5.5 WallpaperManager

**File:** `src/wallpapermanager.cpp` / `src/wallpapermanager.h`  
**QML binding:** `WallpaperManager`

Manages desktop wallpaper via **swww** and drives adaptive theming.

| Property / Method | Description |
|---|---|
| `currentWallpaper` | Active wallpaper path |
| `wallpaperList` | Scanned wallpapers from `~/Pictures/Wallpapers` |
| `isDark` | Computed from wallpaper average luminance — drives light/dark design tokens in `Main.qml` |
| `scanWallpapers()` | Rescan directory |
| `openFilePicker()` | Native file dialog for custom wallpaper |
| `setCurrentWallpaper(path)` | Apply via `swww img` |

---

### 5.6 AudioBackend

**File:** `src/audiobackend.cpp` / `src/audiobackend.h`  
**QML binding:** `AudioBackend`

The most complex backend module. See [Section 7](#7-audio-subsystem) for full details.

| Property | Description |
|---|---|
| `masterVolume` / `masterMuted` | Default audio sink |
| `micVolume` / `micMuted` | Default audio source |
| `activeOutput` | Current sink device name |
| `eqLevels` | 24-bar real-time spectrum from Cava |
| `activeEqProfile` | Selected EQ preset name |
| `customGains` | 10-band user-defined gains (dB) |
| `spatialAudio` | Haas-effect stereo widening toggle |
| `spatialWidth` | Spatial width intensity (0–100) |

---

## 6. User Interface

### 6.1 Window Shell

`qml/Main.qml` defines a **frameless, transparent** application window (1160×740, min 920×580) with:

- **Custom title bar drag** — top 52 px MouseArea for window repositioning.
- **Adaptive design tokens** — color palette switches between dark and light based on `WallpaperManager.isDark`.
- **Sidebar navigation** — 220 px sidebar with 8 pages and SVG icons.
- **Accent color propagation** — `SettingsBackend.accentColor` flows to all pages.

### 6.2 Reusable Components

| Component | Purpose |
|---|---|
| `SettingsCard.qml` | Card container with optional title header |
| `TitanSlider.qml` | Custom-styled range slider with accent fill |
| `TitanSwitch.qml` | Toggle switch matching ArchTitan design language |
| `TitanButton.qml` | Primary/secondary action button |
| `SidebarItem.qml` | Navigation item with icon + label + active indicator |
| `SectionHeader.qml` | Section title with optional subtitle |
| `StatusBadge.qml` | Colored status pill (online, offline, warning) |

### 6.3 Settings Pages

#### Appearance (`AppearancePage.qml`)
Theme selector, accent color picker, glassmorphism toggle, panel opacity slider, icon theme dropdown, font family and size controls. Changes call `SettingsBackend.applyAndSave()` on commit.

#### Display (`DisplayPage.qml`)
Brightness slider, Night Light toggle with color temperature control, resolution/refresh rate display (read-only), display scale factor selector.

#### Network (`NetworkPage.qml`)
Wi-Fi radio toggle, network scan list, connect dialog with password input, connection details panel (IP, MAC, link speed, signal).

#### Audio (`AudioPage.qml`)
Output volume/mute, microphone volume/mute, active device display, 24-bar Cava visualizer, EQ profile selector (Flat, Bass Boost, Vocal, Electronic, Acoustic, Custom), 10-band custom EQ sliders with reset, spatial audio toggle with width slider.

#### Power (`PowerPage.qml`)
The most feature-rich page (~940 lines):

- Live battery gauge with level, charging state, AC connection status
- Battery health, cycle count, power draw, estimated runtime
- Hardware-adaptive charge protection UI (threshold slider / conservation toggle / ASUS 3-way)
- Rapid charge toggle (where supported)
- Power profile cards (Power Saver, Balanced, Performance) with 3D neon dial assets
- Screen-off and suspend timeout selectors
- Screen timeout and suspend timeout write to swayidle config on change

#### Security (`SecurityPage.qml`)
Screen autolock toggle and delay, Titan Sandbox status badge, firewall (ufw/firewalld) status, quick lock action (`swaylock -f`).

#### System (`SystemPage.qml`)
Live CPU, RAM, and disk usage meters with animated bars. Hardware summary (CPU model, GPU, total RAM, disk). Uptime display. Titan service status indicators.

#### About (`AboutPage.qml`)
Modernized hero section with animated accent rings, quick stats row (version, platform, runtime, license), system information grid, tech stack chip cloud, project links (GitHub, issues, wiki), footer credits.

---

## 7. Audio Subsystem

The audio subsystem underwent the largest development effort on the current branch, evolving from a basic `wpctl` volume wrapper to a full native PipeWire effects pipeline.

### 7.1 Architecture

```
Media Player ──► [Spatial Sink] ──► [EQ Sink] ──► Hardware Output
                  (optional)         (always)
                     │                   │
              archtitan-spatial    archtitan-eq
              filter-chain         filter-chain
              .conf                .conf
```

When spatial audio is **disabled**, streams route directly to the EQ sink. When **enabled**, streams route through the spatial sink first, then into the EQ sink downstream.

### 7.2 Volume & Mute Control

- **Read:** `wpctl get-volume @DEFAULT_AUDIO_SINK@` / `@DEFAULT_AUDIO_SOURCE@`
- **Write:** `wpctl set-volume` / `wpctl set-mute`
- **Sync:** `pactl subscribe` event listener with 50 ms debounce timer — eliminates polling and keeps UI in sync with external changes (keyboard media keys, other apps).

### 7.3 Real-Time Visualizer (Cava)

On startup, `AudioBackend` writes a temporary Cava config to `/tmp/archtitan-cava.conf`:

```ini
[general]
bars = 24
framerate = 60
[output]
method = raw
raw_target = /dev/stdout
data_format = ascii
ascii_max_range = 100
```

Cava output is parsed line-by-line into the `eqLevels` property (24 integer values), which drives the animated bar chart in `AudioPage.qml`. Cava auto-restarts if it exits (e.g., after a PipeWire service restart).

### 7.4 Native PipeWire Equalizer

The equalizer was migrated from **EasyEffects** to native **PipeWire filter-chain** modules, removing a heavy dependency and enabling live parameter updates.

#### EQ Profiles

| Profile | Description | Key Adjustments |
|---|---|---|
| Flat | Neutral | All bands at 0 dB |
| Bass Boost | Enhanced low end | +6 dB @ 32 Hz, +5 dB @ 64 Hz |
| Vocal | Mid-range emphasis | +4 dB @ 500–2000 Hz, cut lows/highs |
| Electronic | V-shaped | Boost lows and highs, cut mids |
| Acoustic | Natural warmth | Gentle mid/high boost |
| Custom | User-defined | 10 adjustable bands, persisted in QSettings |

#### Band Configuration

Each profile defines 10 bands at standard ISO frequencies:

```
32 Hz · 64 Hz · 125 Hz · 250 Hz · 500 Hz · 1 kHz · 2 kHz · 4 kHz · 8 kHz · 16 kHz
```

Filter types: **lowshelf** (band 1), **peaking** (bands 2–9), **highshelf** (band 10), Q = 0.707.

#### Config Generation

`applyEqProfile()` generates SPA-JSON filter-chain configuration and writes it to:

```
~/.config/pipewire/filter-chain.conf.d/archtitan-eq.conf
```

#### Live Gain Updates

Instead of restarting PipeWire (which caused 1–3 second audio gaps), gain changes are applied atomically via:

```bash
pw-cli set-param "$NODE_ID" Props '{ params: [ "eq_band_N:Gain" X.X ... ] }'
```

Node ID is resolved quickly via `pactl list sinks` (reads only sink properties, < 100 ms). Active streams are re-routed with `pactl move-sink-input` to prevent playback gaps.

When spatial audio is active, EQ profiles are forced to **Flat** to avoid interference with the spatial processing chain.

### 7.5 Spatial Audio (Haas Effect)

Spatial audio implements stereo widening using a **Haas-effect cross-feed** built entirely from PipeWire builtin filters — no external impulse response files.

#### Signal Flow

```
Input L ──► copyL ──┬──► mixL (direct) ──► Output L
                     └──► delayL ──► mixR (cross) ──► Output R

Input R ──► copyR ──┬──► mixR (direct) ──► Output R
                     └──► delayR ──► mixL (cross) ──► Output L
```

- **Delay:** 15 ms Haas delay on cross-channel paths
- **Cross gain:** 0.0–0.65 (mapped from width slider 0–100)
- **Direct gain:** `1.0 - crossGain × 0.32`

Config written to `~/.config/pipewire/filter-chain.conf.d/archtitan-spatial.conf`.

#### Configuration Template

The spatial audio node is defined in `archtitan-spatial.conf` using PipeWire's built-in `delay` and `mixer` filters. To prevent initialization crashes, the delay node configuration must separate the static maximum buffer size (`max-delay` in `config`) from the active delay value (`Delay (s)` in `control` in seconds):

```spa-json
context.modules = [
    { name = libpipewire-module-filter-chain
        flags = [ nofail ]
        args = {
            node.description = "ArchTitan Spatial"
            media.name       = "ArchTitan Spatial"
            filter.graph = {
                nodes = [
                    { type = builtin label = copy  name = copyL }
                    { type = builtin label = copy  name = copyR }
                    { type = builtin label = delay name = delayR
                        config = { "max-delay" = 1.0 }
                        control = { "Delay (s)" = 0.015 } }
                    { type = builtin label = delay name = delayL
                        config = { "max-delay" = 1.0 }
                        control = { "Delay (s)" = 0.015 } }
                    { type = builtin label = mixer name = mixL
                        control = { "Gain 1" = 0.7900 "Gain 2" = 0.5200 } }
                    { type = builtin label = mixer name = mixR
                        control = { "Gain 1" = 0.7900 "Gain 2" = 0.5200 } }
                ]
                links = [
                    { output = "copyL:Out"  input = "mixL:In 1" }
                    { output = "copyR:Out"  input = "mixR:In 1" }
                    { output = "copyR:Out"  input = "delayR:In" }
                    { output = "delayR:Out" input = "mixL:In 2" }
                    { output = "copyL:Out"  input = "delayL:In" }
                    { output = "delayL:Out" input = "mixR:In 2" }
                ]
                inputs  = [ "copyL:In" "copyR:In" ]
                outputs = [ "mixL:Out" "mixR:Out" ]
            }
            audio.channels = 2
            audio.position = [ FL FR ]
            capture.props = {
                node.name   = "effect_input.archtitan_spatial"
                media.class = Audio/Sink
            }
            playback.props = {
                node.name    = "effect_output.archtitan_spatial"
                node.passive = true
            }
        }
    }
]
```

#### Gapless Operation

- **Width/preset changes:** Live parameter update via `pw-cli set-param` — no PipeWire restart.
- **Toggle on/off:** PipeWire restart only when the spatial node doesn't exist yet; MPRIS auto-resume via `playerctl play` prevents playback interruption.
- **EQ interaction:** Spatial ON disables EQ profile gains (Flat forced); streams route through spatial sink → EQ sink.

### 7.6 Custom Equalizer

Users can define a 10-band custom profile:

- Gains stored in QSettings (`audio/customGains`)
- Per-band live update via `setCustomBandGain(index, gain)`
- Reset to flat via `resetCustomGains()`
- Selecting "Custom" profile applies user gains through the same filter-chain pipeline

### 7.7 Audio Evolution Timeline

| Phase | Approach | Issue |
|---|---|---|
| v0 | Random QML bar animation | No real audio data |
| v1 | EasyEffects + D-Bus preset loading | Heavy dependency, slow switching |
| v2 | Native PipeWire filter-chain + file generation | 1–3 s gaps on profile switch |
| v3 | `pw-cli set-param` live updates | Fast EQ, but spatial caused pauses |
| v4 (current) | Gapless spatial + Cava auto-restart + MPRIS resume | Production-ready |

---

## 8. Power & Battery Management

### 8.1 Power Profiles

Three profiles managed via **power-profiles-daemon** D-Bus:

| Display Name | D-Bus ID | Behavior |
|---|---|---|
| Power Saver | `power-saver` | Reduced CPU/GPU clocks, extended battery life |
| Balanced | `balanced` | Default smart performance |
| Performance | `performance` | Maximum CPU/GPU output |

The UI displays 3D neon dial/needle assets for each profile. External changes (keyboard shortcut) are reflected via D-Bus property listener.

### 8.2 Universal Profile Cycling Shortcut

`scripts/cycle_power.sh` implements stateful cycling:

```
Power Saver → Balanced → Performance → Power Saver → ...
```

Bound in Hyprland config as **`Super + P`** and **`Fn + P`** for universal hardware compatibility. Uses direct `busctl` calls to `net.hadess.PowerProfiles` and shows a `notify-send` toast on switch.

### 8.3 Screen & Suspend Timeouts

Configured via `swayidle` config at `~/.config/swayidle/config`:

```
timeout <screenTimeout> 'hyprctl dispatch dpmsoff' resume 'hyprctl dispatch dpmson'
timeout <suspendTimeout> 'systemctl suspend'
before-sleep 'swaylock -f'
```

The config is regenerated and `swayidle` restarted on every `applyAndSave()`.

### 8.4 Battery Charge Protection

Hardware detection at startup determines which UI to render:

**Threshold mode** (ThinkPad, ASUS, Samsung, Framework, etc.):
- Slider from 60% to 100% charge limit
- Writes to `charge_control_end_threshold` sysfs

**Conservation mode** (Lenovo IdeaPad):
- Binary toggle capping charge at ~60%
- Writes to `conservation_mode` sysfs

**ASUS ROG 3-way mode:**
- Balanced (default) / Full (100%) / Gaming (100% + fast charge)
- Writes to `charge_mode` sysfs

**Permission setup:** `scripts/install-battery-rules.sh` deploys udev rules granting the `users` group write access to all battery control sysfs files. Must be run once with sudo.

### 8.5 AC Power Detection

The `acConnected` property reads `AC*/online` independently of `batteryCharging`. This correctly shows "Plugged In" when the battery is full and not actively charging — a common edge case on laptops with charge protection enabled.

---

## 9. System Integration

### 9.1 Integration Matrix

| Subsystem | Tool / API | Backend | Direction |
|---|---|---|---|
| Window borders | `hyprctl keyword` | SettingsBackend | Write |
| Display scale | `hyprctl keyword monitor` | DisplayManager | Read/Write |
| DPMS (screen off) | `hyprctl dispatch dpmsoff` | SettingsBackend (via swayidle) | Write |
| GTK icon theme | `gsettings` | SettingsBackend | Write |
| GTK fonts | `gsettings` | SettingsBackend | Write |
| Wi-Fi | `nmcli` | NetworkManager | Read/Write |
| Audio volume | `wpctl` | AudioBackend | Read/Write |
| Audio events | `pactl subscribe` | AudioBackend | Read |
| Audio EQ | PipeWire filter-chain | AudioBackend | Write |
| Audio spatial | PipeWire filter-chain | AudioBackend | Write |
| Power profiles | D-Bus (system bus) | SettingsBackend | Read/Write |
| Battery sysfs | `/sys/class/power_supply` | SystemInfo | Read/Write |
| CPU/RAM/Disk | `/proc/*` | SystemInfo | Read |
| Brightness | `brightnessctl` | DisplayManager | Read/Write |
| Night light | `wlsunset` | DisplayManager | Write |
| Wallpaper | `swww img` | WallpaperManager | Write |
| Screen lock | `swaylock -f` | SecurityPage (direct) | Write |
| Idle management | `swayidle` | SettingsBackend | Write |
| Media resume | `playerctl` (MPRIS) | AudioBackend | Write |

### 9.2 Configuration File Locations

| File | Purpose |
|---|---|
| `~/.config/ArchTitan/archtitan-settings.conf` | QSettings persistence |
| `~/.config/swayidle/config` | Screen off / suspend / lock |
| `~/.config/pipewire/filter-chain.conf.d/archtitan-eq.conf` | EQ filter-chain |
| `~/.config/pipewire/filter-chain.conf.d/archtitan-spatial.conf` | Spatial filter-chain |
| `/etc/udev/rules.d/90-archtitan-battery.rules` | Battery sysfs permissions |
| `/tmp/archtitan-cava.conf` | Runtime Cava config |
| `/tmp/cycle_power.log` | Power cycle debug log |

---

## 10. Build & Deployment

### 10.1 Requirements

```bash
# Arch / ArchTitan:
sudo pacman -S qt6-base qt6-declarative qt6-quickcontrols2 cmake ninja

# Runtime dependencies:
sudo pacman -S pipewire wireplumber networkmanager brightnessctl wlsunset swww \
                 swayidle swaylock cava playerctl power-profiles-daemon
```

### 10.2 Build

```bash
chmod +x build.sh
./build.sh
# Binary: build/archtitan-settings
```

For development with live QML reloading:

```bash
QML_SOURCE_DIR=/path/to/archtitan-settings/qml ./build/archtitan-settings
```

### 10.3 CMake Configuration

- **Target:** `archtitan-settings` v1.0.0
- **QML module:** `ArchTitanSettings` URI, version 1.0
- **Linked libraries:** Qt6::Core, Quick, QuickControls2, DBus, Network, Concurrent
- **Install prefix:** `/usr` (Release build)

### 10.4 Post-Install Setup

```bash
# Battery charge control (one-time, requires sudo):
sudo bash scripts/install-battery-rules.sh

# Power profile keyboard shortcut (add to Hyprland config):
bind = SUPER, P, exec, /path/to/scripts/cycle_power.sh
bind = , XF86Launch1, exec, /path/to/scripts/cycle_power.sh  # Fn+P on some laptops
```

---

## 11. Scripts & Utilities

### 11.1 `scripts/cycle_power.sh`

Cycles power profiles via D-Bus. Logs transitions to `/tmp/cycle_power.log`. Shows desktop notification on switch.

### 11.2 `scripts/install-battery-rules.sh`

Deploys udev rules for universal battery sysfs write access. Covers:
- ThinkPad / ASUS / Samsung / Framework / Huawei / MSI (`charge_control_end_threshold`)
- Lenovo IdeaPad (`conservation_mode`, `rapid_charge`)
- ASUS ROG/TUF (`charge_mode`)

Applies permissions immediately to existing paths and reloads udev.

### 11.3 `build.sh` / `rebuild.sh`

Standard CMake configure + make workflow. `rebuild.sh` adds asset copy steps for development iteration.

### 11.4 Development Utilities

| Script | Purpose |
|---|---|
| `fix_fonts.py` | Batch-fix font weight references in QML |
| `fix_font_weights.py` | Numeric font weight normalization |
| `remove_bg.py` / `remove_black_bg.py` | Asset background removal for power profile icons |

---

## 12. Known Issues & Resolutions

The following issues were identified and resolved during development (see also `problems_and_solutions.txt`):

### 12.1 Resolved

| # | Problem | Solution |
|---|---|---|
| 1 | AC power shown as "On Battery" when plugged in but not charging | Added `acConnected` property reading `AC*/online` independently of charging state |
| 2 | Battery UI not adapting to different hardware vendors | Dynamic `chargeProtectionMode` detection with vendor-specific QML UI components |
| 3 | Battery sysfs write permission denied | `install-battery-rules.sh` udev rules for `users` group |
| 4 | Power profile keyboard shortcut broken | Direct D-Bus via `cycle_power.sh`, bypassing broken desktop layers |
| 5 | Generic/corrupted power profile icons | Replaced with 3D neon assets; fixed Qt resource system and `MultiEffect` colorization |
| 6 | CPU model and RAM showing "unknown" | Fixed `/proc/cpuinfo` and `/proc/meminfo` parsing in `SystemInfo` |
| 7 | Settings not persisting after reboot | Fixed `applyAndSave()` to write both QSettings and system configs (`gsettings`, swayidle) |
| 8 | Audio UI out of sync with external volume changes | `pactl subscribe` listener with debounce replaces polling |
| 9 | Microphone state wrong on startup | Constructor calls `sync()` to query real `wpctl` state |
| 10 | Fake EQ animation (random QML bars) | Integrated Cava for real-time 24-bar spectrum visualization |
| 11 | EasyEffects dependency too heavy | Migrated to native PipeWire filter-chain |
| 12 | EQ profile switch caused 1–3 s audio freeze | Live `pw-cli set-param` updates instead of PipeWire restart |
| 13 | Spatial audio toggle paused playback | MPRIS auto-resume via `playerctl play` after PipeWire restart |
| 14 | Spatial preset/width changes caused audio gaps | Live parameter updates when spatial node already loaded |
| 15 | Cava visualizer died after PipeWire restart | Auto-restart with 1 s delay on process exit |
| 16 | PipeWire builtin delay filter misconfigured | Corrected delay filter `config` and `control` parameters |

### 12.2 Open / Pending

| # | Problem | Proposed Solution |
|---|---|---|
| 1 | No audio output device switching in UI | Parse `wpctl status` / `pactl list sinks` and expose dropdown in AudioPage |
| 2 | Not integrated into ISO build | Add to `custom-os-build` packaging pipeline |
| 3 | Security page firewall detection may vary by distro config | Add support for both `ufw` and `firewalld` status checks |

---

## 13. Pending Work & Roadmap

### 13.1 Short Term

- [ ] Audio output device selector dropdown
- [ ] ISO build integration (`custom-os-build`)
- [ ] Install desktop entry and `.desktop` file
- [ ] System tray quick-settings panel (optional)

### 13.2 Medium Term

- [ ] Bluetooth device management page
- [ ] Display multi-monitor configuration UI
- [ ] Keyboard shortcut customization
- [ ] Notification settings integration
- [ ] User account management (basic)

### 13.3 Long Term

- [ ] Plugin system for third-party settings modules
- [ ] Remote management API (D-Bus service interface)
- [ ] Automated testing (QTest for backends, QML test framework for UI)
- [ ] Flatpak / AppImage packaging for non-ArchTitan systems

---

## 14. Dependencies

### 14.1 Build Dependencies

| Package | Purpose |
|---|---|
| `qt6-base` | Core, GUI, DBus, Network |
| `qt6-declarative` | QML engine |
| `qt6-quickcontrols2` | UI controls |
| `cmake` (≥ 3.22) | Build system |
| `ninja` or `make` | Build backend |
| `gcc` / `clang` | C++17 compiler |

### 14.2 Runtime Dependencies

| Package | Used By | Required |
|---|---|---|
| `pipewire` | AudioBackend | Yes |
| `wireplumber` | AudioBackend | Yes |
| `cava` | AudioBackend (visualizer) | Yes |
| `playerctl` | AudioBackend (MPRIS resume) | Recommended |
| `networkmanager` | NetworkManager | Yes |
| `brightnessctl` | DisplayManager | Yes |
| `wlsunset` | DisplayManager | Optional |
| `swww` | WallpaperManager | Optional |
| `hyprland` + `hyprctl` | SettingsBackend, DisplayManager | Yes (ArchTitan) |
| `swayidle` | SettingsBackend | Recommended |
| `swaylock` | SecurityPage | Recommended |
| `power-profiles-daemon` | SettingsBackend | Recommended |
| `gsettings` (glib2) | SettingsBackend | Recommended |
| `pavucontrol` | AudioBackend (openMixer) | Optional |
| `inter-font` | UI typography | Recommended |

---

## 15. Appendix

### 15.1 QML Context Property Reference

| QML Name | C++ Class | Scope |
|---|---|---|
| `SettingsBackend` | `SettingsBackend` | Global |
| `SystemInfo` | `SystemInfo` | Global |
| `NetworkManager` | `NetworkManager` | Global |
| `DisplayManager` | `DisplayManager` | Global |
| `AudioBackend` | `AudioBackend` | Global |
| `WallpaperManager` | `WallpaperManager` | Global |

### 15.2 QSettings Key Map

```
appearance/colorTheme       → "Dark"
appearance/accentColor      → "#7AA2F7"
appearance/glassmorphism    → true
appearance/panelOpacity     → 0.88
appearance/iconTheme        → "Papirus-Dark"
appearance/fontFamily       → "Inter"
appearance/fontSize         → 13
power/screenTimeout         → 300
power/suspendTimeout        → 600
power/profile               → "Balanced"
security/autolockEnabled    → true
security/autolockDelay      → 300
audio/customGains           → [0.0 × 10]
audio/spatialAudio          → false
audio/spatialWidth          → 80
```

### 15.3 Branch History Summary (`feature/audio-eq-and-about-page`)

Key commits on the active development branch:

```
fix(audio): correct PipeWire builtin delay filter parameters
fix(audio): auto-restart visualizer (Cava) when PipeWire is restarted
fix(audio): make spatial preset switching and slider moves gapless
fix(audio): speed up EQ switching and disable EQ profiles during Spatial Audio
fix(audio): spatial audio now actually works — audible effect + auto-resume
fix(audio): spatial audio toggle no longer pauses playback
feat(audio): add spatial stereo widening (Haas-effect, no external files)
feat(audio): add reset button to custom equalizer profile
feat(audio): add custom equalizer profile with user-adjustable sliders
fix(audio): resolve dynamic equalizer profile switching and add persistence
Implement native PipeWire EQ and modernize About page
feat: optimize EQ updates using pw-cli for live gain adjustments
refactor: replace EasyEffects with native PipeWire filter-chain
feat: add EasyEffects equalizer profile management (superseded)
feat: implement real-time audio visualization using cava integration
refactor: replace polling timer with pactl subscription and debounce
feat: implement hardware-adaptive battery charge protection
feat: universal power profile cycling shortcut (Super+P / Fn+P)
```

**Total diff vs `main`:** 19 files changed, ~2,868 insertions, ~236 deletions.

### 15.4 Glossary

| Term | Definition |
|---|---|
| **Haas Effect** | Psychoacoustic phenomenon where a delayed copy of a signal creates a sense of spatial width |
| **Filter-chain** | PipeWire module that chains audio processing filters (EQ, delay, mixer) in a graph |
| **SPA-JSON** | PipeWire's JSON-based configuration format for filter nodes |
| **MPRIS** | D-Bus interface for media player control (play, pause, stop) |
| **DPMS** | Display Power Management Signaling — turning off the display backlight |
| **Sysfs** | Linux virtual filesystem exposing kernel device attributes (`/sys/`) |
| **Cava** | Console-based Audio Visualizer for ALSA/PulseAudio/PipeWire |

---

*ArchTitan Settings — Final Year Project · © 2026 ArchTitan Project*
