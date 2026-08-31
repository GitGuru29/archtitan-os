#include "systemcontroller.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QDebug>
#include <unistd.h>

static QString readSysFile(const QString &path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        return in.readLine().trimmed();
    }
    return QString();
}

SystemController::SystemController(QObject *parent)
    : QObject(parent)
{
    // Initialize state
    updateDateTime();
    updateCpuTemperature();
    updateBattery();
    updateGpu();
    initPowerProfilesDBus();
    initAvailablePages();

    // ── Timers ──
    // 1. Auto-scroll carousel timer (4 seconds default)
    m_autoScrollTimer = new QTimer(this);
    m_autoScrollTimer->setInterval(m_autoScrollInterval);
    connect(m_autoScrollTimer, &QTimer::timeout, this, &SystemController::onAutoScrollTimer);
    if (m_autoScrollEnabled && m_availablePages.size() > 1) {
        m_autoScrollTimer->start();
    }

    // 2. Hardware sensors timer (every 3 seconds)
    m_sensorsTimer = new QTimer(this);
    m_sensorsTimer->setInterval(3000);
    connect(m_sensorsTimer, &QTimer::timeout, this, &SystemController::onSensorsTimer);
    m_sensorsTimer->start();

    // 3. Clock timer (every 1 second)
    m_timeTimer = new QTimer(this);
    m_timeTimer->setInterval(1000);
    connect(m_timeTimer, &QTimer::timeout, this, &SystemController::onTimeTimer);
    m_timeTimer->start();

    // ── Hyprland Active Window IPC ──
    initHyprlandIpc();
}

SystemController::~SystemController()
{
    if (m_hyprlandSocket) {
        m_hyprlandSocket->close();
        delete m_hyprlandSocket;
        m_hyprlandSocket = nullptr;
    }
}

void SystemController::initAvailablePages()
{
    m_availablePages.clear();

    // 1. Media Page (Always present)
    m_availablePages.append(QStringLiteral("media"));

    // 2. Power Profile Page (Always present in ArchTitan)
    m_availablePages.append(QStringLiteral("power"));

    // 3. Active Window Page (Always present in Hyprland)
    m_availablePages.append(QStringLiteral("window"));

    // 4. CPU Temperature Page (Always present)
    m_availablePages.append(QStringLiteral("temperature"));

    // 5. Date & Time Page (Always present)
    m_availablePages.append(QStringLiteral("datetime"));

    // 6. Battery Page (Only if hardware battery is present)
    if (m_hasBattery) {
        m_availablePages.append(QStringLiteral("battery"));
    }

    // 7. GPU Page (Only if GPU hardware is detected)
    if (m_hasGpu) {
        m_availablePages.append(QStringLiteral("gpu"));
    }

    if (m_currentPageIndex >= m_availablePages.size()) {
        m_currentPageIndex = 0;
        emit currentPageIndexChanged();
    }

    emit availablePagesChanged();
}

QString SystemController::currentPageName() const
{
    if (m_currentPageIndex >= 0 && m_currentPageIndex < m_availablePages.size()) {
        return m_availablePages.at(m_currentPageIndex);
    }
    return QStringLiteral("media");
}

void SystemController::setAutoScrollInterval(int msec)
{
    if (m_autoScrollInterval == msec || msec < 1000) return;
    m_autoScrollInterval = msec;
    if (m_autoScrollTimer) {
        m_autoScrollTimer->setInterval(m_autoScrollInterval);
    }
    emit autoScrollIntervalChanged();
}

void SystemController::setAutoScrollEnabled(bool enabled)
{
    if (m_autoScrollEnabled == enabled) return;
    m_autoScrollEnabled = enabled;
    if (m_autoScrollTimer) {
        if (m_autoScrollEnabled && m_availablePages.size() > 1) {
            m_autoScrollTimer->start();
        } else {
            m_autoScrollTimer->stop();
        }
    }
    emit autoScrollEnabledChanged();
}

