# Backend Modules

[← Home](Home.md)

Six C++ backend classes are instantiated in `main.cpp` and exposed to QML as global context properties. Each class wraps a specific domain of system functionality.

| QML Name | C++ Class | File |
|---|---|---|
| `SettingsBackend` | `SettingsBackend` | `src/settingsbackend.*` |
| `SystemInfo` | `SystemInfo` | `src/systeminfo.*` |
| `NetworkManager` | `NetworkManager` | `src/networkmanager.*` |
| `DisplayManager` | `DisplayManager` | `src/displaymanager.*` |
| `AudioBackend` | `AudioBackend` | `src/audiobackend.*` |
| `WallpaperManager` | `WallpaperManager` | `src/wallpapermanager.*` |

---

## SettingsBackend

Central persistence and system-apply layer for appearance, power, and security settings.

### Properties

| Property | Type | Default | System Apply Target |
|---|---|---|---|
| `colorTheme` | QString | `"Dark"` | QSettings only |
| `accentColor` | QString | `"#7AA2F7"` | `hyprctl` border colors |
| `glassmorphism` | bool | `true` | QSettings only |
| `panelOpacity` | double | `0.88` | QSettings only |
| `iconTheme` | QString | `"Papirus-Dark"` | `gsettings` icon-theme |
| `fontFamily` | QString | `"Inter"` | `gsettings` font-name |
| `fontSize` | int | `13` | `gsettings` font-name |
| `screenTimeout` | int | `300` | `swayidle` config |
| `suspendTimeout` | int | `600` | `swayidle` config |
| `powerProfile` | QString | `"Balanced"` | D-Bus power-profiles-daemon |
| `autolockEnabled` | bool | `true` | `swayidle` lifecycle |
| `autolockDelay` | int | `300` | QSettings (swayidle integration) |

### Key Methods

| Method | Description |
|---|---|
| `applyAndSave()` | Persist to QSettings, then apply to live system |
| `applyPowerProfileNow(profile)` | Immediate D-Bus profile switch |
| `applyScreenTimeoutNow(seconds)` | Write swayidle config and restart |
| `applySuspendTimeoutNow(seconds)` | Write swayidle config and restart |
| `resetToDefaults()` | Restore factory defaults |
| `version()` | Returns `"1.0.0"` |

### D-Bus Integration

Listens for external power profile changes on:
- `net.hadess.PowerProfiles` (power-profiles-daemon ≥ 0.10)
- `org.freedesktop.UPower.PowerProfiles` (legacy fallback)

Keeps the UI in sync when profiles are changed via `Super+P` or `scripts/cycle_power.sh`.

---

## SystemInfo

Real-time system and battery monitor. Refreshes on a 2-second `QTimer`.

### System Metrics

| Property | Source |
|---|---|
| `hostname` | `gethostname()` |
| `kernelVersion` | `uname -r` |
| `osVersion` | `/etc/os-release` |
| `cpuModel` | `/proc/cpuinfo` (parsed once at init) |
| `gpuModel` | `lspci` or `/sys/class/drm` |
| `totalRam` / `usedRam` | `/proc/meminfo` |
| `cpuUsage` | `/proc/stat` delta calculation |
| `diskUsedGb` / `diskTotalGb` | `statvfs("/")` |
| `uptime` | `/proc/uptime` |

### Battery Metrics

| Property | Source |
|---|---|
| `batteryLevel` | `BAT*/capacity` |
| `batteryCharging` | `BAT*/status` |
| `acConnected` | `AC*/online` (independent of charging state) |
| `batteryCycles` | `BAT*/cycle_count` |
| `batteryHealth` | Calculated from `energy_full_design` vs `energy_full` |
| `powerNow` | `BAT*/power_now` (µW) |
| `estimatedRuntime` | Derived from capacity and discharge rate |

### Hardware-Adaptive Charge Protection

At startup, `initBatteryStatics()` probes sysfs to detect the vendor mechanism:

