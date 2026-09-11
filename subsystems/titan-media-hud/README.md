# Titan Media HUD (`titan-media-hud`)

**ArchTitan OS — Dynamic Contextual Media Drawer**

## Overview
`titan-media-hud` is a native Qt6/QML Wayland companion application that provides an interactive media drawer physically attached beneath the top Waybar HUD.

## Design Philosophy
> **"The OS stays invisible until you need it."**

- **When no media is playing:** The drawer is 100% hidden, using zero CPU cycles.
- **When media starts playing:** Automatically animates down from underneath the Waybar with smooth slide and fade transitions.
- **When media stops / pauses:** Smoothly retracts and hides.

## Features
- **MPRIS D-Bus Integration:** Compatible with Spotify, VLC, Firefox, Brave, Chromium, Titan Browser, MPV, etc.
- **Dynamic Artwork Caching:** Asynchronously caches local and remote album artwork (`/tmp/titan_media_hud/`).
- **Interactive Seek Bar:** Live progress tracking and interactive click-to-seek.
- **Media Controls:** Previous, Play/Pause toggle, Next.
- **Industrial Styling:** 1px subtle blue accent border, dark translucent surface (`#11111b`), monospace technical typography.

## Architecture
```
Qt6 Quick / QML Layer
        │
MprisController (C++)
        │
   playerctl / D-Bus
        │
   MPRIS Players (Spotify, VLC, Web Browsers, MPV)
```

## Build & Install
```bash
./build.sh
```
Binary installs directly to `airootfs/usr/local/bin/titan-media-hud`.
