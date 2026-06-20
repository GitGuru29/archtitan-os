#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/resource.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <thread>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <atomic>
#include <mutex>
#include <cstdlib>
#include <optional>
#include <map>
#include <functional>

// ── Titan Sandbox integration ─────────────────────────────────────────────────
// thm_sandbox_integration.h provides:
//   - resolve_policy()        : WorkloadType → policy file path
//   - SandboxLauncher         : launch helper (used by exec-hook, not here)
//   - POLICY_DIR constexpr    : policy install dir (defined in the header)
// THM's role: read the sandbox registry written by titan-exec-hook to map
// original PIDs → box PIDs, then use box PIDs for cgroup tier assignment.
#include "sandbox/thm_sandbox_integration.h"

namespace fs = std::filesystem;
using ms_clock = std::chrono::steady_clock;
using time_point = std::chrono::time_point<ms_clock>;

static const std::unordered_set<std::string> AUDIO_WHITELIST = {
    "spotify","spotifyd","mpd","mpdris2","mpv","vlc","rhythmbox",
    "strawberry","deadbeef","cmus","ncmpcpp","cantata","audacious",
    "elisa","playerctld","pipewire","pipewire-pulse","wireplumber","pulseaudio"
};

// ==========================================
// WorkloadType — granular profile enum
// ==========================================
enum class WorkloadType { SYSTEM_DEV, WEB_DEV, ANDROID_DEV, CASUAL, NEUTRAL };
// Legacy DevProfile alias kept for CLI/state compatibility
using DevProfile = WorkloadType;

static const char* wt_label(WorkloadType t) {
    switch(t) {
        case WorkloadType::SYSTEM_DEV:  return "System Dev";
        case WorkloadType::WEB_DEV:     return "Web Dev";
        case WorkloadType::ANDROID_DEV: return "Android Dev";
        case WorkloadType::CASUAL:      return "Casual";
        default:                        return "Neutral";
    }
}

// ==========================================
// Config
// ==========================================
struct Config {
    int   debounce_ms        = 800;
    float ram_freeze_pct     = 50.0f;
    float ram_kill_pct       = 25.0f;
    int   thermal_hot_mc     = 80000;
    int   oom_protect        = -200;
    int   oom_expose         = 300;
    int   sigterm_grace_ms   = 500;
    // workspace → profile mapping
    std::unordered_set<int> casual_ws{1}, web_ws{2}, android_ws{3}, system_ws{4,5};
    std::vector<std::string> web_extra, android_extra, system_extra;
    // [classifier]
    std::vector<std::string> lsp_binaries   = {"clangd","ccls","rust-analyzer","tsserver","eslint_d"};
    std::vector<std::string> build_daemons  = {"gradle","cargo","webpack","vite","java","cmake","adb"};
    std::vector<std::string> ai_inference   = {"ollama","llama-server","python3"};
    std::vector<std::string> known_ides     = {"code","cursor","zed","windsurf","antigravity",
                                               "fleet","idea","android-studio"};
    // [workspace_tiers]
    float active_ceiling      = 0.70f;
    float protected_reserve   = 0.20f;
    float freezeable_budget   = 0.05f;
    float system_reserve      = 0.05f;
    int   age_soft_decay_min  = 5;
    int   age_hard_decay_min  = 15;
    // [multi_context]
    bool  lsp_protection          = true;
    bool  build_daemon_protection = true;
    bool  aggressive_swap         = false;
};

static std::vector<std::string> csv(std::string s) {
    if (!s.empty() && s.front() == '[') s.erase(0, 1);
    if (!s.empty() && s.back() == ']') s.pop_back();
    std::vector<std::string> r; std::istringstream ss(s); std::string t;
    while (std::getline(ss, t, ',')) {
        t.erase(0,t.find_first_not_of(" \t\"'")); t.erase(t.find_last_not_of(" \t\"'")+1);
        if (!t.empty()) r.push_back(t);
    }
    return r;
}
static std::unordered_set<int> csv_int(const std::string& s) {
    std::unordered_set<int> r;
    for (auto& t : csv(s)) try { r.insert(std::stoi(t)); } catch (...) {}
    return r;
}

static Config load_config() {
    Config c;
    const char* h = getenv("HOME"); if (!h) return c;
    std::ifstream f(std::string(h)+"/.config/titan-hwm/config");
    std::string line;
    while (std::getline(f, line)) {
        auto cm = line.find('#'); if (cm!=std::string::npos) line=line.substr(0,cm);
        auto eq = line.find('='); if (eq==std::string::npos) continue;
        std::string k=line.substr(0,eq), v=line.substr(eq+1);
        k.erase(0,k.find_first_not_of(" \t")); k.erase(k.find_last_not_of(" \t")+1);
        v.erase(0,v.find_first_not_of(" \t")); v.erase(v.find_last_not_of(" \t")+1);
        try {
        if      (k=="debounce_ms")             c.debounce_ms      = std::stoi(v);
        else if (k=="ram_freeze_pct")          c.ram_freeze_pct   = std::stof(v);
        else if (k=="ram_kill_pct")            c.ram_kill_pct     = std::stof(v);
        else if (k=="thermal_hot_temp")        c.thermal_hot_mc   = std::stoi(v)*1000;
        else if (k=="oom_protect_score")       c.oom_protect      = std::stoi(v);
        else if (k=="oom_expose_score")        c.oom_expose       = std::stoi(v);
        else if (k=="sigterm_grace_ms")        c.sigterm_grace_ms = std::stoi(v);
        else if (k=="casual_workspaces")       c.casual_ws        = csv_int(v);
        else if (k=="web_workspaces")          c.web_ws           = csv_int(v);
        else if (k=="android_workspaces")      c.android_ws       = csv_int(v);
        else if (k=="system_workspaces")       c.system_ws        = csv_int(v);
        else if (k=="web_extra_tools")         c.web_extra        = csv(v);
        else if (k=="android_extra_tools")     c.android_extra    = csv(v);
        else if (k=="system_extra_tools")      c.system_extra     = csv(v);
        else if (k=="lsp_binaries")            c.lsp_binaries     = csv(v);
        else if (k=="build_daemons")           c.build_daemons    = csv(v);
        else if (k=="ai_inference")            c.ai_inference     = csv(v);
        else if (k=="known_ides")              c.known_ides       = csv(v);
        else if (k=="active_ceiling")          c.active_ceiling   = std::stof(v);
        else if (k=="protected_reserve")       c.protected_reserve= std::stof(v);
        else if (k=="age_soft_decay_min")      c.age_soft_decay_min=std::stoi(v);
        else if (k=="age_hard_decay_min")      c.age_hard_decay_min=std::stoi(v);
        else if (k=="lsp_protection")          c.lsp_protection   = (v=="true"||v=="1");
        else if (k=="aggressive_swap")         c.aggressive_swap  = (v=="true"||v=="1");
        } catch (const std::exception& e) {
            std::cerr << "[Config] Bad value for '" << k << "': " << e.what() << " — skipping\n";
        }
    }
    return c;
}

static void write_default_config() {
    const char* h = getenv("HOME"); if (!h) return;
    std::string path = std::string(h)+"/.config/titan-hwm/config";
    if (fs::exists(path)) return;
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream f(path);
    f << "# Titan Hardware Manager Config v2\n"
      << "debounce_ms = 800\nram_freeze_pct = 50\nram_kill_pct = 25\n"
      << "thermal_hot_temp = 80\noom_protect_score = -200\noom_expose_score = 300\n"
      << "sigterm_grace_ms = 500\ncasual_workspaces = [1]\nweb_workspaces = [2]\n"
      << "android_workspaces = [3]\nsystem_workspaces = [4, 5]\n"
      << "web_extra_tools = []\nandroid_extra_tools = []\nsystem_extra_tools = []\n"
      << "\n[classifier]\n"
      << "lsp_binaries = [\"clangd\",\"ccls\",\"rust-analyzer\",\"tsserver\",\"eslint_d\"]\n"
      << "build_daemons = [\"gradle\",\"cargo\",\"webpack\",\"vite\",\"java\",\"cmake\"]\n"
      << "ai_inference = [\"ollama\",\"llama-server\",\"python3\"]\n"
      << "known_ides = [\"code\",\"cursor\",\"zed\",\"windsurf\",\"antigravity\",\"fleet\",\"idea\",\"android-studio\"]\n"
      << "\n[workspace_tiers]\n"
      << "active_ceiling = 0.70\nprotected_reserve = 0.20\n"
      << "age_soft_decay_min = 5\nage_hard_decay_min = 15\n"
      << "\n[multi_context]\nlsp_protection = true\naggressive_swap = false\n";
    std::cout << "[HWM] Default config written to " << path << "\n";
}

// ==========================================
// MemInfo / Thermal / SysTuner / OOM
// ==========================================
struct MemInfo { long total_kb=0, available_kb=0; float pct() const { return total_kb?((float)available_kb/total_kb*100.0f):100.0f; } };
static MemInfo read_meminfo() {
    MemInfo m; std::ifstream f("/proc/meminfo"); std::string l;
    while (std::getline(f,l)) {
        if (l.rfind("MemTotal:",0)==0){std::istringstream ss(l.substr(9));ss>>m.total_kb;}
        if (l.rfind("MemAvailable:",0)==0){std::istringstream ss(l.substr(13));ss>>m.available_kb;}
        if (m.total_kb&&m.available_kb) break;
    }
    return m;
}
static int read_max_temp_mc() {
    int mx=0;
    try { for (const auto& z:fs::directory_iterator("/sys/class/thermal")) {
        if (z.path().filename().string().rfind("thermal_zone",0)!=0) continue;
        std::ifstream f(z.path().string()+"/temp"); int t=0; if (f>>t) mx=std::max(mx,t);
    }} catch(...) {}
    return mx;
}
static void set_cpu_governor(const std::string& gov) {
    try { for (const auto& cpu:fs::directory_iterator("/sys/devices/system/cpu")) {
        std::string n=cpu.path().filename().string();
        if (n.rfind("cpu",0)!=0||!std::isdigit(n[3])) continue;
        std::string p=cpu.path().string()+"/cpufreq/scaling_governor";
        if (!fs::exists(p)) continue;
        std::ofstream f(p); if (f) f<<gov;
    } std::cout<<"[SysTuner] governor → "<<gov<<"\n"; } catch(...) {}
}
static void set_swappiness(int v) { std::ofstream f("/proc/sys/vm/swappiness"); if (f) { f<<v; std::cout<<"[SysTuner] swappiness → "<<v<<"\n"; } }
// §5.2.7 — I/O scheduler assignment per profile
static void set_io_scheduler(const std::string& sched) {
    try {
        for (const auto& q : fs::directory_iterator("/sys/block")) {
            std::string dev = q.path().filename().string();
            if (dev.rfind("loop",0)==0||dev.rfind("ram",0)==0) continue;
            std::string p = q.path().string()+"/queue/scheduler";
            if (!fs::exists(p)) continue;
            std::ofstream f(p); if (f) f<<sched;
        }
        std::cout<<"[SysTuner] io_scheduler → "<<sched<<"\n";
    } catch(...) {}
}
static void set_oom(pid_t pid, int score) { std::ofstream f("/proc/"+std::to_string(pid)+"/oom_score_adj"); if (f) f<<score; }

