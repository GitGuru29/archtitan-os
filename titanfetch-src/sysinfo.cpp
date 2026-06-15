#include "sysinfo.h"

#include <QFile>
#include <QDir>
#include <QString>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ─────────────────────────────────────────────────────────────────────────────
// Low-level helpers — pure C stdio, no Qt I/O
// ─────────────────────────────────────────────────────────────────────────────

// Read entire small file into a QString (trimmed). Returns {} on failure.
static QString readSysFile(const char *path)
{
    FILE *f = std::fopen(path, "r");
    if (!f) return {};
    char buf[512] = {};
    std::fgets(buf, sizeof(buf), f);
    std::fclose(f);
    // trim trailing whitespace / newline
    std::size_t len = std::strlen(buf);
    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r' || buf[len-1] == ' '))
        buf[--len] = '\0';
    return QString::fromUtf8(buf);
}

// Scan a text file line-by-line; return value after ':' on the line starting
// with `key`. Handles tabs between key and colon (e.g. /proc/cpuinfo).
static QString procFind(const char *path, const char *key)
{
    FILE *f = std::fopen(path, "r");
    if (!f) return {};
    const std::size_t keyLen = std::strlen(key);
    char buf[512];
    QString result;
    while (std::fgets(buf, sizeof(buf), f)) {
        if (std::strncmp(buf, key, keyLen) == 0) {
            const char *colon = std::strchr(buf, ':');
            if (colon) {
                const char *val = colon + 1;
                while (*val == ' ' || *val == '\t') ++val;
                char tmp[512];
                std::strncpy(tmp, val, sizeof(tmp) - 1);
                tmp[sizeof(tmp) - 1] = '\0';
                std::size_t end = std::strlen(tmp);
                while (end > 0 && (tmp[end-1]=='\n'||tmp[end-1]=='\r'||tmp[end-1]==' '))
                    tmp[--end] = '\0';
                result = QString::fromUtf8(tmp);
            }
            break;
        }
    }
    std::fclose(f);
    return result;
}

// Count entries in a directory (non-dot entries = package count for pacman)
static int countDir(const char *path)
{
    DIR *d = ::opendir(path);
    if (!d) return -1;
    int count = 0;
    struct dirent *ent;
    while ((ent = ::readdir(d)) != nullptr) {
        if (ent->d_name[0] != '.') ++count;
    }
    ::closedir(d);
    return count;
}

// Walk up the process tree to find the terminal emulator
static bool isKnownTerminal(const QString &name)
{
    static const char *known[] = {
        "kitty", "alacritty", "wezterm", "foot", "footclient",
        "gnome-terminal", "konsole", "xterm", "urxvt", "st",
        "tilix", "terminator", "sakura", "roxterm", "xfce4-terminal",
        "lxterminal", "qterminal", "hyper", "blackbox"
    };
    for (const char *k : known)
        if (name == QLatin1String(k)) return true;
    return false;
}

static QString detectTerminal()
{
    // First check env vars set by terminal emulators
    const QString termProg = QString::fromUtf8(qgetenv("TERM_PROGRAM"));
    if (!termProg.isEmpty() && termProg != QLatin1String("unknown"))
        return termProg;

    // Walk process tree up to 8 levels
    pid_t pid = ::getppid();
    for (int i = 0; i < 8; ++i) {
        // read comm
        char commPath[64];
        std::snprintf(commPath, sizeof(commPath), "/proc/%d/comm", (int)pid);
        const QString name = readSysFile(commPath);
        if (isKnownTerminal(name)) return name;

        // read exe symlink for a fuller name
        char exePath[64], exeBuf[512] = {};
        std::snprintf(exePath, sizeof(exePath), "/proc/%d/exe", (int)pid);
        if (::readlink(exePath, exeBuf, sizeof(exeBuf) - 1) > 0) {
            const QString exeName = QString::fromUtf8(exeBuf).section(u'/', -1);
            if (isKnownTerminal(exeName)) return exeName;
        }

        // walk to parent
        char statusPath[64];
        std::snprintf(statusPath, sizeof(statusPath), "/proc/%d/status", (int)pid);
        FILE *f = std::fopen(statusPath, "r");
        if (!f) break;
        char buf[256];
        pid_t ppid = -1;
        while (std::fgets(buf, sizeof(buf), f))
            if (std::strncmp(buf, "PPid:", 5) == 0) {
                std::sscanf(buf + 5, "%d", (int*)&ppid);
                break;
            }
        std::fclose(f);
        if (ppid <= 1) break;
        pid = ppid;
    }

    // Last resort: $TERM (filter out unhelpful values)
    const QString term = QString::fromUtf8(qgetenv("TERM"));
    if (!term.isEmpty() && term != QLatin1String("dumb") && term != QLatin1String("unknown"))
        return term;
    return QStringLiteral("Unknown");
}

