# 🌌 ArchTitan Native Quick Settings & Notification Center

> **Status:** Integrated into Core OS (`airootfs/etc/skel/.config/swaync`)  
> **Backend:** SwayNC (Sway Notification Center) + Waybar + Hyprland  
> **Shortcut:** <kbd>Super</kbd> + <kbd>N</kbd> (or Waybar notification icon)

---

## Overview

The **ArchTitan Quick Settings & Notification Center** is a native Wayland desktop control panel built with **SwayNC** and integrated with **Hyprland** and **Waybar**. It replaces legacy standalone notification daemons (like Mako) with a cohesive, slide-in obsidian glassmorphic control drawer that provides both interactive Freedesktop desktop notifications and quick-setting toggles.

```
┌────────────────────────────────────────────────────────────┐
│ [▲ ArchTitan] Up 25m                    [✎] [↻] [⚙] [⏻]   │  <- Header Bar (Uptime & Quick System Actions)
├────────────────────────────────────────────────────────────┤
│  [ (•) Internet         ] [ ᛒ Bluetooth       ] [ ☕ ]     │  <- Row 1 Quick Toggles (Network, BT, Caffeine)
│       moto g24 power            Not connected              │
│  [ 🎙 Mute ] [ 🔊 Audio: Muted ] [ 🌙 Night Light: Inactive ]│  <- Row 2 Quick Toggles (Mic, Audio, Night Light)
│  [ ◐ ]   [ ılı ]   [ ≈ ]   [ ☁🔒 ]   [ 🎮 ]                │  <- Row 3 Squircle Toggles (Theme, Audio, Waves, VPN, Game)
│  [ ⛶ ]   [ 💉 ]   [ ⌨ ]   [ 🔔 ]    [ 🎵 ]                │  <- Row 4 Squircle Toggles (Snip, Eyedropper, Kbd, DND, Music)
│  [ 🕬 ]                                                     │  <- Row 5 Toggle
├────────────────────────────────────────────────────────────┤
│ ╭────────────────────────────────────────────────────────╮ │
│ │ (•) Network Management         September 12    [21 ▾] │ │  <- Grouped Notification Thread 1
│ │     No Network Connection   You are no longer conn...  │ │
│ ╰────────────────────────────────────────────────────────╯ │
│ ╭────────────────────────────────────────────────────────╮ │
│ │ [A] Antigravity IDE            September 11   [441 ▾] │ │  <- Grouped Notification Thread 2
│ │     Antigravity IDE   Host System Troubleshooting Ass...│
│ ╰────────────────────────────────────────────────────────╯ │
│ ╭────────────────────────────────────────────────────────╮ │
│ │ [💬] notify-send               September 10  [8503 ▾] │ │  <- Grouped Notification Thread 3
│ │     THM: Reclaiming idle workload "1387655" is idle... │ │
│ ╰────────────────────────────────────────────────────────╯ │
│ ╭────────────────────────────────────────────────────────╮ │
│ │ [💻] Hyprland                  September 08     [2 ▾] │ │  <- Grouped Notification Thread 4
│ │     Exited Virtual Machine submap   Keybinds re-ena... │ │
│ ╰────────────────────────────────────────────────────────╯ │
├────────────────────────────────────────────────────────────┤
│ [ 🔕 ]               8,967 notifications              [ 🗑 ] │  <- Notification Footer (DND, Count, Clear All)
├────────────────────────────────────────────────────────────┤
│ ▲  Thursday, September 17 • 0 tasks                        │  <- Bottom Calendar & Task Bar
└────────────────────────────────────────────────────────────┘
```

---

## Key Files & Locations

| Component | Path | Description |
| :--- | :--- | :--- |
| **SwayNC Config** | `airootfs/etc/skel/.config/swaync/config.json` | Widget tree, dimensions, timeout, and buttons-grid definitions |
| **SwayNC Theme** | `airootfs/etc/skel/.config/swaync/style.css` | Obsidian glassmorphic styling, cyan glow active states, GTK styles |
| **Toggle Backend** | `airootfs/usr/local/bin/titan-quicksettings-toggle` | Executable handler for caffeine, nightlight, gamemode, colorpicker, screensnip |
| **Hyprland Keybinds** | `airootfs/etc/skel/.config/hypr/hyprland.conf` | `Super+N` binding, `exec-once = swaync`, and layer blur rules |
| **Waybar Integration** | `airootfs/etc/skel/.config/waybar/config` | `custom/notification` icon module connecting to `swaync-client` |
| **Waybar Style** | `airootfs/etc/skel/.config/waybar/style.css` | Cyan hover & unread notification indicator styling |
| **Package Definition** | `packages.x86_64` | Added `swaync`, `hyprsunset`, `hyprpicker` |
| **Permissions** | `profiledef.sh` | Sets `0:0:755` permissions for `titan-quicksettings-toggle` |

---

## Keybindings & Controls

- **Toggle Drawer:** Press <kbd>Super</kbd> + <kbd>N</kbd> or click the notification bell in Waybar.
- **Dismiss / Close:** Press <kbd>Escape</kbd>, click outside the drawer, or press <kbd>Super</kbd> + <kbd>N</kbd> again.
- **Clear All:** Click the `󰆴 Clear All` button in the drawer header.
- **Toggle DND:** Right-click the Waybar notification icon or toggle the Bell button inside the panel.
