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

// ─────────────────────────────────────────────────────────────────────────────
// Collateral-only tier
//
// A browser is a 2-4GB RSS consumer. Registering chrome/brave/firefox in
// protected_names() would hand it permanent OOM immunity, so a runaway browser
// could never be reclaimed and the kernel OOM killer would be forced to take out
// some innocent neighbour instead. Browsers are therefore NOT hard-protected.
//
// They do need protection from one thing: a targeted dev-tool tree walk. Reclaim
// decisions are made per-workload, so
//     node -> playwright/puppeteer -> chromium      (headless test browsers)
//     node -> electron                              (Electron dev shells)
// makes a reclaim decision about the node tree sweep the browser with it, even
// though the reclaim verdict was never about the browser's memory.
//
// This second tier closes that gap without granting memory immunity: it is
// consulted only by the signal/kill path, never by the deprioritize or OOM paths.
// ─────────────────────────────────────────────────────────────────────────────

inline const std::unordered_set<std::string>& browser_names() {
    static const std::unordered_set<std::string> names = {
        "chrome",          "chromium",       "chromium_browser",
        "headless_shell",  "chrome_crashpad_handler",
        "google-chrome",   "google-chrome-stable", "google-chrome-beta",
        "firefox",         "firefox-bin",    "gecko-main",
        "isready",         "Web Content",    "Web Content Process",
        "brave",           "brave-browser",  "brave_crashpad_handler",
        "titan-browser",   "titanbrowser",
        "microsoft-edge",  "msedge",         "opera",
        "vivaldi",         "vivaldi-bin",    "vivaldi-bin-launcher",
    };
    return names;
}

inline const std::vector<std::string>& browser_exe_prefixes() {
    static const std::vector<std::string> prefixes = {
        "/usr/bin/chromium",     "/usr/lib/chromium",    "/usr/lib64/chromium",
        "/opt/chromium",
        "/usr/bin/google-chrome", "/opt/google/chrome",
        "/usr/bin/firefox",       "/usr/lib/firefox",     "/opt/firefox",
        "/usr/bin/brave",         "/usr/lib/brave",       "/opt/brave",
        "/usr/bin/microsoft-edge","/opt/microsoft-edge",
        "/usr/bin/vivaldi",       "/usr/lib/vivaldi",     "/usr/share/vivaldi",
        "/usr/bin/opera",         "/opt/opera",
        "/usr/local/bin/titan-browser", "/usr/bin/titanbrowser",
        // Playwright/Puppeteer keep per-user browser builds outside the package
        // manager, so the executable resolves under the invoking user's cache.
        "/root/.cache/ms-playwright", "/opt/playwright",
        "/usr/lib/brave", "/usr/lib/microsoft-edge",
    };
    return prefixes;
}

class ProtectedRegistry {
public:
    // ─── Query ───────────────────────────────────────────────────────────────

    // Returns true if this pid/comm combination must never be signalled or frozen.
    // Checks in order: explicit PID registration, name set, exe path prefix.
    bool is_protected(pid_t pid, const std::string& comm) const;

    // Convenience: returns true if ANY pid in the range is protected.
    bool is_protected_any(const std::vector<pid_t>& pids) const;

    // ─── Collateral-only tier (see browser_names above) ─────────────────────
    // Returns true if this pid/comm is a browser that must not be swept up by
    // another process's tree operation. This is WEAKER than is_protected() on
    // purpose: consult it from signal/kill paths only. Using it on a
    // deprioritize or OOM path would grant a multi-gigabyte process permanent
    // memory immunity, which is the opposite of the intent.
    bool is_collateral_protected(pid_t pid, const std::string& comm) const;

    // Convenience form, mirroring is_protected_any(). Used by kill_tree().
    bool is_collateral_protected_any(const std::vector<pid_t>& pids) const;

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

