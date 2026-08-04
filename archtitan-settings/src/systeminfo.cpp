#include "systeminfo.h"
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>
#include <sys/statvfs.h>
#include <unistd.h>

SystemInfo::SystemInfo(QObject *parent) : QObject(parent) {
    // Eager init of CONSTANT properties before QML first reads them
    initCpuModel();
    initTotalRam();
    initDiskTotal();
    initBatteryStatics();

    m_timer.setInterval(2000);
    connect(&m_timer, &QTimer::timeout, this, &SystemInfo::refresh);
    m_timer.start();
    refresh();
}

void SystemInfo::refresh() {
    readCpuUsage();
    readMemory();
    readDisk();
    readBattery();
}

QString SystemInfo::hostname() const {
    char buf[256];
    if (gethostname(buf, sizeof(buf)) == 0)
        return QString::fromLocal8Bit(buf);
    return "archtitan";
}

QString SystemInfo::kernelVersion() const {
    QFile f("/proc/version");
    if (f.open(QIODevice::ReadOnly)) {
        QString line = QTextStream(&f).readLine();
        // "Linux version X.Y.Z ..."
        QStringList parts = line.split(' ');
        if (parts.size() >= 3) return parts[2];
    }
    return "unknown";
}

QString SystemInfo::osVersion() const {
    QFile f("/etc/os-release");
    if (f.open(QIODevice::ReadOnly)) {
        QTextStream ts(&f);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (line.startsWith("PRETTY_NAME="))
                return line.mid(13).remove('"');
        }
    }
    return "ArchTitan OS";
}

void SystemInfo::initCpuModel() {
    QFile f("/proc/cpuinfo");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream ts(&f);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (line.startsWith("model name") || line.startsWith("Model name")) {
                m_cpuModel = line.section(':', 1).trimmed();
                while (m_cpuModel.contains("  ")) m_cpuModel.replace("  ", " ");
                return;
            }
        }
    }
    // Fallback: try /proc/cpuinfo Hardware field (ARM)
    QFile f2("/proc/cpuinfo");
    if (f2.open(QIODevice::ReadOnly)) {
        QTextStream ts(&f2);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (line.startsWith("Hardware")) {
                m_cpuModel = line.section(':', 1).trimmed();
                return;
            }
        }
    }
    m_cpuModel = "Unknown CPU";
}

void SystemInfo::initTotalRam() {
    QFile f("/proc/meminfo");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream ts(&f);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (line.startsWith("MemTotal:")) {
                QString val = line.mid(9).trimmed(); // "16148448 kB"
                m_totalRam = val.section(' ', 0, 0).toLongLong() / 1024;
                break;
            }
        }
    }
    if (m_totalRam == 0) {
        QProcess p;
        p.start("bash", {"-c", "free -m | awk '/^Mem:/{print $2}'"});
        p.waitForFinished(1000);
        m_totalRam = p.readAllStandardOutput().trimmed().toInt();
    }
}

void SystemInfo::initDiskTotal() {
    struct statvfs st;
    if (statvfs("/", &st) == 0)
        m_diskTotalGb = (double)(st.f_blocks * st.f_frsize) / (1024.0*1024.0*1024.0);
}

QString SystemInfo::cpuModel() const {
    if (!m_cpuModel.isEmpty() && m_cpuModel != "Unknown CPU") return m_cpuModel;
    QProcess p;
    p.start("bash", {"-c", "lscpu 2>/dev/null | grep 'Model name:' | sed 's/.*: *//'"});
    p.waitForFinished(1000);
    m_cpuModel = p.readAllStandardOutput().trimmed();
    if (m_cpuModel.isEmpty()) m_cpuModel = "Unknown CPU";
    return m_cpuModel;
}

