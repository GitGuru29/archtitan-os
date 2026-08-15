# Roadmap

[← Home](Home.md)

Development plans for ArchTitan Settings, organized by timeframe. Items marked with ✅ are completed on the current branch.

---

## Completed (Current Branch)

- ✅ Native PipeWire equalizer (6 presets + custom 10-band)
- ✅ Spatial audio (Haas-effect stereo widening)
- ✅ Real-time Cava audio visualizer
- ✅ Gapless EQ and spatial preset switching
- ✅ Hardware-adaptive battery charge protection
- ✅ Universal power profile cycling shortcut (`Super+P` / `Fn+P`)
- ✅ AC power detection independent of charging state
- ✅ Modernized About page
- ✅ Settings persistence via QSettings + system apply
- ✅ External audio volume sync via `pactl subscribe`

---

## Short Term

| Item | Description | Status |
|---|---|---|
| Audio output device selector | Dropdown to switch between available sinks | Planned |
| ISO build integration | Package into `custom-os-build` pipeline | Planned |
| Desktop entry | `.desktop` file with icon for app launcher | Planned |
| System tray quick-settings | Optional tray icon with volume/brightness toggles | Under consideration |

---

## Medium Term

| Item | Description |
|---|---|
| Bluetooth management page | Device pairing, connection, battery level |
| Multi-monitor configuration | Per-monitor scale, position, rotation via `hyprctl` |
| Keyboard shortcut customization | User-configurable hotkeys for common actions |
| Notification settings | Do Not Disturb, app notification permissions |
| User account management | Basic account info, password change, avatar |
| Light theme polish | Full adaptive token coverage across all pages |

---

## Long Term

| Item | Description |
|---|---|
| Plugin system | Third-party settings modules loaded at runtime |
| Remote management API | D-Bus service interface for external control |
| Automated testing | QTest for backends, QML test framework for UI |
| Cross-distro packaging | Flatpak / AppImage for non-ArchTitan systems |
| Accessibility | Screen reader support, high-contrast mode, keyboard navigation |
| Localization | Multi-language support via Qt translation system |

---

## Branch History (`feature/audio-eq-and-about-page`)

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
feat: implement real-time audio visualization using cava integration
refactor: replace polling timer with pactl subscription and debounce
feat: implement hardware-adaptive battery charge protection
feat: universal power profile cycling shortcut (Super+P / Fn+P)
```

**Total diff vs `main`:** 19 files changed, ~2,868 insertions, ~236 deletions.

---

## Related Pages

- [Known Issues](Known-Issues.md) — open items tracked as bugs
- [Build & Deployment](Build-and-Deployment.md) — current deployment status
- [Developer Guide](Developer-Guide.md) — how to contribute

[← Home](Home.md)