| Mode | Hardware | UI Control | Sysfs Path |
|---|---|---|---|
| `threshold` | ThinkPad, ASUS, Samsung, Framework, Huawei, MSI | Percentage slider (60–100%) | `charge_control_end_threshold` |
| `conservation` | Lenovo IdeaPad | Binary toggle (~60% cap) | `conservation_mode` |
| `asus_mode` | ASUS ROG/TUF | 3-way selector (Balanced / Full / Gaming) | `charge_mode` |
| `none` | Unsupported hardware | Hidden | — |

### Invokable Methods

| Method | Description |
|---|---|
| `setChargeProtection(bool)` | Toggle conservation mode (IdeaPad) |
| `setChargeLimit(int)` | Set threshold percentage (60–100) |
| `setAsusChargeMode(int)` | ASUS ROG mode: 0=Balanced, 1=Full, 2=Gaming |
| `setRapidCharge(bool)` | Toggle Lenovo rapid charge |

---

## NetworkManager

Wraps `nmcli` for Wi-Fi management.

| Property / Method | Description |
|---|---|
| `wifiEnabled` | Radio on/off via `nmcli radio wifi` |
| `connectedSsid` | Currently connected network name |
| `signalStrength` | Signal percentage |
| `ipAddress` / `ipv6Address` / `macAddress` | Connection details |
| `linkSpeed` | Negotiated link speed |
| `isConnected` | Whether a Wi-Fi connection is active |
| `availableNetworks` | SSID list from scan |
| `scanNetworks()` | Triggers rescan + refresh |
| `connectToNetwork(ssid, password)` | Connect with credentials |
| `disconnectNetwork()` | Drop active connection |

Emits `connectionError(message)` on failed connections.

---

## DisplayManager

| Property | System Tool |
|---|---|
| `brightness` (0–100) | `brightnessctl set` |
| `nightLightEnabled` | Starts/stops `wlsunset` process |
| `nightLightTemp` (2700–6500 K) | `wlsunset -t` color temperature |
| `resolution` | `hyprctl monitors` (read-only) |
| `refreshRate` | `hyprctl monitors` (read-only) |
| `scaleFactor` | `hyprctl keyword monitor scale` |

An `m_initialized` guard prevents startup property bindings from triggering unintended side effects before the UI is ready.

---

## WallpaperManager

Manages desktop wallpaper via **swww** and drives adaptive theming.

| Property / Method | Description |
|---|---|
| `currentWallpaper` | Active wallpaper path |
| `wallpaperList` | Scanned wallpapers from `~/Pictures/Wallpapers` |
| `isDark` | Computed from wallpaper average luminance — drives light/dark tokens in `Main.qml` |
| `scanWallpapers()` | Rescan directory |
| `openFilePicker()` | Native file dialog for custom wallpaper |
| `setCurrentWallpaper(path)` | Apply via `swww img` |

---

## AudioBackend

The most complex backend module. See [Audio System](Audio-System.md) for full pipeline details.

| Property | Description |
|---|---|
| `masterVolume` / `masterMuted` | Default audio sink |
| `micVolume` / `micMuted` | Default audio source |
| `activeOutput` | Current sink device name |
| `eqLevels` | 24-bar real-time spectrum from Cava |
| `activeEqProfile` | Selected EQ preset name |
| `customGains` | 10-band user-defined gains (dB) |
| `spatialAudio` | Haas-effect stereo widening toggle |
| `spatialWidth` | Spatial width intensity (0–100) |

| Method | Description |
|---|---|
| `openMixer()` | Launch `pavucontrol` |
| `setCustomBandGain(index, gain)` | Update single EQ band live |
| `resetCustomGains()` | Reset all custom bands to 0 dB |

---

## Related Pages

- [Architecture](Architecture.md) — system design and integration matrix
- [Audio System](Audio-System.md) — full audio pipeline documentation
- [Power Management](Power-Management.md) — power profiles and battery protection
- [Developer Guide](Developer-Guide.md) — QSettings key map and conventions

[← Home](Home.md)
