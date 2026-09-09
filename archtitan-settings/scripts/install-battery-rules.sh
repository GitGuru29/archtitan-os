#!/usr/bin/env bash
# ArchTitan Settings — Universal Battery Control Permission Installer
# Grants write access to battery charge control sysfs files for the 'users' group.
# Run once with: sudo bash scripts/install-battery-rules.sh

set -e

RULES_FILE="/etc/udev/rules.d/90-archtitan-battery.rules"

echo "[ArchTitan] Installing universal battery control udev rules..."

cat > "$RULES_FILE" << 'EOF'
# ArchTitan Settings — Universal Battery Charge Control
# Grants the 'users' group write access to battery management sysfs files.
# Covers: ThinkPad, ASUS/ROG, Samsung, Framework, Huawei, MSI (charge_control_end_threshold)
#         Lenovo IdeaPad (conservation_mode, rapid_charge)
#         ASUS ROG/TUF newer (charge_mode)

# ThinkPad / ASUS / Samsung / Framework / Huawei / MSI — threshold style
SUBSYSTEM=="power_supply", KERNEL=="BAT*", \
    ATTR{charge_control_end_threshold}=="*", \
    RUN+="/bin/chmod g+w /sys%p/charge_control_end_threshold", \
    RUN+="/bin/chgrp users /sys%p/charge_control_end_threshold"

SUBSYSTEM=="power_supply", KERNEL=="BAT*", \
    ATTR{charge_control_start_threshold}=="*", \
    RUN+="/bin/chmod g+w /sys%p/charge_control_start_threshold", \
    RUN+="/bin/chgrp users /sys%p/charge_control_start_threshold"

# Lenovo IdeaPad — conservation_mode and rapid_charge
SUBSYSTEM=="platform", DRIVER=="ideapad_acpi", \
    ATTR{conservation_mode}=="*", \
    RUN+="/bin/chmod g+w /sys%p/conservation_mode", \
    RUN+="/bin/chgrp users /sys%p/conservation_mode"

SUBSYSTEM=="platform", DRIVER=="ideapad_acpi", \
    ATTR{rapid_charge}=="*", \
    RUN+="/bin/chmod g+w /sys%p/rapid_charge", \
    RUN+="/bin/chgrp users /sys%p/rapid_charge"

# ASUS ROG/TUF — charge_mode (3-way)
SUBSYSTEM=="platform", DRIVER=="asus-wmi", \
    ATTR{charge_mode}=="*", \
    RUN+="/bin/chmod g+w /sys%p/charge_mode", \
    RUN+="/bin/chgrp users /sys%p/charge_mode"

SUBSYSTEM=="platform", DRIVER=="asus-nb-wmi", \
    ATTR{charge_mode}=="*", \
    RUN+="/bin/chmod g+w /sys%p/charge_mode", \
    RUN+="/bin/chgrp users /sys%p/charge_mode"
EOF

echo "[ArchTitan] Rules written to $RULES_FILE"

# Apply permissions immediately to any currently-existing paths
apply_if_exists() {
    local path="$1"
    if [ -f "$path" ]; then
        chgrp users "$path" 2>/dev/null && chmod g+w "$path" 2>/dev/null && \
            echo "  [OK] $path" || echo "  [SKIP] $path (could not set)"
    fi
}

echo "[ArchTitan] Applying permissions immediately..."

# ThinkPad / ASUS / Samsung / Framework / Huawei
for bat in /sys/class/power_supply/BAT*; do
    apply_if_exists "$bat/charge_control_end_threshold"
    apply_if_exists "$bat/charge_control_start_threshold"
done

# Lenovo IdeaPad / ASUS ROG via platform devices
for dev in /sys/bus/platform/devices/*; do
    apply_if_exists "$dev/conservation_mode"
    apply_if_exists "$dev/rapid_charge"
    apply_if_exists "$dev/charge_mode"
done

# Also try resolved symlink paths
for f in $(find /sys/devices -maxdepth 8 \
    \( -name "conservation_mode" -o -name "rapid_charge" \
    -o -name "charge_mode" -o -name "charge_control_end_threshold" \) \
    2>/dev/null); do
    apply_if_exists "$f"
done

udevadm control --reload-rules 2>/dev/null && \
udevadm trigger 2>/dev/null && \
echo "[ArchTitan] udev rules reloaded." || true

echo ""
echo "[ArchTitan] Done! Battery control is now available without root."
echo "            Supported hardware will be auto-detected at next app launch."
echo "            Log out and back in if controls are still greyed out."