// ==========================================
// PSI
// ==========================================
struct PSI { float some_avg10=0.0f, full_avg10=0.0f; bool valid=false; };
static PSI read_memory_psi() {
    PSI p; std::ifstream f("/proc/pressure/memory"); std::string line;
    while (std::getline(f,line)) {
        auto parse=[&](const std::string& l, float& out){ auto pos=l.find("avg10="); if(pos==std::string::npos)return; try{out=std::stof(l.substr(pos+6));}catch(...){} };
        if (line.rfind("some",0)==0){parse(line,p.some_avg10);p.valid=true;}
        if (line.rfind("full",0)==0) parse(line,p.full_avg10);
    }
    return p;
}

// ==========================================
// Notify
// ==========================================
static void notify(const std::string& title, const std::string& body) {
    // Sanitize: strip single quotes to prevent shell injection from window titles
    auto sanitize = [](std::string s) {
        s.erase(std::remove(s.begin(), s.end(), '\''), s.end()); return s;
    };
    std::string t = sanitize(title), b = sanitize(body);
    system(("notify-send --app-name='Titan HWM' --icon=dialog-information '"+t+"' '"+b+"' &").c_str());
}


// ==========================================
// ProcessNode + MemoryGraph
// ==========================================
struct ProcessNode { pid_t pid=0,ppid=0; std::string name; char state='?'; long rss_kb=0; std::vector<pid_t> children; };
static long read_rss(pid_t pid) {
    std::ifstream f("/proc/"+std::to_string(pid)+"/status"); std::string l; long ra=0,vr=0;
    while(std::getline(f,l)){if(l.rfind("RssAnon:",0)==0){std::istringstream ss(l.substr(8));ss>>ra;} if(l.rfind("VmRSS:",0)==0){std::istringstream ss(l.substr(6));ss>>vr;}}
    return ra>0?ra:vr;
}

// Read /proc/<pid>/cmdline — returns first token (argv[0])
static std::string read_cmdline(pid_t pid) {
    std::ifstream f("/proc/"+std::to_string(pid)+"/cmdline");
    std::string s; std::getline(f,s,'\0'); // null-delimited
    // strip path prefix
    auto sl = s.rfind('/'); return sl!=std::string::npos ? s.substr(sl+1) : s;
}

// Read /proc/<pid>/cwd symlink
static std::string read_cwd(pid_t pid) {
    try { return fs::read_symlink("/proc/"+std::to_string(pid)+"/cwd").string(); } catch(...) { return ""; }
}

class MemoryGraph {
public:
    std::unordered_map<pid_t,ProcessNode> graph;
    void build() {
        graph.clear();
        for (const auto& e:fs::directory_iterator("/proc")) {
            if (!e.is_directory()) continue;
            std::string d=e.path().filename().string();
            if (!std::all_of(d.begin(),d.end(),::isdigit)) continue;
            pid_t pid=std::stoi(d); ProcessNode n; n.pid=pid;
            std::ifstream sf(e.path().string()+"/stat"); std::string l;
            if (std::getline(sf,l)) {
                size_t lp=l.find('('),rp=l.rfind(')');
                if (lp!=std::string::npos&&rp!=std::string::npos) {
                    n.name=l.substr(lp+1,rp-lp-1);
                    std::istringstream iss(l.substr(rp+2)); iss>>n.state>>n.ppid;
                }
            }
            n.rss_kb=read_rss(pid); graph[pid]=n;
        }
        for (auto& [pid,n]:graph) if (n.ppid&&graph.count(n.ppid)) graph[n.ppid].children.push_back(pid);
    }
};

// §5.2.1 — Recursive child walk: collects all descendant cmdlines up to depth limit
static void walk_children_recursive(
    pid_t pid,
    const std::unordered_map<pid_t,ProcessNode>& graph,
    std::vector<std::string>& out,
    int depth = 3)
{
    if (depth <= 0) return;
    auto it = graph.find(pid);
    if (it == graph.end()) return;
    for (pid_t child : it->second.children) {
        std::string cmd = read_cmdline(child);
        if (!cmd.empty()) out.push_back(cmd);
        walk_children_recursive(child, graph, out, depth-1);
    }
}

// ==========================================
// PROBLEM 1 — FusionClassifier
// 3-signal fusion: children→cmdline, window title, cwd project markers
// ==========================================
struct FusionResult {
    WorkloadType type  = WorkloadType::NEUTRAL;
    bool ai_modifier   = false;   // ollama/llama-server detected
    bool has_lsp       = false;   // LSP server child running
    bool has_build     = false;   // build daemon child running
    float confidence   = 0.0f;
};

class FusionClassifier {
public:
    // Signal 1: walk children of pid, match cmdline against known sets
    static FusionResult from_children(pid_t pid,
                                       const std::unordered_map<pid_t,ProcessNode>& graph,
                                       const Config& cfg) {
        FusionResult r;
        std::unordered_set<std::string> lsp_set(cfg.lsp_binaries.begin(), cfg.lsp_binaries.end());
        std::unordered_set<std::string> build_set(cfg.build_daemons.begin(), cfg.build_daemons.end());
        std::unordered_set<std::string> ai_set(cfg.ai_inference.begin(), cfg.ai_inference.end());

        // Scores per workload type accumulated from child processes
        std::map<WorkloadType,float> scores;

        // §5.2.1 — Use recursive walk (depth 3) to catch grandchild LSP servers
        std::vector<std::string> all_cmds;
        walk_children_recursive(pid, graph, all_cmds, 3);

        for (const std::string& cmd : all_cmds) {
            if (cmd.empty()) continue;

            // AI modifier — set flag and continue (doesn't override profile)
            if (ai_set.count(cmd)) { r.ai_modifier = true; continue; }

            // LSP detection (highest confidence → 0.6 weight)
            if (lsp_set.count(cmd)) {
                r.has_lsp = true;
                if (cmd=="clangd"||cmd=="ccls"||cmd=="rust-analyzer") scores[WorkloadType::SYSTEM_DEV]+=0.6f;
                else if (cmd=="tsserver"||cmd=="eslint_d")             scores[WorkloadType::WEB_DEV]+=0.6f;
                continue;
            }
            // Build daemon detection (highest confidence → 0.6 weight)
            if (build_set.count(cmd)) {
                r.has_build = true;
                // §5.2.1 spec: gradle/java/adb → ANDROID_DEV
                if (cmd=="gradle"||cmd=="java"||cmd=="adb") scores[WorkloadType::ANDROID_DEV]+=0.6f;
                else if (cmd=="cargo"||cmd=="cmake")      scores[WorkloadType::SYSTEM_DEV]+=0.6f;
                else if (cmd=="webpack"||cmd=="vite")     scores[WorkloadType::WEB_DEV]+=0.6f;
                continue;
            }
            // §5.2.1 spec: adb is in build_daemons — routed via build_set above.
            // These are supplementary lower-confidence signals for unlisted child processes.
            if (cmd=="kotlin-compiler"||cmd=="aapt2") { r.has_build=true; scores[WorkloadType::ANDROID_DEV]+=0.6f; }
            else if (cmd=="node"||cmd=="esbuild"||cmd=="bun")   scores[WorkloadType::WEB_DEV]+=0.3f;
            else if (cmd=="gdb"||cmd=="make"||cmd=="ninja")     scores[WorkloadType::SYSTEM_DEV]+=0.3f;
        }
        if (!scores.empty()) {
            auto best = std::max_element(scores.begin(), scores.end(),
                [](auto& a, auto& b){ return a.second < b.second; });
            r.type = best->first; r.confidence = best->second;
        }
        return r;
    }

    // Signal 2: parse window title for file extension / project markers
    // Uses suffix checks to avoid false matches (e.g. ".ts" inside "settings")
    static std::optional<WorkloadType> from_title(const std::string& title) {
        auto has  = [&](const std::string& s){ return title.find(s) != std::string::npos; };
        // Suffix check: does title end with ext or contain " ext" / "/ext"
        auto has_ext = [&](const std::string& ext) {
            // Check bare suffix at end of string
            if (title.size() >= ext.size() &&
                title.compare(title.size()-ext.size(), ext.size(), ext)==0) return true;
            // Check preceded by space, slash, or colon (path separator in window titles)
            for (char sep : {' ', '/', '\\', ':', '|'}) {
                std::string needle = sep + ext;
                if (title.find(needle) != std::string::npos) return true;
            }
            return false;
        };
        // Android — highest specificity, checked first
        if (has("build.gradle")||has("AndroidManifest")||has(".apk")||has("gradlew"))
            return WorkloadType::ANDROID_DEV;
        // Web
        if (has_ext(".ts")||has_ext(".tsx")||has_ext(".jsx")||has_ext(".vue")||
            has("package.json")||has_ext(".html")||has_ext(".svelte"))
            return WorkloadType::WEB_DEV;
        // System — use has_ext to avoid ".c" matching ".class" etc.
        if (has_ext(".cpp")||has_ext(".rs")||has_ext(".c")||has_ext(".h")||
            has_ext(".cc")||has_ext(".cxx")||has("CMakeLists")||has("Cargo.toml"))
            return WorkloadType::SYSTEM_DEV;
        return std::nullopt;
    }

