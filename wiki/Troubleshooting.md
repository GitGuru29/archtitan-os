# Troubleshooting Guide

This guide provides solutions for common issues encountered during **ISO building**, **Virtual Machine testing**, **Wayland session execution**, and **custom daemon operations**.

---

## Quick Diagnostic Index

- [ISO Build Failures](#iso-build-failures)
- [Virtual Machine Issues (VirtualBox & QEMU)](#virtual-machine-issues)
- [Hyprland & Wayland Issues](#hyprland--wayland-issues)
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

### 2. Qt6 / CMake Build Failure During `airootfs` Overlay

**Symptom**: `mkarchiso` exits when building TitanFetch with error `Qt6::Widgets not found`.

**Cause**: Missing Qt6 development headers on host system.

**Solution**:
Install Qt6 base and CMake packages on the host machine:
```bash
sudo pacman -S qt6-base cmake base-devel
```

---

### 3. Permission Denied or Stale Build Workspace

**Symptom**: Build fails with `Permission denied` when writing to `out/` or `tmp-work/`.

**Cause**: Previous interrupted build left root-owned temporary files.

**Solution**:
Clean previous artifacts completely before starting a build:
```bash
sudo rm -rf tmp-work out
```

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

## Hyprland & Wayland Issues

### 1. NVIDIA GPU Screen Tear or Unresponsive Windows

**Symptom**: Running ArchTitan on a physical NVIDIA GPU shows flickering or unresponsive windows.

**Cause**: DRM mode-setting is disabled or missing Wayland environment flags.

**Solution**:
1. Ensure `nvidia_drm.modeset=1` is passed in boot parameters.
2. In `~/.config/hypr/hyprland.conf`, verify the following variables:
   ```ini
   env = LIBVA_DRIVER_NAME,nvidia
   env = XDG_SESSION_TYPE,wayland
   env = GBM_BACKEND,nvidia-drm
   env = __GLX_VENDOR_LIBRARY_NAME,nvidia
   env = NVD_BACKEND,direct
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

### 2. cgroup v2 Controllers Missing

**Symptom**: `titan_hw_manager` logs error `cgroup v2 controllers unavailable`.

**Cause**: System booted with legacy v1 cgroups or unified hierarchy is disabled in kernel command line.

**Solution**:
Verify that `/sys/fs/cgroup/cgroup.controllers` exists:
```bash
cat /sys/fs/cgroup/cgroup.controllers
```
If missing, ensure your bootloader does not pass `systemd.unified_cgroup_hierarchy=0`.

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
