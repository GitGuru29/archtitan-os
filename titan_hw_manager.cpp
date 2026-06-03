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
#include <atomic>
#include <mutex>

namespace fs = std::filesystem;

// ==========================================
// /proc/meminfo Reader
// ==========================================
struct MemInfo {
    long total_kb     = 0;
    long available_kb = 0;

    float available_pct() const {
        if (total_kb == 0) return 100.0f;
        return (float)available_kb / (float)total_kb * 100.0f;
    }
};

static MemInfo read_meminfo() {
    MemInfo info;
    std::ifstream f("/proc/meminfo");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("MemTotal:",     0) == 0) { std::istringstream ss(line.substr(9));  ss >> info.total_kb; }
        if (line.rfind("MemAvailable:", 0) == 0) { std::istringstream ss(line.substr(13)); ss >> info.available_kb; }
        if (info.total_kb && info.available_kb) break;
    }
    return info;
}

// ==========================================
// Process Graph
// ==========================================
struct ProcessNode {
    pid_t pid   = 0;
    pid_t ppid  = 0;
    std::string name;
    char  state = '?';
    long  rss_kb = 0;
    std::vector<pid_t> children;
};

static long read_rss(pid_t pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/status");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            std::istringstream ss(line.substr(6));
            long v = 0; ss >> v; return v;
        }
    }
    return 0;
}

class MemoryGraph {
public:
    std::unordered_map<pid_t, ProcessNode> graph;

    void build() {
        graph.clear();
        for (const auto& entry : fs::directory_iterator("/proc")) {
            if (!entry.is_directory()) continue;
            std::string d = entry.path().filename().string();
            if (!std::all_of(d.begin(), d.end(), ::isdigit)) continue;

            pid_t pid = std::stoi(d);
            ProcessNode node;
            node.pid = pid;

            std::ifstream stat_file(entry.path().string() + "/stat");
            std::string line;
            if (std::getline(stat_file, line)) {
                size_t lp = line.find('('), rp = line.rfind(')');
                if (lp != std::string::npos && rp != std::string::npos) {
                    node.name = line.substr(lp + 1, rp - lp - 1);
                    std::istringstream iss(line.substr(rp + 2));
                    iss >> node.state >> node.ppid;
                }
            }
            node.rss_kb = read_rss(pid);
            graph[pid] = node;
        }
        for (auto& [pid, node] : graph)
            if (node.ppid && graph.count(node.ppid))
                graph[node.ppid].children.push_back(pid);
    }
};

// ==========================================
// Pressure-Aware Pruner
// ==========================================
//  >50% RAM free  → no action (system is fine)
//  25–50% free    → SIGSTOP  (freeze, save state, kernel can reclaim pages slowly)
//  <25% free      → SIGTERM  (hard free, heaviest consumers first)
// ==========================================
class ProcessPruner {
public:
    static void signal_tree(pid_t pid,
                            const std::unordered_map<pid_t, ProcessNode>& graph,
                            int sig)
    {
        auto it = graph.find(pid);
        if (it == graph.end()) return;
        for (pid_t c : it->second.children) signal_tree(c, graph, sig);

        const char* label = (sig == SIGSTOP) ? "Freezing" :
                            (sig == SIGCONT) ? "Thawing"  : "Terminating";
        std::cout << "[Pruner] " << label << " PID:" << pid
                  << " (" << it->second.name << ") "
                  << it->second.rss_kb / 1024 << "MB\n";
        kill(pid, sig);

        if (sig == SIGTERM) {
            // Short grace, then SIGKILL — critical on 8 GB, can't wait
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (kill(pid, 0) == 0) kill(pid, SIGKILL);
        }
    }

    static void thaw(const std::unordered_set<std::string>& targets,
                     const std::unordered_map<pid_t, ProcessNode>& graph)
    {
        for (const auto& [pid, node] : graph)
            if (targets.count(node.name) && node.state == 'T')
                signal_tree(pid, graph, SIGCONT);
    }