void SystemController::nextPage()
{
    if (m_availablePages.isEmpty()) return;
    m_currentPageIndex = (m_currentPageIndex + 1) % m_availablePages.size();
    emit currentPageIndexChanged();
}

void SystemController::previousPage()
{
    if (m_availablePages.isEmpty()) return;
    m_currentPageIndex = (m_currentPageIndex - 1 + m_availablePages.size()) % m_availablePages.size();
    emit currentPageIndexChanged();
}

void SystemController::setPage(int index)
{
    if (index >= 0 && index < m_availablePages.size() && m_currentPageIndex != index) {
        m_currentPageIndex = index;
        emit currentPageIndexChanged();
    }
}

void SystemController::userScrolled(int delta)
{
    // delta < 0: scroll down -> next page
    // delta > 0: scroll up -> previous page
    if (delta < 0) {
        nextPage();
    } else if (delta > 0) {
        previousPage();
    }

    // Debounce: reset auto-scroll timer so user has full interval to view page
    if (m_autoScrollTimer && m_autoScrollEnabled) {
        m_autoScrollTimer->start();
    }
}

void SystemController::onAutoScrollTimer()
{
    if (m_availablePages.size() > 1) {
        nextPage();
    }
}

void SystemController::onSensorsTimer()
{
    updateCpuTemperature();
    updateBattery();
    updateGpu();
}

void SystemController::onTimeTimer()
{
    updateDateTime();
}

void SystemController::refreshAll()
{
    updateDateTime();
    updateCpuTemperature();
    updateBattery();
    updateGpu();
    updatePowerProfile();
    updateActiveWindowFromHyprctl();
}

// ─── POWER PROFILES (D-Bus) ──────────────────────────────────────────────────
void SystemController::initPowerProfilesDBus()
{
    // Connect to D-Bus PropertiesChanged on PowerProfiles
    QDBusConnection::systemBus().connect(
        QStringLiteral("org.freedesktop.UPower.PowerProfiles"),
        QStringLiteral("/org/freedesktop/UPower/PowerProfiles"),
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("PropertiesChanged"),
        this,
        SLOT(onDBusPropertiesChanged(QString,QVariantMap,QStringList))
    );

    QDBusConnection::systemBus().connect(
        QStringLiteral("net.hadess.PowerProfiles"),
        QStringLiteral("/net/hadess/PowerProfiles"),
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("PropertiesChanged"),
        this,
        SLOT(onDBusPropertiesChanged(QString,QVariantMap,QStringList))
    );

    updatePowerProfile();
}

void SystemController::onDBusPropertiesChanged(const QString &interface, const QVariantMap &changedProps, const QStringList &)
{
    if (changedProps.contains(QStringLiteral("ActiveProfile"))) {
        QString profile = changedProps.value(QStringLiteral("ActiveProfile")).toString();
        if (!profile.isEmpty()) {
            if (profile.compare("performance", Qt::CaseInsensitive) == 0) {
                m_powerProfile = QStringLiteral("Performance");
                m_powerProfileDesc = QStringLiteral("Maximum CPU & GPU performance unleashed");
                m_powerProfileIcon = QStringLiteral("⚡");
            } else if (profile.compare("power-saver", Qt::CaseInsensitive) == 0) {
                m_powerProfile = QStringLiteral("Power Saver");
                m_powerProfileDesc = QStringLiteral("Quiet operation and extended battery life");
                m_powerProfileIcon = QStringLiteral("🌱");
            } else {
                m_powerProfile = QStringLiteral("Balanced");
                m_powerProfileDesc = QStringLiteral("Optimized for performance and efficiency");
                m_powerProfileIcon = QStringLiteral("⚡");
            }
            emit powerProfileChanged();
        }
    }
}

