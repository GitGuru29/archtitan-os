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

namespace fs = std::filesystem;

static const std::unordered_set<std::string> AUDIO_WHITELIST = {
    "spotify","spotifyd","mpd","mpdris2","mpv","vlc","rhythmbox",
    "strawberry","deadbeef","cmus","ncmpcpp","cantata","audacious",
    "elisa","playerctld","pipewire","pipewire-pulse","wireplumber","pulseaudio"
};

// ==========================================
// Config
// ==========================================
struct Config {
    int   debounce_ms      = 800;
    float ram_freeze_pct   = 50.0f;
    float ram_kill_pct     = 25.0f;
    int   thermal_hot_mc   = 80000;
    std::unordered_set<int> casual_ws{1}, web_ws{2}, android_ws{3}, system_ws{4,5};
    std::vector<std::string> web_extra, android_extra, system_extra;
};

static std::vector<std::string> csv(const std::string& s) {
    std::vector<std::string> r; std::istringstream ss(s); std::string t;
    while (std::getline(ss, t, ',')) {
        t.erase(0,t.find_first_not_of(" \t")); t.erase(t.find_last_not_of(" \t")+1);
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
        if (k=="debounce_ms")           c.debounce_ms     = std::stoi(v);
        else if (k=="ram_freeze_pct")   c.ram_freeze_pct  = std::stof(v);
        else if (k=="ram_kill_pct")     c.ram_kill_pct    = std::stof(v);
        else if (k=="thermal_hot_temp") c.thermal_hot_mc  = std::stoi(v)*1000;
        else if (k=="casual_workspaces")  c.casual_ws     = csv_int(v);
        else if (k=="web_workspaces")     c.web_ws        = csv_int(v);
        else if (k=="android_workspaces") c.android_ws    = csv_int(v);
        else if (k=="system_workspaces")  c.system_ws     = csv_int(v);
        else if (k=="web_extra_tools")    c.web_extra     = csv(v);
        else if (k=="android_extra_tools")c.android_extra = csv(v);
        else if (k=="system_extra_tools") c.system_extra  = csv(v);
    }
    return c;
}

static void write_default_config() {
    const char* h = getenv("HOME"); if (!h) return;
    std::string path = std::string(h)+"/.config/titan-hwm/config";
    if (fs::exists(path)) return;
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream f(path);
    f << "# Titan Hardware Manager Config\n"
      << "debounce_ms = 800\n"
      << "ram_freeze_pct = 50\n"
      << "ram_kill_pct = 25\n"
      << "thermal_hot_temp = 80\n"
      << "casual_workspaces = 1\n"
      << "web_workspaces = 2\n"
      << "android_workspaces = 3\n"
      << "system_workspaces = 4, 5\n"
      << "web_extra_tools =\n"
      << "android_extra_tools =\n"
      << "system_extra_tools =\n";
    std::cout << "[HWM] Default config written to " << path << "\n";
}

// ==========================================
// meminfo
// ==========================================
struct MemInfo {
    long total_kb=0, available_kb=0;
    float pct() const { return total_kb?((float)available_kb/total_kb*100.0f):100.0f; }
};
static MemInfo read_meminfo() {
    MemInfo m; std::ifstream f("/proc/meminfo"); std::string l;
    while (std::getline(f,l)) {
        if (l.rfind("MemTotal:",0)==0){std::istringstream ss(l.substr(9));ss>>m.total_kb;}
        if (l.rfind("MemAvailable:",0)==0){std::istringstream ss(l.substr(13));ss>>m.available_kb;}
        if (m.total_kb&&m.available_kb) break;
    }
    return m;
}

// ==========================================
// Thermal
// ==========================================
static int read_max_temp_mc() {
    int mx=0;
    try {
        for (const auto& z : fs::directory_iterator("/sys/class/thermal")) {
            if (z.path().filename().string().rfind("thermal_zone",0)!=0) continue;
            std::ifstream f(z.path().string()+"/temp"); int t=0;
            if (f>>t) mx=std::max(mx,t);
        }
    } catch (...) {}
    return mx;
}

