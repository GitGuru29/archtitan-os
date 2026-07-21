// titan-hwm CLI — Titan Hardware Manager control tool
// Communicates directly with the HWM daemon via UNIX domain socket.
// No external dependencies (no socat, no bash, no nc).
//
// Usage:
//   titan-hwm switch web|android|system|casual
//   titan-hwm status

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <vector>
#include <iomanip>
#include <thread>
#include <chrono>
#include <sstream>
static const char* SOCK_PATH = "/tmp/titan_hwm.sock";

static void print_usage() {
    std::cerr << "Titan Hardware Manager CLI\n\n"
              << "Usage: titan-hwm <command>\n\n"
              << "Commands:\n"
              << "  switch web       Force Web Dev profile\n"
              << "  switch android   Force Android Dev profile\n"
              << "  switch system    Force System Dev profile\n"
              << "  switch casual    Force Casual profile\n"
              << "  status           Show current profile, RAM, CPU temp\n"
              << "  metrics          Live telemetry dashboard (PSI, cgroups)\n";
}

struct PSI { float avg10 = 0, avg60 = 0, avg300 = 0; };

static PSI read_psi(const std::string& path, const std::string& prefix) {
    PSI p; std::ifstream f(path); std::string line;
    while (std::getline(f, line)) {
        if (line.rfind(prefix, 0) == 0) {
            auto extract = [&](const std::string& key, float& out) {
                auto pos = line.find(key + "=");
                if (pos != std::string::npos) try { out = std::stof(line.substr(pos + key.size() + 1)); } catch (...) {}
            };
            extract("avg10", p.avg10); extract("avg60", p.avg60); extract("avg300", p.avg300);
        }
    }
    return p;
}

static std::string read_file(const std::string& path) {
    std::ifstream f(path); std::string s;
    if (f) f >> s;
    return s;
}

static long read_mem_mb(const std::string& slice) {
    std::string val = read_file("/sys/fs/cgroup/" + slice + "/memory.current");
    try { return std::stol(val) / 1024 / 1024; } catch (...) { return 0; }
}

static void show_metrics() {
    std::cout << "\033[2J\033[H"; // Clear screen
    std::cout << "\033[1;36m=== Titan Hardware Manager Telemetry ===\033[0m\n\n";

    // PSI
    PSI mem_some = read_psi("/proc/pressure/memory", "some");
    PSI mem_full = read_psi("/proc/pressure/memory", "full");
    PSI cpu_some = read_psi("/proc/pressure/cpu", "some");
    PSI io_some  = read_psi("/proc/pressure/io", "some");

    std::cout << "\033[1;33m[ Pressure Stall Information (PSI) - 10s avg ]\033[0m\n";
    std::cout << "  Memory (Some): " << std::fixed << std::setprecision(2) << mem_some.avg10 << "%\n";
    std::cout << "  Memory (Full): " << mem_full.avg10 << "%\n";
    std::cout << "  CPU    (Some): " << cpu_some.avg10 << "%\n";
    std::cout << "  I/O    (Some): " << io_some.avg10 << "%\n\n";

    // Cgroups
    long mem_active = read_mem_mb("titan-active.slice");
    long mem_bg     = read_mem_mb("titan-background.slice");
    long mem_frozen = read_mem_mb("titan-frozen.slice");

    std::cout << "\033[1;32m[ Cgroup v2 Slices - Memory ]\033[0m\n";
    std::cout << "  Active Slice:     " << mem_active << " MB\n";
    std::cout << "  Background Slice: " << mem_bg << " MB\n";
    std::cout << "  Frozen Slice:     " << mem_frozen << " MB\n\n";
    
    // Status
    std::string state_prof = "Unknown";
    std::string state_act  = "Unknown";
    std::ifstream sf("/tmp/titan_hwm_state"); std::string sl;
    while (std::getline(sf, sl)) {
        if (sl.rfind("profile=", 0) == 0) state_prof = sl.substr(8);
        if (sl.rfind("last_action=", 0) == 0) state_act = sl.substr(12);
    }
    std::cout << "\033[1;35m[ Daemon State ]\033[0m\n";
    std::cout << "  Active Profile: " << state_prof << "\n";
    std::cout << "  Last Action:    " << state_act << "\n";
    
    std::cout << "\n\033[90mPress Ctrl+C to exit...\033[0m\n";
}


int main(int argc, char* argv[]) {
    if (argc < 2) { print_usage(); return 1; }

    // Build command string from all argv
    std::string cmd;
    for (int i = 1; i < argc; i++) {
        if (i > 1) cmd += ' ';
        cmd += argv[i];
    }

    // Validate command before touching the socket
    bool is_switch = (cmd.rfind("switch ", 0) == 0 || cmd.rfind("sw ", 0) == 0);
    bool is_status = (cmd == "status");
    bool is_metrics = (cmd == "metrics");
    
    if (is_metrics) {
        while (true) {
            show_metrics();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return 0;
    }

    if (!is_switch && !is_status) {
        std::cerr << "titan-hwm: unknown command '" << cmd << "'\n\n";
        print_usage();
        return 1;
    }

    // Check socket exists
    struct stat st{};
    if (stat(SOCK_PATH, &st) != 0 || !S_ISSOCK(st.st_mode)) {
        std::cerr << "titan-hwm: daemon socket not found at " << SOCK_PATH << "\n"
                  << "           Is titan_hw_manager running?  (systemctl status titan_hw_manager)\n";
        return 1;
    }

    // Connect
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { std::perror("titan-hwm: socket"); return 1; }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::perror("titan-hwm: connect");
        close(fd);
        return 1;
    }

    // Send command (no newline — daemon reads until EOF/buffer)
    if (write(fd, cmd.c_str(), cmd.size()) < 0) {
        std::perror("titan-hwm: write");
        close(fd);
        return 1;
    }

    // Read reply (status sends one; switch commands don't)
    char buf[1024]{};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n > 0) {
        // Daemon sent a reply (status command)
        std::cout << buf;
    } else if (is_switch) {
        // Switch commands are fire-and-forget — confirm to user
        std::cout << "→ Sent: " << cmd << "\n";
    }

    return 0;
}
