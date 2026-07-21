# Building the ISO

ArchTitan is built with the official Arch Linux `archiso` tooling. You need a running Arch Linux (or Arch-based) host with sufficient disk space and network access.

---

## Prerequisites

### Required packages

```bash
sudo pacman -S archiso git base-devel cmake qt6-base
```

| Package | Why |
| :--- | :--- |
| `archiso` | ISO generation (`mkarchiso`) |
| `base-devel` | Compiles THM and TitanFetch during overlay merge |
| `cmake` + `qt6-base` | TitanFetch build dependencies |

### Hardware recommendations

| Resource | Minimum | Recommended |
| :--- | :--- | :--- |
| Free disk | 15 GB | 30 GB+ |
| RAM | 4 GB | 8 GB+ |
| CPU | 2 cores | 4+ cores (SquashFS compression is CPU-heavy) |

---

## Clone & Prepare

```bash
git clone https://github.com/GitGuru29/archtitan-os.git
cd archtitan-os
```

Clean previous artifacts before a fresh build:

```bash
sudo rm -rf tmp-work out
```

---

## Build Command

```bash
sudo mkarchiso -v -w tmp-work/ -o out/ ./
```

### Flags explained

| Flag | Meaning |
| :--- | :--- |
| `-v` | Verbose output |
| `-w tmp-work/` | Build workspace (can be deleted after success) |
| `-o out/` | Output directory for the final ISO |
| `./` | Path to this archiso profile (repo root) |

On success, the ISO appears as:

```
out/archtitan-YYYY.MM.DD-x86_64.iso
```

The date comes from `profiledef.sh` → `iso_version`.

---

## Build Pipeline Internals

```mermaid
flowchart TD
    A[mkarchiso starts] --> B[pacstrap packages.x86_64]
    B --> C[Copy airootfs overlay]
    C --> D[Compile titan_hw_manager + titan-hwm]
    C --> E[Compile titanfetch via CMake/Qt6]
    C --> F[Install systemd units & policies]
    D --> G[Apply profiledef.sh permissions]
    E --> G
    F --> G
    G --> H[mkinitcpio for live image]
    H --> I[SquashFS compress airootfs]
    I --> J[Assemble EFI + GRUB boot structure]
    J --> K[Write ISO to out/]
```

### What `profiledef.sh` controls

- **ISO identity** — name, label, publisher, version string
- **Boot mode** — UEFI via GRUB (`bootmodes=('uefi.grub')`)
- **Compression** — SquashFS with zstd, 2 GB memory budget
- **File permissions** — shadow files, sandbox log dirs, binary modes

---

## Rebuilding Individual Components

During development you can rebuild THM without a full ISO cycle:

```bash
# From repo root
./install.sh
```

This compiles `titan_hw_manager.cpp` and `titan_hwm_cli.cpp`, installs to `/usr/local/bin/`, and enables the systemd service on your **host** system.

For TitanFetch:

```bash
cd titanfetch-src
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cp build/titanfetch /usr/bin/titanfetch
```

---

## Reproducible Builds

Set `SOURCE_DATE_EPOCH` before building to pin timestamps in the ISO label and version:

```bash
export SOURCE_DATE_EPOCH=$(date -d '2026-01-01' +%s)
sudo mkarchiso -v -w tmp-work/ -o out/ ./
```

---

## Verify the ISO

```bash
# Check file type
file out/archtitan-*.iso

# Mount and inspect (optional)
sudo mount -o loop out/archtitan-*.iso /mnt
ls /mnt/arch/
sudo umount /mnt
```

---

## Quick VM Smoke Test

After building:

```bash
./run-vm.sh
```

Requires `qemu-system-x86_64` and `edk2-ovmf`. See [Installation Guide → QEMU](Installation-Guide#qemu-kvm).

---

## Common Build Failures

| Symptom | Likely cause | Fix |
| :--- | :--- | :--- |
| `pacstrap` mirror errors | Stale mirror list | Edit `pacman.conf` or run reflector on host |
| Qt6 not found | Missing dev package | `sudo pacman -S qt6-base cmake` |
| Permission denied on `out/` | Previous root-owned files | `sudo rm -rf out tmp-work` |
| SquashFS OOM | Low RAM during compression | Close apps; reduce `-mem` in `profiledef.sh` or add swap |

More details: [Troubleshooting → Build Issues](Troubleshooting#build-issues).
