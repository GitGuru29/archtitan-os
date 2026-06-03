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
// GLOBAL AUDIO WHITELIST
// These processes are NEVER frozen or killed,
// regardless of which profile is active.
// Developers listen to music while coding — respect that.
// ==========================================
static const std::unordered_set<std::string> AUDIO_WHITELIST = {
    "spotify", "spotifyd", "mpd", "mpdris2", "mpv",
    "vlc", "rhythmbox", "strawberry", "deadbeef",
    "cmus", "ncmpcpp", "cantata", "audacious",
    "elisa", "lollypop", "clementine", "quodlibet",
    "playerctld", "pipewire", "pipewire-pulse",
    "wireplumber", "pulseaudio", "pavucontrol"
};

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
    pid_t pid    = 0;
    pid_t ppid   = 0;
    std::string name;
    char  state  = '?';
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

            std::ifstream sf(entry.path().string() + "/stat");
            std::string line;
            if (std::getline(sf, line)) {
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
//
//  >50% RAM free  → skip prune (system fine)
//  25–50% free    → SIGSTOP  (freeze, kernel reclaims pages)
//  <25%  free     → SIGTERM → SIGKILL (hard free, heaviest first)
//
//  AUDIO_WHITELIST is checked at every level — music never stops.
// ==========================================
class ProcessPruner {
public:
    static void signal_tree(pid_t pid,
                            const std::unordered_map<pid_t, ProcessNode>& graph,
                            int sig)
    {
        auto it = graph.find(pid);
        if (it == graph.end()) return;

        // Never touch audio processes
        if (AUDIO_WHITELIST.count(it->second.name)) {
            std::cout << "[Pruner] Skipping audio process: " << it->second.name
                      << " (PID:" << pid << ") 🎵\n";
            return;
        }

        for (pid_t c : it->second.children) signal_tree(c, graph, sig);

        const char* label = (sig == SIGSTOP) ? "Freezing"    :
                            (sig == SIGCONT) ? "Thawing"     : "Terminating";
        std::cout << "[Pruner] " << label << " PID:" << pid
                  << " (" << it->second.name << ") "
                  << it->second.rss_kb / 1024 << " MB\n";
        kill(pid, sig);

        if (sig == SIGTERM) {
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
            std::cout << "[Pruner] RAM OK (" << (int)avail_pct << "% free) — no prune needed\n";
            return;
        }

        // Collect candidates (excluding audio), sort heaviest RSS first
        std::vector<const ProcessNode*> hits;
        for (const auto& [pid, node] : graph)
            if (targets.count(node.name) && !AUDIO_WHITELIST.count(node.name))
                hits.push_back(&node);

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
// State file — readable by waybar / scripts
// ==========================================
static void write_state(const std::string& profile, float avail_pct, const std::string& action) {
    std::ofstream f("/tmp/titan_hwm_state");
    if (!f) return;
    f << "profile="           << profile       << "\n"
      << "mem_available_pct=" << (int)avail_pct << "\n"
      << "last_action="       << action         << "\n"
      << "timestamp="         << std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch()).count() << "\n";
}

// ==========================================
// Developer & Casual Profiles
// ==========================================
enum class DevProfile { CASUAL, WEB_DEV, ANDROID_DEV, SYSTEM_DEV, NEUTRAL };

class TitanHardwareManager {
private:
    DevProfile current_profile = DevProfile::NEUTRAL;

    // Debounce: absorb rapid window switches (800 ms)
    std::atomic<bool> have_pending{false};
    std::mutex        pending_mtx;
    std::string       pending_class;
    std::thread       debounce_thd;

    // --- Tool sets per profile ---

    // Casual / daily-use apps — not needed during deep dev work
    const std::unordered_set<std::string> casual_tools = {
        "telegram-desktop", "discord", "slack", "element",
        "thunderbird", "evolution", "geary",
        "nautilus", "dolphin", "thunar",
        "gnome-software", "pamac", "packagekitd",
        "tracker-miner-f", "tracker-miner-a", "baloo_file",
        "krunner", "zeitgeist-datah", "gsd-media-keys",
        "mission-center", "gnome-usage"
    };

    // Dev toolchains per profile (thawed when entering that profile)
    const std::unordered_set<std::string> web_tools     = {"node","webpack","vite","esbuild","bun","figma-linux"};
    const std::unordered_set<std::string> android_tools = {"studio","java","gradle","emulator","adb"};
    const std::unordered_set<std::string> system_tools  = {"rust-analyzer","clangd","cargo","make","cmake","gdb"};

    DevProfile classify(const std::string& raw) {
        std::string wc = raw;
        std::transform(wc.begin(), wc.end(), wc.begin(), ::tolower);

        // Dev profiles
        if (wc == "code" || wc == "figma" || wc == "firefox-developer-edition")    return DevProfile::WEB_DEV;
        if (wc.find("studio") != std::string::npos || wc == "emulator")             return DevProfile::ANDROID_DEV;
        if (wc == "alacritty" || wc == "kitty" || wc == "clion" || wc == "neovim") return DevProfile::SYSTEM_DEV;

        // Casual profile — ONLY dedicated communication & file manager apps.
        // Browsers (firefox, chromium, brave) are intentionally excluded:
        // they are multipurpose — dev work, music streaming, docs, etc.
        // Focusing a browser mid-coding should NEVER freeze your dev environment.
        if (wc == "telegram-desktop" || wc == "discord"  || wc == "slack"  ||
            wc == "thunderbird"       || wc == "element"  || wc == "geary"  ||
            wc == "nautilus"          || wc == "dolphin"  || wc == "thunar")
            return DevProfile::CASUAL;

        return DevProfile::NEUTRAL;
    }

    const char* profile_label(DevProfile p) {
        switch (p) {
            case DevProfile::CASUAL:      return "Casual / Daily";
            case DevProfile::WEB_DEV:     return "Web Development";
            case DevProfile::ANDROID_DEV: return "Android Development";
            case DevProfile::SYSTEM_DEV:  return "System Programming";
            default:                      return "Neutral";
        }
    }

    void do_transition(const std::string& wc) {
        DevProfile np = classify(wc);
        if (np == DevProfile::NEUTRAL || np == current_profile) return;

        DevProfile old = current_profile;
        current_profile = np;

        MemInfo mem = read_meminfo();
        float   pct = mem.available_pct();

        MemoryGraph g; g.build();

        std::cout << "\n============================================\n"
                  << "[HWM] " << profile_label(old) << "  →  " << profile_label(np) << "\n"
                  << "[HWM] RAM: " << mem.available_kb / 1024 << " MB free / "
                  << mem.total_kb / 1024 << " MB  (" << (int)pct << "%)\n";

        // ---- CASUAL → DEV transition --------------------------------
        // Freeze casual background apps; thaw the target dev toolchain.
        // Audio whitelist is always respected inside prune().
        if (np == DevProfile::WEB_DEV || np == DevProfile::ANDROID_DEV || np == DevProfile::SYSTEM_DEV) {

            // 1. Prune casual daily apps (not needed while deep in dev work)
            std::cout << "[HWM] Suspending casual background apps...\n";
            ProcessPruner::prune(casual_tools, g.graph, pct);

            // 2. Prune competing dev toolchains
            std::unordered_set<std::string> dev_prune, dev_thaw;
            std::string theme;

            if (np == DevProfile::WEB_DEV) {
                dev_prune = {"studio","java","gradle","emulator","rust-analyzer","clangd","cargo"};
                dev_thaw  = web_tools;
                theme     = "keyword general:col.active_border rgba(00d8ffff) rgba(f7df1eff) 45deg";
            } else if (np == DevProfile::ANDROID_DEV) {
                dev_prune = {"node","webpack","vite","bun","rust-analyzer","clangd","docker","dockerd"};
                dev_thaw  = android_tools;
                theme     = "keyword general:col.active_border rgba(3ddc84ff) rgba(073042ff) 45deg";
            } else { // SYSTEM_DEV
                dev_prune = {"studio","java","gradle","emulator","node","webpack","electron","dockerd"};
                dev_thaw  = system_tools;
                theme     = "keyword general:col.active_border rgba(1793d1ff) rgba(333333ff) 45deg";
            }

            // 3. Thaw this profile's tools first (instant responsiveness)
            std::cout << "[HWM] Thawing " << profile_label(np) << " toolchain...\n";
            ProcessPruner::thaw(dev_thaw, g.graph);

            // 4. Prune competing dev tools
            ProcessPruner::prune(dev_prune, g.graph, pct);

            HyprlandIPC::send(theme);
        }

        // ---- DEV → CASUAL transition --------------------------------
        // Thaw casual apps; freeze all dev toolchains to save RAM.
        else if (np == DevProfile::CASUAL) {

            // 1. Thaw casual apps
            std::cout << "[HWM] Restoring casual environment...\n";
            ProcessPruner::thaw(casual_tools, g.graph);

            // 2. Freeze all dev toolchains (they're irrelevant right now)
            std::unordered_set<std::string> all_dev_tools;
            for (const auto& s : web_tools)     all_dev_tools.insert(s);
            for (const auto& s : android_tools) all_dev_tools.insert(s);
            for (const auto& s : system_tools)  all_dev_tools.insert(s);
            all_dev_tools.insert({"studio","java","gradle","emulator","node","webpack",
                                   "vite","bun","electron","dockerd","docker","rust-analyzer",
                                   "clangd","cargo","make","cmake","gdb"});

            ProcessPruner::prune(all_dev_tools, g.graph, pct);

            // Neutral border for casual mode
            HyprlandIPC::send("keyword general:col.active_border rgba(cba6f7ff) rgba(89b4faff) 45deg");
        }

        // Write state
        std::string action = (pct < 25.0f) ? "terminate" : (pct < 50.0f ? "freeze" : "skip");
        write_state(profile_label(np), pct, action);

        std::cout << "[HWM] " << profile_label(np) << " environment ready. "
                  << "🎵 Music: protected by audio whitelist.\n"
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

            std::cout << "[HWM] Titan Hardware Manager online.\n"
                      << "[HWM] Profiles: CASUAL | WEB_DEV | ANDROID_DEV | SYSTEM_DEV\n"
                      << "[HWM] Audio whitelist active — music always protected.\n";

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