// Get first non-loopback IPv4 address
static QString localIP()
{
    struct ifaddrs *ifap = nullptr;
    if (::getifaddrs(&ifap) != 0) return QStringLiteral("Unknown");
    QString result;
    for (struct ifaddrs *ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;
        if (std::strcmp(ifa->ifa_name, "lo") == 0) continue;
        char buf[INET_ADDRSTRLEN];
        auto *sa = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
        if (::inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf))) {
            result = QString::fromUtf8(buf);
            result += QStringLiteral(" (");
            result += QString::fromUtf8(ifa->ifa_name);
            result += QStringLiteral(")");
            break;
        }
    }
    ::freeifaddrs(ifap);
    return result.isEmpty() ? QStringLiteral("Unknown") : result;
}

// GPU name: try NVIDIA driver first, then fall back to vendor string
static QString detectGPU()
{
    // NVIDIA: /proc/driver/nvidia/gpus/<addr>/information
    DIR *gpuDir = ::opendir("/proc/driver/nvidia/gpus");
    if (gpuDir) {
        struct dirent *ent;
        while ((ent = ::readdir(gpuDir)) != nullptr) {
            if (ent->d_name[0] == '.') continue;
            char infoPath[128];
            std::snprintf(infoPath, sizeof(infoPath),
                          "/proc/driver/nvidia/gpus/%s/information", ent->d_name);
            const QString model = procFind(infoPath, "Model:");
            if (!model.isEmpty()) {
                ::closedir(gpuDir);
                return model;
            }
        }
        ::closedir(gpuDir);
    }

    // Intel / AMD: read DRM card1 uevent for PCI_ID, map vendor
    const QString uevent = []() -> QString {
        // Try card0 and card1
        for (const char *c : {"/sys/class/drm/card0/device/uevent",
                               "/sys/class/drm/card1/device/uevent"}) {
            FILE *f = std::fopen(c, "r");
            if (!f) continue;
            char buf[256];
            QString vendor, pciId;
            while (std::fgets(buf, sizeof(buf), f)) {
                if (std::strncmp(buf, "PCI_ID=", 7) == 0) {
                    pciId = QString::fromUtf8(buf + 7).trimmed();
                }
                if (std::strncmp(buf, "PCI_SUBSYS_ID=", 14) == 0) {
                    // extract vendor from first 4 hex digits
                    const QString sub = QString::fromUtf8(buf + 14).trimmed();
                    const QString vid = sub.left(4).toLower();
                    if (vid == QLatin1String("8086")) vendor = QStringLiteral("Intel");
                    else if (vid == QLatin1String("1002")) vendor = QStringLiteral("AMD");
                    else if (vid == QLatin1String("10de")) vendor = QStringLiteral("NVIDIA");
                }
            }
            std::fclose(f);
            if (!vendor.isEmpty())
                return vendor + QStringLiteral(" GPU (") + pciId + QStringLiteral(")");
        }
        return {};
    }();
    return uevent.isEmpty() ? QStringLiteral("Unknown") : uevent;
}

// Disk usage for a given mount point using statvfs
static QString diskUsage(const char *mountPoint)
{
    struct statvfs st {};
    if (::statvfs(mountPoint, &st) != 0) return QStringLiteral("Unknown");
    const unsigned long long blockSize = st.f_frsize;
    const unsigned long long total = (blockSize * st.f_blocks) / (1024ULL * 1024 * 1024);
    const unsigned long long free  = (blockSize * st.f_bfree)  / (1024ULL * 1024 * 1024);
    const unsigned long long used  = total - free;
    return QStringLiteral("%1 GiB / %2 GiB").arg(used).arg(total);
}

// Screen resolution from DRM sysfs
static QString screenResolution()
{
    // Check card1 connectors first (typically discrete GPU output)
    for (const char *base : {"/sys/class/drm/card1", "/sys/class/drm/card0"}) {
        DIR *d = ::opendir(base);
        if (!d) continue;
        struct dirent *ent;
        while ((ent = ::readdir(d)) != nullptr) {
            if (ent->d_name[0] == '.') continue;
            // connector dirs contain a hyphen (e.g. card1-eDP-1)
            if (std::strchr(ent->d_name, '-') == nullptr) continue;
            char modesPath[256];
            std::snprintf(modesPath, sizeof(modesPath), "%s/%s/modes", base, ent->d_name);
            const QString res = readSysFile(modesPath);
            if (!res.isEmpty()) {
                ::closedir(d);
                return res;
            }
        }
        ::closedir(d);
    }
    return QStringLiteral("Unknown");
}

