# Desktop Environment

ArchTitan OS delivers a Wayland-native desktop built around the **Hyprland** dynamic tiling window manager, unified visually with the **Catppuccin Mocha** dark palette.

---

## Ecosystem Breakdown

```mermaid
graph TD
    subgraph Core["Session & Compositor"]
        HYPR[Hyprland Compositor]
        SDDM[SDDM Display Manager]
    end

    subgraph Interface["UI Components"]
        WAY[Waybar Status Bar]
        ROFI[Rofi App Launcher]
        DUNST[Dunst Notification Daemon]
    end

    subgraph Terminal["Terminal & Shell"]
        KITTY[Kitty Terminal Emulator]
        FISH[Fish Shell]
        STAR[Starship Prompt]
    end

    subgraph Theming["Design System"]
        CAT[Catppuccin Mocha Theme]
        PAP[Papirus Dark Icons]
        PLYM[Plymouth Boot Splash]
    end

    HYPR --- WAY
    HYPR --- ROFI
    HYPR --- DUNST
    HYPR --- KITTY
    KITTY --- FISH
    FISH --- STAR
```

---

## Desktop Components & Configurations

| Component | Function | Configuration File Location |
| :--- | :--- | :--- |
| **Hyprland** | Wayland Tiling Compositor | `~/.config/hypr/hyprland.conf` |
| **Waybar** | Top status panel with active THM profile badge | `~/.config/waybar/config` & `style.css` |
| **Rofi (Wayland)** | Application launcher, window switcher, power menu | `~/.config/rofi/config.rasi` |
| **Kitty** | GPU-accelerated terminal emulator | `~/.config/kitty/kitty.conf` |
| **Fish Shell** | Interactive shell with auto-suggestions & syntax highlighting | `~/.config/fish/config.fish` |
| **Starship** | Cross-shell developer prompt | `~/.config/starship.toml` |
| **Dunst** | Notification daemon styled with rounded borders | `~/.config/dunst/dunstrc` |

---

## Essential Keybindings Reference

All keybindings use the **Super** key (Windows/Cmd key) as the main modifier:

### System & Launchers

| Keybinding | Action |
| :--- | :--- |
| <kbd>Super</kbd> + <kbd>Return</kbd> | Open Kitty Terminal |
| <kbd>Super</kbd> / <kbd>Super</kbd> + <kbd>Space</kbd> | Open Rofi App Launcher |
| <kbd>Super</kbd> + <kbd>W</kbd> | Launch Default Web Browser (Chromium / Firefox) |
| <kbd>Super</kbd> + <kbd>E</kbd> | Launch Ranger File Manager |
| <kbd>Super</kbd> + <kbd>I</kbd> | Launch Calamares Graphical Installer |
| <kbd>Super</kbd> + <kbd>V</kbd> | Open Rofi Clipboard History Manager |
| <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>S</kbd> | Take Region Screenshot (via `grim` + `slurp`) |
| <kbd>Super</kbd> + <kbd>Escape</kbd> | Open Rofi Power Menu (Lock, Exit, Reboot, Shutdown) |

### Window Management

| Keybinding | Action |
| :--- | :--- |
| <kbd>Super</kbd> + <kbd>Q</kbd> | Close active window |
| <kbd>Super</kbd> + <kbd>F</kbd> | Toggle Fullscreen mode |
| <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>Space</kbd> | Toggle Floating mode for active window |
| <kbd>Super</kbd> + <kbd>H</kbd> / <kbd>J</kbd> / <kbd>K</kbd> / <kbd>L</kbd> | Move focus Left / Down / Up / Right |
| <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>H</kbd> / <kbd>J</kbd> / <kbd>K</kbd> / <kbd>L</kbd> | Move window position Left / Down / Up / Right |
| <kbd>Super</kbd> + Left Click Drag | Move floating window |
| <kbd>Super</kbd> + Right Click Drag | Resize floating window |

### Workspaces

| Keybinding | Action |
| :--- | :--- |
| <kbd>Super</kbd> + <kbd>1</kbd> .. <kbd>9</kbd> | Switch to Workspace 1 .. 9 |
| <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>1</kbd> .. <kbd>9</kbd> | Move active window to Workspace 1 .. 9 |
| <kbd>Super</kbd> + <kbd>Mouse Scroll</kbd> | Cycle through active workspaces |

---

## Catppuccin Mocha Theme Specification

ArchTitan OS enforces the Catppuccin Mocha color palette across all applications:

```
Base      : #1e1e2e  (Background)
Mantle    : #181825  (Darker background)
Crust     : #11111b  (Deepest dark)
Text      : #cdd6f4  (Primary text)
Subtext0  : #a6adc8  (Secondary text)
Blue      : #89b4fa  (Accent primary)
Lavender  : #b4befe  (Accent secondary)
Sapphire  : #74c7ec  (Active window border gradient 1)
Teal      : #94e2d5  (Active window border gradient 2)
Red       : #f38ba8  (Error / urgent)
Green     : #a6e3a1  (Success / active state)
```

### Window Decoration Settings

- **Border Width**: `2px`
- **Active Border**: `gradient(45deg, 0xff74c7ec, 0xff94e2d5)`
- **Inactive Border**: `0xff313244`
- **Corner Rounding**: `10px`
- **Blur**: Enabled (`size = 6`, `passes = 2`, `new_optimizations = true`)
- **Drop Shadow**: Enabled (`range = 15`, `render_power = 3`)
- **Animations**: Overshot spring curve (`bezier = overshot, 0.05, 0.9, 0.1, 1.05`)

---

## Custom Waybar Integration

Waybar includes a dedicated module that polls THM's state file `/tmp/titan_hwm_state`:

- **THM Badge**: Displays current workload profile (`[SYS]`, `[WEB]`, `[AND]`, `[CAS]`) in vibrant accent colors.
- **Hardware Telemetry**: Displays live CPU usage %, RAM consumption %, CPU temperature, and active PipeWire volume level.
- **Click Actions**: Clicking the THM module opens the `titan-hwm metrics` dashboard in Kitty.
