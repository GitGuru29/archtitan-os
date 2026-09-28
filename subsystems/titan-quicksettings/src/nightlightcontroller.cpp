#include "nightlightcontroller.h"
#include <QProcess>

NightLightController::NightLightController(QObject *parent)
    : QObject(parent)
{
    // Check if gammastep or hyprsunset is running
    QProcess check;
    check.start("bash", {"-c", "pgrep -x gammastep >/dev/null || pgrep -x hyprsunset >/dev/null"});
    if (check.waitForFinished(500)) {
        m_active = (check.exitCode() == 0);
    }
}

void NightLightController::toggleNightLight()
{
    m_active = !m_active;
    emit activeChanged();
    emit statusTextChanged();

    if (m_active) {
        QProcess::startDetached("bash", {"-c", "which hyprsunset >/dev/null 2>&1 && hyprsunset -t 4500 & || which gammastep >/dev/null 2>&1 && gammastep -O 4500 &"});
    } else {
        QProcess::startDetached("bash", {"-c", "pkill -x hyprsunset || pkill -x gammastep || true"});
    }
}