// Battery: capacity% [status]
static QString batteryStatus()
{
    for (const char *bat : {"/sys/class/power_supply/BAT0",
                             "/sys/class/power_supply/BAT1"}) {
        char capPath[128], statusPath[128];
        std::snprintf(capPath,    sizeof(capPath),    "%s/capacity", bat);
        std::snprintf(statusPath, sizeof(statusPath), "%s/status",   bat);
        const QString cap    = readSysFile(capPath);
        const QString status = readSysFile(statusPath);
        if (!cap.isEmpty())
            return cap + QStringLiteral("% [") + (status.isEmpty() ? QStringLiteral("Unknown") : status) + QStringLiteral("]");
    }
    return {}; // no battery
}

// Memory: used / total in MiB
static void parseMeminfo(long &usedMiB, long &totalMiB)
{
    usedMiB = totalMiB = 0;
    FILE *f = std::fopen("/proc/meminfo", "r");
    if (!f) return;
    long total = 0, available = 0;
    char buf[256];
    while (std::fgets(buf, sizeof(buf), f)) {
        long val = 0;
        if (std::sscanf(buf, "MemTotal: %ld kB", &val) == 1)       total = val;
        else if (std::sscanf(buf, "MemAvailable: %ld kB", &val) == 1) available = val;
        if (total && available) break;
    }
    std::fclose(f);
    totalMiB = total / 1024;
    usedMiB  = (total - available) / 1024;
}

// ─────────────────────────────────────────────────────────────────────────────
// SysInfo::fetch
// ─────────────────────────────────────────────────────────────────────────────