    // Signal 3: stat working directory for project marker files
    static std::optional<WorkloadType> from_cwd(pid_t pid) {
        std::string cwd = read_cwd(pid); if (cwd.empty()) return std::nullopt;
        fs::path p(cwd);
        if (fs::exists(p/"build.gradle")||fs::exists(p/"app"/"src")) return WorkloadType::ANDROID_DEV;
        if (fs::exists(p/"package.json")&&fs::exists(p/"node_modules")) return WorkloadType::WEB_DEV;
        if (fs::exists(p/"CMakeLists.txt")||fs::exists(p/"Cargo.toml")) return WorkloadType::SYSTEM_DEV;
        return std::nullopt;
    }

    // Main fusion entry point — accumulates weighted votes from all 3 signals
    // Signal 1 (child walk) = weight 0.6+  per matching process
    // Signal 2 (title)      = weight 0.3   single vote
    // Signal 3 (cwd stat)   = weight 0.2   tiebreaker vote
    // All signals vote into the same score map — disagreement is handled by
    // the highest accumulated score winning, not by silent signal suppression.
    static FusionResult classify(pid_t pid, const std::string& title,
                                 const std::unordered_map<pid_t,ProcessNode>& graph,
                                 const Config& cfg) {
        // Signal 1 — builds FusionResult with its own scores map + metadata
        FusionResult r = from_children(pid, graph, cfg);

        // Build a unified score accumulator: start with Signal 1's result
        std::map<WorkloadType, float> vote;
        if (r.type != WorkloadType::NEUTRAL)
            vote[r.type] += r.confidence;   // seed from S1 accumulated score

        // Signal 2 — medium confidence: 0.3 vote regardless of S1 agreement
        auto t2 = from_title(title);
        if (t2 && *t2 != WorkloadType::NEUTRAL)
            vote[*t2] += 0.3f;

        // Signal 3 — tiebreaker: 0.2 vote regardless of S1/S2 agreement
        auto t3 = from_cwd(pid);
        if (t3 && *t3 != WorkloadType::NEUTRAL)
            vote[*t3] += 0.2f;

        // Pick winner — highest accumulated vote wins
        if (!vote.empty()) {
            auto best = std::max_element(vote.begin(), vote.end(),
                [](const auto& a, const auto& b){ return a.second < b.second; });
            r.type       = best->first;
            r.confidence = best->second;
        }
        return r;
    }
};

// ==========================================
// PROBLEM 2 — WindowContext + WorkspaceProfile
// PROBLEM 3 — WorkspaceTier + WorkspaceState
// PROBLEM 4 — Age Decay
// PROBLEM 5 — Multi-Monitor Visibility
// ==========================================

// Per-window context built for every visible window on a workspace
struct WindowContext {
    pid_t        pid            = 0;
    std::string  wm_class;
    std::string  title;
    WorkloadType inferred_type  = WorkloadType::NEUTRAL;
    float        resource_weight= 0.1f;
    bool         is_focused     = false;
    bool         has_active_lsp = false;
    bool         has_build_daemon= false;
    bool         ai_modifier    = false;
};

// Composite profile for an entire workspace (all visible windows)
struct WorkspaceProfile {
    WorkloadType dominant    = WorkloadType::NEUTRAL;
    WorkloadType secondary   = WorkloadType::NEUTRAL;
    bool         multi_context = false;   // 2+ different WorkloadTypes present
    bool         ai_modifier   = false;
    float        memory_pressure = 0.0f;
};

// Workspace tier — controls how aggressively THM manages it
enum class WorkspaceTier { ACTIVE, PROTECTED, FREEZEABLE };

// ==========================================
// §5.2.5 — Frozen PID Registry
// Tracks all SIGSTOP-suspended PIDs with tier metadata and RssAnon
// ==========================================
struct FrozenEntry {
    pid_t         pid;
    WorkspaceTier tier;
    long          rss_anon_kb;
    time_point    frozen_at;
};
static std::mutex frozen_mtx;
static std::unordered_map<pid_t, FrozenEntry> frozen_registry;

static void registry_freeze(pid_t pid, WorkspaceTier tier, long rss_kb) {
    std::lock_guard<std::mutex> lk(frozen_mtx);
    FrozenEntry e; e.pid=pid; e.tier=tier; e.rss_anon_kb=rss_kb; e.frozen_at=ms_clock::now();
    frozen_registry[pid] = e;
}
static void registry_thaw(pid_t pid) {
    std::lock_guard<std::mutex> lk(frozen_mtx);
    frozen_registry.erase(pid);
}
static bool registry_is_frozen(pid_t pid) {
    std::lock_guard<std::mutex> lk(frozen_mtx);
    return frozen_registry.count(pid) > 0;
}

struct WorkspaceState {
    int                        workspace_id    = 0;
    bool                       is_visible      = false;  // ← P5: multi-monitor
    std::vector<WindowContext> windows;
    WorkspaceProfile           composite;
    bool                       has_protected_daemons = false;
    time_point                 last_active_ms  = ms_clock::now();
    // Start PROTECTED so a newly-seen workspace isn't immediately eligible for freeze
    WorkspaceTier              tier            = WorkspaceTier::PROTECTED;
};

// Build WorkspaceProfile from all windows using weighted scoring (P2)
static WorkspaceProfile build_composite(const std::vector<WindowContext>& windows) {
    WorkspaceProfile wp;
    std::map<WorkloadType,float> scores;
    bool any_ai = false;

    for (const auto& w : windows) {
        float base = 0.1f;
        if (w.has_build_daemon) base += 0.5f;
        if (w.has_active_lsp)   base += 0.3f;
        if (w.is_focused)       base *= 1.2f;
        if (w.inferred_type == WorkloadType::CASUAL)  base = 0.05f;
        if (w.ai_modifier) any_ai = true;
        scores[w.inferred_type] += base;
    }

    wp.ai_modifier = any_ai;
    if (scores.empty()) return wp;

    // Pick dominant (highest score) and secondary (second highest)
    WorkloadType dom = WorkloadType::NEUTRAL; float top = 0;
    WorkloadType sec = WorkloadType::NEUTRAL; float sec_top = 0;
    for (auto& [t,s] : scores) {
        if (s > top)     { sec=dom; sec_top=top; dom=t; top=s; }
        else if (s > sec_top) { sec=t; sec_top=s; }
    }
    wp.dominant  = dom;
    wp.secondary = sec;
    // multi_context = 2+ non-neutral, non-casual types present at meaningful weight
    int meaningful = 0;
    for (auto& [t,s] : scores) if (s > 0.2f && t!=WorkloadType::NEUTRAL && t!=WorkloadType::CASUAL) ++meaningful;
    wp.multi_context = (meaningful >= 2);
    return wp;
}

// Tier evaluation — called on every workspace switch (P3 + P4)
static WorkspaceTier evaluate_tier(const WorkspaceState& ws, int active_id, const Config& cfg) {
    if (ws.workspace_id == active_id || ws.is_visible) return WorkspaceTier::ACTIVE;
    if (ws.has_protected_daemons) return WorkspaceTier::PROTECTED;
    auto age_min = std::chrono::duration_cast<std::chrono::minutes>(
        ms_clock::now() - ws.last_active_ms).count();
    if (age_min > cfg.age_hard_decay_min) return WorkspaceTier::FREEZEABLE;
    return WorkspaceTier::PROTECTED; // < 15 min, no daemons → still protected
}

// Query Hyprland IPC for visible workspaces on all monitors (P5)
// Parses: /tmp/hypr/$SIG/.socket.sock  command: j/monitors
static std::unordered_set<int> query_visible_workspaces(const std::string& sock_path) {
    std::unordered_set<int> visible;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0); if (fd < 0) return visible;
    struct sockaddr_un a{}; a.sun_family = AF_UNIX;
    strncpy(a.sun_path, sock_path.c_str(), sizeof(a.sun_path)-1);
    if (connect(fd, (struct sockaddr*)&a, sizeof(a)) < 0) { close(fd); return visible; }
    const char* cmd = "j/monitors";
    ::send(fd, cmd, strlen(cmd), 0);
    // Read full response
    std::string resp; char buf[4096]; ssize_t n;
    while ((n=read(fd,buf,sizeof(buf)-1))>0) { buf[n]='\0'; resp+=buf; }
    close(fd);
    // Minimal JSON parse: find "activeWorkspace":{"id":<N>
    // We just scan for the pattern — no dep on nlohmann
    size_t pos = 0;
    while ((pos = resp.find("\"activeWorkspace\"", pos)) != std::string::npos) {
        auto id_pos = resp.find("\"id\":", pos);
        if (id_pos == std::string::npos) break;
        id_pos += 5;
        try {
            size_t end; int id = std::stoi(resp.substr(id_pos), &end);
            visible.insert(id);
            pos = id_pos + end;
        } catch(...) { break; }
    }
    return visible;
}