    // Resolve /proc/<pid>/exe and check against browser_exe_prefixes().
    bool exe_is_browser(pid_t pid) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Protected process name set (comm field from /proc/pid/stat)
// These names are protected regardless of UID or cgroup membership.
// ─────────────────────────────────────────────────────────────────────────────
inline const std::unordered_set<std::string>& protected_names() {
    static const std::unordered_set<std::string> names = {
        // ── THM itself ───────────────────────────────────────────────────────
        // "titan-hwm" is the v3 daemon (titan-hwm-daemon). "titan_hw_manager" is
        // the removed v1/v2 monolith, kept listed deliberately: an instance
        // installed outside the ISO predating its removal must not be reclaimed
        // by the daemon replacing it.
        "titan_hw_manager", "titan-hwm", "titan-hwm-daemon",

        // ── Titan OS ecosystem ───────────────────────────────────────────────
        "titan-ai",        "titanai",
        "titan-share",     "titanshare",
        "titan-mirror",    "titanmirror",
        "titan-shield",    "titanshield",
        "titan-gpu",       "titangpu",
        "titan-bar",       "titan-bar-daemon",
        "titan-media-hud", "titan-task-manager",

        // ── Audio server stack ────────────────────────────────────────────────
        "pipewire",        "pipewire-pulse",
        "wireplumber",     "pulseaudio",

        // ── Media players ─────────────────────────────────────────────────────
        // The audio server surviving is useless if the player that feeds it is
        // reclaimed, so players are protected as first-class as pipewire.
        "spotify",         "spotifyd",     "mpd",           "mpdris2",
        "mpv",             "vlc",          "rhythmbox",     "strawberry",
        "deadbeef",        "cmus",         "ncmpcpp",       "cantata",
        "audacious",       "elisa",        "playerctld",

        // Flatpak / portal comm variants. A sandboxed player reports the
        // application ID as its comm, not the binary name, so without these
        // the native names above never match for Flatpak installs.
        "com.spotify.Client", "io.mpv.Mpv", "org.videolan.VLC",

        // ── Bluetooth ────────────────────────────────────────────────────────
        // bluetoothd only. The kernel-side "bluetooth" comm is deliberately
        // NOT listed: kernel threads have no /proc/<pid>/exe, so is_protected()
        // could never satisfy its exe cross-check and would log a spoof warning
        // on every tick. It is also unfreezable by definition. The userspace
        // routing daemon is the process whose suspension actually drops BT audio.
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
        "/usr/bin/titan",
        "/usr/local/bin/archtitan",
        "/usr/bin/archtitan",
        "/usr/lib/titan",
        "/opt/titan",
        // Desktop notification & shell daemons
        "/usr/bin/swaync",
        "/usr/bin/dunst",
        "/usr/bin/mako",
        "/usr/bin/swaylock",
        "/usr/lib/xdg-desktop-portal",
        // Network daemons
        "/usr/bin/NetworkManager",
        "/usr/bin/wpa_supplicant",
        "/usr/bin/iwd",
        "/usr/bin/sddm",
        "/usr/bin/greetd",
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
        // Media players
        "/usr/bin/spotify",   "/usr/lib/spotify",  "/opt/spotify",
        "/usr/bin/spotifyd",  "/usr/bin/mpd",      "/usr/bin/mpdris2",
        "/usr/bin/mpv",       "/usr/lib/mpv",      "/usr/bin/vlc",
        "/usr/lib/vlc",       "/usr/bin/rhythmbox","/usr/bin/strawberry",
        "/usr/bin/deadbeef",  "/usr/bin/cmus",     "/usr/bin/ncmpcpp",
        "/usr/bin/cantata",   "/usr/bin/audacious","/usr/bin/elisa",
        "/usr/bin/playerctld",
        // Flatpak sandbox mounts; comm is the app ID but exe resolves to these
        "/app/bin/spotify",   "/app/bin/vlc",      "/app/bin/mpv",
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
