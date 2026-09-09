# Power Management

[← Home](Home.md)

Power management spans three areas: **CPU/GPU power profiles**, **screen and suspend timeouts**, and **hardware-adaptive battery charge protection**. The Power page (`qml/pages/PowerPage.qml`) is the most feature-rich settings page in the application.

**Backends:** `SettingsBackend` (profiles, timeouts) · `SystemInfo` (battery metrics, charge protection)

---

## Power Profiles

Three profiles managed via **power-profiles-daemon** D-Bus:

| Display Name | D-Bus ID | Behavior |
|---|---|---|
| Power Saver | `power-saver` | Reduced CPU/GPU clocks, extended battery life |
| Balanced | `balanced` | Default smart performance |
| Performance | `performance` | Maximum CPU/GPU output |

The UI displays 3D neon dial/needle assets for each profile. External changes (keyboard shortcut) are reflected via D-Bus property listener in `SettingsBackend`.

### Applying Profiles

```cpp
// SettingsBackend::applyPowerProfileDBus()
busctl set-property net.hadess.PowerProfiles \
    /net/hadess/PowerProfiles \
    net.hadess.PowerProfiles \
    ActiveProfile s "balanced"
```

Fallback to `powerprofilesctl` if D-Bus call fails.

---

## Universal Profile Cycling Shortcut

`scripts/cycle_power.sh` implements stateful cycling:

```
Power Saver → Balanced → Performance → Power Saver → ...
```

### Hyprland Bindings

Add to your Hyprland config:

```ini
bind = SUPER, P, exec, /path/to/scripts/cycle_power.sh
bind = , XF86Launch1, exec, /path/to/scripts/cycle_power.sh  # Fn+P on some laptops
```

The script:
- Reads current profile via `busctl get-property`
- Advances to the next tier
- Shows a `notify-send` desktop notification
- Logs transitions to `/tmp/cycle_power.log`

This design supports both **`Super + P`** (desktop keyboards) and **`Fn + P`** (laptop keyboards) without separate hotkeys for each profile.

---

## Screen & Suspend Timeouts

Configured via `swayidle` at `~/.config/swayidle/config`:

```
timeout <screenTimeout> 'hyprctl dispatch dpmsoff' resume 'hyprctl dispatch dpmson'
timeout <suspendTimeout> 'systemctl suspend'
before-sleep 'swaylock -f'
```

The config is regenerated and `swayidle` restarted on every `SettingsBackend::applyAndSave()`.

### Available Options (UI)

**Screen off:** 1 m, 2 m, 5 m, 10 m, 15 m, 30 m (custom values supported)

**Suspend:** Never, 5 m, 10 m, 15 m, 30 m, 1 h, 2 h

---

## Battery Monitoring

`SystemInfo` provides live battery data refreshed every 2 seconds:

| Metric | Property | Description |
|---|---|---|
| Level | `batteryLevel` | Percentage (0–100) |
| Charging | `batteryCharging` | Active charge state |
| AC connected | `acConnected` | Plugged in (independent of charging) |
| Health | `batteryHealth` | Calculated from design vs full capacity |
| Cycles | `batteryCycles` | Charge cycle count |
| Power draw | `powerNow` | Current draw in µW |
| Runtime | `estimatedRuntime` | Estimated hours remaining |

### AC Power Detection

The `acConnected` property reads `AC*/online` independently of `batteryCharging`. This correctly shows "Plugged In" when the battery is full and not actively charging — a common edge case on laptops with charge protection enabled.

---

## Battery Charge Protection

Hardware detection at startup (`initBatteryStatics()`) determines which UI to render.

### Threshold Mode

**Hardware:** ThinkPad, ASUS, Samsung, Framework, Huawei, MSI

- Slider from 60% to 100% charge limit
- Writes to `BAT*/charge_control_end_threshold` sysfs
- Optional start threshold on ThinkPad (`charge_control_start_threshold`)

### Conservation Mode

**Hardware:** Lenovo IdeaPad

- Binary toggle capping charge at ~60%
- Writes to `conservation_mode` sysfs via `ideapad_acpi` platform driver

### ASUS ROG 3-Way Mode

**Hardware:** ASUS ROG/TUF laptops

| Mode | Value | Behavior |
|---|---|---|
| Balanced | 0 | Default charge behavior |
| Full | 1 | Charge to 100% |
| Gaming | 2 | 100% + fast charge |

Writes to `charge_mode` sysfs via `asus-wmi` or `asus-nb-wmi` driver.

### Rapid Charge

Supported on Lenovo IdeaPad (`rapid_charge` sysfs) and some ASUS models. Toggle exposed in Power page when `rapidChargeSupported` is true.

---

## Battery Permissions Setup

Battery sysfs files require root write access by default. Run once:

```bash
sudo bash scripts/install-battery-rules.sh
```

This deploys `/etc/udev/rules.d/90-archtitan-battery.rules`, granting the `users` group write access to:

- `charge_control_end_threshold` / `charge_control_start_threshold`
- `conservation_mode` / `rapid_charge`
- `charge_mode`

Log out and back in if controls remain greyed out after installation.

---

## Related Pages

- [Backend Modules](Backend-Modules.md) — SystemInfo and SettingsBackend reference
- [Known Issues](Known-Issues.md) — AC detection and battery UI fixes
- [Build & Deployment](Build-and-Deployment.md) — post-install setup
- [Developer Guide](Developer-Guide.md) — cycle_power.sh details

[← Home](Home.md)
