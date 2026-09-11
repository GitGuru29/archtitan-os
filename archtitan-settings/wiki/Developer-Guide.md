# Developer Guide

[← Home](Home.md)

This guide covers the development workflow, coding conventions, utility scripts, and reference tables for contributing to ArchTitan Settings.

---

## Getting Started

### Clone and Build

```bash
git clone https://github.com/GitGuru29/archtitan-settings.git
cd archtitan-settings
chmod +x build.sh
./build.sh
```

### Live QML Development

Edit QML files without recompiling C++:

```bash
QML_SOURCE_DIR=$(pwd)/qml ./build/archtitan-settings
```

Changes to `.qml` files take effect on app restart. C++ changes require a rebuild.

### Rebuild After C++ Changes

```bash
./rebuild.sh
# or manually:
cd build && make -j$(nproc)
```

---

## Project Conventions

### C++ Backend

- All backends inherit from `QObject` with `Q_OBJECT` macro
- State exposed via `Q_PROPERTY` with `READ`, `WRITE`, `NOTIFY` signals
- System calls via `QProcess` — no raw `system()` calls
- Persistent settings use `QSettings("ArchTitan", "archtitan-settings")`
- Invokable methods for QML actions use `Q_INVOKABLE`
- Keep backends focused — one domain per class

### QML Frontend

- Pages live in `qml/pages/`, components in `qml/components/`
- Color tokens passed as properties from `Main.qml` (never hardcode colors in pages)
- Font: Inter with explicit `font.weight` (use numeric constants, not string names)
- User actions that persist settings call `SettingsBackend.applyAndSave()`
- Use existing components (`TitanSlider`, `SettingsCard`, etc.) before creating new ones

### Adding a New Backend Property

1. Declare `Q_PROPERTY` in the header with getter, setter, and signal
2. Implement getter/setter in `.cpp` — setter should emit signal on change
3. The property is automatically available in QML via the context property name
4. No QML registration needed (context properties, not `qmlRegisterType`)

### Adding a New Settings Page

1. Create `qml/pages/NewPage.qml`
2. Add to `CMakeLists.txt` under `QML_FILES`
3. Add entry to the `pages` array in `Main.qml`
4. Add SVG icon to `assets/icons/` and `CMakeLists.txt` RESOURCES

---

## Utility Scripts

| Script | Purpose |
|---|---|
| `build.sh` | Standard Release build |
| `rebuild.sh` | Clean rebuild with asset handling |
| `fix_fonts.py` | Batch-fix font weight references in QML |
| `fix_font_weights.py` | Convert string font weights to numeric constants |
| `remove_bg.py` | Remove backgrounds from PNG assets |
| `remove_black_bg.py` | Remove black backgrounds from power profile icons |
| `scripts/cycle_power.sh` | Power profile keyboard cycling (D-Bus) |
| `scripts/install-battery-rules.sh` | Deploy udev rules for battery sysfs |

---

## QML Context Properties

| QML Name | C++ Class | Scope |
|---|---|---|
| `SettingsBackend` | `SettingsBackend` | Global |
| `SystemInfo` | `SystemInfo` | Global |
| `NetworkManager` | `NetworkManager` | Global |
| `DisplayManager` | `DisplayManager` | Global |
| `AudioBackend` | `AudioBackend` | Global |
| `WallpaperManager` | `WallpaperManager` | Global |

Registered in `main.cpp`:

```cpp
engine.rootContext()->setContextProperty("SettingsBackend", &settingsBackend);
// ... etc
```

---

## QSettings Key Map

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

Stored at: `~/.config/ArchTitan/archtitan-settings.conf`

---

## Debugging Tips

### Audio Issues

```bash
# Check PipeWire sinks
pactl list sinks | grep -A5 archtitan

# Monitor volume events
pactl subscribe

# Test EQ config
cat ~/.config/pipewire/filter-chain.conf.d/archtitan-eq.conf

# Restart audio stack
systemctl --user restart pipewire pipewire-pulse wireplumber
```

### Power Profile Issues

```bash
# Current profile
busctl get-property net.hadess.PowerProfiles \
    /net/hadess/PowerProfiles net.hadess.PowerProfiles ActiveProfile

# Cycle log
tail -f /tmp/cycle_power.log
```

### Battery Sysfs

```bash
# List battery paths
ls /sys/class/power_supply/

# Check charge control support
cat /sys/class/power_supply/BAT0/charge_control_end_threshold 2>/dev/null
cat /sys/bus/platform/devices/*/conservation_mode 2>/dev/null
```

### QML Errors

Run with Qt logging enabled:

```bash
QT_LOGGING_RULES="qt.qml.debug=true" ./build/archtitan-settings
```

---

## Git Workflow

| Branch | Purpose |
|---|---|
| `main` | Stable baseline |
| `feature/audio-eq-and-about-page` | Current active development |
| `feature/audio-sync-fix` | Audio sync improvements |
| `feature/power-settings-refining` | Power page enhancements |

Commit message style follows conventional prefixes: `feat:`, `fix:`, `refactor:`, `docs:`.

---

## Glossary

| Term | Definition |
|---|---|
| **Haas Effect** | Psychoacoustic spatial widening via delayed cross-channel signal |
| **Filter-chain** | PipeWire module chaining audio processing filters in a graph |
| **SPA-JSON** | PipeWire's JSON configuration format for filter nodes |
| **MPRIS** | D-Bus media player control interface |
| **DPMS** | Display Power Management Signaling |
| **Sysfs** | Linux virtual filesystem for kernel device attributes (`/sys/`) |
| **Cava** | Console-based Audio Visualizer for PipeWire/PulseAudio |

---

## Related Pages

- [Architecture](Architecture.md) — system design overview
- [Backend Modules](Backend-Modules.md) — backend API reference
- [Build & Deployment](Build-and-Deployment.md) — build requirements and install
- [Known Issues](Known-Issues.md) — bug tracking and workarounds

[← Home](Home.md)