    static void prune(const std::unordered_set<std::string>& targets,
                      const std::unordered_map<pid_t, ProcessNode>& graph,
                      float avail_pct)
    {
        if (avail_pct > 50.0f) {
            std::cout << "[Pruner] RAM OK (" << (int)avail_pct << "% free) — skipping prune\n";
            return;
        }

        // Collect matching candidates, sort heaviest RSS first
        std::vector<const ProcessNode*> hits;
        for (const auto& [pid, node] : graph)
            if (targets.count(node.name)) hits.push_back(&node);

        std::sort(hits.begin(), hits.end(),
                  [](const ProcessNode* a, const ProcessNode* b){ return a->rss_kb > b->rss_kb; });

        int sig;
        if (avail_pct < 25.0f) {
            sig = SIGTERM;
            std::cout << "[Pruner] CRITICAL (" << (int)avail_pct << "% free) → TERMINATE\n";
        } else {
            sig = SIGSTOP;
            std::cout << "[Pruner] MODERATE (" << (int)avail_pct << "% free) → FREEZE\n";
        }

        for (const auto* node : hits)
            signal_tree(node->pid, graph, sig);
    }
};

// ==========================================
// Hyprland IPC
// ==========================================
class HyprlandIPC {
public:
    static std::string find_socket(const std::string& name) {
        for (const auto& base : {"/run/user/1000/hypr/", "/tmp/hypr/"}) {
            if (!fs::exists(base)) continue;
            for (const auto& e : fs::directory_iterator(base)) {
                if (!e.is_directory()) continue;
                std::string p = e.path().string() + "/" + name;
                if (fs::exists(p)) return p;
            }
        }
        return "";
    }

    static void send(const std::string& cmd) {
        std::string path = find_socket(".socket.sock");
        if (path.empty()) return;
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) return;
        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0)
            ::send(fd, cmd.c_str(), cmd.size(), 0);
        close(fd);
    }
};

// ==========================================
// State file — readable by waybar/scripts
// ==========================================
static void write_state(const std::string& profile, float avail_pct, const std::string& action) {
    std::ofstream f("/tmp/titan_hwm_state");
    if (!f) return;
    f << "profile=" << profile << "\n"
      << "mem_available_pct=" << (int)avail_pct << "\n"
      << "last_action=" << action << "\n"
      << "timestamp=" << std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch()).count() << "\n";
}

// ==========================================
// Developer Profiles
// ==========================================
enum class DevProfile { WEB_DEV, ANDROID_DEV, SYSTEM_DEV, NEUTRAL };

class TitanHardwareManager {
private:
    DevProfile  current_profile = DevProfile::NEUTRAL;

    // Debounce: buffer rapid window switches (800 ms)
    std::atomic<bool>  have_pending{false};
    std::mutex         pending_mtx;
    std::string        pending_class;
    std::thread        debounce_thd;

    // Tool sets per profile (used for thaw-on-return)
    const std::unordered_set<std::string> web_tools     = {"node","webpack","vite","esbuild","bun","figma-linux"};
    const std::unordered_set<std::string> android_tools = {"studio","java","gradle","emulator","adb"};
    const std::unordered_set<std::string> system_tools  = {"rust-analyzer","clangd","cargo","make","cmake","gdb"};

    DevProfile classify(const std::string& raw) {
        std::string wc = raw;
        std::transform(wc.begin(), wc.end(), wc.begin(), ::tolower);
        if (wc == "code" || wc == "figma" || wc == "firefox-developer-edition")      return DevProfile::WEB_DEV;
        if (wc.find("studio") != std::string::npos || wc == "emulator")               return DevProfile::ANDROID_DEV;
        if (wc == "alacritty" || wc == "kitty" || wc == "clion" || wc == "neovim")   return DevProfile::SYSTEM_DEV;
        return DevProfile::NEUTRAL;
    }

