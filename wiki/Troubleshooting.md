# Troubleshooting Guide

This guide provides solutions for common issues encountered during **ISO building**, **Virtual Machine testing**, **Wayland session execution**, **Calamares installation**, and **custom daemon operations**.

---

## Quick Diagnostic Index

- [ISO Build Failures](#iso-build-failures)
- [Virtual Machine Issues (VirtualBox & QEMU)](#virtual-machine-issues)
- [Calamares Installer Issues](#calamares-installer-issues)
- [Hyprland & Wayland Issues](#hyprland--wayland-issues)
- [Live ISO Boot & Immutability Guard Issues](#live-iso-boot--immutability-guard-issues)
- [Titan Hardware Manager (`titan-hwm`) Issues](#titan-hardware-manager-issues)
- [Titan Sandbox Issues](#titan-sandbox-issues)

---

## ISO Build Failures

### 1. `pacstrap` Mirror Errors or Timeout

**Symptom**: `mkarchiso` fails during the initial package download step with HTTP 404 or connection reset errors.

**Cause**: Stale host mirror list or network timeout during package retrieval.

**Solution**:
1. Update your host mirror list before running `mkarchiso`:
   ```bash
   sudo reflector --latest 10 --protocol https --sort rate --save /etc/pacman.d/mirrorlist
   ```
2. Verify that `/etc/pacman.conf` and `pacman.conf` in the repo root have valid mirrors.

---

### 2. Qt6 / WebEngine Build Failure During `airootfs` Overlay

**Symptom**: `mkarchiso` exits when building TitanFetch, TitanBrowser, or ArchTitan Settings with error `Qt6::Widgets` or `Qt6::WebEngineCore not found`.

**Cause**: Missing Qt6 development headers on host system.

**Solution**:
Install Qt6 base, Qt6 WebEngine, and CMake packages on the host machine:
```bash
sudo pacman -S qt6-base qt6-webengine cmake base-devel
```

---

### 3. SquashFS Memory Exhaustion (OOM During Compression)

**Symptom**: `mkarchiso` crashes during the SquashFS compression stage with `Out of memory` or `Killed`.

**Cause**: `mksquashfs` attempting to use all available CPU cores and excessive thread memory on a system with limited RAM.

**Solution**:
Ensure `profiledef.sh` restricts `-processors` option (e.g. `-processors 6` or lower depending on system RAM), or build with an active swap file.

---

## Virtual Machine Issues

### 1. Hyprland Crashes / Black Screen in VirtualBox

**Symptom**: The live ISO boots to GRUB, but starting Hyprland results in a black screen or immediate crash back to tty.

**Cause**: VirtualBox 3D acceleration or graphics controller is misconfigured for Wayland compositors.

**Solution**:
You **must** configure VirtualBox with the following exact settings before launching:
- **System -> Motherboard**: Check `Enable EFI (special OSes only)`
- **Display -> Screen**: Set Video Memory to `128 MB`
- **Display -> Screen**: Check `Enable 3D Acceleration`
- **Display -> Screen**: Set Graphics Controller to `VMSVGA`

---

### 2. QEMU Boot Failure (`No bootable device`)

**Symptom**: `./run-vm.sh` fails to boot the ISO in QEMU.

**Cause**: Missing OVMF UEFI firmware files on host.

**Solution**:
Install the `edk2-ovmf` package:
```bash
sudo pacman -S edk2-ovmf qemu-desktop
```

---

## Calamares Installer Issues

### 1. Calamares Fails to Launch on Live ISO (Display Permission Error)

**Symptom**: Clicking **Install ArchTitan OS** or running `sudo calamares` emits `qt.qpa.xcb: could not connect to display` or `Could not connect to Wayland socket`.

**Cause**: Calamares running as root cannot access the unprivileged live user's Wayland/XWayland display socket.

**Solution**:
Launch Calamares using the built-in helper script, which handles display permissions automatically:
```bash
launch-installer
```
The helper exports `XDG_RUNTIME_DIR=/run/user/1000` and configures `xhost +local:root` automatically.

---

## Live ISO Boot & Immutability Guard Issues

### 1. Live ISO Boot Delay (Hangs for 3–5 Minutes before Plymouth/Hyprland)

**Symptom**: ISO boot hangs for up to 5 minutes on a black screen or early systemd targets after selecting an entry in GRUB before Plymouth or Hyprland starts.

**Cause**:
1. `systemd-time-wait-sync.service` was linked inside `sysinit.target.wants`, forcing early boot (`sysinit.target`) to pause until Network Time Protocol (NTP) synchronized over the network.
2. `archtitan-immutable-guard.service` was placed in `sysinit.target.wants` and called `systemctl mask --now` before D-Bus (`dbus.service`) was running.
3. GPG keyring initialization (`pacman-key --init`) stalled during live boot in virtual machines due to `/dev/random` entropy exhaustion.

**Solution**:
1. **Unlinked Time Wait Sync**: Removed `systemd-time-wait-sync.service` from `airootfs/etc/systemd/system/sysinit.target.wants/` so clock sync does not block early boot.
2. **Deferred Immutability Guard**: Moved `archtitan-immutable-guard.service` to `multi-user.target` so it runs after D-Bus initialization without delaying early boot.
3. **Entropy Daemon (`haveged`)**: Added `haveged` to `packages.x86_64` and enabled `haveged.service` in `multi-user.target.wants` to supply instant entropy for GPG initialization in VMs.

---

### 2. Custom GRUB Highlight Cyan Box or Text Formatting Issues

**Symptom**: GRUB selection bar appears as an opaque cyan block (`black/cyan`) with graphical layout glitches.

**Cause**: Experimental `gfxterm` color definitions (`set menu_color_highlight=black/cyan`) overriding default GRUB text console mode.

**Solution**:
Reverted `grub/grub.cfg` to clean standard default `archiso` GRUB configuration:
- `terminal_output console`
- `gfxmode="auto"`
- Default GRUB menu styling without custom cyan overrides.

---

## Hyprland & Wayland Issues

### 1. Hardware Cursor Invisible or Glitching in VMs

**Symptom**: Mouse cursor is invisible or leaves artifacts in QEMU / VirtualBox under Hyprland 0.56+.

**Cause**: Hardware cursor planes unsupported by virtualized display drivers (Aquamarine backend).

**Solution**:
Ensure the following settings are active in `~/.config/hypr/hyprland.conf`:
```ini
env = AQ_NO_MODIFIERS,1

cursor {
    no_hardware_cursors = true
}
```

---

## Titan Hardware Manager Issues

### 1. Daemon Socket Not Found (`/tmp/titan_hwm.sock missing`)

**Symptom**: Running `titan-hwm status` displays:
```
titan-hwm: daemon socket not found at /tmp/titan_hwm.sock
           Is titan_hw_manager running?
```

**Cause**: The `titan_hw_manager.service` is stopped or failed to start.

**Solution**:
1. Check the systemd service status:
   ```bash
   systemctl status titan_hw_manager
   ```
2. View detailed journal logs:
   ```bash
   journalctl -u titan_hw_manager -b --no-pager -n 50
   ```
3. Restart the service:
   ```bash
   sudo systemctl restart titan_hw_manager
   ```

---

## Titan Sandbox Issues

### 1. Application Fails to Launch under Sandbox Policy

**Symptom**: Launching a tool via Rofi fails silently or emits a policy resolution error.

**Cause**: Application binary requested path or device access restricted by security policy.

**Solution**:
1. Inspect the sandbox execution log:
   ```bash
   tail -n 50 /var/log/titan-sandbox/sandbox.log
   ```
2. Test running under a specific policy manually to see stderr:
   ```bash
   titan-sandboxd --policy /etc/titan-sandbox/policies/system-dev.toml -- /usr/bin/<app-name>
   ```
3. Adjust the corresponding policy file in `/etc/titan-sandbox/policies/` to grant necessary read/write paths.
