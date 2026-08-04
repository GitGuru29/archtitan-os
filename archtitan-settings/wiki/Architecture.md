# Architecture

[← Home](Home.md)

**ArchTitan Settings** is built as a two-layer application: a **QML frontend** for UI and six **C++ backend singletons** for system integration. Backends communicate with the host OS through subprocess calls, D-Bus, and direct sysfs reads.

---

## High-Level Diagram

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

---

## Application Entry Point

`src/main.cpp` performs this initialization sequence:

1. **Wayland alpha buffer** — `QSurfaceFormat` alpha buffer size set to 8 for transparent window compositing.
2. **Font loading** — Inter font weights loaded from common Arch Linux paths.
3. **Backend instantiation** — six singleton backend objects created.
4. **QML context registration** — backends exposed via `engine.rootContext()->setContextProperty()`.
5. **QML loading** — dev-mode via `QML_SOURCE_DIR` env var, or embedded `qrc:/` resources in production.

---

## Data Flow Pattern

All backends follow a consistent reactive pattern:

```
Hardware / OS ──► C++ read/write ──► Q_PROPERTY ──► QML binding ──► UI update
                      ▲                                    │
                      └──────── user interaction ──────────┘
```

Settings that must survive reboots are persisted in **QSettings** (`~/.config/ArchTitan/archtitan-settings.conf`) and applied to the live system on save via `SettingsBackend::applyAndSave()`.

---

## Directory Structure

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

## Technology Stack

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

## System Integration Matrix

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

---

## Configuration File Locations

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

## Related Pages

- [Backend Modules](Backend-Modules.md) — detailed backend class reference
- [UI Design](UI-Design.md) — frontend structure and components
- [Developer Guide](Developer-Guide.md) — dev workflow and conventions

[← Home](Home.md)
