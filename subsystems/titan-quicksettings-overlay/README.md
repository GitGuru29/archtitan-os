# ArchTitan Quick Settings & Notification Center Web Overlay

## Overview
This implementation plan documents the architecture, visual design system, and technical specifications for the web-based Quick Settings & Notification Center overlay that replicates the ArchTitan OS desktop control panel (Hyprland Wayland environment). The overlay is designed to slide in smoothly when the user presses **<kbd>Super</kbd> + <kbd>N</kbd>** (or **<kbd>Alt</kbd> + <kbd>N</kbd>** / on-screen toggle button), featuring obsidian glassmorphism, interactive toggles, grouped notification threads, and live calendar/task integration.

---

## Visual & Functional Breakdown (Matching Reference Image)

```
┌────────────────────────────────────────────────────────────┐
│ [▲ ArchTitan] Up 25m                    [✎] [↻] [⚙] [⏻]   │  <- Header Bar
├────────────────────────────────────────────────────────────┤
│  [ (•) Internet         ] [ ᛒ Bluetooth       ] [ ☕ ]     │  <- Row 1 Toggles
│       moto g24 power            Not connected              │
│  [ 🎙 Mute ] [ 🔊 Audio: Muted ] [ 🌙 Night Light: Off ]    │  <- Row 2 Toggles
│  [ ◐ ]   [ ılı ]   [ ≈ ]   [ ☁🔒 ]   [ 🎮 ]                │  <- Row 3 Icon Toggles
│  [ ⛶ ]   [ 💉 ]   [ ⌨ ]   [ 🔔 ]    [ 🎵 ]                │  <- Row 4 Icon Toggles
│  [ 🕬 ]                                                     │  <- Row 5 Toggle
├────────────────────────────────────────────────────────────┤
│ ╭────────────────────────────────────────────────────────╮ │
│ │ (•) Network Management         September 12    [21 ▾] │ │  <- Notification 1
│ │     No Network Connection   You are no longer conn...  │ │
│ ╰────────────────────────────────────────────────────────╯ │
│ ╭────────────────────────────────────────────────────────╮ │
│ │ [A] Antigravity IDE            September 11   [441 ▾] │ │  <- Notification 2
│ │     Antigravity IDE   Host System Troubleshooting Ass...│
│ ╰────────────────────────────────────────────────────────╯ │
│ ╭────────────────────────────────────────────────────────╮ │
│ │ [💬] notify-send               September 10  [8503 ▾] │ │  <- Notification 3
│ │     THM: Reclaiming idle workload "1387655" is idle... │ │
│ ╰────────────────────────────────────────────────────────╯ │
│ ╭────────────────────────────────────────────────────────╮ │
│ │ [💻] Hyprland                  September 08     [2 ▾] │ │  <- Notification 4
│ │     Exited Virtual Machine submap   Keybinds re-ena... │ │
│ ╰────────────────────────────────────────────────────────╯ │
├────────────────────────────────────────────────────────────┤
│ [ 🔕 ]               8,967 notifications              [ 🗑 ] │  <- Notification Footer
├────────────────────────────────────────────────────────────┤
│ ▲  Thursday, September 17 • 0 tasks                        │  <- Bottom Calendar Bar
└────────────────────────────────────────────────────────────┘
```

---

## Architecture & Technical Decisions

### 1. Browser Keyboard Shortcut Handling (<kbd>Super</kbd> + <kbd>N</kbd>)
- **Super (Meta / Windows / Command)** key in web browsers:
  - In standard desktop browsers, the OS or window manager (Hyprland, Windows Shell, macOS) may catch `Super + N` or `Cmd + N` before it reaches page JavaScript.
  - In browsers like Chromium or Firefox, `e.metaKey && (e.key === 'n' || e.key === 'N')` triggers the event, where `e.preventDefault()` must be called immediately.
  - **Fail-safe Fallback Support**:
    1. Primary: <kbd>Super</kbd> + <kbd>N</kbd> (`e.metaKey && e.code === 'KeyN'`).
    2. Fallback: <kbd>Alt</kbd> + <kbd>N</kbd> and <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>N</kbd>.
    3. Graphical UI trigger: An on-screen Waybar status capsule icon or floating trigger button.