SysData SysInfo::fetch()
{
    SysData data;
    auto &F = data.fields;

    // ── user ─────────────────────────────────────────────────────────────────
    {
        char buf[256] = {};
        if (::getlogin_r(buf, sizeof(buf)) == 0 && buf[0])
            data.user = QString::fromUtf8(buf);
        if (data.user.isEmpty())
            data.user = QString::fromUtf8(qgetenv("USER"));
        if (data.user.isEmpty())
            data.user = QStringLiteral("titan");
    }

    // ── host ─────────────────────────────────────────────────────────────────
    {
        char buf[256] = {};
        if (::gethostname(buf, sizeof(buf)) == 0 && buf[0])
            data.host = QString::fromUtf8(buf);
        if (data.host.isEmpty()) {
            QFile hf(QStringLiteral("/etc/hostname"));
            if (hf.open(QIODevice::ReadOnly))
                data.host = QString::fromUtf8(hf.readAll()).trimmed();
        }
        if (data.host.isEmpty())
            data.host = QStringLiteral("archtitan");
    }

    // ── OS ───────────────────────────────────────────────────────────────────
    F.append({QStringLiteral("OS"), QStringLiteral("ArchTitan x86_64")});

    // ── Host model (DMI) ─────────────────────────────────────────────────────
    {
        const QString vendor  = readSysFile("/sys/devices/virtual/dmi/id/sys_vendor");
        const QString product = readSysFile("/sys/devices/virtual/dmi/id/product_name");
        const QString version = readSysFile("/sys/devices/virtual/dmi/id/product_version");
        // Use the more descriptive of product_name vs product_version
        // Lenovo and some OEMs use short codes (e.g. "82KB") for product_name
        QString model = vendor;
        if (!model.isEmpty()) model += QStringLiteral(" ");
        // Prefer version if it's longer and more descriptive than product_name
        if (!version.isEmpty() && version.length() > product.length())
            model += version;
        else if (!product.isEmpty())
            model += product;
        if (!model.isEmpty())
            F.append({QStringLiteral("Host"), model.trimmed()});
    }

    // ── Kernel ───────────────────────────────────────────────────────────────
    {
        struct utsname ub {};
        if (::uname(&ub) == 0)
            F.append({QStringLiteral("Kernel"), QString::fromUtf8(ub.release)});
    }

    // ── Uptime ───────────────────────────────────────────────────────────────
    {
        FILE *f = std::fopen("/proc/uptime", "r");
        if (f) {
            double up = 0.0;
            if (std::fscanf(f, "%lf", &up) == 1) {
                const int d = static_cast<int>(up) / 86400;
                const int h = (static_cast<int>(up) % 86400) / 3600;
                const int m = (static_cast<int>(up) % 3600) / 60;
                QString s;
                if (d > 0) s += QStringLiteral("%1d ").arg(d);
                if (h > 0) s += QStringLiteral("%1h ").arg(h);
                s += QStringLiteral("%1m").arg(m);
                F.append({QStringLiteral("Uptime"), s.trimmed()});
            }
            std::fclose(f);
        }
    }

    // ── Packages (pacman) ────────────────────────────────────────────────────
    {
        const int count = countDir("/var/lib/pacman/local");
        if (count > 0)
            F.append({QStringLiteral("Packages"),
                      QStringLiteral("%1 (pacman)").arg(count)});
    }

    // ── Shell ────────────────────────────────────────────────────────────────
    {
        const pid_t ppid = ::getppid();
        char commPath[64];
        std::snprintf(commPath, sizeof(commPath), "/proc/%d/comm", (int)ppid);
        const QString shell = readSysFile(commPath);
        F.append({QStringLiteral("Shell"),
                  shell.isEmpty() ? QStringLiteral("Unknown") : shell});
    }

    // ── Resolution ───────────────────────────────────────────────────────────
    {
        const QString res = screenResolution();
        if (res != QLatin1String("Unknown"))
            F.append({QStringLiteral("Resolution"), res});
    }

    // ── DE / WM ──────────────────────────────────────────────────────────────
    {
        QString de = QString::fromUtf8(qgetenv("XDG_CURRENT_DESKTOP"));
        if (de.isEmpty())
            de = qgetenv("WAYLAND_DISPLAY").isEmpty()
                 ? QStringLiteral("Unknown")
                 : QStringLiteral("Hyprland (Wayland)");
        F.append({QStringLiteral("DE"), de});
    }

    // ── Terminal ─────────────────────────────────────────────────────────────
    F.append({QStringLiteral("Terminal"), detectTerminal()});

    // ── CPU ──────────────────────────────────────────────────────────────────
    {
        QString cpu = procFind("/proc/cpuinfo", "model name");
        if (cpu.isEmpty()) cpu = QStringLiteral("Unknown");
        // Append core count
        FILE *f = std::fopen("/proc/cpuinfo", "r");
        int cores = 0;
        if (f) {
            char buf[128];
            while (std::fgets(buf, sizeof(buf), f))
                if (std::strncmp(buf, "processor", 9) == 0) ++cores;
            std::fclose(f);
        }
        if (cores > 0)
            cpu += QStringLiteral(" (%1)").arg(cores);
        F.append({QStringLiteral("CPU"), cpu});
    }

    // ── GPU ──────────────────────────────────────────────────────────────────
    F.append({QStringLiteral("GPU"), detectGPU()});

    // ── Memory ─────────────────────────────────────────────────────
    {
        long used = 0, total = 0;
        parseMeminfo(used, total);
        data.memUsedMiB  = used;
        data.memTotalMiB = total;
        F.append({QStringLiteral("Memory"),
                  QStringLiteral("%1 MiB / %2 MiB").arg(used).arg(total)});
    }

    // ── Disk ─────────────────────────────────────────────────────
    {
        struct statvfs st {};
        if (::statvfs("/", &st) == 0) {
            const unsigned long long bs = st.f_frsize;
            data.diskTotalGiB = static_cast<long>((bs * st.f_blocks) / (1024ULL*1024*1024));
            data.diskUsedGiB  = data.diskTotalGiB -
                                static_cast<long>((bs * st.f_bfree)  / (1024ULL*1024*1024));
        }
        F.append({QStringLiteral("Disk (/)"), diskUsage("/")});
    }

    // ── Local IP ─────────────────────────────────────────────────────────────
    F.append({QStringLiteral("Local IP"), localIP()});

    // ── Battery ─────────────────────────────────────────────────────
    {
        for (const char *bat : {"/sys/class/power_supply/BAT0",
                                 "/sys/class/power_supply/BAT1"}) {
            char capPath[128];
            std::snprintf(capPath, sizeof(capPath), "%s/capacity", bat);
            const QString cap = readSysFile(capPath);
            if (!cap.isEmpty()) {
                data.batteryPct = cap.toInt();
                break;
            }
        }
        const QString batt = batteryStatus();
        if (!batt.isEmpty())
            F.append({QStringLiteral("Battery"), batt});
    }

    return data;
}