    void do_transition(const std::string& wc) {
        DevProfile np = classify(wc);
        if (np == DevProfile::NEUTRAL || np == current_profile) return;
        current_profile = np;

        MemInfo mem = read_meminfo();
        float   pct = mem.available_pct();

        MemoryGraph g; g.build();

        std::unordered_set<std::string> to_prune, to_thaw;
        std::string theme, profile_name;

        switch (np) {
            case DevProfile::WEB_DEV:
                profile_name = "Web Development";
                to_prune = {"studio","java","gradle","emulator","rust-analyzer","clangd","cargo"};
                to_thaw  = web_tools;
                theme    = "keyword general:col.active_border rgba(00d8ffff) rgba(f7df1eff) 45deg";
                break;
            case DevProfile::ANDROID_DEV:
                profile_name = "Android Development";
                to_prune = {"node","webpack","vite","bun","rust-analyzer","clangd","docker","dockerd"};
                to_thaw  = android_tools;
                theme    = "keyword general:col.active_border rgba(3ddc84ff) rgba(073042ff) 45deg";
                break;
            case DevProfile::SYSTEM_DEV:
                profile_name = "System Programming";
                to_prune = {"studio","java","gradle","emulator","node","webpack","electron","dockerd"};
                to_thaw  = system_tools;
                theme    = "keyword general:col.active_border rgba(1793d1ff) rgba(333333ff) 45deg";
                break;
            default: return;
        }

        std::cout << "\n============================================\n"
                  << "[HWM] → " << profile_name << "\n"
                  << "[HWM] RAM: " << mem.available_kb / 1024 << " MB free / "
                  << mem.total_kb / 1024 << " MB total  (" << (int)pct << "%)\n";

        // Thaw new profile's tools first (instant responsiveness)
        ProcessPruner::thaw(to_thaw, g.graph);

        // Pressure-aware prune of background toolchains
        ProcessPruner::prune(to_prune, g.graph, pct);

        // Visual feedback via Hyprland IPC
        HyprlandIPC::send(theme);

        // State file for waybar / external scripts
        write_state(profile_name, pct, pct < 25.0f ? "terminate" : (pct < 50.0f ? "freeze" : "skip"));

        std::cout << "[HWM] " << profile_name << " ready.\n"
                  << "============================================\n";
    }

    void schedule(const std::string& wc) {
        {
            std::lock_guard<std::mutex> lk(pending_mtx);
            pending_class = wc;
            have_pending  = true;
        }
        if (debounce_thd.joinable()) debounce_thd.detach();
        debounce_thd = std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            if (!have_pending.load()) return;
            std::string wc;
            {
                std::lock_guard<std::mutex> lk(pending_mtx);
                if (!have_pending.load()) return;
                wc = pending_class;
                have_pending = false;
            }
            do_transition(wc);
        });
    }

public:
    ~TitanHardwareManager() { if (debounce_thd.joinable()) debounce_thd.join(); }

    void run() {
        std::string sock = HyprlandIPC::find_socket(".socket2.sock");
        if (sock.empty()) { std::this_thread::sleep_for(std::chrono::seconds(5)); return; }

        while (true) {
            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd < 0) exit(EXIT_FAILURE);

            struct sockaddr_un addr{};
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, sock.c_str(), sizeof(addr.sun_path) - 1);

            if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                close(fd);
                std::this_thread::sleep_for(std::chrono::seconds(3));
                continue;
            }

            std::cout << "[HWM] Titan Hardware Manager online. Watching developer contexts.\n";

            char buf[1024];
            std::string leftover;
            while (true) {
                ssize_t n = read(fd, buf, sizeof(buf) - 1);
                if (n <= 0) break;
                buf[n] = '\0';
                std::string data = leftover + buf;
                size_t pos;
                while ((pos = data.find('\n')) != std::string::npos) {
                    std::string ev = data.substr(0, pos);
                    data.erase(0, pos + 1);
                    if (ev.rfind("activewindow>>", 0) == 0) {
                        std::string info = ev.substr(14);
                        size_t c = info.find(',');
                        if (c != std::string::npos) schedule(info.substr(0, c));
                    }
                }
                leftover = data;
            }
            close(fd);
        }
    }
};

int main() {
    while (true) {
        TitanHardwareManager daemon;
        daemon.run();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