// ==========================================
// System Tuner
// ==========================================
static void set_cpu_governor(const std::string& gov) {
    try {
        for (const auto& cpu : fs::directory_iterator("/sys/devices/system/cpu")) {
            std::string n=cpu.path().filename().string();
            if (n.rfind("cpu",0)!=0||!std::isdigit(n[3])) continue;
            std::string p=cpu.path().string()+"/cpufreq/scaling_governor";
            if (!fs::exists(p)) continue;
            std::ofstream f(p); if (f) f<<gov;
        }
        std::cout<<"[SysTuner] CPU governor → "<<gov<<"\n";
    } catch (...) { std::cout<<"[SysTuner] governor set failed (need root)\n"; }
}
static void set_swappiness(int v) {
    std::ofstream f("/proc/sys/vm/swappiness");
    if (f) { f<<v; std::cout<<"[SysTuner] swappiness → "<<v<<"\n"; }
}
static void set_oom(pid_t pid, int score) {
    std::ofstream f("/proc/"+std::to_string(pid)+"/oom_score_adj");
    if (f) f<<score;
}

// ==========================================
// Notify
// ==========================================
static void notify(const std::string& title, const std::string& body) {
    std::string cmd="notify-send --app-name='Titan HWM' --icon=dialog-information '"
                    +title+"' '"+body+"' &";
    system(cmd.c_str());
}

