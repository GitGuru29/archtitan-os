# Installation Guide

ArchTitan ships as a UEFI-only live ISO. Installation uses **Calamares** — a graphical installer preconfigured for ArchTitan branding, BTRFS/ext4 partitioning, and desktop customization.

---

## Before You Begin

- **Firmware:** UEFI boot required (Legacy BIOS is not supported by this profile).
- **Target disk:** Back up existing data. Installation will partition and format selected disks.
- **Network:** Ethernet or Wi-Fi via NetworkManager (available in live session).

---

## Create Boot Media

### Linux

```bash
# Replace /dev/sdX with your USB device — NOT a partition
sudo dd if=out/archtitan-YYYY.MM.DD-x86_64.iso of=/dev/sdX bs=4M status=progress oflag=sync
```

### Windows / macOS

Use [Balena Etcher](https://etcher.balena.io/) or [Rufus](https://rufus.ie/) in **DD/ISO mode** (not Windows-to-Go).

---

## Live Session

1. Boot from USB in UEFI mode.
2. SDDM loads the ArchTitan theme; the live user session starts Hyprland automatically.
3. You can launch Calamares anytime with **Super + I** or via the desktop shortcut **Install ArchTitan OS**.

### Live session notes

- **Screen lock is disabled** on the live ISO (`Super + L` does nothing) to prevent lockout without a known password.
- Default live credentials follow archiso conventions (`archtitan:archtitan`).
- `sudo` is passwordless for the live user.
- **`launch-installer` Helper**: Launching Calamares executes `/usr/local/bin/launch-installer`, which automatically detects the active Wayland socket, sets `XDG_RUNTIME_DIR`, grants `xhost` display access for root, and invokes Calamares under appropriate display privileges.

---

## Calamares Installation Steps

| Step | What to expect |
| :--- | :--- |
| **Welcome** | ArchTitan branding, language selection |
| **Location** | Timezone, locale, keyboard |
| **Keyboard** | Layout confirmation |
| **Partitions** | Manual or guided partitioning (BTRFS supported with `@` subvolumes) |
| **Users** | Create username, password, hostname |
| **Summary** | Review before install |
| **Install** | Copy squashfs, configure bootloader, mkinitcpio |

After completion, reboot and remove the USB drive.

---

## Post-Install First Boot

1. SDDM presents the ArchTitan login theme.
2. Hyprland starts with default user configuration from `/etc/skel/`.
3. Titan Hardware Manager enables on graphical session start.
4. Copy or customize configs in `~/.config/hypr/`, `~/.config/waybar/`, `~/.config/titan-hwm/`.
5. Launch **ArchTitan Settings** (<kbd>Super</kbd> + <kbd>Space</kbd> -> search `titan-settings`) to configure monitor layouts, themes, and THM profiles.

### Optional: Android development stack

Run the bundled helper (requires AUR access):

```bash
dev-setup
```

This installs `yay-bin` and Android Studio via AUR. Expect a long first run on slow networks.

---

## Virtual Machines

### QEMU/KVM (recommended for development)

The repo includes `run-vm.sh`, which automatically detects the newest ISO in `out/` or `Downloads/`:

```bash
./run-vm.sh          # Boot ISO — install to qcow2
./run-vm.sh --disk   # Boot installed system
./run-vm.sh --fresh  # Wipe qcow2 and reinstall
```

**Requirements:**

```bash
sudo pacman -S qemu-system-x86 edk2-ovmf
sudo usermod -aG kvm $USER   # then re-login
```

Default VM spec: 4 GB RAM, 4 vCPUs, 60 GB qcow2, virtio-gpu with GL acceleration.

### VirtualBox

VirtualBox requires explicit settings or Hyprland will fail to start:

| Setting | Value |
| :--- | :--- |
| System → Motherboard → Enable EFI | ✅ Checked |
| Display → Video Memory | 128 MB minimum |
| Display → Enable 3D Acceleration | ✅ Checked |
| Display → Graphics Controller | VMSVGA |

Hyprland automatically detects virtualized environments and applies `AQ_NO_MODIFIERS=1` and `no_hardware_cursors = true` for hardware cursor compatibility. Guest additions (`virtualbox-guest-utils`) are included in `packages.x86_64`.

---

## Partitioning Recommendations

| Use case | Suggested layout |
| :--- | :--- |
| **Simple desktop** | 512 MB EFI + rest ext4 on `/` |
| **Snapshot-friendly** | 512 MB EFI + BTRFS subvolumes (`@`, `@home`, `@snapshots`) |
| **Dual boot** | Shrink Windows partition first; leave EFI entry intact |

Calamares `partition.conf` in `airootfs/etc/calamares/modules/` defines ArchTitan defaults.

---

## Bootloader

GRUB is the default bootloader, installed to the EFI System Partition with graphical theme and unicode font support (`unicode.pf2`). Theme files live in:

- `grub/themes/archtitan/` (ISO)
- `/usr/share/grub/themes/archtitan/` (installed system)

After kernel updates on an installed system:

```bash
sudo mkinitcpio -P
sudo grub-mkconfig -o /boot/grub/grub.cfg
```

---

## Next Steps

- [Desktop Environment](Desktop-Environment) — keybindings, Waybar, and customization
- [Titan Browser](Titan-Browser) — first-party web browser guide
- [ArchTitan Settings](ArchTitan-Settings) — system control center usage
- [Titan Hardware Manager](Titan-Hardware-Manager) — tune workload profiles
- [Troubleshooting](Troubleshooting) — black screen, audio, display socket issues
