#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "=== Building ArchTitan Quick Settings & Notification Center ==="
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
cmake ..
make -j"$(nproc)"

echo "=== Installing to airootfs ==="
mkdir -p "${SCRIPT_DIR}/../../airootfs/usr/local/bin"
cp -f "${BUILD_DIR}/titan-quicksettings" "${SCRIPT_DIR}/../../airootfs/usr/local/bin/titan-quicksettings"
chmod +x "${SCRIPT_DIR}/../../airootfs/usr/local/bin/titan-quicksettings"

echo "✓ Titan Quick Settings successfully built and installed to airootfs."
