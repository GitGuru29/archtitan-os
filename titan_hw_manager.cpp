#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <csignal>
#include <thread>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <cstring>

namespace fs = std::filesystem;

// ==========================================
// Process Structures & Memory Graph Phase
// ==========================================
struct ProcessNode {
    pid_t pid;
    pid_t ppid;
    std::string name;
    char state;
    long memory_usage;
    std::vector<pid_t> children;
};

class MemoryGraph {
public:
    std::unordered_map<pid_t, ProcessNode> graph;

    void build() {
        graph.clear();
        for (const auto& entry : fs::directory_iterator("/proc")) {
            if (!entry.is_directory()) continue;
            
            std::string dirname = entry.path().filename().string();
            if (!std::all_of(dirname.begin(), dirname.end(), ::isdigit)) continue;
            
            pid_t pid = std::stoi(dirname);
            ProcessNode node;
            node.pid = pid;
            
            std::string stat_path = entry.path().string() + "/stat";
            std::ifstream stat_file(stat_path);
            if (!stat_file.is_open()) continue;
            
            std::string line;
            if (std::getline(stat_file, line)) {
                size_t lparen = line.find('(');
                size_t rparen = line.rfind(')');
                if (lparen != std::string::npos && rparen != std::string::npos) {
                    node.name = line.substr(lparen + 1, rparen - lparen - 1);
                    std::istringstream iss(line.substr(rparen + 2));
                    iss >> node.state >> node.ppid;
                }
            }
            stat_file.close();
            
            graph[pid] = node;
        }
        
        for (auto& [pid, node] : graph) {
            if (node.ppid != 0 && graph.find(node.ppid) != graph.end()) {
                graph[node.ppid].children.push_back(pid);
            }
        }
    }
};

class ProcessPruner {
public:
    static void prune_recursive(pid_t target_pid, const std::unordered_map<pid_t, ProcessNode>& graph) {
        auto it = graph.find(target_pid);
        if (it == graph.end()) return;
        
        for (pid_t child_pid : it->second.children) {
            prune_recursive(child_pid, graph);
        }
        
        std::cout << "[Pruner] Freeing context -> PID: " << target_pid 
                  << " | Process: " << it->second.name << std::endl;
                  
        if (kill(target_pid, SIGTERM) == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
};

class HyprlandIPC {
public:
    static std::string find_hyprland_socket(const std::string& sock_name) {
        std::vector<std::string> base_paths = {"/run/user/1000/hypr/", "/tmp/hypr/"};
        for (const auto& base : base_paths) {
            if (!std::filesystem::exists(base)) continue;
            for (const auto& entry : std::filesystem::directory_iterator(base)) {
                if (entry.is_directory()) {
                    std::string sock_path = entry.path().string() + "/" + sock_name;
                    if (std::filesystem::exists(sock_path)) return sock_path;
                }
            }
        }
        return "";
    }

    static void send_command(const std::string& command) {
        std::string hypr_socket_path = find_hyprland_socket(".socket.sock");
        if (hypr_socket_path.empty()) return;

        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) return;

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, hypr_socket_path.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            send(sock, command.c_str(), command.length(), 0);
        }
        close(sock);
    }
};

// ==========================================
// Developer Profiles & OS Hook Phase
// ==========================================
enum class DevProfile {
    WEB_DEV,
    ANDROID_DEV,
    SYSTEM_DEV,
    NEUTRAL
};

class TitanHardwareManager {
private:
    DevProfile current_profile = DevProfile::NEUTRAL;

    std::string get_hyprland_event_socket() {
        return HyprlandIPC::find_hyprland_socket(".socket2.sock");
    }