void SystemController::updatePowerProfile()
{
    // Try D-Bus interface first
    QDBusInterface iface(QStringLiteral("org.freedesktop.UPower.PowerProfiles"),
                         QStringLiteral("/org/freedesktop/UPower/PowerProfiles"),
                         QStringLiteral("org.freedesktop.DBus.Properties"),
                         QDBusConnection::systemBus());

    if (iface.isValid()) {
        QDBusReply<QDBusVariant> reply = iface.call(QStringLiteral("Get"),
                                                    QStringLiteral("org.freedesktop.UPower.PowerProfiles"),
                                                    QStringLiteral("ActiveProfile"));
        if (reply.isValid()) {
            QString profile = reply.value().variant().toString();
            if (!profile.isEmpty()) {
                if (profile.compare("performance", Qt::CaseInsensitive) == 0) {
                    m_powerProfile = QStringLiteral("Performance");
                    m_powerProfileDesc = QStringLiteral("Maximum CPU & GPU performance unleashed");
                    m_powerProfileIcon = QStringLiteral("⚡");
                } else if (profile.compare("power-saver", Qt::CaseInsensitive) == 0) {
                    m_powerProfile = QStringLiteral("Power Saver");
                    m_powerProfileDesc = QStringLiteral("Quiet operation and extended battery life");
                    m_powerProfileIcon = QStringLiteral("🌱");
                } else {
                    m_powerProfile = QStringLiteral("Balanced");
                    m_powerProfileDesc = QStringLiteral("Optimized for performance and efficiency");
                    m_powerProfileIcon = QStringLiteral("⚡");
                }
                emit powerProfileChanged();
                return;
            }
        }
    }

    // Sysfs platform profile fallback
    QString platformProfile = readSysFile(QStringLiteral("/sys/firmware/acpi/platform_profile"));
    if (!platformProfile.isEmpty()) {
        if (platformProfile.contains("performance", Qt::CaseInsensitive)) {
            m_powerProfile = QStringLiteral("Performance");
            m_powerProfileDesc = QStringLiteral("Maximum CPU & GPU performance unleashed");
            m_powerProfileIcon = QStringLiteral("⚡");
        } else if (platformProfile.contains("low-power", Qt::CaseInsensitive) || platformProfile.contains("quiet", Qt::CaseInsensitive)) {
            m_powerProfile = QStringLiteral("Power Saver");
            m_powerProfileDesc = QStringLiteral("Quiet operation and extended battery life");
            m_powerProfileIcon = QStringLiteral("🌱");
        } else {
            m_powerProfile = QStringLiteral("Balanced");
            m_powerProfileDesc = QStringLiteral("Optimized for performance and efficiency");
            m_powerProfileIcon = QStringLiteral("⚡");
        }
        emit powerProfileChanged();
    }
}

// ─── HYPRLAND ACTIVE WINDOW (IPC Socket2) ────────────────────────────────────
void SystemController::initHyprlandIpc()
{
    QString runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
    QString his = qEnvironmentVariable("HYPRLAND_INSTANCE_SIGNATURE");

    if (!runtimeDir.isEmpty() && !his.isEmpty()) {
        QString socketPath = QStringLiteral("%1/hypr/%2/.socket2.sock").arg(runtimeDir, his);
        if (QFile::exists(socketPath)) {
            m_hyprlandSocket = new QLocalSocket(this);
            connect(m_hyprlandSocket, &QLocalSocket::readyRead, this, &SystemController::onHyprlandSocketReadyRead);
            connect(m_hyprlandSocket, &QLocalSocket::errorOccurred, this, &SystemController::onHyprlandSocketError);
            m_hyprlandSocket->connectToServer(socketPath);
        }
    }

    // Initial query via hyprctl
    updateActiveWindowFromHyprctl();
}

void SystemController::onHyprlandSocketReadyRead()
{
    if (!m_hyprlandSocket) return;
    while (m_hyprlandSocket->canReadLine()) {
        QString line = QString::fromUtf8(m_hyprlandSocket->readLine()).trimmed();
        parseHyprlandEvent(line);
    }
}

void SystemController::onHyprlandSocketError()
{
    // Fallback: poll or retry later
}