QString SystemInfo::gpuModel() const {
    if (!m_gpuModel.isEmpty()) return m_gpuModel;
    QProcess p;
    p.start("bash", {"-c", "lspci 2>/dev/null | grep -iE 'VGA|3D|Display' | sed 's/.*: //'"});
    p.waitForFinished(1000);
    QString raw = p.readAllStandardOutput().trimmed();
    if (!raw.isEmpty()) {
        QStringList lines = raw.split('\n', Qt::SkipEmptyParts);
        QStringList gpus;
        for (const QString &line : lines) {
            QString str = line.trimmed();
            if (str.isEmpty()) continue;
            
            // Remove (rev xx)
            int revIdx = str.indexOf("(rev ");
            if (revIdx != -1) str = str.left(revIdx).trimmed();

            QString vendor;
            if (str.contains("NVIDIA", Qt::CaseInsensitive)) vendor = "NVIDIA";
            else if (str.contains("Intel", Qt::CaseInsensitive)) vendor = "Intel";
            else if (str.contains("AMD", Qt::CaseInsensitive) || str.contains("ATI", Qt::CaseInsensitive)) vendor = "AMD";

            int firstBracket = str.lastIndexOf('[');
            int lastBracket = str.lastIndexOf(']');
            if (firstBracket != -1 && lastBracket > firstBracket) {
                QString model = str.mid(firstBracket + 1, lastBracket - firstBracket - 1).trimmed();
                if (!vendor.isEmpty() && !model.startsWith(vendor, Qt::CaseInsensitive)) {
                    gpus.append(vendor + " " + model);
                } else {
                    gpus.append(model);
                }
            } else {
                str.remove("Corporation", Qt::CaseInsensitive);
                str.remove("Inc.", Qt::CaseInsensitive);
                gpus.append(str.simplified());
            }
        }
        m_gpuModel = gpus.join("\n");
    }
    if (m_gpuModel.isEmpty()) m_gpuModel = "Unknown GPU";
    return m_gpuModel;
}

int SystemInfo::totalRam() const { return m_totalRam; }

double SystemInfo::diskTotalGb() const { return m_diskTotalGb; }

// ─── Accessors ──────────────────────────────────────────────────────────────

int    SystemInfo::usedRam()           const { return m_usedRam; }
double SystemInfo::cpuUsage()          const { return m_cpuUsage; }
double SystemInfo::diskUsedGb()        const { return m_diskUsedGb; }
int    SystemInfo::batteryLevel()      const { return m_batteryLevel; }
bool   SystemInfo::batteryCharging()   const { return m_batteryCharging; }
bool   SystemInfo::acConnected()       const { return m_acConnected; }
int    SystemInfo::batteryCycles()     const { return m_batteryCycles; }
int    SystemInfo::batteryHealth()     const { return m_batteryHealth; }
int    SystemInfo::powerNow()          const { return m_powerNow; }
double SystemInfo::estimatedRuntime()  const { return m_estimatedRuntime; }

bool    SystemInfo::chargeProtectionSupported() const { return !m_chargeProtectionMode.isEmpty() && m_chargeProtectionMode != "none"; }
QString SystemInfo::chargeProtectionMode()      const { return m_chargeProtectionMode; }
bool    SystemInfo::chargeProtectionEnabled()   const { return m_chargeProtectionEnabled; }
int     SystemInfo::chargeLimit()               const { return m_chargeLimit; }
int     SystemInfo::asusChargeMode()            const { return m_asusChargeMode; }
bool    SystemInfo::rapidChargeSupported()      const { return !m_rapidChargePath.isEmpty() || m_usesAcpiCallForRapidCharge; }
bool    SystemInfo::rapidChargeEnabled()        const { return m_rapidChargeEnabled; }

// ─── Static helpers ──────────────────────────────────────────────────────────

QString SystemInfo::readSysFile(const QString &path) {
    QFile f(path);
    if (f.open(QIODevice::ReadOnly))
        return QTextStream(&f).readLine().trimmed();
    return {};
}

bool SystemInfo::writeSysFile(const QString &path, const QString &value) {
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(value.toUtf8());
        return true;
    }
    return false;
}

QString SystemInfo::findSysPath(const QStringList &candidates) {
    for (const QString &p : candidates) {
        if (QFile::exists(p)) return p;
    }
    return {};
}

