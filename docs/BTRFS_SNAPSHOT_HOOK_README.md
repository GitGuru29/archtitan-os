# BTRFS Automatic Snapshot Hook — Developer & User Guide

The **BTRFS Automatic Snapshot Hook** feature provides automated pre-update recovery and bootloader integration for ArchTitan OS.

---

## How It Works

1. **Automatic Pre-Transaction Trigger**:
   Whenever `pacman` or `yay` performs an install, upgrade, or remove operation, pacman triggers `/etc/pacman.d/hooks/50-archtitan-btrfs-pre-snapshot.hook`.

2. **Safe Snapshot Generation**:
   The hook runs `/usr/local/bin/archtitan-btrfs-snapshot pre`. If the system is running on a BTRFS root filesystem, a read-only snapshot of `/` is saved to `/.snapshots/@pre-pacman-YYYYMMDD-HHMMSS`.

3. **Smart Retention & Rotation**:
   To prevent disk space exhaustion, ArchTitan OS automatically keeps the **10 most recent** pre-update snapshots and safely deletes older snapshots.

4. **Instant Bootable Rollback in GRUB**:
   The `grub-btrfs` daemon automatically detects new pre-update snapshots and exposes them on the GRUB boot menu under **ArchTitan Snapshots / Rollback**. If a system update breaks a driver or service, rebooting into a pre-update snapshot restores system operation immediately.

---

## File Map

| Path | Role |
| :--- | :--- |
| `airootfs/etc/pacman.d/hooks/50-archtitan-btrfs-pre-snapshot.hook` | Pacman pre-transaction trigger hook |
| `airootfs/usr/local/bin/archtitan-btrfs-snapshot` | Snapshot creation, rotation, and GRUB sync script |
| `profiledef.sh` | Sets executable `755` permissions for the script |
| `packages.x86_64` | Manifest including `btrfs-progs`, `snapper`, and `grub-btrfs` |
| `docs/BTRFS_SNAPSHOT_HOOK_PLAN.md` | Feature architectural design plan |

---

## Testing & Manual Execution

To manually trigger a pre-update snapshot check:

```bash
/usr/local/bin/archtitan-btrfs-snapshot pre
```

To list existing snapshots:

```bash
ls -la /.snapshots/
```