// §5.2.2 — Query Hyprland j/clients to get all PIDs on a specific workspace
// Returns map of workspace_id → set of PIDs
static std::unordered_map<int, std::vector<pid_t>> query_workspace_pids(const std::string& sock_path) {
    std::unordered_map<int, std::vector<pid_t>> result;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0); if (fd < 0) return result;
    struct sockaddr_un a{}; a.sun_family = AF_UNIX;
    strncpy(a.sun_path, sock_path.c_str(), sizeof(a.sun_path)-1);
    if (connect(fd, (struct sockaddr*)&a, sizeof(a)) < 0) { close(fd); return result; }
    const char* cmd = "j/clients";
    ::send(fd, cmd, strlen(cmd), 0);
    std::string resp; char buf[16384]; ssize_t n;
    while ((n=read(fd,buf,sizeof(buf)-1))>0) { buf[n]='\0'; resp+=buf; }
    close(fd);
    // Minimal scan: find consecutive "workspace":{"id":N, then "pid":P patterns
    // Each client block has both. We track current workspace context.
    size_t pos = 0;
    while (pos < resp.size()) {
        // Find next client pid
        auto pid_pos = resp.find("\"pid\":", pos);
        if (pid_pos == std::string::npos) break;
        pid_pos += 6;
        pid_t pid = 0;
        try { size_t end; pid = std::stoi(resp.substr(pid_pos), &end); pos = pid_pos + end; }
        catch(...) { ++pos; continue; }
        // Look backwards from pid_pos for most recent workspace id in this block
        // Find "workspace" before this pid_pos
        auto ws_search_start = (pid_pos > 2000) ? pid_pos - 2000 : 0;
        auto ws_pos = resp.rfind("\"workspace\"", pid_pos);
        if (ws_pos == std::string::npos || ws_pos < ws_search_start) continue;
        auto id_pos = resp.find("\"id\":", ws_pos);
        if (id_pos == std::string::npos || id_pos > pid_pos) continue;
        id_pos += 5;
        try { size_t end; int ws_id = std::stoi(resp.substr(id_pos), &end); result[ws_id].push_back(pid); }
        catch(...) {}
    }
    return result;
}

// ==========================================
// CgroupManager
// ==========================================
class CgroupManager {
    static constexpr const char* ROOT = "/sys/fs/cgroup";
    static std::string path(const std::string& slice, const std::string& file) { return std::string(ROOT)+"/"+slice+"/"+file; }
    static bool write_cg(const std::string& p, const std::string& val) { std::ofstream f(p); if(!f)return false; f<<val; return true; }
public:
    static bool ensure(const std::string& slice) {
        std::string dir=std::string(ROOT)+"/"+slice;
        if (!fs::exists(dir)) try{fs::create_directories(dir);}catch(...){return false;}
        return true;
    }
    static bool move_pid(pid_t pid, const std::string& slice) { ensure(slice); return write_cg(path(slice,"cgroup.procs"),std::to_string(pid)); }
    static bool set_cpu_weight(const std::string& slice, int w) { return write_cg(path(slice,"cpu.weight"),std::to_string(w)); }
    static bool set_memory_high(const std::string& slice, const std::string& val) { return write_cg(path(slice,"memory.high"),val); }
    static bool set_memory_low(const std::string& slice, const std::string& val) { return write_cg(path(slice,"memory.low"),val); }
    static bool freeze(const std::string& slice, bool do_freeze) { ensure(slice); return write_cg(path(slice,"cgroup.freeze"),do_freeze?"1":"0"); }
    static long memory_current(const std::string& slice) { std::ifstream f(path(slice,"memory.current")); long v=0; if(f)f>>v; return v; }
};

// cgroup helpers — LSP/build protection in freeze path, frozen_registry tracking
static void cg_escalate_to_frozen(
    const std::unordered_set<std::string>& targets,
    const std::unordered_map<pid_t,ProcessNode>& g,
    const Config* cfg = nullptr)   // §5.2.2: pass cfg to protect LSP/build in MULTI_CONTEXT
{
    std::unordered_set<std::string> lsp_set, bd_set;
    if (cfg) {
        lsp_set.insert(cfg->lsp_binaries.begin(), cfg->lsp_binaries.end());
        bd_set.insert(cfg->build_daemons.begin(), cfg->build_daemons.end());
    }
    for (const auto& [pid,n] : g) {
        if (!targets.count(n.name) || AUDIO_WHITELIST.count(n.name)) continue;
        // §5.2.2 MULTI_CONTEXT: never freeze LSP servers or build daemons
        if (cfg && (lsp_set.count(n.name) || bd_set.count(n.name))) {
            std::cout<<"[Cgroup] Freeze-skip protected daemon: "<<n.name<<"\n";
            continue;
        }
        // §5.2.5: SIGSTOP *first*, then cgroup migrate, to prevent run-window in frozen slice
        if (!registry_is_frozen(pid)) {
            ::kill(pid, SIGSTOP);
            registry_freeze(pid, WorkspaceTier::FREEZEABLE, n.rss_kb);
        }
        CgroupManager::move_pid(pid, "titan-frozen.slice");
    }
    CgroupManager::freeze("titan-frozen.slice", true);
    std::cout<<"[Cgroup] Escalated to frozen slice\n";
}
static void cg_thaw_from_frozen(
    const std::unordered_set<std::string>& targets,
    const std::unordered_map<pid_t,ProcessNode>& g)
{
    CgroupManager::freeze("titan-frozen.slice", false);
    for (const auto& [pid,n] : g) {
        if (!targets.count(n.name) || AUDIO_WHITELIST.count(n.name)) continue;
        CgroupManager::move_pid(pid, "titan-background.slice");
        // §5.2.5: clear from frozen registry and send SIGCONT
        if (registry_is_frozen(pid)) {
            ::kill(pid, SIGCONT);
            registry_thaw(pid);
        }
    }
    std::cout<<"[Cgroup] Thawed from frozen slice\n";
}

// ==========================================
// Pruner — cgroup v2 PSI-aware escalation ladder
// Multi-context aware: never freezes LSP/build daemons
// ==========================================
class Pruner {
public:
    static void deprioritize(const std::unordered_set<std::string>& targets,
                             const std::unordered_map<pid_t,ProcessNode>& g,
                             int oom_expose=300, bool protect_lsp=true,
                             const Config* cfg=nullptr) {
        CgroupManager::ensure("titan-background.slice");
        CgroupManager::set_cpu_weight("titan-background.slice",20);
        CgroupManager::set_memory_high("titan-background.slice","max");
        for (const auto& [pid,n]:g) {
            if (!targets.count(n.name)||AUDIO_WHITELIST.count(n.name)) continue;
            // P2: protect LSP + build daemons in multi-context
            if (protect_lsp && cfg) {
                std::unordered_set<std::string> lsp(cfg->lsp_binaries.begin(),cfg->lsp_binaries.end());
                std::unordered_set<std::string> bd(cfg->build_daemons.begin(),cfg->build_daemons.end());
                if (lsp.count(n.name)||bd.count(n.name)) { std::cout<<"[Cgroup] Protected (LSP/build): "<<n.name<<"\n"; continue; }
            }
            CgroupManager::move_pid(pid,"titan-background.slice");
            set_oom(pid,oom_expose);
            std::cout<<"[Cgroup] Deprioritized "<<n.name<<" ("<<n.rss_kb/1024<<"MB)\n";
        }
    }
    static void throttle_memory(long available_kb) {
        long cap=(available_kb*1024L*6L)/10L;
        if (cap<64*1024*1024) cap=64*1024*1024;
        CgroupManager::set_memory_high("titan-background.slice",std::to_string(cap));
        std::cout<<"[Cgroup] memory.high → "<<cap/1024/1024<<"MB\n";
    }
    static void freeze_targets(
        const std::unordered_set<std::string>& targets,
        const std::unordered_map<pid_t,ProcessNode>& g,
        const Config* cfg = nullptr)   // §5.2.2: forward cfg for LSP protection
    { cg_escalate_to_frozen(targets, g, cfg); }
    static void thaw(const std::unordered_set<std::string>& targets, const std::unordered_map<pid_t,ProcessNode>& g, int oom=-200) {
        cg_thaw_from_frozen(targets,g);
        CgroupManager::ensure("titan-active.slice");
        CgroupManager::set_cpu_weight("titan-active.slice",500);
        for (const auto& [pid,n]:g) {
            if (!targets.count(n.name)||AUDIO_WHITELIST.count(n.name)) continue;
            CgroupManager::move_pid(pid,"titan-active.slice"); set_oom(pid,oom);
            std::cout<<"[Cgroup] Activated "<<n.name<<"\n";
        }
    }
    static long emergency_kill(const std::unordered_set<std::string>& targets, const std::unordered_map<pid_t,ProcessNode>& g, int grace_ms=500, const Config* cfg=nullptr) {
        std::vector<const ProcessNode*> hits;
        for (const auto& [pid,n]:g) {
            if (!targets.count(n.name)||AUDIO_WHITELIST.count(n.name)) continue;
            if (cfg) {
                std::unordered_set<std::string> lsp(cfg->lsp_binaries.begin(),cfg->lsp_binaries.end());
                std::unordered_set<std::string> bd(cfg->build_daemons.begin(),cfg->build_daemons.end());
                if (lsp.count(n.name)||bd.count(n.name)) continue; // never kill LSP/build
            }
            hits.push_back(&n);
        }
        std::sort(hits.begin(),hits.end(),[](auto a,auto b){return a->rss_kb>b->rss_kb;});
        long freed=0;
        for (auto* n:hits) {
            std::cout<<"[Pruner] EMERGENCY KILL "<<n->name<<" ("<<n->rss_kb/1024<<"MB)\n";
            kill(n->pid,SIGTERM); std::this_thread::sleep_for(std::chrono::milliseconds(grace_ms));
            if (kill(n->pid,0)==0) kill(n->pid,SIGKILL);
            freed+=n->rss_kb;
        }
        return freed;
    }
    static long manage(const std::unordered_set<std::string>& targets,
                       const std::unordered_map<pid_t,ProcessNode>& g,
                       float ram_pct, float freeze_t, float kill_t,
                       long available_kb, int oom_expose=300, int grace_ms=500,
                       bool multi_ctx=false, const Config* cfg=nullptr) {
        PSI psi=read_memory_psi();
        std::cout<<"[PSI] some_avg10="<<psi.some_avg10<<"% full_avg10="<<psi.full_avg10<<"%\n";
        bool protect = multi_ctx && cfg && cfg->lsp_protection;
        // Healthy: ram ok AND psi low — just deprioritize, no freeze
        if (ram_pct>=freeze_t && psi.some_avg10<5.0f)  { deprioritize(targets,g,oom_expose,protect,cfg); return 0; }
        // Medium pressure: ram tightening — deprioritize + throttle memory.high
        if (ram_pct>=kill_t   && psi.some_avg10<20.0f) { deprioritize(targets,g,oom_expose,protect,cfg); throttle_memory(available_kb); return 0; }
        // High PSI but not yet critical — only freeze if ram is also actually low
        // §5.2.2: pass cfg into freeze so LSP/build daemons are never frozen in MULTI_CONTEXT
        if (psi.some_avg10<40.0f && ram_pct<freeze_t)  { std::cout<<"[Pruner] HIGH — cgroup freeze\n"; freeze_targets(targets,g,cfg); return 0; }
        // Healthy ram + mid PSI (e.g. burst I/O) — just deprioritize, don't freeze
        if (psi.some_avg10<40.0f) { deprioritize(targets,g,oom_expose,protect,cfg); return 0; }
        std::cout<<"[Pruner] CRITICAL PSI ("<<psi.some_avg10<<"%) — emergency kill\n";
        return emergency_kill(targets,g,grace_ms,cfg);
    }
};

