#!/usr/bin/env bash
# cycle_power.sh
# Cycles: Power Saver -> Balanced -> Performance -> Power Saver
# Uses net.hadess.PowerProfiles (confirmed working on this system)

CURRENT=$(busctl get-property net.hadess.PowerProfiles \
    /net/hadess/PowerProfiles \
    net.hadess.PowerProfiles \
    ActiveProfile 2>/dev/null | awk -F'"' '{print $2}')

# Debug — log to a temp file so we can verify it's reading correctly
echo "[cycle_power] Current: '$CURRENT' at $(date)" >> /tmp/cycle_power.log

case "$CURRENT" in
    "power-saver")
        NEXT="balanced"
        LABEL="Balanced"
        ;;
    "balanced")
        NEXT="performance"
        LABEL="Performance"
        ;;
    *)
        # performance OR empty/unknown → go to power-saver
        NEXT="power-saver"
        LABEL="Power Saver"
        ;;
esac

echo "[cycle_power] Switching to: '$NEXT'" >> /tmp/cycle_power.log

busctl set-property net.hadess.PowerProfiles \
    /net/hadess/PowerProfiles \
    net.hadess.PowerProfiles \
    ActiveProfile s "$NEXT" 2>&1 | tee -a /tmp/cycle_power.log

notify-send -a "Power Manager" "Power Profile" "Switched to $LABEL" -t 2000