// ==========================================
// Process Graph
// ==========================================
struct ProcessNode {
    pid_t pid=0,ppid=0; std::string name; char state='?'; long rss_kb=0;
    std::vector<pid_t> children;
};
static long read_rss(pid_t pid) {
    std::ifstream f("/proc/"+std::to_string(pid)+"/status"); std::string l;
    while (std::getline(f,l)) if (l.rfind("VmRSS:",0)==0){
        std::istringstream ss(l.substr(6)); long v=0; ss>>v; return v;
    }
    return 0;
}
class MemoryGraph {
public:
    std::unordered_map<pid_t,ProcessNode> graph;
    void build() {
        graph.clear();
        for (const auto& e : fs::directory_iterator("/proc")) {
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

// ==========================================
// Pruner
// ==========================================
class Pruner {
public:
    static void sig_tree(pid_t pid, const std::unordered_map<pid_t,ProcessNode>& g, int sig) {
        auto it=g.find(pid); if (it==g.end()) return;
        if (AUDIO_WHITELIST.count(it->second.name)) { std::cout<<"[Pruner] Skip audio: "<<it->second.name<<" 🎵\n"; return; }
        for (pid_t c:it->second.children) sig_tree(c,g,sig);
        const char* lbl=(sig==SIGSTOP)?"Freeze":(sig==SIGCONT)?"Thaw":"Kill";
        std::cout<<"[Pruner] "<<lbl<<" "<<it->second.name<<" ("<<it->second.rss_kb/1024<<"MB)\n";
        if (sig==SIGSTOP) setpriority(PRIO_PROCESS,pid,19);
        if (sig==SIGCONT) { setpriority(PRIO_PROCESS,pid,0); set_oom(pid,-300); }
        kill(pid,sig);
        if (sig==SIGTERM) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); if (kill(pid,0)==0) kill(pid,SIGKILL); }
    }
    static void thaw(const std::unordered_set<std::string>& t, const std::unordered_map<pid_t,ProcessNode>& g) {
        for (const auto& [pid,n]:g) if (t.count(n.name)&&n.state=='T') sig_tree(pid,g,SIGCONT);
    }
    static long prune(const std::unordered_set<std::string>& targets,
                      const std::unordered_map<pid_t,ProcessNode>& g,
                      float pct, float freeze_t, float kill_t) {
        if (pct>freeze_t) { std::cout<<"[Pruner] RAM OK ("<<(int)pct<<"%) skip\n"; return 0; }
        std::vector<const ProcessNode*> hits;
        for (const auto& [pid,n]:g) {
            if (targets.count(n.name)&&!AUDIO_WHITELIST.count(n.name)) hits.push_back(&n);
            if (targets.count(n.name)&&!AUDIO_WHITELIST.count(n.name)) set_oom(pid,500);
        }
        std::sort(hits.begin(),hits.end(),[](auto a,auto b){return a->rss_kb>b->rss_kb;});
        int sig=(pct<kill_t)?SIGTERM:SIGSTOP;
        std::cout<<"[Pruner] "<<(sig==SIGTERM?"TERMINATE":"FREEZE")<<" ("<<(int)pct<<"% free)\n";
        long freed=0;
        for (auto* n:hits) { freed+=n->rss_kb; sig_tree(n->pid,g,sig); }
        return freed;
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
// State file
// ==========================================
static void write_state(const std::string& profile,float pct,const std::string& action,long freed_mb,int temp_c) {
    std::ofstream f("/tmp/titan_hwm_state"); if (!f) return;
    f<<"profile="<<profile<<"\nmem_available_pct="<<(int)pct<<"\nlast_action="<<action
     <<"\nfreed_mb="<<freed_mb<<"\ncpu_temp_c="<<temp_c
     <<"\ntimestamp="<<std::chrono::duration_cast<std::chrono::seconds>(
         std::chrono::system_clock::now().time_since_epoch()).count()<<"\n";
}

// ==========================================
// Profiles
// ==========================================
enum class DevProfile { CASUAL, WEB_DEV, ANDROID_DEV, SYSTEM_DEV, NEUTRAL };

class TitanHardwareManager {
    Config cfg;
    DevProfile cur = DevProfile::NEUTRAL;
    std::atomic<bool> have_pending{false}, running{true};
    std::mutex mtx;
    std::string pending_class; int pending_ws=-1; bool pending_is_ws=false;
    std::thread debounce_thd, cli_thd;

    std::unordered_set<std::string> web_tools{"node","webpack","vite","esbuild","bun","figma-linux"};
    std::unordered_set<std::string> android_tools{"studio","java","gradle","emulator","adb"};
    std::unordered_set<std::string> system_tools{"rust-analyzer","clangd","cargo","make","cmake","gdb"};
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

    DevProfile classify_win(const std::string& raw) {
        std::string w=raw; std::transform(w.begin(),w.end(),w.begin(),::tolower);
        if (w=="code"||w=="figma"||w=="firefox-developer-edition") return DevProfile::WEB_DEV;
        if (w.find("studio")!=std::string::npos||w=="emulator")    return DevProfile::ANDROID_DEV;
        if (w=="alacritty"||w=="kitty"||w=="clion"||w=="neovim")   return DevProfile::SYSTEM_DEV;
        if (w=="telegram-desktop"||w=="discord"||w=="slack"||
            w=="thunderbird"||w=="element"||w=="geary"||
            w=="nautilus"||w=="dolphin"||w=="thunar")              return DevProfile::CASUAL;
        return DevProfile::NEUTRAL;
    }
    DevProfile classify_ws(int ws) {
        if (cfg.web_ws.count(ws))     return DevProfile::WEB_DEV;
        if (cfg.android_ws.count(ws)) return DevProfile::ANDROID_DEV;
        if (cfg.system_ws.count(ws))  return DevProfile::SYSTEM_DEV;
        if (cfg.casual_ws.count(ws))  return DevProfile::CASUAL;
        return DevProfile::NEUTRAL;
    }
    const char* lbl(DevProfile p) {
        switch(p){case DevProfile::CASUAL:return "Casual";case DevProfile::WEB_DEV:return "Web Dev";
                  case DevProfile::ANDROID_DEV:return "Android Dev";case DevProfile::SYSTEM_DEV:return "System Dev";default:return "Neutral";}
    }

    void do_transition(DevProfile np) {
        if (np==DevProfile::NEUTRAL||np==cur) return;
        DevProfile old=cur; cur=np;
        MemInfo mem=read_meminfo(); float pct=mem.pct();
        int temp_mc=read_max_temp_mc(), temp_c=temp_mc/1000;
        bool hot=(temp_mc>=cfg.thermal_hot_mc);
        float fkill=cfg.ram_kill_pct+(hot?10.0f:0.0f);
        MemoryGraph g; g.build();

        std::cout<<"\n============================================\n"
                 <<"[HWM] "<<lbl(old)<<" → "<<lbl(np)<<"\n"
                 <<"[HWM] RAM: "<<mem.available_kb/1024<<"MB free/"<<mem.total_kb/1024<<"MB ("<<(int)pct<<"%)"
                 <<(hot?"  🌡️ HOT":"")<<"\n";

        std::unordered_set<std::string> to_prune, to_thaw;
        std::string theme;

        if (np==DevProfile::WEB_DEV) {
            to_prune={"studio","java","gradle","emulator","rust-analyzer","clangd","cargo"};
            to_thaw=web_tools; theme="keyword general:col.active_border rgba(00d8ffff) rgba(f7df1eff) 45deg";
            set_cpu_governor("performance"); set_swappiness(10);
        } else if (np==DevProfile::ANDROID_DEV) {
            to_prune={"node","webpack","vite","bun","rust-analyzer","clangd","docker","dockerd"};
            to_thaw=android_tools; theme="keyword general:col.active_border rgba(3ddc84ff) rgba(073042ff) 45deg";
            set_cpu_governor("performance"); set_swappiness(10);
        } else if (np==DevProfile::SYSTEM_DEV) {
            to_prune={"studio","java","gradle","emulator","node","webpack","electron","dockerd"};
            to_thaw=system_tools; theme="keyword general:col.active_border rgba(1793d1ff) rgba(333333ff) 45deg";
            set_cpu_governor("performance"); set_swappiness(10);
        } else { // CASUAL
            to_thaw=casual_tools;
            std::unordered_set<std::string> all_dev;
            for (auto& s:web_tools) all_dev.insert(s);
            for (auto& s:android_tools) all_dev.insert(s);
            for (auto& s:system_tools) all_dev.insert(s);
            for (auto& s:{"studio","java","gradle","emulator","node","webpack","vite","bun","electron","dockerd","docker","rust-analyzer","clangd","cargo","make","cmake","gdb"}) all_dev.insert(s);
            to_prune=all_dev;
            theme="keyword general:col.active_border rgba(cba6f7ff) rgba(89b4faff) 45deg";
            set_cpu_governor("schedutil"); set_swappiness(60);
        }

        // Also prune casual apps when entering dev
        if (np!=DevProfile::CASUAL) {
            std::cout<<"[HWM] Suspending casual apps...\n";
            Pruner::prune(casual_tools,g.graph,pct,cfg.ram_freeze_pct,fkill);
        } else {
            Pruner::thaw(casual_tools,g.graph);
        }

        Pruner::thaw(to_thaw,g.graph);
        long freed_kb=Pruner::prune(to_prune,g.graph,pct,cfg.ram_freeze_pct,fkill);
        HyprlandIPC::send(theme);

        long freed_mb=freed_kb/1024;
        std::string action=(pct<fkill)?"terminate":(pct<cfg.ram_freeze_pct)?"freeze":"skip";
        write_state(lbl(np),pct,action,freed_mb,temp_c);

        std::string notif_body="RAM freed: "+std::to_string(freed_mb)+"MB  |  "
                               +std::to_string((int)pct)+"% available"
                               +(hot?"  🌡️ "+std::to_string(temp_c)+"°C":"");
        notify(std::string(lbl(np))+" Profile Active", notif_body);
        std::cout<<"[HWM] Done. Freed ~"<<freed_mb<<"MB\n============================================\n";
    }

    void schedule(const std::string& wc, int ws=-1, bool is_ws=false) {
        { std::lock_guard<std::mutex> lk(mtx); pending_class=wc; pending_ws=ws; pending_is_ws=is_ws; have_pending=true; }
        if (debounce_thd.joinable()) debounce_thd.detach();
        debounce_thd=std::thread([this](){
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.debounce_ms));
            if (!have_pending.load()) return;
            std::string wc; int ws; bool is_ws;
            { std::lock_guard<std::mutex> lk(mtx); if (!have_pending.load()) return;
              wc=pending_class; ws=pending_ws; is_ws=pending_is_ws; have_pending=false; }
            DevProfile np = is_ws ? classify_ws(ws) : classify_win(wc);
            do_transition(np);
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
            char buf[64]{}; read(cl,buf,sizeof(buf)-1); close(cl);
            std::string cmd(buf); cmd.erase(cmd.find_last_not_of(" \t\r\n")+1);
            if      (cmd=="switch casual"||cmd=="sw casual")   do_transition(DevProfile::CASUAL);
            else if (cmd=="switch web"||cmd=="sw web")         do_transition(DevProfile::WEB_DEV);
            else if (cmd=="switch android"||cmd=="sw android") do_transition(DevProfile::ANDROID_DEV);
            else if (cmd=="switch system"||cmd=="sw system")   do_transition(DevProfile::SYSTEM_DEV);
            else if (cmd=="status") {
                MemInfo m=read_meminfo();
                std::string r="profile="+std::string(lbl(cur))+" ram="+std::to_string((int)m.pct())+"%\n";
                int sv=socket(AF_UNIX,SOCK_STREAM,0);
                // just re-write state file for status
                write_state(lbl(cur),m.pct(),"idle",0,read_max_temp_mc()/1000);
            }
        }
        close(srv); unlink("/tmp/titan_hwm.sock");
    }

public:
    ~TitanHardwareManager() {
        running=false;
        if (debounce_thd.joinable()) debounce_thd.join();
        if (cli_thd.joinable()) cli_thd.detach();
    }

    void run() {
        cfg=load_config(); apply_extras();
        cli_thd=std::thread(&TitanHardwareManager::run_cli_server,this);

        std::string sock=HyprlandIPC::find_sock(".socket2.sock");
        if (sock.empty()) { std::this_thread::sleep_for(std::chrono::seconds(5)); return; }

        while (true) {
            int fd=socket(AF_UNIX,SOCK_STREAM,0); if (fd<0) exit(1);
            struct sockaddr_un a{}; a.sun_family=AF_UNIX;
            strncpy(a.sun_path,sock.c_str(),sizeof(a.sun_path)-1);
            if (connect(fd,(struct sockaddr*)&a,sizeof(a))<0) {
                close(fd); std::this_thread::sleep_for(std::chrono::seconds(3)); continue;
            }
            std::cout<<"[HWM] Online | CPU-gov | swappiness | OOM | thermal | CLI | notify\n";
            char buf[1024]; std::string leftover;
            while (true) {
                ssize_t n=read(fd,buf,sizeof(buf)-1); if (n<=0) break;
                buf[n]='\0'; std::string data=leftover+buf; size_t pos;
                while ((pos=data.find('\n'))!=std::string::npos) {
                    std::string ev=data.substr(0,pos); data.erase(0,pos+1);
                    // Window focus event
                    if (ev.rfind("activewindow>>",0)==0) {
                        std::string info=ev.substr(14); size_t c=info.find(',');
                        if (c!=std::string::npos) schedule(info.substr(0,c));
                    }
                    // Workspace switch event — more reliable than window class
                    if (ev.rfind("workspace>>",0)==0) {
                        std::string ws_str=ev.substr(11);
                        try { int ws=std::stoi(ws_str); schedule("",ws,true); } catch(...) {}
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