// ─── Invokables ──────────────────────────────────────────────────────────────

// setChargeProtection — for "conservation" mode (binary on/off, Lenovo IdeaPad)
void SystemInfo::setChargeProtection(bool enabled) {
    if (m_chargeProtectionMode != "conservation") return;
    if (m_chargeProtectionEnabled == enabled) return;
    if (writeSysFile(m_conservationPath, enabled ? "1" : "0")) {
        m_chargeProtectionEnabled = enabled;
        emit chargeProtectionEnabledChanged();
    } else {
        qWarning() << "[Battery] Permission denied on conservation_mode — run install-battery-rules.sh";
    }
}

// setChargeLimit — for "threshold" mode (ThinkPad, ASUS, Samsung, Framework, Huawei, MSI…)
void SystemInfo::setChargeLimit(int percent) {
    if (m_chargeProtectionMode != "threshold") return;
    percent = qBound(20, percent, 100);
    if (m_chargeLimit == percent) return;
    // Some ThinkPads need start < end; set start to end-5 when possible
    if (!m_thresholdStartPath.isEmpty())
        writeSysFile(m_thresholdStartPath, QString::number(qMax(percent - 5, 20)));
    if (writeSysFile(m_thresholdEndPath, QString::number(percent))) {
        m_chargeLimit = percent;
        emit chargeLimitChanged();
        // If limit < 100 → protection is "on"; at 100 → off
        bool prot = (percent < 100);
        if (m_chargeProtectionEnabled != prot) {
            m_chargeProtectionEnabled = prot;
            emit chargeProtectionEnabledChanged();
        }
    } else {
        qWarning() << "[Battery] Permission denied on charge threshold — run install-battery-rules.sh";
    }
}

// setAsusChargeMode — for "asus_mode" (0=Balanced ~80%, 1=Full 100%, 2=Gaming always-on)
void SystemInfo::setAsusChargeMode(int mode) {
    if (m_chargeProtectionMode != "asus_mode") return;
    mode = qBound(0, mode, 2);
    if (m_asusChargeMode == mode) return;
    if (writeSysFile(m_asusChargeModePath, QString::number(mode))) {
        m_asusChargeMode = mode;
        emit asusChargeModeChanged();
        bool prot = (mode != 1);  // mode 1 = Full charge = no protection
        if (m_chargeProtectionEnabled != prot) {
            m_chargeProtectionEnabled = prot;
            emit chargeProtectionEnabledChanged();
        }
    } else {
        qWarning() << "[Battery] Permission denied on ASUS charge_mode — run install-battery-rules.sh";
    }
}

// setRapidCharge — Lenovo IdeaPad rapid_charge
void SystemInfo::setRapidCharge(bool enabled) {
    if (m_rapidChargeEnabled == enabled) return;
    
    if (m_usesAcpiCallForRapidCharge) {
        QString val = enabled ? "0x07" : "0x08";
        // Attempt writes to both common ACPI paths for Lenovo
        bool r1 = writeSysFile("/proc/acpi/call", "\\_SB.PCI0.LPCB.EC0.VPC0.SBMC " + val);
        bool r2 = writeSysFile("/proc/acpi/call", "\\_SB.PCI0.LPC0.EC0.VPC0.SBMC " + val);
        
        if (r1 || r2) {
            m_rapidChargeEnabled = enabled;
            emit rapidChargeEnabledChanged();
        } else {
            qWarning() << "[Battery] Permission denied on /proc/acpi/call. Ensure it is writable by your user.";
        }
        return;
    }
    
    if (m_rapidChargePath.isEmpty()) return;
    
    if (writeSysFile(m_rapidChargePath, enabled ? "1" : "0")) {
        m_rapidChargeEnabled = enabled;
        emit rapidChargeEnabledChanged();
    } else {
        qWarning() << "[Battery] Permission denied on rapid_charge — run install-battery-rules.sh";
    }
}

// ─── readBattery (called every 2 s) ─────────────────────────────────────────

