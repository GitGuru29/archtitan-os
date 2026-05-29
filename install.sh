#!/bin/bash
# install.sh - Automates building and installing the Titan Hardware Manager

set -e

echo "[*] Compiling Titan Hardware Manager..."
g++ -std=c++17 -O3 -Wall -o titan_hw_manager titan_hw_manager.cpp

echo "[*] Stopping existing service if running..."
sudo systemctl stop titan_hw_manager.service || true

echo "[*] Installing binary to /usr/local/bin..."
sudo cp titan_hw_manager /usr/local/bin/
sudo chmod +x /usr/local/bin/titan_hw_manager

echo "[*] Installing systemd service..."
sudo cp titan_hw_manager.service /etc/systemd/system/

echo "[*] Reloading systemd daemon..."
sudo systemctl daemon-reload

echo "[*] Enabling and starting Titan Hardware Manager service..."
sudo systemctl enable --now titan_hw_manager.service

echo "[*] Installation complete! The daemon is now running in the background."
echo "[*] You can check its status with: systemctl status titan_hw_manager"
