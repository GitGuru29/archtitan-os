# UI Design

[← Home](Home.md)

The ArchTitan Settings UI is built entirely in **QML** with a custom dark-first design system. The window is frameless and transparent, composited over the Hyprland desktop with Wayland alpha support.

**Root:** `qml/Main.qml` · **Components:** `qml/components/` · **Pages:** `qml/pages/`

---

## Window Shell

`Main.qml` defines a **frameless, transparent** application window:

| Property | Value |
|---|---|
| Default size | 1160 × 740 px |
| Minimum size | 920 × 580 px |
| Window flags | `Qt.Window \| Qt.FramelessWindowHint` |
| Background | Transparent (Wayland alpha buffer) |
| Corner radius | 12 px |
| Sidebar width | 220 px |

### Title Bar Drag

A top 52 px `MouseArea` allows window repositioning by click-and-drag, replacing the standard window title bar.

### Navigation

Eight pages accessible via sidebar with SVG icons:

| Index | Page | Icon | File |
|---|---|---|---|
| 0 | Appearance | `appearance.svg` | `AppearancePage.qml` |
| 1 | Display | `display.svg` | `DisplayPage.qml` |
| 2 | Network | `network.svg` | `NetworkPage.qml` |
| 3 | Audio | `audio.svg` | `AudioPage.qml` |
| 4 | Power | `power.svg` | `PowerPage.qml` |
| 5 | Security | `security.svg` | `SecurityPage.qml` |
| 6 | System | `system.svg` | `SystemPage.qml` |
| 7 | About | `about.svg` | `AboutPage.qml` |

---

## Design Tokens

Color palette is defined as readonly properties in `Main.qml` and adapts based on `WallpaperManager.isDark`.

### Dark Theme (default)

| Token | Value | Usage |
|---|---|---|
| `bg0` | `#0D0D0D` | Deepest background |
| `bg1` | `#B3111111` | Window shell (semi-transparent) |
| `bg2` | `#99171717` | Card backgrounds |
| `bg3` | `#1C1C1C` | Elevated surfaces |
| `bg4` | `#242424` | Hover states |
| `border0` | `#2A2A2A` | Primary borders |
| `border1` | `#1F1F1F` | Subtle dividers |
| `textHigh` | `#EBEBEB` | Primary text |
| `textMid` | `#8C8C8C` | Secondary text |
| `textLow` | `#707070` | Tertiary text |

### Semantic Colors

| Token | Value | Usage |
|---|---|---|
| `accent` | `SettingsBackend.accentColor` | Interactive elements, highlights |
| `accentDim` | accent @ 25% opacity | Subtle accent backgrounds |
| `green` | `#4CAF82` | Success, online, healthy |
| `red` | `#E05C6A` | Error, muted, danger |
| `orange` | `#D4853A` | Warning, medium volume |
| `purple` | `#7C6FCD` | Audio, special features |

### Adaptive Theming

When `WallpaperManager.isDark` is `false`, the global background and text tokens switch to a light palette. The accent color remains user-configurable via Appearance settings.

### Typography

- **Font family:** Inter (Regular, Medium, SemiBold, Bold)
- **Default size:** 13 px (configurable via Appearance)
- Loaded at startup from `/usr/share/fonts/inter/` or TTF fallbacks

---

## Reusable Components

| Component | File | Purpose |
|---|---|---|
| `SettingsCard` | `SettingsCard.qml` | Card container with optional title header |
| `TitanSlider` | `TitanSlider.qml` | Custom range slider with accent fill color |
| `TitanSwitch` | `TitanSwitch.qml` | Toggle switch matching ArchTitan design |
| `TitanButton` | `TitanButton.qml` | Primary/secondary action button |
| `SidebarItem` | `SidebarItem.qml` | Navigation item with icon, label, active indicator |
| `SectionHeader` | `SectionHeader.qml` | Section title with optional subtitle |
| `StatusBadge` | `StatusBadge.qml` | Colored status pill (online, offline, warning) |

All components accept color properties from the parent page (`textHigh`, `textMid`, `accent`, etc.) for consistent theming.

---

## Settings Pages

### Appearance

Theme selector, accent color picker, glassmorphism toggle, panel opacity slider, icon theme dropdown, font family and size controls. Changes call `SettingsBackend.applyAndSave()` on commit.

### Display

Brightness slider, Night Light toggle with color temperature control (2700–6500 K), resolution and refresh rate display (read-only), display scale factor selector.

### Network

Wi-Fi radio toggle, network scan list with signal strength, connect dialog with password input, connection details panel (IP, IPv6, MAC, link speed).

### Audio

Output volume/mute with color-coded slider fill, microphone volume/mute, active device name display, 24-bar Cava visualizer, EQ profile chips (Flat, Bass Boost, Vocal, Electronic, Acoustic, Custom), 10-band custom EQ sliders with reset button, spatial audio toggle with width slider.

### Power

The most feature-rich page (~940 lines):

- Live battery gauge with level, charging state, AC connection status
- Battery health, cycle count, power draw, estimated runtime
- Hardware-adaptive charge protection UI (threshold slider / conservation toggle / ASUS 3-way)
- Rapid charge toggle (where supported)
- Power profile cards with 3D neon dial assets
- Screen-off and suspend timeout selectors

### Security

Screen autolock toggle and delay selector, Titan Sandbox status badge, firewall status (ufw/firewalld), quick lock action button (`swaylock -f`).

### System

Live CPU, RAM, and disk usage meters with animated progress bars. Hardware summary (CPU model, GPU, total RAM, disk capacity). Uptime display. Titan service status indicators.

### About

Modernized hero section with animated accent rings and pulsing glow, logo circle, quick stats row (version, platform, runtime, license), system information grid, tech stack chip cloud with hover effects, project links (GitHub, issues, wiki), footer credits.

---

## Asset System

Icons are embedded via Qt Resource System (`CMakeLists.txt` RESOURCES section):

- SVG icons for sidebar navigation (`assets/icons/*.svg`)
- PNG branding assets (`LOGO.png`, power profile dials/needles)
- `MultiEffect` used in Power page for accent colorization of 3D assets

---

## Related Pages

- [Architecture](Architecture.md) — QML/C++ binding architecture
- [Backend Modules](Backend-Modules.md) — data sources for UI bindings
- [Developer Guide](Developer-Guide.md) — QML dev workflow with live reload

[← Home](Home.md)
