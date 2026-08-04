#!/usr/bin/env bash
# Run this in your terminal: bash rebuild.sh
set -e
cd "$(dirname "$0")"

LOGO_ASSET="assets/icons/LOGO.png"
LOGO_SOURCE="$HOME/Downloads/LOGO.png"

# Only copy the real ArchTitan logo if the asset is missing
if [ ! -f "$LOGO_ASSET" ] || [ ! -s "$LOGO_ASSET" ]; then
    if [ -f "$LOGO_SOURCE" ]; then
        echo "Copying ArchTitan logo from Downloads..."
        cp "$LOGO_SOURCE" "$LOGO_ASSET"
    else
        echo "WARNING: $LOGO_SOURCE not found. Logo asset is missing."
    fi
else
    echo "Logo asset already present: $LOGO_ASSET"
fi
# Remove black background from performance icon if it hasn't been generated
if [ ! -f "assets/icons/performance_nobg.png" ] && [ -f "assets/icons/performance.png" ]; then
    echo "Running background removal for performance icon..."
    python3 remove_bg.py
fi



mkdir -p build
cd build
rm -rf .rcc
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -Wno-dev
make -j$(nproc)
echo "BUILD DONE"
echo "Run: cd $(dirname "$0")/build && ./archtitan-settings"
