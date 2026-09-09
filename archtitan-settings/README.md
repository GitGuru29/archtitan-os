# ArchTitan Settings

A native Qt6/QML settings application for **ArchTitan OS** — a custom Arch Linux distribution with a Hyprland desktop environment.

## Features

| Page | Description |
|---|---|
| 🎨 **Appearance** | Theme selector, accent colors, glassmorphism toggle, panel opacity, icon themes, font size |
| 🖥 **Display** | Brightness, Night Light (wlsunset), resolution info, display scale factor |
| 📶 **Network** | Wi-Fi toggle, scan & connect to networks, connection details (nmcli) |
| 🔊 **Audio** | Output & microphone volume, mute controls, animated EQ visualizer (PipeWire/wpctl) |
| ⚡ **Power** | Power Saver / Balanced / Performance profiles, screen/suspend timeouts, live battery meter |
| 🔒 **Security** | Screen autolock, Titan Sandbox status, firewall status, quick lock action |
| ⚙ **System** | Live CPU/RAM/Disk meters (from /proc), hardware info, Titan service status |
| ◆ **About** | OS version, kernel, Qt version, credits |

## Tech Stack

- **Qt6** (6.11+) with **QML / Qt Quick Controls 2**
- **C++17** backend for system integration
- **D-Bus** for system service interaction
- **PipeWire** (`wpctl`) for audio
- **NetworkManager** (`nmcli`) for WiFi
- **Hyprland** (`hyprctl`) for display/scale
- **swww** for wallpaper management
- **brightnessctl** / **wlsunset** for display controls

## Universal Power Profile Toggle

To ensure a seamless user experience across different hardware generations, the power profile management system utilizes a **universal stateful toggle shortcut**.

Instead of forcing users to memorize distinct key combinations for each individual power state, a single, repeatable action (**`Super + P`** or **`Fn + P`**) cycles through the system's power tiers sequentially:

1. **First Press:** The system transitions from its current state into **Power Saver** mode to maximize battery longevity.
2. **Subsequent Press:** The system advances to **Balanced** mode for optimized, smart performance.
3. **Following Press:** The system enters **Performance** mode to unlock maximum CPU and GPU output before looping back to Power Saver mode.

By mapping the universal `Super` key alongside the hardware `Fn` key string, this cycling mechanism remains completely compatible with modern laptops as well as older keyboards that lack standard multimedia keys.

**Quick Summary:**
* **Intuitive Stateful Cycling:** Users can cycle through **Power Saver**, **Balanced**, and **Performance** profiles sequentially using a single shortcut combination.
* **Universal Hardware Compatibility:** Binds to **`Super + P`** as a universal fallback for older desktop keyboards, while simultaneously supporting native **`Fn + P`** combinations on modern laptop configurations.
* **Streamlined Control:** Eliminates keyboard clutter by removing the need for separate dedicated hotkeys for every unique power level.

*(Note: See `scripts/cycle_power.sh` for the implementation logic to bind in your Hyprland config.)*

## Building

```bash
chmod +x build.sh
./build.sh
```

**Requirements:** `qt6-base`, `qt6-declarative`, `qt6-quickcontrols2`, `cmake`, `ninja`/`make`

```bash
# Arch / ArchTitan:
sudo pacman -S qt6-base qt6-declarative qt6-quickcontrols2 cmake
```

## Directory Structure

```
archtitan-settings/
├── CMakeLists.txt          # Build configuration
├── build.sh                # Quick build script
├── src/
│   ├── main.cpp            # Entry point
│   ├── settingsbackend.*   # Persistent settings (QSettings)
│   ├── systeminfo.*        # /proc reader — CPU, RAM, disk, battery
│   ├── networkmanager.*    # nmcli Wi-Fi wrapper
│   ├── displaymanager.*    # Brightness, night light, scale
│   ├── audiobackend.*      # wpctl PipeWire wrapper
│   └── wallpapermanager.*  # swww wallpaper switcher
├── qml/
│   ├── Main.qml            # Root window (frameless, dark)
│   ├── components/         # Reusable UI components
│   │   ├── SidebarItem.qml
│   │   ├── SettingsCard.qml
│   │   ├── TitanSwitch.qml
│   │   ├── TitanSlider.qml
│   │   ├── TitanButton.qml
│   │   ├── SectionHeader.qml
│   │   └── StatusBadge.qml
│   └── pages/              # Settings pages
│       ├── AppearancePage.qml
│       ├── DisplayPage.qml
│       ├── NetworkPage.qml
│       ├── AudioPage.qml
│       ├── PowerPage.qml
│       ├── SecurityPage.qml
│       ├── SystemPage.qml
│       └── AboutPage.qml
└── assets/icons/           # SVG icons
```

## Status

> ⚡ **Active — Core Component.** This is a core OS component of ArchTitan, living at the repository root alongside `titan-hwm-source/`, `titanfetch-src/`, and `sandbox/`. Not a subsystem.

---

*ArchTitan OS — Final Year Project*