// ==========================================
// Hyprland IPC
// ==========================================
class HyprlandIPC {
public:
    static std::string find_sock(const std::string& name) {
        for (const auto& base:{"/run/user/1000/hypr/","/tmp/hypr/"}) {
            if (!fs::exists(base)) continue;
            for (const auto& e:fs::directory_iterator(base)) {
                if (!e.is_directory()) continue;
                std::string p=e.path().string()+"/"+name;
                if (fs::exists(p)) return p;
            }
        }
        return "";
    }
    static void send(const std::string& cmd) {
        std::string path=find_sock(".socket.sock"); if (path.empty()) return;
        int fd=socket(AF_UNIX,SOCK_STREAM,0); if (fd<0) return;
        struct sockaddr_un a{}; a.sun_family=AF_UNIX;
        strncpy(a.sun_path,path.c_str(),sizeof(a.sun_path)-1);
        if (connect(fd,(struct sockaddr*)&a,sizeof(a))==0) ::send(fd,cmd.c_str(),cmd.size(),0);
        close(fd);
    }
};

// ==========================================
// State file — extended with multi-context fields
// ==========================================
static void write_state(const std::string& profile, float pct, const std::string& action,
                        long freed_mb, int temp_c, float psi_some=0.0f,
                        bool multi_ctx=false, bool ai_mod=false) {
    std::ofstream f("/tmp/titan_hwm_state"); if (!f) return;
    f << "profile="          << profile     << "\n"
      << "mem_available_pct="<< (int)pct    << "\n"
      << "last_action="      << action      << "\n"
      << "freed_mb="         << freed_mb    << "\n"
      << "cpu_temp_c="       << temp_c      << "\n"
      << "psi_some_avg10="   << psi_some    << "\n"
      << "multi_context="    << (multi_ctx?"true":"false") << "\n"
      << "ai_modifier="      << (ai_mod?"true":"false")    << "\n"
      << "timestamp="        << std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch()).count() << "\n";
}

// ==========================================
// TitanHardwareManager — main daemon class
// ==========================================
class TitanHardwareManager {
    Config cfg;
    WorkloadType cur = WorkloadType::NEUTRAL;
    std::atomic<bool> have_pending{false}, running{true};
    std::atomic<int>  debounce_gen{0};
    std::mutex mtx;

    // Pending event state
    std::string pending_class, pending_title;
    pid_t       pending_pid = 0;
    int         pending_ws  = -1;
    bool        pending_is_ws = false;

    std::thread debounce_thd, cli_thd, guard_thd, hotreload_thd;

    // Workspace state registry (P3/P4/P5)
    std::mutex ws_mtx;
    std::unordered_map<int,WorkspaceState> workspace_states;
    int active_workspace_id = 1;

    // ── Sandbox PID registry ──────────────────────────────────────────────────
    // Tracks every PID that has already been processed by the sandbox layer.
    // A PID absent from this set = new launch → route through titan-sandboxd.
    // A PID present = existing process that just gained focus → skip sandbox.
    std::mutex sandbox_mtx;
    std::unordered_set<pid_t> sandboxed_pids;

    // Rogue DM guard list
    const std::unordered_set<std::string> rogue_dms{
        "gdm","gdm3","lightdm","lxdm","xdm","slim","ly","greetd","emptty","lemurs",
        "gnome-shell","gnome-session","plasmashell","kwin_wayland","kwin_x11","xfwm4","openbox","Xorg","X"
    };

    // Tool sets — filled from config + apply_extras()
    std::unordered_set<std::string> web_tools{"node","webpack","vite","esbuild","bun","figma-linux"};
    std::unordered_set<std::string> android_tools{"studio","java","gradle","emulator","adb"};
    std::unordered_set<std::string> system_tools{"rust-analyzer","clangd","cargo","make","cmake","gdb","ccls"};
    const std::unordered_set<std::string> casual_tools{
        "telegram-desktop","discord","slack","element","thunderbird","evolution","geary",
        "nautilus","dolphin","thunar","gnome-software","pamac","packagekitd",
        "tracker-miner-f","tracker-miner-a","baloo_file","krunner","mission-center"
    };

    void apply_extras() {
        for (auto& t:cfg.web_extra)     web_tools.insert(t);
        for (auto& t:cfg.android_extra) android_tools.insert(t);
        for (auto& t:cfg.system_extra)  system_tools.insert(t);
    }

    // Check if binary name is a known polyglot IDE
    bool is_known_ide(const std::string& name) const {
        for (const auto& ide : cfg.known_ides) if (name==ide) return true;
        return false;
    }

    // Classify a window — routes IDEs through fusion, others via legacy lookup
    // Returns FusionResult for full metadata
    FusionResult classify_window(const std::string& wm_class, const std::string& title,
                                 pid_t pid, const std::unordered_map<pid_t,ProcessNode>& graph) {
        std::string w = wm_class;
        std::transform(w.begin(),w.end(),w.begin(),::tolower);

        // Known IDEs → full 3-signal fusion classifier (P1)
        if (is_known_ide(w)) {
            std::cout<<"[Fusion] IDE detected: "<<w<<" — running 3-signal fusion\n";
            return FusionClassifier::classify(pid, title, graph, cfg);
        }

        // Legacy fast path for non-IDE binaries
        FusionResult r;
        if (w.find("studio")!=std::string::npos||w=="emulator") r.type=WorkloadType::ANDROID_DEV;
        else if (w=="alacritty"||w=="kitty"||w=="clion"||w=="neovim") r.type=WorkloadType::SYSTEM_DEV;
        else if (w=="telegram-desktop"||w=="discord"||w=="slack"||
                 w=="thunderbird"||w=="element"||w=="geary"||
                 w=="nautilus"||w=="dolphin"||w=="thunar") r.type=WorkloadType::CASUAL;
        else if (w=="figma"||w=="firefox-developer-edition") r.type=WorkloadType::WEB_DEV;
        else r.type=WorkloadType::NEUTRAL;
        r.confidence = 1.0f; // direct match = full confidence
        return r;
    }

    WorkloadType classify_ws(int ws) {
        if (cfg.web_ws.count(ws))     return WorkloadType::WEB_DEV;
        if (cfg.android_ws.count(ws)) return WorkloadType::ANDROID_DEV;
        if (cfg.system_ws.count(ws))  return WorkloadType::SYSTEM_DEV;
        if (cfg.casual_ws.count(ws))  return WorkloadType::CASUAL;
        return WorkloadType::NEUTRAL;
    }

    // Apply age decay to a workspace (§5.2.4)
    void apply_age_decay(WorkspaceState& ws, const std::unordered_map<pid_t,ProcessNode>& graph, const std::vector<pid_t>& current_pids) {
        auto age_min = std::chrono::duration_cast<std::chrono::minutes>(
            ms_clock::now() - ws.last_active_ms).count();
        if (age_min < cfg.age_soft_decay_min) return;

        std::unordered_set<std::string> lsp(cfg.lsp_binaries.begin(),cfg.lsp_binaries.end());
        std::unordered_set<std::string> bd(cfg.build_daemons.begin(),cfg.build_daemons.end());

        bool frozen_any = false;
        for (pid_t root_pid : current_pids) {
            // §5.2.4: recursively collect all descendant PIDs (not just direct children)
            // so LSP grandchildren are correctly filtered by the daemon protection check
            std::vector<std::string> descendant_cmds;
            walk_children_recursive(root_pid, graph, descendant_cmds, 3);

            // Build the PID list: parent + all descendants
            std::vector<pid_t> pids = {root_pid};
            std::function<void(pid_t,int)> collect_pids = [&](pid_t p, int depth) {
                if (depth <= 0) return;
                auto it = graph.find(p);
                if (it == graph.end()) return;
                for (pid_t child : it->second.children) {
                    pids.push_back(child);
                    collect_pids(child, depth-1);
                }
            };
            collect_pids(root_pid, 3);

            for (pid_t pid : pids) {
                auto nit = graph.find(pid);
                if (nit == graph.end()) continue;
                const auto& n = nit->second;
                if (AUDIO_WHITELIST.count(n.name)||lsp.count(n.name)||bd.count(n.name)) continue;

                if (age_min >= cfg.age_hard_decay_min) {
                    // §5.2.4 hard decay: SIGSTOP + cgroup freeze + frozen registry
                    long rss = read_rss(pid);
                    if (!registry_is_frozen(pid)) {
                        ::kill(pid, SIGSTOP);
                        registry_freeze(pid, ws.tier, rss);
                        CgroupManager::move_pid(pid, "titan-frozen.slice");
                        std::cout<<"[Decay] SIGSTOP+cgroup-freeze "
                                 <<n.name<<" (ws"<<ws.workspace_id<<" age="<<age_min<<"min rss="<<rss/1024<<"MB)\n";
                    }
                    frozen_any = true;
                } else if (age_min >= cfg.age_soft_decay_min) {
                    // §5.2.4 soft decay: renice to +5, move to background slice
                    ::setpriority(PRIO_PROCESS, pid, 5);
                    CgroupManager::move_pid(pid, "titan-background.slice");
                    std::cout<<"[Decay] Soft renice+bg "
                             <<n.name<<" (ws"<<ws.workspace_id<<" age="<<age_min<<"min)\n";
                }
            }
        }
        if (frozen_any) CgroupManager::freeze("titan-frozen.slice", true);
    }

