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

static const char* SOCK_PATH = "/tmp/titan_hwm.sock";

static void print_usage() {
    std::cerr << "Titan Hardware Manager CLI\n\n"
              << "Usage: titan-hwm <command>\n\n"
              << "Commands:\n"
              << "  switch web       Force Web Dev profile\n"
              << "  switch android   Force Android Dev profile\n"
              << "  switch system    Force System Dev profile\n"
              << "  switch casual    Force Casual profile\n"
              << "  status           Show current profile, RAM, CPU temp\n";
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