    void handle_workload_change(const std::string& active_window_class) {
        DevProfile new_profile = DevProfile::NEUTRAL;
        
        // 1. Detect the Software Engineering Workflow
        // Convert window class to lowercase for robust matching
        std::string win_class = active_window_class;
        std::transform(win_class.begin(), win_class.end(), win_class.begin(), ::tolower);

        if (win_class == "code" || win_class == "figma" || win_class == "firefox-developer-edition") {
            new_profile = DevProfile::WEB_DEV;
        } else if (win_class.find("studio") != std::string::npos || win_class == "emulator") {
            new_profile = DevProfile::ANDROID_DEV;
        } else if (win_class == "alacritty" || win_class == "kitty" || win_class == "clion" || win_class == "neovim") {
            new_profile = DevProfile::SYSTEM_DEV;
        }

        // Only transition if the workflow actually changes
        if (new_profile == current_profile || new_profile == DevProfile::NEUTRAL) return;
        current_profile = new_profile;

        MemoryGraph mem_graph;
        mem_graph.build();
        
        std::unordered_set<std::string> workloads_to_prune;
        std::string theme_command;
        std::string profile_name;

        // 2. Define the Profiles and Cross-Pruning rules
        switch (current_profile) {
            case DevProfile::WEB_DEV:
                profile_name = "Web Development";
                // If coding Web, prune heavy Android/System toolchains
                workloads_to_prune = {"studio", "java", "qemu-system-x86_64", "rust-analyzer", "clangd"};
                // JavaScript Yellow & React Blue borders
                theme_command = "keyword general:col.active_border rgba(00d8ffff) rgba(f7df1eff) 45deg";
                break;
                
            case DevProfile::ANDROID_DEV:
                profile_name = "Android Development";
                // If coding Android, prune heavy Web/Docker/System toolchains
                workloads_to_prune = {"node", "docker", "dockerd", "rust-analyzer", "clangd"};
                // Android Green & Dark Blue borders
                theme_command = "keyword general:col.active_border rgba(3ddc84ff) rgba(073042ff) 45deg";
                break;

            case DevProfile::SYSTEM_DEV:
                profile_name = "System Programming (C++/Rust)";
                // If coding Systems, prune massive Electron apps, Java, and Emulators
                workloads_to_prune = {"studio", "java", "node", "emulator", "electron"};
                // Arch Linux Blue & Terminal Gray borders
                theme_command = "keyword general:col.active_border rgba(1793d1ff) rgba(333333ff) 45deg";
                break;
                
            default:
                break;
        }

        std::cout << "\n============================================\n";
        std::cout << "[Daemon] Shifting Workflow to: " << profile_name << std::endl;
        
        // 3. Surgically prune unused development toolchains
        for (const auto& [pid, node] : mem_graph.graph) {
            if (workloads_to_prune.find(node.name) != workloads_to_prune.end()) {
                ProcessPruner::prune_recursive(pid, mem_graph.graph);
            }
        }
        
        // 4. Inject Visual IPC
        HyprlandIPC::send_command(theme_command);
        std::cout << "[Daemon] " << profile_name << " Environment Optimized.\n";
        std::cout << "============================================\n";
    }

public:
    void run() {
        std::string sock_path = get_hyprland_event_socket();
        if (sock_path.empty()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return;
        }
        
        while (true) {
            int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (server_fd < 0) exit(EXIT_FAILURE);
            
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);
            
            if (connect(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                close(server_fd);
                std::this_thread::sleep_for(std::chrono::seconds(3));
                continue;
            }
            
            std::cout << "[Daemon] Titan Hardware Manager initialized. Monitoring Developer Contexts." << std::endl;
            
            char buffer[1024];
            std::string leftover = "";
            while (true) {
                ssize_t bytes_read = read(server_fd, buffer, sizeof(buffer) - 1);
                if (bytes_read <= 0) break; 
                
                buffer[bytes_read] = '\0';
                std::string data = leftover + buffer;
                size_t pos = 0;
                
                while ((pos = data.find('\n')) != std::string::npos) {
                    std::string event = data.substr(0, pos);
                    data.erase(0, pos + 1);
                    
                    if (event.find("activewindow>>") == 0) {
                        std::string window_info = event.substr(14);
                        size_t comma = window_info.find(',');
                        if (comma != std::string::npos) {
                            handle_workload_change(window_info.substr(0, comma));
                        }
                    }
                }
                leftover = data;
            }
            close(server_fd);
        }
    }
};

int main() {
    while (true) {
        TitanHardwareManager daemon;
        daemon.run();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
}
