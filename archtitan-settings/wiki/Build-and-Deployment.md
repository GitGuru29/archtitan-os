# Build & Deployment

[← Home](Home.md)

---

## Requirements

### Build Dependencies

```bash
# Arch / ArchTitan:
sudo pacman -S qt6-base qt6-declarative qt6-quickcontrols2 cmake ninja
```

| Package | Purpose |
|---|---|
| `qt6-base` | Core, GUI, DBus, Network |
| `qt6-declarative` | QML engine |
| `qt6-quickcontrols2` | UI controls |
| `cmake` (≥ 3.22) | Build system |
| `ninja` or `make` | Build backend |
| `gcc` / `clang` | C++17 compiler |

### Runtime Dependencies

```bash
sudo pacman -S pipewire wireplumber networkmanager brightnessctl wlsunset swww \
                 swayidle swaylock cava playerctl power-profiles-daemon inter-font
```

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

## Building

### Quick Build

```bash
chmod +x build.sh
./build.sh
```

Output binary: `build/archtitan-settings`

### Manual Build

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
```

### CMake Configuration

| Setting | Value |
|---|---|
| Target | `archtitan-settings` v1.0.0 |
| QML module | `ArchTitanSettings` URI, version 1.0 |
| C++ standard | C++17 |
| Linked libraries | Qt6::Core, Quick, QuickControls2, DBus, Network, Concurrent |
| Install prefix | `/usr` (Release build) |
| AUTOMOC / AUTORCC | Enabled |

---

## Running

### Production

```bash
./build/archtitan-settings
```

### Development (Live QML Reload)

Skip recompilation when editing QML files:

```bash
QML_SOURCE_DIR=/path/to/archtitan-settings/qml ./build/archtitan-settings
```

The app loads `Main.qml` from the filesystem and adds the QML directory to the import path.

---

## Post-Install Setup

### Battery Charge Control

Required once for battery protection features:

```bash
sudo bash scripts/install-battery-rules.sh
```

Log out and back in if controls remain greyed out.

### Power Profile Keyboard Shortcut

Add to Hyprland config (`~/.config/hypr/hyprland.conf`):

```ini
bind = SUPER, P, exec, /path/to/archtitan-settings/scripts/cycle_power.sh
bind = , XF86Launch1, exec, /path/to/archtitan-settings/scripts/cycle_power.sh
```

### System Install (Optional)

```bash
cd build
sudo make install
# Installs to /usr/bin/archtitan-settings
```

---

## Scripts

| Script | Purpose |
|---|---|
| `build.sh` | Configure + build (Release) |
| `rebuild.sh` | Clean rebuild with asset copy steps |
| `scripts/cycle_power.sh` | Power profile keyboard cycling |
| `scripts/install-battery-rules.sh` | udev rules for battery sysfs |

---

## Deployment Status

| Item | Status |
|---|---|
| ISO integration | Not yet in `custom-os-build` |
| `.desktop` file | Not yet created |
| System tray | Not implemented |
| Package (AUR/official) | Not yet published |

See [Roadmap](Roadmap.md) for planned deployment milestones.

---

## Related Pages

- [Developer Guide](Developer-Guide.md) — dev workflow and utility scripts
- [Power Management](Power-Management.md) — post-install power setup
- [Architecture](Architecture.md) — config file locations

[← Home](Home.md)
