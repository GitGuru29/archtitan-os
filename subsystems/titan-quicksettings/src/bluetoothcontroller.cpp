#include "bluetoothcontroller.h"
#include <QProcess>
#include <QDebug>

BluetoothController::BluetoothController(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    queryStatus();
    connect(m_timer, &QTimer::timeout, this, &BluetoothController::queryStatus);
    m_timer->start(5000);
}

void BluetoothController::queryStatus()
{
    QProcess showProc;
    showProc.start("bash", {"-c", "bluetoothctl show 2>/dev/null"});
    if (showProc.waitForFinished(1000)) {
        QString out = showProc.readAllStandardOutput();
        bool isPowered = out.contains("Powered: yes", Qt::CaseInsensitive);
        if (isPowered != m_powered) {
            m_powered = isPowered;
            emit poweredChanged();
        }
    }

    if (!m_powered) {
        if (m_connected) {
            m_connected = false;
            emit connectedChanged();
        }
        if (m_statusText != "Off") {
            m_statusText = "Off";
            emit statusTextChanged();
        }
        return;
    }

    // Check connected devices
    QProcess infoProc;
    infoProc.start("bash", {"-c", "bluetoothctl devices Connected 2>/dev/null | head -n 1 | cut -d ' ' -f 3-"});
    if (infoProc.waitForFinished(1000)) {
        QString device = infoProc.readAllStandardOutput().trimmed();
        if (!device.isEmpty()) {
            if (!m_connected) {
                m_connected = true;
                emit connectedChanged();
            }
            if (m_statusText != device) {
                m_statusText = device;
                emit statusTextChanged();
            }
            return;
        }
    }

    if (m_connected) {
        m_connected = false;
        emit connectedChanged();
    }
    if (m_statusText != "Not connected") {
        m_statusText = "Not connected";
        emit statusTextChanged();
    }
}

void BluetoothController::toggleBluetooth()
{
    m_powered = !m_powered;
    emit poweredChanged();
    QProcess::startDetached("bluetoothctl", {"power", m_powered ? "on" : "off"});
    QTimer::singleShot(800, this, &BluetoothController::queryStatus);
}

void BluetoothController::refresh()
{
    queryStatus();
}
