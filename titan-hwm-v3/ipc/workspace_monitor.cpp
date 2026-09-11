// =============================================================================
// titan-hwm-v3/ipc/workspace_monitor.cpp
// Phase 8 — implementation
// =============================================================================
#include "workspace_monitor.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace fs = std::filesystem;

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// Hyprland IPC socket path: $XDG_RUNTIME_DIR/hypr/$HYPRLAND_INSTANCE_SIGNATURE/.socket2.sock
// ─────────────────────────────────────────────────────────────────────────────
std::string WorkspaceMonitor::hyprland_socket_path() {
    const char* xdg = std::getenv("XDG_RUNTIME_DIR");
    const char* sig = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (!xdg || !sig) return {};
    return std::string(xdg) + "/hypr/" + sig + "/.socket2.sock";
}

// ─────────────────────────────────────────────────────────────────────────────
// start / stop
// ─────────────────────────────────────────────────────────────────────────────
bool WorkspaceMonitor::start() {
    std::string path = hyprland_socket_path();
    if (path.empty()) {
        std::cerr << "[WS] HYPRLAND_INSTANCE_SIGNATURE not set — "
                  << "workspace monitoring disabled.\n";
        return false;
    }
    if (!fs::exists(path)) {
        std::cerr << "[WS] Hyprland socket not found: " << path << "\n";
        return false;
    }
    running_ = true;
    listener_thread_ = std::thread(&WorkspaceMonitor::listen_loop, this);
    std::cout << "[WS] Listening on " << path << "\n";
    return true;
}

void WorkspaceMonitor::stop() {
    running_ = false;
    if (listener_thread_.joinable())
        listener_thread_.join();
}

// ─────────────────────────────────────────────────────────────────────────────
// listen_loop — blocking read from Hyprland socket2 (event stream)
// ─────────────────────────────────────────────────────────────────────────────
void WorkspaceMonitor::listen_loop() {
    std::string path = hyprland_socket_path();

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "[WS] socket() failed: " << std::strerror(errno) << "\n";
        return;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[WS] connect() failed: " << std::strerror(errno) << "\n";
        close(fd);
        return;
    }

    char buf[4096];
    std::string leftover;

    while (running_) {
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) {
            if (errno == EINTR) continue;
            break;
        }
        buf[n] = '\0';
        leftover += buf;

        // Split on newlines (Hyprland events are newline-delimited)
        size_t pos;
        while ((pos = leftover.find('\n')) != std::string::npos) {
            std::string line = leftover.substr(0, pos);
            leftover = leftover.substr(pos + 1);
            if (!line.empty()) handle_event(line);
        }
    }

    close(fd);
}