void SystemController::parseHyprlandEvent(const QString &line)
{
    // Format: activewindow>>windowclass,windowtitle
    if (line.startsWith(QLatin1String("activewindow>>"))) {
        QString data = line.mid(14);
        int commaIdx = data.indexOf(QLatin1Char(','));
        if (commaIdx != -1) {
            QString wClass = data.left(commaIdx).trimmed();
            QString wTitle = data.mid(commaIdx + 1).trimmed();

            m_activeWindowClass = wClass.isEmpty() ? QStringLiteral("Desktop") : wClass;
            m_activeWindowTitle = wTitle.isEmpty() ? QStringLiteral("ArchTitan Workspace") : wTitle;

            // User-friendly app name
            if (m_activeWindowClass.compare("code", Qt::CaseInsensitive) == 0 || m_activeWindowClass.contains("vscode", Qt::CaseInsensitive)) {
                m_activeWindowApp = QStringLiteral("VS Code");
            } else if (m_activeWindowClass.compare("kitty", Qt::CaseInsensitive) == 0 || m_activeWindowClass.compare("alacritty", Qt::CaseInsensitive)) {
                m_activeWindowApp = QStringLiteral("Terminal");
            } else if (m_activeWindowClass.contains("firefox", Qt::CaseInsensitive) || m_activeWindowClass.contains("browser", Qt::CaseInsensitive)) {
                m_activeWindowApp = QStringLiteral("Web Browser");
            } else if (m_activeWindowClass.compare("thunar", Qt::CaseInsensitive) == 0 || m_activeWindowClass.compare("nautilus", Qt::CaseInsensitive)) {
                m_activeWindowApp = QStringLiteral("File Manager");
            } else if (m_activeWindowClass.contains("settings", Qt::CaseInsensitive)) {
                m_activeWindowApp = QStringLiteral("Titan Settings");
            } else {
                m_activeWindowApp = QStringLiteral("ArchTitan OS Development");
            }

            emit activeWindowChanged();
        }
    } else if (line.startsWith(QLatin1String("activewindowv2>>"))) {
        // Just trigger updateActiveWindowFromHyprctl for rich data
        updateActiveWindowFromHyprctl();
    }
}

void SystemController::updateActiveWindowFromHyprctl()
{
    QProcess process;
    process.start(QStringLiteral("hyprctl"), {QStringLiteral("activewindow"), QStringLiteral("-j")});
    if (process.waitForFinished(300)) {
        QByteArray output = process.readAllStandardOutput();
        QJsonDocument doc = QJsonDocument::fromJson(output);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QString title = obj.value(QStringLiteral("title")).toString();
            QString wclass = obj.value(QStringLiteral("class")).toString();

            if (!title.isEmpty()) {
                m_activeWindowTitle = title;
                m_activeWindowClass = wclass.isEmpty() ? QStringLiteral("Desktop") : wclass;
                m_activeWindowApp = QStringLiteral("ArchTitan OS Development");
                emit activeWindowChanged();
            }
        }
    }
}

// ─── HARDWARE SENSORS (CPU, Battery, GPU) ────────────────────────────────────
void SystemController::updateCpuTemperature()
{
    int maxTemp = 0;

    // 1. Scan hwmon for coretemp / k10temp / zenpower
    QDir hwmonDir(QStringLiteral("/sys/class/hwmon"));
    if (hwmonDir.exists()) {
        const auto entries = hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &dirName : entries) {
            QString path = QStringLiteral("/sys/class/hwmon/%1").arg(dirName);
            for (int i = 1; i <= 8; ++i) {
                QString tempFile = QStringLiteral("%1/temp%2_input").arg(path).arg(i);
                if (QFile::exists(tempFile)) {
                    QString val = readSysFile(tempFile);
                    bool ok = false;
                    long t = val.toLong(&ok);
                    if (ok && t > 0) {
                        int degC = static_cast<int>(t / 1000);
                        if (degC > maxTemp && degC < 130) {
                            maxTemp = degC;
                        }
                    }
                }
            }
        }
    }

    // 2. Scan thermal_zone fallback
    if (maxTemp == 0) {
        QDir thermalDir(QStringLiteral("/sys/class/thermal"));
        if (thermalDir.exists()) {
            const auto entries = thermalDir.entryList(QStringList() << QStringLiteral("thermal_zone*"), QDir::Dirs);
            for (const QString &dirName : entries) {
                QString tempFile = QStringLiteral("/sys/class/thermal/%1/temp").arg(dirName);
                QString val = readSysFile(tempFile);
                bool ok = false;
                long t = val.toLong(&ok);
                if (ok && t > 0) {
                    int degC = static_cast<int>(t / 1000);
                    if (degC > maxTemp && degC < 130) {
                        maxTemp = degC;
                    }
                }
            }
        }
    }

    if (maxTemp > 0 && maxTemp != m_cpuTemp) {
        m_cpuTemp = maxTemp;
        emit cpuTempChanged();
    }
}

