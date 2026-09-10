#!/bin/bash
# install.sh — Build and install Titan Hardware Manager v3
# Builds the C++20 daemon via CMake and installs systemd slice and service.

set -e

echo "[*] Compiling Titan Hardware Manager v3 daemon..."
cd titan-hwm-v3
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
cd ..

echo "[*] Stopping existing service if running..."
sudo systemctl stop titan-hwm.service 2>/dev/null || sudo systemctl stop titan_hw_manager.service 2>/dev/null || true

echo "[*] Installing binaries to /usr/local/bin/..."
sudo cp titan-hwm-v3/build/titan-hwm-daemon /usr/local/bin/titan-hwm-daemon
sudo cp airootfs/usr/local/bin/titan-hwm          /usr/local/bin/titan-hwm
sudo cp airootfs/usr/local/bin/titan-hwm-waybar   /usr/local/bin/titan-hwm-waybar
sudo chmod +x /usr/local/bin/titan-hwm-daemon \
              /usr/local/bin/titan-hwm \
              /usr/local/bin/titan-hwm-waybar

echo "[*] Installing systemd slice and service..."
sudo cp titan-hwm-v3/archtitan.slice /etc/systemd/system/archtitan.slice
sudo cp titan-hwm-v3/titan-hwm.service /etc/systemd/system/titan-hwm.service

echo "[*] Reloading systemd and enabling service..."
sudo systemctl daemon-reload
sudo systemctl enable --now titan-hwm.service

echo ""
echo "   Installation complete!"
echo "   Daemon status : systemctl status titan-hwm"
echo "   CLI usage     : titan-hwm switch web|android|system|casual"
echo "   Live status   : titan-hwm status"
echo "   Telemetry     : titan-hwm metrics"

