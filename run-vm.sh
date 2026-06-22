#!/bin/bash
# run-vm.sh — Launch ArchTitan in QEMU/KVM for testing
# Usage:
#   ./run-vm.sh         → boots ISO (fresh install each time)
#   ./run-vm.sh --disk  → boots from qcow2 disk (installed system)
#   ./run-vm.sh --fresh → wipes qcow2 and boots ISO for clean install

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ISO="$SCRIPT_DIR/out/archtitan-2026.06.20-x86_64.iso"
DISK="$SCRIPT_DIR/archtitan-drive.qcow2"
OVMF_CODE="/usr/share/OVMF/OVMF_CODE.fd"
OVMF_VARS="$SCRIPT_DIR/OVMF_VARS.fd"

# Fallback OVMF paths for different Arch packages
for f in \
    "/usr/share/OVMF/OVMF_CODE.fd" \
    "/usr/share/ovmf/OVMF.fd" \
    "/usr/share/edk2/x64/OVMF_CODE.fd"; do
    [ -f "$f" ] && OVMF_CODE="$f" && break
done

# ── Flags ────────────────────────────────────────────────────────────────────
BOOT_ISO=true

case "${1:-}" in
    --disk)
        BOOT_ISO=false
        echo "[run-vm] Booting from installed disk: $DISK"
        ;;
    --fresh)
        echo "[run-vm] Wiping disk and starting fresh install..."
        qemu-img create -f qcow2 "$DISK" 60G
        echo "[run-vm] Created fresh 60G qcow2 at $DISK"
        BOOT_ISO=true
        ;;
    "")
        echo "[run-vm] Booting ISO for installation: $ISO"
        ;;
    *)
        echo "Usage: $0 [--disk|--fresh]"
        echo "  (no args)  Boot from ISO"
        echo "  --disk     Boot from installed qcow2 disk"
        echo "  --fresh    Wipe disk + boot ISO (clean reinstall)"
        exit 1
        ;;
esac

# ── Sanity checks ────────────────────────────────────────────────────────────
if [ ! -f "$OVMF_CODE" ]; then
    echo "[run-vm] ERROR: OVMF not found. Install with:"
    echo "         sudo pacman -S edk2-ovmf"
    exit 1
fi

if [ ! -f "$DISK" ]; then
    echo "[run-vm] No disk image found — creating 60G qcow2..."
    qemu-img create -f qcow2 "$DISK" 60G
fi

if $BOOT_ISO && [ ! -f "$ISO" ]; then
    echo "[run-vm] ERROR: ISO not found at $ISO"
    echo "         Run: sudo mkarchiso -v -w /tmp/archiso-work -o out ."
    exit 1
fi

# ── KVM check ────────────────────────────────────────────────────────────────
KVM_FLAG=""
if [ -w /dev/kvm ]; then
    KVM_FLAG="-enable-kvm -cpu host"
    echo "[run-vm] KVM acceleration: ON"
else
    echo "[run-vm] WARNING: /dev/kvm not accessible — running without KVM (slow)"
    echo "         Fix: sudo usermod -aG kvm $USER && newgrp kvm"
    KVM_FLAG="-cpu qemu64"
fi

# ── Build QEMU args ──────────────────────────────────────────────────────────
QEMU_ARGS=(
    $KVM_FLAG
    -m 4G
    -smp 4
    -machine type=q35,smm=on

    # UEFI firmware
    -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE"
    -drive if=pflash,format=raw,file="$OVMF_VARS"

    # Main disk
    -drive file="$DISK",format=qcow2,if=virtio,cache=writeback

    # Display — virtio-gpu for better performance (Hyprland/Wayland)
    -device virtio-gpu-pci
    -display gtk,gl=on

    # Audio
    -audiodev pa,id=snd0
    -device intel-hda
    -device hda-duplex,audiodev=snd0

    # Network — user mode (internet works, no bridging needed)
    -netdev user,id=net0
    -device virtio-net-pci,netdev=net0

    # USB for input
    -device qemu-xhci
    -device usb-tablet

    # Better performance
    -object memory-backend-memfd,id=mem,size=4G,share=on
    -numa node,memdev=mem
)

# Boot ISO or disk
if $BOOT_ISO; then
    QEMU_ARGS+=(
        -drive file="$ISO",format=raw,if=none,id=iso,readonly=on
        -device virtio-blk-pci,drive=iso
        -boot d
    )
fi

echo ""
echo "══════════════════════════════════════════════"
echo "  ArchTitan VM Starting"
echo "  RAM:  4GB  |  CPUs: 4  |  Disk: 60GB"
echo "  UEFI: $OVMF_CODE"
if $BOOT_ISO; then
echo "  Boot: ISO → install with Calamares"
else
echo "  Boot: Installed disk"
fi
echo "══════════════════════════════════════════════"
echo ""

exec qemu-system-x86_64 "${QEMU_ARGS[@]}"