void SystemInfo::readBattery() {
    if (m_batPath.isEmpty()) {
        QDir bat("/sys/class/power_supply");
        for (const QString &e : bat.entryList({"BAT*"}, QDir::Dirs)) {
            m_batPath = "/sys/class/power_supply/" + e;
            break;
        }
    }
    if (m_batPath.isEmpty()) return;

    // Level & status
    int lvl = readSysFile(m_batPath + "/capacity").toInt();
    QString status = readSysFile(m_batPath + "/status");
    bool charging = (status == "Charging" || status == "Full");
    if (m_batteryLevel    != lvl)     { m_batteryLevel    = lvl;     emit batteryLevelChanged(); }
    if (m_batteryCharging != charging){ m_batteryCharging = charging; emit batteryChargingChanged(); }

    if (!m_acPath.isEmpty()) {
        bool ac = (readSysFile(m_acPath + "/online") == "1");
        if (m_acConnected != ac) { m_acConnected = ac; emit acConnectedChanged(); }
    } else {
        bool ac = (status != "Discharging" && status != "Unknown");
        if (m_acConnected != ac) { m_acConnected = ac; emit acConnectedChanged(); }
    }

    // Cycle count
    int cycles = readSysFile(m_batPath + "/cycle_count").toInt();
    if (m_batteryCycles != cycles) { m_batteryCycles = cycles; emit batteryCyclesChanged(); }

    // Health: energy_full / energy_full_design × 100
    long long eFull   = readSysFile(m_batPath + "/energy_full").toLongLong();
    long long eDesign = readSysFile(m_batPath + "/energy_full_design").toLongLong();
    if (eDesign > 0) {
        int health = qBound(0, (int)qRound(100.0 * eFull / eDesign), 100);
        if (m_batteryHealth != health) { m_batteryHealth = health; emit batteryHealthChanged(); }
    }

    // Live power draw + estimated runtime
    int pNow = readSysFile(m_batPath + "/power_now").toInt();
    if (m_powerNow != pNow) { m_powerNow = pNow; emit powerNowChanged(); }

    double runtime = -1.0;
    if (!charging && pNow > 0) {
        long long eNow = readSysFile(m_batPath + "/energy_now").toLongLong();
        runtime = (double)eNow / (double)pNow;
    }
    if (!qFuzzyCompare(m_estimatedRuntime + 1.0, runtime + 1.0)) {
        m_estimatedRuntime = runtime;
        emit estimatedRuntimeChanged();
    }

    // Sync protection state from hardware (catches external changes)
    if (m_chargeProtectionMode == "threshold" && !m_thresholdEndPath.isEmpty()) {
        int lim = readSysFile(m_thresholdEndPath).toInt();
        if (lim > 0 && m_chargeLimit != lim) { m_chargeLimit = lim; emit chargeLimitChanged(); }
        bool prot = (m_chargeLimit < 100);
        if (m_chargeProtectionEnabled != prot) { m_chargeProtectionEnabled = prot; emit chargeProtectionEnabledChanged(); }
    } else if (m_chargeProtectionMode == "conservation" && !m_conservationPath.isEmpty()) {
        bool en = (readSysFile(m_conservationPath) == "1");
        if (m_chargeProtectionEnabled != en) { m_chargeProtectionEnabled = en; emit chargeProtectionEnabledChanged(); }
    } else if (m_chargeProtectionMode == "asus_mode" && !m_asusChargeModePath.isEmpty()) {
        int mode = readSysFile(m_asusChargeModePath).toInt();
        if (m_asusChargeMode != mode) { m_asusChargeMode = mode; emit asusChargeModeChanged(); }
        bool prot = (mode != 1);
        if (m_chargeProtectionEnabled != prot) { m_chargeProtectionEnabled = prot; emit chargeProtectionEnabledChanged(); }
    }

    // Sync rapid charge
    if (!m_rapidChargePath.isEmpty()) {
        bool rc = (readSysFile(m_rapidChargePath) == "1");
        if (m_rapidChargeEnabled != rc) { m_rapidChargeEnabled = rc; emit rapidChargeEnabledChanged(); }
    }
}