    // §5.2.3 — Apply strict tier memory budgets
    void apply_tier_budgets() {
        long total_bytes = read_meminfo().total_kb * 1024L;
        if (total_bytes <= 0) return;

        // ACTIVE: 70% ceiling (memory.high)
        long active_high = total_bytes * cfg.active_ceiling;
        CgroupManager::ensure("titan-active.slice");
        CgroupManager::set_memory_high("titan-active.slice", std::to_string(active_high));

        // PROTECTED: 20% reservation (memory.low)
        long protected_low = total_bytes * cfg.protected_reserve;
        CgroupManager::ensure("titan-background.slice");
        CgroupManager::set_memory_low("titan-background.slice", std::to_string(protected_low));

        // FREEZEABLE: 5% budget (memory.high)
        long frozen_high = total_bytes * cfg.freezeable_budget;
        CgroupManager::ensure("titan-frozen.slice");
        CgroupManager::set_memory_high("titan-frozen.slice", std::to_string(frozen_high));

        std::cout << "[Tier] Applied memory budgets: ACTIVE ceiling=" << active_high/1024/1024
                  << "MB, PROTECTED reserve=" << protected_low/1024/1024
                  << "MB, FREEZEABLE budget=" << frozen_high/1024/1024 << "MB\n";
    }

    // Rebalance all workspaces after a workspace switch (§5.2.3 + §5.2.4 + §5.2.5)
    void rebalance_workspaces(const std::unordered_map<pid_t,ProcessNode>& graph) {
        // Apply cgroup memory budgets based on spec
        apply_tier_budgets();

        // §5.2.5 multi-monitor: query monitors for visible workspaces
        std::string sock = HyprlandIPC::find_sock(".socket.sock");
        auto visible = query_visible_workspaces(sock);
        // §5.2.2: get all PIDs per workspace from j/clients
        auto ws_pids_map = query_workspace_pids(sock);

        std::unordered_set<std::string> lsp_set(cfg.lsp_binaries.begin(), cfg.lsp_binaries.end());
        std::unordered_set<std::string> bd_set(cfg.build_daemons.begin(),  cfg.build_daemons.end());

        std::lock_guard<std::mutex> lk(ws_mtx);
        for (auto& [id, ws] : workspace_states) {
            ws.is_visible = visible.count(id) > 0;

            // §5.2.3 — Populate has_protected_daemons by scanning actual child processes
            ws.has_protected_daemons = false;
            auto pit = ws_pids_map.find(id);
            if (pit != ws_pids_map.end()) {
                for (pid_t wpid : pit->second) {
                    std::vector<std::string> cmds;
                    walk_children_recursive(wpid, graph, cmds, 3);
                    for (const auto& cmd : cmds) {
                        if (lsp_set.count(cmd) || bd_set.count(cmd)) {
                            ws.has_protected_daemons = true;
                            break;
                        }
                    }
                    if (ws.has_protected_daemons) break;
                }
            }

            ws.tier = evaluate_tier(ws, active_workspace_id, cfg);
            const char* tier_str = ws.tier==WorkspaceTier::ACTIVE?"ACTIVE":
                                   ws.tier==WorkspaceTier::PROTECTED?"PROTECTED":"FREEZEABLE";
            std::cout<<"[Tier] WS"<<id<<" → "<<tier_str
                     <<" daemons="<<ws.has_protected_daemons<<"\n";

            if (ws.tier != WorkspaceTier::ACTIVE) {
                std::vector<pid_t> empty_pids;
                apply_age_decay(ws, graph, pit != ws_pids_map.end() ? pit->second : empty_pids);
            } else {
                // Thaw processes that upgraded to ACTIVE
                std::string target_slice = "titan-active.slice";
                auto pit2 = ws_pids_map.find(id);
                if (pit2 != ws_pids_map.end()) {
                    for (pid_t wpid : pit2->second) {
                        std::vector<pid_t> pids_to_move = {wpid};
                        auto it = graph.find(wpid);
                        if (it != graph.end())
                            for (pid_t c : it->second.children) pids_to_move.push_back(c);
                        for (pid_t pid : pids_to_move) {
                            auto nit = graph.find(pid);
                            if (nit == graph.end()) continue;
                            if (AUDIO_WHITELIST.count(nit->second.name)) continue;
                            // §5.2.5: SIGCONT if in frozen registry
                            if (registry_is_frozen(pid)) {
                                ::kill(pid, SIGCONT);
                                registry_thaw(pid);
                                std::cout<<"[Tier] SIGCONT "<<nit->second.name<<"\n";
                            }
                            // Restore nice value
                            ::setpriority(PRIO_PROCESS, pid, 0);
                            CgroupManager::move_pid(pid, target_slice);
                        }
                    }
                }
            }
        }
    }

    // Core transition logic — now composite + AI-modifier aware
    void do_transition(WorkloadType np, bool multi_ctx=false, bool ai_mod=false) {
        // Serialize transitions — debounce thread and CLI thread both call this
        std::lock_guard<std::mutex> lk(mtx);
        if (np==WorkloadType::NEUTRAL||np==cur) return;
        WorkloadType old=cur; cur=np;
        MemInfo mem=read_meminfo(); float pct=mem.pct();
        int temp_mc=read_max_temp_mc(), temp_c=temp_mc/1000;
        bool hot=(temp_mc>=cfg.thermal_hot_mc);
        float fkill=cfg.ram_kill_pct+(hot?10.0f:0.0f);

        // PROBLEM 6: AI modifier raises memory ceiling by 30% (capped at 85%)
        if (ai_mod) {
            float boosted = std::min(cfg.active_ceiling * 1.3f, 0.85f);
            std::cout<<"[AI] ai_modifier=true — memory ceiling raised to "<<(int)(boosted*100)<<"%\n";
            fkill = std::max(fkill, (1.0f - boosted) * 100.0f);
        }

        PSI psi=read_memory_psi();
        MemoryGraph g; g.build();

        std::cout<<"\n============================================\n"
                 <<"[HWM] "<<wt_label(old)<<" → "<<wt_label(np)
                 <<"  multi_ctx="<<(multi_ctx?"yes":"no")
                 <<"  ai="<<(ai_mod?"yes":"no")<<"\n"
                 <<"[HWM] RAM: "<<mem.available_kb/1024<<"MB free/"<<mem.total_kb/1024<<"MB ("<<(int)pct<<"%)"
                 <<(hot?"  🌡️ HOT":"")<<"  PSI="<<psi.some_avg10<<"%\n";

        std::unordered_set<std::string> to_prune, to_thaw;
        std::string theme;

        if (np==WorkloadType::WEB_DEV) {
            to_prune={"studio","java","gradle","emulator","rust-analyzer","clangd","ccls","cargo"};
            to_thaw=web_tools; theme="keyword general:col.active_border rgba(00d8ffff) rgba(f7df1eff) 45deg";
            set_cpu_governor(hot ? "powersave" : "performance"); set_swappiness(ai_mod?5:10);
            set_io_scheduler("mq-deadline");   // §5.2.7: web dev — low-latency I/O
        } else if (np==WorkloadType::ANDROID_DEV) {
            to_prune={"node","webpack","vite","bun","rust-analyzer","clangd","ccls","docker","dockerd"};
            to_thaw=android_tools; theme="keyword general:col.active_border rgba(3ddc84ff) rgba(073042ff) 45deg";
            set_cpu_governor(hot ? "powersave" : "performance"); set_swappiness(ai_mod?5:10);
            set_io_scheduler("bfq");            // §5.2.7: android — balanced I/O for emulator/gradle
        } else if (np==WorkloadType::SYSTEM_DEV) {
            to_prune={"studio","java","gradle","emulator","node","webpack","electron","dockerd"};
            to_thaw=system_tools; theme="keyword general:col.active_border rgba(1793d1ff) rgba(333333ff) 45deg";
            set_cpu_governor(hot ? "powersave" : "performance"); set_swappiness(ai_mod?5:10);
            set_io_scheduler("mq-deadline");   // §5.2.7: system dev — fast compile I/O
        } else { // CASUAL
            to_thaw=casual_tools;
            std::unordered_set<std::string> all_dev;
            for (auto& s:web_tools) all_dev.insert(s);
            for (auto& s:android_tools) all_dev.insert(s);
            for (auto& s:system_tools) all_dev.insert(s);
            for (auto& s:{"studio","java","gradle","emulator","node","webpack","vite","bun",
                          "electron","dockerd","docker","rust-analyzer","clangd","ccls","cargo","make","cmake","gdb"}) all_dev.insert(s);
            to_prune=all_dev;
            theme="keyword general:col.active_border rgba(cba6f7ff) rgba(89b4faff) 45deg";
            set_cpu_governor(hot ? "powersave" : "schedutil"); set_swappiness(60);
            set_io_scheduler("bfq");            // §5.2.7: casual — fair I/O
        }

        // P2: When multi_context, remove LSP/build daemons from prune set entirely
        if (multi_ctx && cfg.lsp_protection) {
            for (auto& lsp : cfg.lsp_binaries)  to_prune.erase(lsp);
            for (auto& bd  : cfg.build_daemons) to_prune.erase(bd);
            std::cout<<"[Multi-ctx] LSP/build daemons protected from prune\n";
        }

        // Thaw active tools + protect OOM
        Pruner::thaw(to_thaw, g.graph, cfg.oom_protect);
        for (const auto& [pid,n]:g.graph)
            if (to_thaw.count(n.name)&&!AUDIO_WHITELIST.count(n.name))
                set_oom(pid, cfg.oom_protect);

        // Casual apps
        if (np!=WorkloadType::CASUAL) {
            std::cout<<"[HWM] Managing casual apps...\n";
            Pruner::manage(casual_tools,g.graph,pct,cfg.ram_freeze_pct,fkill,
                           mem.available_kb,cfg.oom_expose,cfg.sigterm_grace_ms,false,nullptr);
        } else {
            Pruner::thaw(casual_tools,g.graph,cfg.oom_protect);
        }

        // Manage competing dev tools via PSI-aware ladder
        long freed_kb = Pruner::manage(to_prune,g.graph,pct,cfg.ram_freeze_pct,fkill,
                                       mem.available_kb,cfg.oom_expose,cfg.sigterm_grace_ms,
                                       multi_ctx, &cfg);

        // Rebalance all workspaces after transition
        rebalance_workspaces(g.graph);
        HyprlandIPC::send(theme);

        long freed_mb=freed_kb/1024;
        std::string action;
        if      (psi.some_avg10>=40.0f) action="emergency-kill";
        else if (psi.some_avg10>=20.0f) action="cgroup-freeze";
        else if (psi.some_avg10>= 5.0f) action="cgroup-throttle";
        else                            action="cgroup-deprioritize";
        if (ai_mod) action+=" [AI+]";

        write_state(wt_label(np),pct,action,freed_mb,temp_c,psi.some_avg10,multi_ctx,ai_mod);

        std::string notif_body="RAM: "+std::to_string((int)pct)+"%"
            +"  PSI="+std::to_string((int)psi.some_avg10)+"%"
            +"  Action: "+action
            +(multi_ctx?" [multi-ctx]":"")
            +(ai_mod?" [AI+]":"")
            +(hot?"  🌡️ "+std::to_string(temp_c)+"°C":"");
        notify(std::string(wt_label(np))+" Profile Active", notif_body);
        std::cout<<"[HWM] Done. Freed ~"<<freed_mb<<"MB  PSI="<<psi.some_avg10<<"%\n============================================\n";
    }

