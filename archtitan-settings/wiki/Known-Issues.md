# Known Issues

[← Home](Home.md)

This page tracks bugs and limitations identified during development. See also `problems_and_solutions.txt` in the repository root.

---

## Resolved

| # | Area | Problem | Solution |
|---|---|---|---|
| 1 | Power | AC power shown as "On Battery" when plugged in but not charging | Added `acConnected` property reading `AC*/online` independently of charging state |
| 2 | Power | Battery UI not adapting to different hardware vendors | Dynamic `chargeProtectionMode` detection with vendor-specific QML UI |
| 3 | Power | Battery sysfs write permission denied | `install-battery-rules.sh` udev rules for `users` group |
| 4 | Power | Power profile keyboard shortcut broken | Direct D-Bus via `cycle_power.sh`, bypassing broken desktop layers |
| 5 | UI | Generic/corrupted power profile icons | Replaced with 3D neon assets; fixed Qt resource system and `MultiEffect` colorization |
| 6 | System | CPU model and RAM showing "unknown" | Fixed `/proc/cpuinfo` and `/proc/meminfo` parsing in `SystemInfo` |
| 7 | Settings | Changes resetting after reboot | Fixed `applyAndSave()` to write both QSettings and system configs |
| 8 | Audio | UI out of sync with external volume changes | `pactl subscribe` listener with 50 ms debounce replaces polling |
| 9 | Audio | Microphone state wrong on startup | Constructor calls `sync()` to query real `wpctl` state |
| 10 | Audio | Fake EQ animation (random QML bars) | Integrated Cava for real-time 24-bar spectrum visualization |
| 11 | Audio | EasyEffects dependency too heavy | Migrated to native PipeWire filter-chain |
| 12 | Audio | EQ profile switch caused 1–3 s audio freeze | Live `pw-cli set-param` updates instead of PipeWire restart |
| 13 | Audio | Spatial audio toggle paused playback | MPRIS auto-resume via `playerctl play` after PipeWire restart |
| 14 | Audio | Spatial preset/width changes caused audio gaps | Live parameter updates when spatial node already loaded |
| 15 | Audio | Cava visualizer died after PipeWire restart | Auto-restart with 1 s delay on process exit |
| 16 | Audio | PipeWire builtin delay filter misconfigured | Corrected delay filter `config` and `control` parameters |

---

## Open / Pending

| # | Area | Problem | Proposed Solution | Priority |
|---|---|---|---|---|
| 1 | Audio | No audio output device switching in UI | Parse `wpctl status` / `pactl list sinks`, expose dropdown in AudioPage | High |
| 2 | Deployment | Not integrated into ISO build | Add to `custom-os-build` packaging pipeline | High |
| 3 | Security | Firewall detection may vary by distro config | Add support for both `ufw` and `firewalld` status checks | Medium |
| 4 | Deployment | No `.desktop` file or app launcher entry | Create Freedesktop entry with icon | Medium |
| 5 | UI | Light theme tokens incomplete on some pages | About page uses hardcoded dark colors instead of adaptive tokens | Low |

---

## Workarounds

### Battery controls greyed out

Run the permission installer and log out:

```bash
sudo bash scripts/install-battery-rules.sh
# Then log out and back in
```

### Power profile shortcut not working

Verify power-profiles-daemon is running:

```bash
systemctl status power-profiles-daemon
busctl get-property net.hadess.PowerProfiles /net/hadess/PowerProfiles \
    net.hadess.PowerProfiles ActiveProfile
```

Check debug log:

```bash
cat /tmp/cycle_power.log
```

### Audio EQ not applying

Verify filter-chain config exists and PipeWire loaded it:

```bash
ls ~/.config/pipewire/filter-chain.conf.d/
pactl list sinks | grep archtitan
systemctl --user restart pipewire pipewire-pulse
```

### Cava visualizer not animating

Ensure Cava is installed and PipeWire is the active audio server:

```bash
which cava
pactl info | grep "Server Name"
```

---

## Reporting New Issues

Report bugs on the [archtitan-os issue tracker](https://github.com/GitGuru29/archtitan-os/issues). Include:

- ArchTitan Settings version and branch
- Hardware model (especially for battery/power issues)
- Steps to reproduce
- Relevant log output (`/tmp/cycle_power.log`, PipeWire journal)

---

## Related Pages

- [Audio System](Audio-System.md) — audio evolution and current implementation
- [Power Management](Power-Management.md) — battery and profile details
- [Roadmap](Roadmap.md) — planned fixes for open items

[← Home](Home.md)