// ─── initBatteryStatics — hardware detection (runs once at startup) ───────────

void SystemInfo::initBatteryStatics() {
    // 1. Resolve BAT path
    QDir bat("/sys/class/power_supply");
    for (const QString &e : bat.entryList({"BAT*"}, QDir::Dirs)) {
        m_batPath = "/sys/class/power_supply/" + e;
        break;
    }

    // 1.b Resolve AC path
    for (const QString &e : bat.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (e.startsWith("BAT")) continue;
        QString path = "/sys/class/power_supply/" + e;
        if (readSysFile(path + "/type") == "Mains") {
            m_acPath = path;
            break;
        }
    }

    // ── Detection priority ───────────────────────────────────────────────────
    //
    // Priority 1: charge_control_end_threshold
    //   Covers: ThinkPad, ASUS/ROG/TUF, Samsung, Framework, Huawei, MSI, HP (some)
    //   Write a % value (20-100) to stop charging there.
    if (!m_batPath.isEmpty()) {
        QString endPath = m_batPath + "/charge_control_end_threshold";
        if (QFile::exists(endPath)) {
            m_thresholdEndPath = endPath;
            m_chargeLimit = readSysFile(endPath).toInt();
            if (m_chargeLimit <= 0) m_chargeLimit = 100;

            // ThinkPad also has a start threshold
            QString startPath = m_batPath + "/charge_control_start_threshold";
            if (QFile::exists(startPath))
                m_thresholdStartPath = startPath;

            m_chargeProtectionMode    = "threshold";
            m_chargeProtectionEnabled = (m_chargeLimit < 100);
        }
    }

    // Priority 2: ASUS ROG/TUF charge_mode (3-way: 0=Balanced ~80%, 1=Full, 2=Gaming)
    //   Only if threshold wasn't found (some ASUS have both; prefer threshold)
    if (m_chargeProtectionMode.isEmpty()) {
        QString asusPath = findSysPath({
            "/sys/devices/platform/asus-nb-wmi/charge_mode",
            "/sys/devices/platform/asus-wmi/charge_mode"
        });
        if (!asusPath.isEmpty()) {
            m_asusChargeModePath      = asusPath;
            m_asusChargeMode          = readSysFile(asusPath).toInt();
            m_chargeProtectionMode    = "asus_mode";
            m_chargeProtectionEnabled = (m_asusChargeMode != 1);
        }
    }

    // Priority 3: Lenovo IdeaPad conservation_mode (binary, caps at ~60%)
    //   Covers: IdeaPad 3/5/Pro, Yoga, Legion (when ideapad_acpi loaded)
    if (m_chargeProtectionMode.isEmpty()) {
        // Search common ACPI device paths; the PCI address varies by machine
        QStringList conservCandidates;
        QDir devDir("/sys/devices");
        // Walk /sys/bus/platform/devices to find ideapad_acpi symlinks
        QDir platDev("/sys/bus/platform/devices");
        for (const QString &dev : platDev.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
            QString candidate = platDev.absoluteFilePath(dev) + "/conservation_mode";
            if (QFile::exists(candidate)) {
                conservCandidates << candidate;
                break;
            }
        }
        // Also try fixed known paths
        conservCandidates << QStringList{
            "/sys/devices/pci0000:00/0000:00:1f.0/PNP0C09:00/VPC2004:00/conservation_mode",
            "/sys/devices/pci0000:00/0000:00:14.3/PNP0C09:00/VPC2004:00/conservation_mode"
        };

        QString conservPath = findSysPath(conservCandidates);
        if (!conservPath.isEmpty()) {
            m_conservationPath        = conservPath;
            m_chargeProtectionMode    = "conservation";
            m_chargeProtectionEnabled = (readSysFile(conservPath) == "1");
        }
    }

    // If nothing found → explicitly mark unsupported
    if (m_chargeProtectionMode.isEmpty())
        m_chargeProtectionMode = "none";

    // ── Rapid / Fast Charging ────────────────────────────────────────────────
    // Lenovo IdeaPad rapid_charge (same ideapad_acpi driver)
    QStringList rapidCandidates;
    QDir platDev2("/sys/bus/platform/devices");
    for (const QString &dev : platDev2.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        QString candidate = platDev2.absoluteFilePath(dev) + "/rapid_charge";
        if (QFile::exists(candidate)) { rapidCandidates << candidate; break; }
    }
    rapidCandidates << QStringList{
        "/sys/devices/pci0000:00/0000:00:1f.0/PNP0C09:00/VPC2004:00/rapid_charge",
        "/sys/devices/pci0000:00/0000:00:14.3/PNP0C09:00/VPC2004:00/rapid_charge"
    };
    m_rapidChargePath   = findSysPath(rapidCandidates);
    if (!m_rapidChargePath.isEmpty()) {
        m_rapidChargeEnabled = (readSysFile(m_rapidChargePath) == "1");
    } else {
        // Fallback: If acpi_call is installed, we can blindly send the Lenovo Rapid Charge ACPI commands
        if (QFile::exists("/proc/acpi/call")) {
            m_usesAcpiCallForRapidCharge = true;
            m_rapidChargeEnabled = false; // Cannot easily read state from acpi_call, assume off
        }
    }
}