    // Debounce-aware event scheduler — now carries title + pid for fusion
    void schedule(const std::string& wm_class, const std::string& title="",
                  pid_t pid=0, int ws=-1, bool is_ws=false) {
        int gen;
        { std::lock_guard<std::mutex> lk(mtx);
          pending_class=wm_class; pending_title=title; pending_pid=pid;
          pending_ws=ws; pending_is_ws=is_ws; have_pending=true; }
        gen = ++debounce_gen;
        // §5.2.5: move old thread out and detach only after gen invalidates it,
        // preventing zombie accumulation on rapid workspace switches
        std::thread old_thd = std::move(debounce_thd);
        if (old_thd.joinable()) old_thd.detach();
        debounce_thd=std::thread([this,gen](){
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.debounce_ms));
            if (debounce_gen.load()!=gen||!have_pending.load()) return;
            std::string wc,ev_title; pid_t ev_pid; int ev_ws; bool ev_is_ws;
            { std::lock_guard<std::mutex> lk(mtx); if(!have_pending.load())return;
              wc=pending_class; ev_title=pending_title; ev_pid=pending_pid;
              ev_ws=pending_ws; ev_is_ws=pending_is_ws; have_pending=false; }
            if (ev_is_ws) {
                // Workspace switch — update active workspace, rebalance tiers (P3/P4/P5)
                { std::lock_guard<std::mutex> lk(ws_mtx);
                  if (workspace_states.count(active_workspace_id))
                      workspace_states[active_workspace_id].last_active_ms = ms_clock::now();
                  active_workspace_id = ev_ws;
                  workspace_states[ev_ws].last_active_ms = ms_clock::now(); }
                WorkloadType np = classify_ws(ev_ws);
                do_transition(np);
            } else {
                // Window focus — run fusion classifier (Signal 1+2+3)
                MemoryGraph g; g.build();
                FusionResult fr = classify_window(wc, ev_title, ev_pid, g.graph);

                // ── Sandbox registry lookup ───────────────────────────────────
                // titan-exec-hook sandboxes processes BEFORE they launch and
                // writes: ORIGINAL_PID=<n> BOX_PID=<m> to /run/titan-sandbox/registry
                // THM reads that file to get the box_pid for cgroup assignment.
                // We never call launch_classified() here — the process is already
                // running by the time activewindowv2 fires; namespaces are set at
                // fork time and cannot be applied retroactively.
                pid_t effective_pid = ev_pid;
                if (ev_pid > 0) {
                    bool already_seen;
                    { std::lock_guard<std::mutex> slk(sandbox_mtx);
                      already_seen = sandboxed_pids.count(ev_pid) > 0;
                      sandboxed_pids.insert(ev_pid); }
                    if (!already_seen) {
                        // Look up this PID in the registry written by titan-exec-hook
                        std::ifstream reg("/run/titan-sandbox/registry");
                        std::string line;
                        while (std::getline(reg, line)) {
                            // Format: ORIGINAL_PID=<n> BOX_PID=<m> BINARY=<name>
                            auto op = line.find("ORIGINAL_PID=");
                            auto bp = line.find(" BOX_PID=");
                            if (op == std::string::npos || bp == std::string::npos) continue;
                            pid_t orig = 0, box = 0;
                            try {
                                orig = std::stoi(line.substr(op + 13, bp - op - 13));
                                auto rest = line.substr(bp + 9);
                                box  = std::stoi(rest.substr(0, rest.find(' ')));
                            } catch (...) { continue; }
                            if (orig == ev_pid && box > 0) {
                                effective_pid = box;
                                CgroupManager::move_pid(box, "titan-active.slice");
                                set_oom(box, cfg.oom_protect);
                                { std::lock_guard<std::mutex> slk(sandbox_mtx);
                                  sandboxed_pids.insert(box); }
                                std::cout << "[Sandbox] PID=" << ev_pid
                                          << " → box_pid=" << box
                                          << " (cgroup: titan-active.slice)\n";
                                break;
                            }
                        }
                        // If not in registry: process launched without exec-hook
                        // (e.g. from terminal, not a keybind). Log and continue —
                        // THM still manages it via cgroup tier, just un-sandboxed.
                        if (effective_pid == ev_pid)
                            std::cout << "[Sandbox] PID=" << ev_pid
                                      << " (" << wc << ") not in registry — unsandboxed\n";
                    }
                }
                // ─────────────────────────────────────────────────────────────

                std::cout<<"[Fusion] Result: "<<wt_label(fr.type)
                         <<" conf="<<fr.confidence
                         <<" lsp="<<fr.has_lsp
                         <<" build="<<fr.has_build
                         <<" ai="<<fr.ai_modifier<<"\n";

                // §5.2.2: Build workspace composite from ALL windows on active workspace
                // Query j/clients to get every PID on this workspace, classify each,
                // then feed into build_composite() for multi-window scoring.
                std::string sock = HyprlandIPC::find_sock(".socket.sock");
                auto ws_pids = query_workspace_pids(sock);
                std::vector<WindowContext> ws_windows;

                int active_ws_snap;
                { std::lock_guard<std::mutex> lk(ws_mtx); active_ws_snap = active_workspace_id; }

                auto pit = ws_pids.find(active_ws_snap);
                if (pit != ws_pids.end()) {
                    for (pid_t wpid : pit->second) {
                        // Find wm_class from graph (name field from /proc/stat)
                        std::string wname;
                        auto nit = g.graph.find(wpid);
                        if (nit != g.graph.end()) wname = nit->second.name;

                        FusionResult wfr = classify_window(wname, "", wpid, g.graph);
                        WindowContext wctx;
                        wctx.pid             = wpid;
                        wctx.wm_class        = wname;
                        wctx.inferred_type   = wfr.type;
                        wctx.has_active_lsp  = wfr.has_lsp;
                        wctx.has_build_daemon= wfr.has_build;
                        wctx.ai_modifier     = wfr.ai_modifier;
                        wctx.is_focused      = (wpid == ev_pid);  // focused = resource hint only
                        ws_windows.push_back(wctx);
                    }
                }
                // Fallback: if j/clients returned nothing, use the focused window alone
                if (ws_windows.empty()) {
                    WindowContext wctx;
                    wctx.pid=ev_pid; wctx.wm_class=wc; wctx.inferred_type=fr.type;
                    wctx.has_active_lsp=fr.has_lsp; wctx.has_build_daemon=fr.has_build;
                    wctx.ai_modifier=fr.ai_modifier; wctx.is_focused=true;
                    ws_windows.push_back(wctx);
                }

                // Persist windows into workspace state for tier/decay logic
                { std::lock_guard<std::mutex> lk(ws_mtx);
                  workspace_states[active_ws_snap].windows = ws_windows; }

                WorkspaceProfile composite = build_composite(ws_windows);
                std::cout<<"[Composite] WS"<<active_ws_snap
                         <<" dominant="<<wt_label(composite.dominant)
                         <<" multi_ctx="<<composite.multi_context
                         <<" ai="<<composite.ai_modifier<<"\n";

                // dominant WorkloadType drives transition; multi_context enables protection
                do_transition(composite.dominant, composite.multi_context, composite.ai_modifier);
            }
        });
    }

    // CLI socket server
    void run_cli_server() {
        unlink("/tmp/titan_hwm.sock");
        int srv=socket(AF_UNIX,SOCK_STREAM,0); if (srv<0) return;
        struct sockaddr_un a{}; a.sun_family=AF_UNIX;
        strncpy(a.sun_path,"/tmp/titan_hwm.sock",sizeof(a.sun_path)-1);
        if (bind(srv,(struct sockaddr*)&a,sizeof(a))<0){close(srv);return;}
        listen(srv,5);
        std::cout<<"[HWM] CLI socket ready: /tmp/titan_hwm.sock\n";
        while (running.load()) {
            int cl=accept(srv,nullptr,nullptr); if (cl<0) continue;
            char buf[64]{}; read(cl,buf,sizeof(buf)-1);
            std::string cmd(buf); cmd.erase(cmd.find_last_not_of(" \t\r\n")+1);
            if      (cmd=="switch casual"||cmd=="sw casual")   do_transition(WorkloadType::CASUAL);
            else if (cmd=="switch web"||cmd=="sw web")         do_transition(WorkloadType::WEB_DEV);
            else if (cmd=="switch android"||cmd=="sw android") do_transition(WorkloadType::ANDROID_DEV);
            else if (cmd=="switch system"||cmd=="sw system")   do_transition(WorkloadType::SYSTEM_DEV);
            else if (cmd=="status") {
                MemInfo m=read_meminfo(); int t_c=read_max_temp_mc()/1000;
                WorkloadType snap_cur;
                { std::lock_guard<std::mutex> lk(mtx); snap_cur = cur; }
                std::string r="profile="+std::string(wt_label(snap_cur))
                             +"\nram_pct="+std::to_string((int)m.pct())
                             +"\nram_free_mb="+std::to_string(m.available_kb/1024)
                             +"\ncpu_temp_c="+std::to_string(t_c)+"\n";
                write(cl,r.c_str(),r.size());
            }
            close(cl);
        }
        close(srv); unlink("/tmp/titan_hwm.sock");
    }

