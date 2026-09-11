// =============================================================================
// titan-hwm-v3/core/protected_registry.hpp
// Phase 0 — Protected Domain Registry
//
// Hard block: any PID that returns true from is_protected() MUST NOT receive
// SIGSTOP, SIGTERM, SIGKILL, or any cgroup enforcement action. This check is
// mandatory at the top of every enforcement path before any other logic runs.
// =============================================================================
#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <sys/types.h>

namespace thm {

class ProtectedRegistry {
public:
    // ─── Query ───────────────────────────────────────────────────────────────

    // Returns true if this pid/comm combination must never be signalled or frozen.
    // Checks in order: explicit PID registration, name set, exe path prefix.
    bool is_protected(pid_t pid, const std::string& comm) const;

    // Convenience: returns true if ANY pid in the range is protected.
    bool is_protected_any(const std::vector<pid_t>& pids) const;

    // ─── Registration ────────────────────────────────────────────────────────

    // Explicitly protect a PID (e.g. THM itself at startup, TitanAI PID).
    void register_pid(pid_t pid, const std::string& reason);

    // Remove a PID from the explicit registry (e.g. process exited cleanly).
    void unregister_pid(pid_t pid);

    // Scan /proc for known Titan service units and pre-populate the PID registry.
    // Called once at startup after systemd units are confirmed running.
    void bootstrap_from_systemd();

private:
    mutable std::mutex          mtx_;

    // Explicitly registered PIDs (THM self + dynamic Titan service PIDs)
    std::unordered_map<pid_t, std::string> explicit_pids_;  // pid → reason

    // Resolve /proc/<pid>/exe and check against protected exe prefixes.
    bool exe_is_protected(pid_t pid) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Protected process name set (comm field from /proc/pid/stat)
// These names are protected regardless of UID or cgroup membership.
// ─────────────────────────────────────────────────────────────────────────────
inline const std::unordered_set<std::string>& protected_names() {
    static const std::unordered_set<std::string> names = {
        // ── THM itself ───────────────────────────────────────────────────────
        "titan_hw_manager", "titan-hwm",

        // ── Titan OS ecosystem ───────────────────────────────────────────────
        "titan-ai",        "titanai",
        "titan-share",     "titanshare",
        "titan-mirror",    "titanmirror",
        "titan-shield",    "titanshield",
        "titan-gpu",       "titangpu",
        "titan-bar",       "titan-bar-daemon",
        "titan-media-hud", "titan-task-manager",

        // ── Audio stack (also covered by AUDIO_WHITELIST + exe verification) ─
        "pipewire",        "pipewire-pulse",
        "wireplumber",     "pulseaudio",

        // ── Bluetooth ────────────────────────────────────────────────────────
        "bluetoothd",      "blueman-applet", "blueman-manager",

        // ── Compositor and display infrastructure ────────────────────────────
        "Hyprland",        "hyprpaper",      "hypridle",   "hyprlock",
        "waybar",          "dunst",          "mako",       "swaync",
        "xdg-desktop-portal", "xdg-desktop-portal-hyprland",
        "xdg-desktop-portal-wlr",

        // ── Session / system services (ppid=1 uid=0 guard still applies) ────
        "systemd",         "systemd-logind",  "dbus-daemon",
        "dbus-broker",     "polkitd",

        // ── Login manager (kept for crash-safety outside session) ────────────
        "sddm",            "greetd",

        // ── Network (disruption would drop AI agent connections) ─────────────
        "NetworkManager",  "systemd-networkd", "wpa_supplicant",
        "iwd",             "dhcpcd",
    };
    return names;
}

// ─────────────────────────────────────────────────────────────────────────────
// Protected executable path prefixes
// Cross-checked against /proc/<pid>/exe to prevent comm spoofing
// ─────────────────────────────────────────────────────────────────────────────
inline const std::vector<std::string>& protected_exe_prefixes() {
    static const std::vector<std::string> prefixes = {
        // Titan OS binaries
        "/usr/local/bin/titan",
        "/usr/lib/titan",
        "/opt/titan",
        // System audio stack
        "/usr/bin/pipewire",
        "/usr/lib/pipewire",
        "/usr/bin/wireplumber",
        "/usr/bin/pulseaudio",
        "/usr/bin/pipewire-pulse",
        // Compositor
        "/usr/bin/Hyprland",
        "/usr/bin/hyprpaper",
        "/usr/bin/waybar",
        // Bluetooth
        "/usr/lib/bluetooth/bluetoothd",
        "/usr/bin/blueman",
        // Session services
        "/usr/lib/systemd/systemd",
        "/usr/bin/dbus-daemon",
        "/usr/bin/dbus-broker",
        "/usr/lib/polkit-1/polkitd",
    };
    return prefixes;
}

} // namespace thm