QString SystemInfo::uptime() const {
    QFile f("/proc/uptime");
    if (f.open(QIODevice::ReadOnly)) {
        double secs = QTextStream(&f).readLine().split(' ')[0].toDouble();
        int h = (int)secs / 3600;
        int m = ((int)secs % 3600) / 60;
        return QString("%1h %2m").arg(h).arg(m);
    }
    return "N/A";
}

void SystemInfo::readCpuUsage() {
    QFile f("/proc/stat");
    if (!f.open(QIODevice::ReadOnly)) return;
    QString line = QTextStream(&f).readLine();
    QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() < 8) return;

    long long user    = parts[1].toLongLong();
    long long nice    = parts[2].toLongLong();
    long long system  = parts[3].toLongLong();
    long long idle    = parts[4].toLongLong();
    long long iowait  = parts[5].toLongLong();
    long long irq     = parts[6].toLongLong();
    long long softirq = parts[7].toLongLong();

    long long total = user + nice + system + idle + iowait + irq + softirq;
    long long dTotal = total - m_prevTotal;
    long long dIdle  = idle - m_prevIdle;

    if (dTotal > 0) {
        double usage = 100.0 * (1.0 - (double)dIdle / dTotal);
        if (!qFuzzyCompare(m_cpuUsage, usage)) {
            m_cpuUsage = usage;
            emit cpuUsageChanged();
        }
    }
    m_prevTotal = total;
    m_prevIdle  = idle;
}

void SystemInfo::readMemory() {
    QFile f("/proc/meminfo");
    long long total = 0, avail = 0;
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream ts(&f);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (line.startsWith("MemTotal:")) {
                total = line.mid(9).trimmed().section(' ', 0, 0).toLongLong();
            } else if (line.startsWith("MemAvailable:")) {
                avail = line.mid(13).trimmed().section(' ', 0, 0).toLongLong();
            }
        }
    }
    
    int used = 0;
    if (total == 0) {
        QProcess p;
        p.start("bash", {"-c", "free -m | awk '/^Mem:/{print $3}'"});
        p.waitForFinished(1000);
        used = p.readAllStandardOutput().trimmed().toInt();
    } else {
        used = (int)((total - avail) / 1024);
    }
    
    if (m_usedRam != used) {
        m_usedRam = used;
        emit usedRamChanged();
    }
}

void SystemInfo::readDisk() {
    struct statvfs st;
    if (statvfs("/", &st) == 0) {
        double used = (double)((st.f_blocks - st.f_bfree) * st.f_frsize) / (1024*1024*1024);
        if (!qFuzzyCompare(m_diskUsedGb, used)) {
            m_diskUsedGb = used;
            emit diskUsedGbChanged();
        }
    }
}

