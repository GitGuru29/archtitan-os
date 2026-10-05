#include "systemcontroller.h"
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QDebug>

SystemController::SystemController(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    updateUptime();
    updateDate();

    connect(m_timer, &QTimer::timeout, this, [this]() {
        updateUptime();
        updateDate();
    });
    m_timer->start(10000); // Check every 10s
}

void SystemController::updateUptime()
{
    QFile file("/proc/uptime");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        double uptimeSeconds = 0;
        in >> uptimeSeconds;
        file.close();

        int totalMinutes = static_cast<int>(uptimeSeconds) / 60;
        int hours = totalMinutes / 60;
        int minutes = totalMinutes % 60;

        QString formatted;
        if (hours > 0) {
            formatted = QString("Up %1h %2m").arg(hours).arg(minutes);
        } else {
            formatted = QString("Up %1m").arg(minutes);
        }

        if (formatted != m_uptime) {
            m_uptime = formatted;
            emit uptimeChanged();
        }
    }
}

void SystemController::updateDate()
{
    QDateTime now = QDateTime::currentDateTime();
    QString formatted = now.toString("dddd, MMMM d") + " • 0 tasks";
    if (formatted != m_currentDateString) {
        m_currentDateString = formatted;
        emit currentDateStringChanged();
    }
}

void SystemController::refresh()
{
    updateUptime();
    updateDate();
}

void SystemController::setCaffeineActive(bool active)
{
    if (m_caffeineActive != active) {
        m_caffeineActive = active;
        emit caffeineActiveChanged();
    }
}

void SystemController::setThemeDark(bool dark)
{
    if (m_themeDark != dark) {
        m_themeDark = dark;
        emit themeDarkChanged();
    }
}

void SystemController::setVisualizerActive(bool active)
{
    if (m_visualizerActive != active) {
        m_visualizerActive = active;
        emit visualizerActiveChanged();
    }
}

void SystemController::setGameModeActive(bool active)
{
    if (m_gameModeActive != active) {
        m_gameModeActive = active;
        emit gameModeActiveChanged();
    }
}

void SystemController::setVpnActive(bool active)
{
    if (m_vpnActive != active) {
        m_vpnActive = active;
        emit vpnActiveChanged();
    }
}

void SystemController::openSettings()
{
    QProcess::startDetached("archtitan-settings", {});
}

void SystemController::openPowerMenu()
{
    QProcess::startDetached("titan-powermenu", {});
}

void SystemController::takeScreenshot()
{
    QProcess::startDetached("bash", {"-c", "grim -g \"$(slurp)\" - | wl-copy"});
}

void SystemController::pickColor()
{
    QProcess::startDetached("hyprpicker", {"-a"});
}

void SystemController::toggleVirtualKeyboard()
{
    QProcess::startDetached("bash", {"-c", "pkill wvkbd || wvkbd-mobintl &"});
}

void SystemController::openMediaWidget()
{
    QProcess::startDetached("titan-bar-media", {});
}