QString SystemController::cpuTempStatus() const
{
    if (m_cpuTemp < 55) return QStringLiteral("Normal");
    if (m_cpuTemp < 75) return QStringLiteral("Warm");
    return QStringLiteral("High");
}

void SystemController::updateBattery()
{
    bool found = false;
    for (const char *batName : {"BAT0", "BAT1", "BAT2"}) {
        QString basePath = QStringLiteral("/sys/class/power_supply/%1").arg(QLatin1String(batName));
        if (QDir(basePath).exists()) {
            found = true;
            QString capStr = readSysFile(basePath + QStringLiteral("/capacity"));
            QString statStr = readSysFile(basePath + QStringLiteral("/status"));
            QString tempStr = readSysFile(basePath + QStringLiteral("/temp"));

            if (!capStr.isEmpty()) {
                m_batteryPercent = capStr.toInt();
            }
            if (!statStr.isEmpty()) {
                m_batteryStatus = statStr;
            }
            if (!tempStr.isEmpty()) {
                m_batteryTemp = tempStr.toInt() / 10;
            } else {
                m_batteryTemp = 37; // Standard temperature fallback
            }

            m_batteryHealth = QStringLiteral("Good health");
            break;
        }
    }

    if (m_hasBattery != found) {
        m_hasBattery = found;
        initAvailablePages();
    }
    emit batteryChanged();
}

void SystemController::updateGpu()
{
    // Check for NVIDIA GPU
    if (QDir(QStringLiteral("/proc/driver/nvidia/gpus")).exists()) {
        m_hasGpu = true;
        m_gpuModel = QStringLiteral("NVIDIA GeForce RTX");
        m_gpuTemp = m_cpuTemp - 4; // Estimation if NVML not linked
        m_gpuUsage = 34;
        emit gpuChanged();
        return;
    }

    // Check for DRM render devices
    if (QFile::exists(QStringLiteral("/sys/class/drm/card0/device/uevent")) ||
        QFile::exists(QStringLiteral("/sys/class/drm/card1/device/uevent"))) {
        m_hasGpu = true;
        m_gpuModel = QStringLiteral("Integrated GPU");
        m_gpuTemp = m_cpuTemp - 2;
        m_gpuUsage = 18;
        emit gpuChanged();
    }
}

// ─── DATE & TIME ─────────────────────────────────────────────────────────────
void SystemController::updateDateTime()
{
    QDateTime now = QDateTime::currentDateTime();
    QString newDate = now.toString(QStringLiteral("dddd, d MMMM yyyy"));
    QString newTime = now.toString(QStringLiteral("HH:mm"));

    int hour = now.time().hour();
    QString newGreeting;
    if (hour >= 5 && hour < 12) {
        newGreeting = QStringLiteral("Have a productive morning! ☀️");
    } else if (hour >= 12 && hour < 18) {
        newGreeting = QStringLiteral("Have a productive day! ☀️");
    } else if (hour >= 18 && hour < 22) {
        newGreeting = QStringLiteral("Good evening! 🌙");
    } else {
        newGreeting = QStringLiteral("Late night coding! ✨");
    }

    if (newDate != m_dateFormatted || newTime != m_timeFormatted || newGreeting != m_greetingText) {
        m_dateFormatted = newDate;
        m_timeFormatted = newTime;
        m_greetingText = newGreeting;
        emit timeChanged();
    }
}
