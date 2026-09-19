#!/usr/bin/env bash
# ==============================================================================
# ArchTitan OS - Test SwayNC Control Center on Host Desktop
# ==============================================================================

set -e

echo "============================================================"
echo "  🌌 ArchTitan Quick Settings & Notification Center Test"
echo "============================================================"

export PATH="$HOME/.local/bin:$PATH"

# Verify swaync is present
if ! command -v swaync >/dev/null 2>&1; then
    echo "[!] Error: swaync binary not found in PATH ($HOME/.local/bin)."
    exit 1
fi

# Check if QuickShell or another notification daemon is holding D-Bus
QS_PID=$(pgrep -x qs 2>/dev/null || true)

if [ -n "$QS_PID" ]; then
    echo ">> Detected running QuickShell (PID: $QS_PID) holding notification bus."
    echo ">> Temporarily stopping QuickShell for this test session..."
    killall qs 2>/dev/null || true
    sleep 1.2
fi

echo ">> Starting ArchTitan SwayNC daemon..."
swaync >/dev/null 2>&1 &
SWAYNC_PID=$!
sleep 1.5

echo ">> Sliding in ArchTitan Quick Settings panel..."
swaync-client -t -sw

echo ">> Dispatching test notifications (matching reference screenshot)..."
notify-send -u low -i network-wireless "Network Management" "No Network Connection: You are no longer connected to a network."
notify-send -u normal -i preferences-system "notify-send" "THM: Reclaiming idle workload \"1387655\" is idle..."
notify-send -u normal -i terminal "Antigravity IDE" "Antigravity IDE: Host System Troubleshooting Assistant..."
notify-send -u normal -i display "Hyprland" "Exited Virtual Machine submap: Keybinds re-enabled."

echo ""
echo "✨ ArchTitan Control Center is now visible on the right of your desktop!"
echo "   - Test the Quick Toggles: Wi-Fi, Bluetooth, Caffeine, Audio, Night Light, DND."
echo "   - Test the Squircle Grid: Game Mode, Snip, Color Picker, Music."
echo "   - Test Grouped Notifications: expand, dismiss, or click '󰆴 Clear All'."
echo ""
read -rp "Press [ENTER] to conclude test and restore your previous desktop shell... "

echo ">> Stopping SwayNC daemon..."
killall swaync 2>/dev/null || true
sleep 0.8

if [ -n "$QS_PID" ]; then
    echo ">> Restoring QuickShell (qs -c ii)..."
    qs -c ii >/dev/null 2>&1 &
    disown
    echo ">> QuickShell successfully restored!"
fi

echo ">> Done!"