// ─────────────────────────────────────────────────────────────────────────────
// handle_event — parse Hyprland IPC event lines
//
// Events we care about:
//   workspace>>N         — user switched to workspace N
//   activewindowv2>>ADDR — active window changed
//   activewindow>>CLASS,TITLE — active window class + title
// ─────────────────────────────────────────────────────────────────────────────
void WorkspaceMonitor::handle_event(const std::string& line) {
    auto split = line.find(">>");
    if (split == std::string::npos) return;

    std::string event   = line.substr(0, split);
    std::string payload = line.substr(split + 2);

    if (event == "workspace") {
        int ws_id = 0;
        try { ws_id = std::stoi(payload); } catch (...) { return; }

        int prev = focused_ws_.exchange(ws_id);

        {
            std::lock_guard<std::mutex> lk(ws_mtx_);
            // Mark previous workspace as unfocused
            if (workspaces_.count(prev))
                workspaces_[prev].is_focused = false;
            // Mark new workspace as focused
            workspaces_[ws_id].id = ws_id;
            workspaces_[ws_id].is_focused = true;
        }

        if (ws_cb_) ws_cb_(ws_id, true);
    }
    else if (event == "activewindow") {
        // Format: CLASS,TITLE
        auto comma = payload.find(',');
        if (comma == std::string::npos) return;
        std::string cls   = payload.substr(0, comma);
        std::string title = payload.substr(comma + 1);

        int ws_id = focused_ws_.load();

        {
            std::lock_guard<std::mutex> lk(ws_mtx_);
            workspaces_[ws_id].active_window_class = cls;
            workspaces_[ws_id].active_window_title = title;
        }

        if (win_cb_) win_cb_(ws_id, cls, title);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// snapshot_workspaces — thread-safe copy of current state
// ─────────────────────────────────────────────────────────────────────────────
std::unordered_map<int, WorkspaceState> WorkspaceMonitor::snapshot_workspaces() const {
    std::lock_guard<std::mutex> lk(ws_mtx_);
    return workspaces_;
}

// ─────────────────────────────────────────────────────────────────────────────
// refresh_visibility — call hyprctl to update is_visible flags
// Needed because Hyprland IPC doesn't emit events for split/group visibility.
// ─────────────────────────────────────────────────────────────────────────────
void WorkspaceMonitor::refresh_visibility() {
    // Use hyprctl monitors to get visible workspaces on each monitor
    FILE* pipe = popen("hyprctl monitors -j 2>/dev/null", "r");
    if (!pipe) return;

    char buf[8192];
    std::string json;
    while (fgets(buf, sizeof(buf), pipe))
        json += buf;
    pclose(pipe);

    // Simple JSON parsing for workspace IDs visible on monitors.
    // Format: "activeWorkspace": { "id": N, ... }
    // We don't bring in a JSON library — just grep for the pattern.
    std::lock_guard<std::mutex> lk(ws_mtx_);

    // Reset all visibility flags
    for (auto& [id, ws] : workspaces_)
        ws.is_visible = ws.is_focused; // focused is always visible

    // Find all "activeWorkspace":{"id":N patterns
    std::string key = "\"id\":";
    size_t pos = 0;
    while ((pos = json.find("\"activeWorkspace\"", pos)) != std::string::npos) {
        pos = json.find(key, pos);
        if (pos == std::string::npos) break;
        pos += key.size();
        // Skip whitespace
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
        int ws_id = 0;
        try { ws_id = std::stoi(json.substr(pos)); } catch (...) { continue; }
        workspaces_[ws_id].id = ws_id;
        workspaces_[ws_id].is_visible = true;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// compute_governor — aggregate system demand → governor hint
//
// Rules (from approved plan):
//   - ANY workload in BACKGROUND_EXECUTING + is_building → PERFORMANCE
//   - ANY workload in ACTIVE + latency_sensitive         → PERFORMANCE
//   - ALL idle + NORMAL pressure                         → POWERSAVE
//   - Otherwise                                          → SCHEDUTIL
// ─────────────────────────────────────────────────────────────────────────────
GovernorHint WorkspaceMonitor::compute_governor(
        const std::unordered_map<uint32_t, Workload>& workloads,
        PressureLevel pressure) {

    bool any_building   = false;
    bool any_latency    = false;
    bool all_idle       = true;

    for (const auto& [id, wl] : workloads) {
        if (wl.state == WorkloadState::ACTIVE ||
            wl.state == WorkloadState::BACKGROUND_EXECUTING) {
            all_idle = false;
        }
        if (wl.state == WorkloadState::BACKGROUND_EXECUTING && wl.is_building)
            any_building = true;
        if (wl.state == WorkloadState::ACTIVE && wl.latency_sensitive)
            any_latency = true;
    }

    if (any_building || any_latency)  return GovernorHint::PERFORMANCE;
    if (all_idle && pressure == PressureLevel::NORMAL) return GovernorHint::POWERSAVE;
    return GovernorHint::SCHEDUTIL;
}

// ─────────────────────────────────────────────────────────────────────────────
// apply_governor — write to all CPU scaling_governor files
// ─────────────────────────────────────────────────────────────────────────────
bool WorkspaceMonitor::apply_governor(GovernorHint hint) {
    const char* name = "schedutil";
    switch (hint) {
        case GovernorHint::POWERSAVE:   name = "powersave";   break;
        case GovernorHint::SCHEDUTIL:   name = "schedutil";   break;
        case GovernorHint::PERFORMANCE: name = "performance"; break;
    }

    bool ok = true;
    try {
        for (const auto& entry : fs::directory_iterator("/sys/devices/system/cpu")) {
            std::string gov_path = entry.path().string() + "/cpufreq/scaling_governor";
            if (!fs::exists(gov_path)) continue;
            std::ofstream f(gov_path);
            if (f.is_open()) {
                f << name;
                ok &= f.good();
            }
        }
    } catch (...) {
        std::cerr << "[Governor] Failed to enumerate CPUs\n";
        return false;
    }
    return ok;
}

} // namespace thm
