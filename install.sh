#!/bin/bash
# install.sh — Build and install Titan Hardware Manager
# Compiles both the daemon and the native CLI binary.

set -e

echo "[*] Compiling Titan Hardware Manager daemon..."
g++ -std=c++17 -O3 -Wall -o titan_hw_manager titan_hw_manager.cpp

echo "[*] Compiling Titan HWM CLI (titan-hwm)..."
g++ -std=c++17 -O2 -Wall -o titan-hwm titan_hwm_cli.cpp

echo "[*] Stopping existing service if running..."
sudo systemctl stop titan_hw_manager.service 2>/dev/null || true

echo "[*] Installing binaries to /usr/local/bin/..."
sudo cp titan_hw_manager  /usr/local/bin/titan_hw_manager
sudo cp titan-hwm          /usr/local/bin/titan-hwm
sudo cp airootfs/usr/local/bin/titan-hwm-waybar /usr/local/bin/titan-hwm-waybar
sudo chmod +x /usr/local/bin/titan_hw_manager \
              /usr/local/bin/titan-hwm \
              /usr/local/bin/titan-hwm-waybar

echo "[*] Installing systemd service..."
sudo cp titan_hw_manager.service /etc/systemd/system/

echo "[*] Reloading systemd and enabling service..."
sudo systemctl daemon-reload
sudo systemctl enable --now titan_hw_manager.service

echo ""
echo "✅ Installation complete!"
echo "   Daemon status : systemctl status titan_hw_manager"
echo "   CLI usage     : titan-hwm switch web|android|system|casual"
echo "   Live status   : titan-hwm status"
echo "   Telemetry     : titan-hwm metrics"
