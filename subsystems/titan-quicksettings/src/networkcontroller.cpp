#include "networkcontroller.h"
#include <QProcess>
#include <QDebug>

NetworkController::NetworkController(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    queryStatus();
    connect(m_timer, &QTimer::timeout, this, &NetworkController::queryStatus);
    m_timer->start(5000);
}

void NetworkController::queryStatus()
{
    // Check radio wifi state
    QProcess radioProc;
    radioProc.start("bash", {"-c", "nmcli radio wifi 2>/dev/null"});
    if (radioProc.waitForFinished(1000)) {
        QString out = radioProc.readAllStandardOutput().trimmed();
        bool enabled = (out.compare("enabled", Qt::CaseInsensitive) == 0);
        if (enabled != m_wifiEnabled) {
            m_wifiEnabled = enabled;
            emit wifiEnabledChanged();
        }
    }

    if (!m_wifiEnabled) {
        if (m_connected) {
            m_connected = false;
            emit connectedChanged();
        }
        if (m_ssid != "Disabled") {
            m_ssid = "Disabled";
            emit ssidChanged();
        }
        return;
    }

    // Check active wifi connection SSID
    QProcess wifiProc;
    wifiProc.start("bash", {"-c", "nmcli -t -f ACTIVE,SSID dev wifi 2>/dev/null | grep '^yes:' | cut -d: -f2"});
    if (wifiProc.waitForFinished(1000)) {
        QString ssid = wifiProc.readAllStandardOutput().trimmed();
        if (!ssid.isEmpty()) {
            if (!m_connected) {
                m_connected = true;
                emit connectedChanged();
            }
            if (m_ssid != ssid) {
                m_ssid = ssid;
                emit ssidChanged();
            }
            return;
        }
    }

    // Secondary check: active connection name
    QProcess conProc;
    conProc.start("bash", {"-c", "nmcli -t -f NAME,TYPE con show --active 2>/dev/null | grep -iE 'wireless|wifi' | cut -d: -f1"});
    if (conProc.waitForFinished(1000)) {
        QString conName = conProc.readAllStandardOutput().trimmed();
        if (!conName.isEmpty()) {
            if (!m_connected) {
                m_connected = true;
                emit connectedChanged();
            }
            if (m_ssid != conName) {
                m_ssid = conName;
                emit ssidChanged();
            }
            return;
        }
    }

    // If on live ISO or dev without wifi active, keep existing or mark Not connected
    if (m_ssid.isEmpty()) {
        m_ssid = "Not connected";
        m_connected = false;
        emit connectedChanged();
        emit ssidChanged();
    }
}

void NetworkController::toggleWifi()
{
    m_wifiEnabled = !m_wifiEnabled;
    emit wifiEnabledChanged();
    QProcess::startDetached("nmcli", {"radio", "wifi", m_wifiEnabled ? "on" : "off"});
    QTimer::singleShot(800, this, &NetworkController::queryStatus);
}

void NetworkController::refresh()
{
    queryStatus();
}