public:
    // Rogue DM guard — extended to protect LSP/build daemons from scan (unchanged logic)
    // Helper: read a field from /proc/<pid>/status
    static std::string read_proc_status_field(const std::string& pid_str, const std::string& field) {
        std::ifstream f("/proc/" + pid_str + "/status");
        std::string line;
        while (std::getline(f, line)) {
            if (line.rfind(field + ":", 0) == 0) {
                size_t start = line.find_first_not_of(" \t", field.size() + 1);
                if (start != std::string::npos) return line.substr(start);
            }
        }
        return "";
    }

    // §5.2.7 — inotify config hot-reload thread
    // Watches ~/.config/titan-hwm/config for IN_CLOSE_WRITE and reloads live
    void run_config_hotreload() {
        const char* h = getenv("HOME"); if (!h) return;
        std::string cfg_path = std::string(h) + "/.config/titan-hwm/config";
        int ifd = inotify_init1(IN_NONBLOCK);
        if (ifd < 0) { std::cerr<<"[HotReload] inotify_init1 failed\n"; return; }
        int wd = inotify_add_watch(ifd, cfg_path.c_str(), IN_CLOSE_WRITE | IN_MODIFY);
        if (wd < 0) {
            // File may not exist yet — watch parent dir for creation
            std::string dir = std::string(h) + "/.config/titan-hwm";
            wd = inotify_add_watch(ifd, dir.c_str(), IN_CREATE | IN_CLOSE_WRITE);
        }
        std::cout<<"[HotReload] Watching config: "<<cfg_path<<"\n";
        char ev_buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            ssize_t len = read(ifd, ev_buf, sizeof(ev_buf));
            if (len <= 0) continue;
            // Any event → reload config
            Config new_cfg = load_config();
            { std::lock_guard<std::mutex> lk(mtx); cfg = new_cfg; }
            apply_extras();
            std::cout<<"[HotReload] Config reloaded from "<<cfg_path<<"\n";
        }
        close(ifd);
    }

    void run_process_guard() {
        std::cout<<"[HWM] Process guard active — waits for confirmed Hyprland session\n";
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(60));
            if (!running.load()) break;

            // SAFETY GATE: only run the guard if a live Hyprland socket exists.
            // If Hyprland is not running, we are still in the SDDM/login phase —
            // killing anything here would destroy the login session.
            std::string hypr_sock = HyprlandIPC::find_sock(".socket.sock");
            if (hypr_sock.empty()) {
                std::cout<<"[Guard] No Hyprland socket — skipping scan (not in session yet)\n";
                continue;
            }

            for (const auto& e : fs::directory_iterator("/proc")) {
                if (!e.is_directory()) continue;
                std::string d = e.path().filename().string();
                if (!std::all_of(d.begin(), d.end(), ::isdigit)) continue;

                std::ifstream sf(e.path().string() + "/stat"); std::string l;
                if (!std::getline(sf, l)) continue;
                size_t lp = l.find('('), rp = l.rfind(')');
                if (lp == std::string::npos || rp == std::string::npos) continue;
                std::string name = l.substr(lp + 1, rp - lp - 1);

                if (!rogue_dms.count(name)) continue;

                pid_t pid = std::stoi(d);

                // SAFETY CHECK 1: read the UID of this process.
                // Only act on processes owned by non-root users (uid > 0) OR
                // root-owned DMs that are NOT PID 1 children (i.e., not core services).
                std::string uid_str = read_proc_status_field(d, "Uid");
                std::string ppid_str = read_proc_status_field(d, "PPid");
                int uid = -1, ppid = -1;
                try { uid = std::stoi(uid_str); } catch(...) { continue; }
                try { ppid = std::stoi(ppid_str); } catch(...) { continue; }

                // SAFETY CHECK 2: if PPID==1 and uid==0, this is a core system daemon
                // started by systemd — never touch it. (SDDM has ppid=1, uid=0)
                if (ppid == 1 && uid == 0) {
                    std::cout<<"[Guard] Skipping " <<name<<" (PID "<<pid<<") — system daemon (ppid=1,uid=0)\n";
                    continue;
                }

                // SAFETY CHECK 3: pid=1 itself is init — never kill.
                if (pid <= 1) continue;

                std::cout<<"[Guard] Rogue DM: "<<name<<" PID:"<<pid<<" uid="<<uid<<" ppid="<<ppid<<" — terminating\n";
                notify("ArchTitan Session Guard", "Blocked rogue DM: " + name);
                kill(pid, SIGTERM);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                if (kill(pid, 0) == 0) kill(pid, SIGKILL);
            }
        }
    }

    ~TitanHardwareManager() {
        running=false;
        if (debounce_thd.joinable())    debounce_thd.join();
        if (cli_thd.joinable())         cli_thd.detach();
        if (guard_thd.joinable())       guard_thd.detach();
        if (hotreload_thd.joinable())   hotreload_thd.detach();
    }

    void run() {
        // ── LIVE ISO GUARD ────────────────────────────────────────────────────
        // On live media (/run/archiso exists) the THM has no useful work to do:
        // there is no persistent dev workspace, and its process guard + cgroup
        // management can destabilize the Calamares installer session.
        // Enter a permanent sleep loop instead of the normal operating path.
        if (fs::exists("/run/archiso")) {
            std::cout << "[HWM] Live ISO detected — entering inert mode. "
                         "All resource management disabled to protect installer.\n";
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
        }
        // ─────────────────────────────────────────────────────────────────────

        cfg=load_config(); apply_extras();

        std::string sock=HyprlandIPC::find_sock(".socket2.sock");
        if (sock.empty()) { std::this_thread::sleep_for(std::chrono::seconds(5)); return; }

        // Fix Waybar "not running" issue: initialize state immediately upon connecting
        MemInfo m=read_meminfo(); int t_c=read_max_temp_mc()/1000;
        PSI psi=read_memory_psi();
        write_state("Neutral", m.pct(), "started", 0, t_c, psi.some_avg10, false, false);

        std::string sock_cmd = HyprlandIPC::find_sock(".socket.sock");
        auto visible = query_visible_workspaces(sock_cmd);
        if (!visible.empty()) {
            schedule("","",-1,*visible.begin(),true);
        }

        cli_thd       = std::thread(&TitanHardwareManager::run_cli_server,    this);
        guard_thd     = std::thread(&TitanHardwareManager::run_process_guard,  this);
        hotreload_thd = std::thread(&TitanHardwareManager::run_config_hotreload, this);

        while (true) {
            int fd=socket(AF_UNIX,SOCK_STREAM,0);
            if (fd<0) { std::cerr<<"[HWM] socket() failed: "<<strerror(errno)<<" — retrying\n"; std::this_thread::sleep_for(std::chrono::seconds(3)); continue; }
            struct sockaddr_un a{}; a.sun_family=AF_UNIX;
            strncpy(a.sun_path,sock.c_str(),sizeof(a.sun_path)-1);
            if (connect(fd,(struct sockaddr*)&a,sizeof(a))<0) {
                close(fd); std::this_thread::sleep_for(std::chrono::seconds(3)); continue;
            }
            std::cout<<"[HWM] v2 Online | Fusion | Tiers | AI-mod | Dual-monitor | PSI\n";
            char buf[4096]; std::string leftover;
            while (true) {
                ssize_t n=read(fd,buf,sizeof(buf)-1); if (n<=0) break;
                buf[n]='\0'; std::string data=leftover+buf; size_t pos;
                while ((pos=data.find('\n'))!=std::string::npos) {
                    std::string ev=data.substr(0,pos); data.erase(0,pos+1);

                    // activewindowv2>>address,pid,wm_class,title — use pid for fusion
                    if (ev.rfind("activewindowv2>>",0)==0) {
                        std::string info=ev.substr(16);
                        // format: addr,pid,class,title
                        size_t c1=info.find(',');
                        if (c1!=std::string::npos) {
                            size_t c2=info.find(',',c1+1);
                            if (c2!=std::string::npos) {
                                pid_t pid=0;
                                try { pid=std::stoi(info.substr(c1+1,c2-c1-1)); } catch(...) {}
                                size_t c3=info.find(',',c2+1);
                                std::string wm_class = (c3!=std::string::npos) ? info.substr(c2+1,c3-c2-1) : info.substr(c2+1);
                                std::string title    = (c3!=std::string::npos) ? info.substr(c3+1) : "";
                                schedule(wm_class, title, pid);
                            }
                        }
                    }
                    // activewindow>>class,title — fallback (no pid, fusion Signal1/3 won't fire)
                    else if (ev.rfind("activewindow>>",0)==0) {
                        std::string info=ev.substr(14);
                        size_t c=info.find(',');
                        if (c!=std::string::npos) {
                            schedule(info.substr(0,c), info.substr(c+1), 0);
                        }
                    }
                    // workspace>>id — P3/P4/P5
                    if (ev.rfind("workspace>>",0)==0) {
                        std::string ws_str=ev.substr(11);
                        try { int ws=std::stoi(ws_str); schedule("","",-1,ws,true); } catch(...) {}
                    }
                }
                leftover=data;
            }
            close(fd);
        }
    }
};

int main() {
    write_default_config();
    while (true) {
        TitanHardwareManager d; d.run();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
