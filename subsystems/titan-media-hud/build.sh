#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "=== Building ArchTitan Media HUD ==="
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
cmake ..
make -j"$(nproc)"

echo "=== Installing to airootfs ==="
mkdir -p "${SCRIPT_DIR}/../../airootfs/usr/local/bin"
cp -f "${BUILD_DIR}/titan-media-hud" "${SCRIPT_DIR}/../../airootfs/usr/local/bin/titan-media-hud"
chmod +x "${SCRIPT_DIR}/../../airootfs/usr/local/bin/titan-media-hud"

echo "✓ Titan Media HUD successfully built and installed to airootfs."