### 2. Panel Container & Backdrop
- **Slide-in Animation**: Smooth slide-in from the right edge with an overshot bezier curve (`cubic-bezier(0.05, 0.9, 0.1, 1.05)`) matching Hyprland's native window animations.
- **Glassmorphic Aesthetic**:
  - Translucent slate/obsidian surface (`rgba(17, 20, 28, 0.88)`).
  - Backdrop blur filter (`backdrop-filter: blur(28px) saturate(160%)`).
  - Subtle glowing border (`1px solid rgba(56, 189, 248, 0.2)`).
  - Rounded corners (`border-radius: 24px`).
  - Atmospheric drop shadow (`0 20px 60px rgba(0, 0, 0, 0.7)`).
- **Outside Dismiss & Escape**: Clicking the translucent backdrop or pressing <kbd>Escape</kbd> smoothly closes the drawer.

### 3. Header Bar
- ArchTitan triangle delta badge with glowing cyan accent.
- Dynamic system uptime ticker (`Up 25m`, auto-incrementing).
- Control buttons: Edit Quick Toggles (`✎`), Refresh Notifications (`↻`), Settings (`⚙`), Power Menu (`⏻`).

### 4. Quick Settings Controls & Live Effects
- **Internet Pill**: Active cyan state (`#38bdf8`), Wifi icon, "Internet" & "moto g24 power" subtitle. Click toggles active/inactive state.
- **Bluetooth Pill**: Inactive slate state, Bluetooth icon, "Bluetooth" & "Not connected" subtitle.
- **Caffeine Pill**: Coffee cup icon toggle.
- **Audio Output & Mic Mute**: Speaker mute toggle and microphone toggle.
- **Night Light**: Moon icon toggle. Clicking activates a real CSS warm filter (`sepia(30%) hue-rotate(-15deg)`) across the page.
- **Squircle Toggle Grid**:
  - Theme/Contrast mode toggle (active cyan).
  - Audio visualizer toggle with animated sound bars (active cyan).
  - Ambient Waves, Cloud Lock (VPN), Game/Performance Mode.
  - Screenshot (captures page/canvas flash), Color Picker (`EyeDropper` API), Virtual Keyboard.
  - Notification Bell (Do Not Disturb toggle, active cyan).
  - Media / Music player toggle.

### 5. Grouped Notification Center
- Grouped cards exactly as seen in reference:
  - **Network Management** (21 items, "No Network Connection").
  - **Antigravity IDE** (441 items, "Host System Troubleshooting Assistant...").
  - **notify-send** (8503 items, "THM: Reclaiming idle workload...").
  - **Hyprland** (2 items, "Exited Virtual Machine submap...").
- Expandable / Collapsible chevron to preview individual entries.
- Hover actions: Individual dismiss button (`×`), clear card, copy content.
- Custom ultra-slim obsidian scrollbar.

### 6. Notification Footer
- Do Not Disturb icon (`🔕`).
- Dynamic notification counter pill (`8,967 notifications`).
- Clear All button (`🗑`): Clears notifications with a staggered slide-out animation and displays a sleek empty state.

### 7. Bottom Calendar & Tasks Drawer
- Expandable bar with current real-time date (`Thursday, September 17 • 0 tasks`).
- Expands to reveal upcoming calendar agenda and system task summary.

---

## File Structure

```
subsystems/titan-quicksettings-overlay/
├── README.md               # This specification & implementation plan
├── index.html              # Standalone interactive demo & desktop simulation
├── titan-overlay.js        # Standalone, zero-dependency ES6 module / component
└── titan-overlay.css       # Obsidian glassmorphism styles and animations
```

---

## Verification Plan

### Automated / Browser Verification
1. **Local Server Execution**:
   - Host overlay files via static HTTP server.
   - Verify static assets, CSS variables, and fonts load with 0 console errors.
2. **Visual & Interaction Testing**:
   - Verify <kbd>Super</kbd> + <kbd>N</kbd> and <kbd>Alt</kbd> + <kbd>N</kbd> key handling.
   - Verify toggle interactions (WiFi, Bluetooth, Night Light, Audio, DND).
   - Test expanding notifications, dismissing individual notifications, and clearing all.
   - Test <kbd>Escape</kbd> and backdrop click to dismiss overlay.
   - Confirm visual parity against reference screenshot.
