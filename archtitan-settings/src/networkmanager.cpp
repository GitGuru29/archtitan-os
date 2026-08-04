#include "networkmanager.h"
#include <QProcess>
#include <QRegularExpression>
#include <QGuiApplication>
#include <QClipboard>
#include <QSet>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <algorithm>

NetworkManager::NetworkManager(QObject *parent) : QObject(parent) {
    refreshStatus();
    scanNetworks();

    // Periodic auto-poll timer every 3 seconds
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &NetworkManager::refreshStatus);
    m_pollTimer->start(3000);

    // Periodic speed & ping timer every 1 second (Started on-demand)
    m_speedTimer = new QTimer(this);
    connect(m_speedTimer, &QTimer::timeout, this, &NetworkManager::updateSpeedAndPing);
}

bool NetworkManager::isSpeedTestRunning() const {
    return m_isSpeedTestRunning;
}

void NetworkManager::toggleSpeedTest() {
    if (m_isSpeedTestRunning) {
        m_speedTimer->stop();
        m_isSpeedTestRunning = false;
        
        m_uploadSpeed = "0 B/s";
        m_downloadSpeed = "0 B/s";
        m_uploadSpeedBps = 0.0;
        m_downloadSpeedBps = 0.0;
        m_pingMs = -1;
        
        emit uploadSpeedChanged();
        emit downloadSpeedChanged();
        emit pingMsChanged();
    } else {
        m_isSpeedTestRunning = true;
        m_prevRxBytes = 0;
        m_prevTxBytes = 0;
        m_lastSpeedTimeMs = 0;
        
        m_speedTimer->start(1000);
    }
    emit isSpeedTestRunningChanged();
}

void NetworkManager::refreshStatus() {
    // 1. Wi-Fi status & active SSID
    QProcess wifiProc;
    wifiProc.start("bash", {"-c", "nmcli -g DEVICE,TYPE,STATE,CONNECTION dev status 2>/dev/null | grep ':wifi:connected:'"});
    wifiProc.waitForFinished(1500);
    QString wifiOut = wifiProc.readAllStandardOutput().trimmed();

    if (wifiOut.isEmpty()) {
        // Fallback to active connection show if dev status returned empty
        QProcess wifiProc2;
        wifiProc2.start("bash", {"-c", "nmcli -t -f NAME,TYPE,STATE con show --active 2>/dev/null | grep -iE 'wireless|wifi'"});
        wifiProc2.waitForFinished(1000);
        wifiOut = wifiProc2.readAllStandardOutput().trimmed();
    }

    QString newConnectedSsid;
    bool newIsConnected = false;
    if (!wifiOut.isEmpty()) {
        QStringList parts = wifiOut.split(':');
        if (parts.size() >= 4 && !parts[3].trimmed().isEmpty()) {
            newConnectedSsid = parts[3].trimmed();
        } else {
            newConnectedSsid = parts.value(0).trimmed();
        }
        newIsConnected = true;
    }

    if (m_connectedSsid != newConnectedSsid) {
        m_connectedSsid = newConnectedSsid;
        emit connectedSsidChanged();
    }
    if (m_isConnected != newIsConnected) {
        m_isConnected = newIsConnected;
        emit isConnectedChanged();
    }

    // Wi-Fi radio status
    QProcess radioProc;
    radioProc.start("bash", {"-c", "nmcli radio wifi 2>/dev/null"});
    radioProc.waitForFinished(800);
    QString radioOut = radioProc.readAllStandardOutput().trimmed();
    bool newWifiEnabled = (radioOut == "enabled");
    if (m_wifiEnabled != newWifiEnabled) {
        m_wifiEnabled = newWifiEnabled;
        emit wifiEnabledChanged();
    }

    // 2. Ethernet (Wired) status check
    QProcess ethProc;
    ethProc.start("bash", {"-c", "nmcli -g DEVICE,TYPE,STATE dev status 2>/dev/null | grep ':ethernet:connected'"});
    ethProc.waitForFinished(1000);
    QString ethOut = ethProc.readAllStandardOutput().trimmed();

    bool newEthConnected = false;
    QString newEthIf = "";
    if (!ethOut.isEmpty()) {
        newEthConnected = true;
        newEthIf = ethOut.split(':').value(0);
    }

    if (m_ethernetConnected != newEthConnected) {
        m_ethernetConnected = newEthConnected;
        emit ethernetConnectedChanged();
    }
    if (m_ethernetInterface != newEthIf) {
        m_ethernetInterface = newEthIf;
        emit ethernetInterfaceChanged();
    }

    // Primary connection type
    QString newPrimary = "none";
    if (m_ethernetConnected) {
        newPrimary = "ethernet";
    } else if (m_isConnected) {
        newPrimary = "wifi";
    }
    if (m_primaryConnectionType != newPrimary) {
        m_primaryConnectionType = newPrimary;
        emit primaryConnectionTypeChanged();
    }

    // Ethernet IP & Speed
    if (m_ethernetConnected && !m_ethernetInterface.isEmpty()) {
        QProcess ethIp;
        ethIp.start("bash", {"-c", QString("ip -4 addr show %1 2>/dev/null | grep 'inet ' | awk '{print $2}' | cut -d/ -f1 | head -1").arg(m_ethernetInterface).toUtf8()});
        ethIp.waitForFinished(800);
        QString newEthIp = ethIp.readAllStandardOutput().trimmed();
        if (m_ethernetIpAddress != newEthIp) {
            m_ethernetIpAddress = newEthIp;
            emit ethernetIpAddressChanged();
        }

        QProcess ethSpd;
        ethSpd.start("bash", {"-c", QString("cat /sys/class/net/%1/speed 2>/dev/null").arg(m_ethernetInterface).toUtf8()});
        ethSpd.waitForFinished(500);
        QString spdVal = ethSpd.readAllStandardOutput().trimmed();
        QString newEthSpeed = (!spdVal.isEmpty() && spdVal.toInt() > 0) ? spdVal + " Mb/s" : "1000 Mb/s";
        if (m_ethernetLinkSpeed != newEthSpeed) {
            m_ethernetLinkSpeed = newEthSpeed;
            emit ethernetLinkSpeedChanged();
        }
    } else {
        if (!m_ethernetIpAddress.isEmpty()) {
            m_ethernetIpAddress.clear();
            emit ethernetIpAddressChanged();
        }
        if (!m_ethernetLinkSpeed.isEmpty()) {
            m_ethernetLinkSpeed.clear();
            emit ethernetLinkSpeedChanged();
        }
    }

    // 3. IPv4
    QProcess ip4;
    ip4.start("bash", {"-c", "ip -4 addr show | grep 'inet ' | grep -v '127\\.' | awk '{print $2}' | cut -d/ -f1 | head -1"});
    ip4.waitForFinished(1000);
    QString newIp = ip4.readAllStandardOutput().trimmed();
    if (m_ipAddress != newIp) {
        m_ipAddress = newIp;
        emit ipAddressChanged();
    }

    // 4. IPv6
    QProcess ip6;
    ip6.start("bash", {"-c", "ip -6 addr show | grep 'inet6' | grep -v 'fe80' | grep -v 'host' | awk '{print $2}' | cut -d/ -f1 | head -1"});
    ip6.waitForFinished(1000);
    QString newIp6 = ip6.readAllStandardOutput().trimmed();
    if (newIp6.isEmpty()) {
        QProcess ip6ll;
        ip6ll.start("bash", {"-c", "ip -6 addr show | grep 'inet6 fe80' | awk '{print $2}' | cut -d/ -f1 | head -1"});
        ip6ll.waitForFinished(800);
        newIp6 = ip6ll.readAllStandardOutput().trimmed();
    }
    if (m_ipv6Address != newIp6) {
        m_ipv6Address = newIp6;
        emit ipv6AddressChanged();
    }

    // 5. MAC Address of active interface
    QProcess mac;
    mac.start("bash", {"-c", "ip link show | grep -A1 'wl\\|en\\|eth' | grep 'link/ether' | awk '{print $2}' | head -1"});
    mac.waitForFinished(800);
    QString newMac = mac.readAllStandardOutput().trimmed().toUpper();
    if (m_macAddress != newMac) {
        m_macAddress = newMac;
        emit macAddressChanged();
    }

    // 6. Link speed (Wi-Fi)
    QProcess spd;
    spd.start("bash", {"-c", "iw dev 2>/dev/null | grep -A5 'Interface' | grep bitrate | awk '{print $3,$4}' | head -1"});
    spd.waitForFinished(800);
    QString rawSpeed = spd.readAllStandardOutput().trimmed();
    if (rawSpeed.isEmpty()) {
        QProcess spd2;
        spd2.start("bash", {"-c", "nmcli -t -f IN-USE,RATE dev wifi 2>/dev/null | grep '^\\*' | cut -d: -f2"});
        spd2.waitForFinished(800);
        rawSpeed = spd2.readAllStandardOutput().trimmed();
    }
    QString newSpeed = rawSpeed.isEmpty() ? "—" : rawSpeed;
    if (m_linkSpeed != newSpeed) {
        m_linkSpeed = newSpeed;
        emit linkSpeedChanged();
    }

    // 7. Signal strength
    QProcess sig;
    sig.start("bash", {"-c", "nmcli -t -f IN-USE,SIGNAL dev wifi 2>/dev/null | grep '^\\*' | cut -d: -f2"});
    sig.waitForFinished(1000);
    int newSig = sig.readAllStandardOutput().trimmed().toInt();
    if (m_signalStrength != newSig) {
        m_signalStrength = newSig;
        emit signalStrengthChanged();
    }

    // 8. Gateway Address
    QProcess gwProc;
    gwProc.start("bash", {"-c", "ip route show | grep default | awk '{print $3}' | head -1"});
    gwProc.waitForFinished(800);
    QString newGw = gwProc.readAllStandardOutput().trimmed();
    if (m_gatewayAddress != newGw) {
        m_gatewayAddress = newGw;
        emit gatewayAddressChanged();
    }

    // 9. DNS Servers
    QProcess dnsProc;
    dnsProc.start("bash", {"-c", "grep nameserver /etc/resolv.conf 2>/dev/null | grep -v '#' | awk '{print $2}' | paste -sd ', ' -"});
    dnsProc.waitForFinished(800);
    QString newDns = dnsProc.readAllStandardOutput().trimmed();
    if (m_dnsServers != newDns) {
        m_dnsServers = newDns;
        emit dnsServersChanged();
    }

    // 10. Saved Networks
    QProcess savedProc;
    savedProc.start("bash", {"-c", "nmcli -g NAME,TYPE con show 2>/dev/null | grep ':802-11-wireless' | cut -d: -f1"});
    savedProc.waitForFinished(1000);
    QStringList newSaved;
    for (const QString &line : savedProc.readAllStandardOutput().trimmed().split('\n')) {
        QString s = line.trimmed();
        if (!s.isEmpty()) newSaved.append(s);
    }
    if (m_savedNetworks != newSaved) {
        m_savedNetworks = newSaved;
        emit savedNetworksChanged();
    }
}

bool NetworkManager::wifiEnabled() const { return m_wifiEnabled; }
void NetworkManager::setWifiEnabled(bool v) {
    if (m_wifiEnabled == v) return;
    m_wifiEnabled = v;
    QProcess::startDetached("nmcli", {"radio", "wifi", v ? "on" : "off"});
    emit wifiEnabledChanged();
    QTimer::singleShot(1500, this, &NetworkManager::scanNetworks);
}

QString NetworkManager::connectedSsid() const { return m_connectedSsid; }
int NetworkManager::signalStrength() const { return m_signalStrength; }
QString NetworkManager::ipAddress() const { return m_ipAddress; }
QString NetworkManager::ipv6Address() const { return m_ipv6Address; }
QString NetworkManager::macAddress() const { return m_macAddress; }
QString NetworkManager::linkSpeed() const { return m_linkSpeed; }
bool NetworkManager::isConnected() const { return m_isConnected; }
QStringList NetworkManager::availableNetworks() const { return m_availableNetworks; }
QVariantList NetworkManager::scannedNetworks() const { return m_scannedNetworks; }
QStringList NetworkManager::savedNetworks() const { return m_savedNetworks; }

bool NetworkManager::ethernetConnected() const { return m_ethernetConnected; }
QString NetworkManager::ethernetInterface() const { return m_ethernetInterface; }
QString NetworkManager::ethernetIpAddress() const { return m_ethernetIpAddress; }
QString NetworkManager::ethernetLinkSpeed() const { return m_ethernetLinkSpeed; }
QString NetworkManager::primaryConnectionType() const { return m_primaryConnectionType; }

QString NetworkManager::gatewayAddress() const { return m_gatewayAddress; }
QString NetworkManager::dnsServers() const { return m_dnsServers; }

bool NetworkManager::isScanning() const { return m_isScanning; }
bool NetworkManager::isConnecting() const { return m_isConnecting; }
QString NetworkManager::connectingSsid() const { return m_connectingSsid; }

QString NetworkManager::uploadSpeed() const { return m_uploadSpeed; }
QString NetworkManager::downloadSpeed() const { return m_downloadSpeed; }
double NetworkManager::uploadSpeedBps() const { return m_uploadSpeedBps; }
double NetworkManager::downloadSpeedBps() const { return m_downloadSpeedBps; }
int NetworkManager::pingMs() const { return m_pingMs; }

QString NetworkManager::formatBytesPerSec(double bytesPerSec) const {
    if (bytesPerSec < 1024.0) {
        return QString::number(bytesPerSec, 'f', 0) + " B/s";
    } else if (bytesPerSec < 1024.0 * 1024.0) {
        return QString::number(bytesPerSec / 1024.0, 'f', 1) + " KB/s";
    } else if (bytesPerSec < 1024.0 * 1024.0 * 1024.0) {
        return QString::number(bytesPerSec / (1024.0 * 1024.0), 'f', 1) + " MB/s";
    } else {
        return QString::number(bytesPerSec / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB/s";
    }
}

void NetworkManager::updateSpeedAndPing() {
    qDebug() << "updateSpeedAndPing called";
    // 1. Calculate Upload & Download speed from /proc/net/dev
    qint64 totalRxBytes = 0;
    qint64 totalTxBytes = 0;

        QFile file("/proc/net/dev");
        if (file.open(QIODevice::ReadOnly)) {
            QString content = file.readAll();
            QStringList lines = content.split('\n');
            for (const QString &rawLine : lines) {
                QString line = rawLine.trimmed();
                if (line.isEmpty() || line.startsWith("Inter-") || line.startsWith("face")) continue;

                int colonIdx = line.indexOf(':');
                if (colonIdx == -1) continue;

                QString ifaceName = line.left(colonIdx).trimmed();
                if (ifaceName == "lo") continue;

                QString stats = line.mid(colonIdx + 1).trimmed();
                QStringList tokens = stats.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (tokens.size() >= 9) {
                    totalRxBytes += tokens[0].toLongLong();
                    totalTxBytes += tokens[8].toLongLong();
                }
            }
            file.close();
        }


    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (m_lastSpeedTimeMs > 0 && nowMs > m_lastSpeedTimeMs) {
        double deltaSec = (nowMs - m_lastSpeedTimeMs) / 1000.0;
        if (deltaSec > 0.1) {
            double rxDiff = (totalRxBytes >= m_prevRxBytes) ? (totalRxBytes - m_prevRxBytes) : 0;
            double txDiff = (totalTxBytes >= m_prevTxBytes) ? (totalTxBytes - m_prevTxBytes) : 0;

            m_downloadSpeedBps = rxDiff / deltaSec;
            m_uploadSpeedBps = txDiff / deltaSec;

            m_downloadSpeed = formatBytesPerSec(m_downloadSpeedBps);
            m_uploadSpeed = formatBytesPerSec(m_uploadSpeedBps);

            emit downloadSpeedChanged();
    qDebug() << "Speed:" << m_downloadSpeed << m_uploadSpeed;
            emit uploadSpeedChanged();
        }
    }

    m_prevRxBytes = totalRxBytes;
    m_prevTxBytes = totalTxBytes;
    m_lastSpeedTimeMs = nowMs;

    // 2. Ping Latency Measurement
    if (!m_isCheckingPing && (m_isConnected || m_ethernetConnected || !m_gatewayAddress.isEmpty())) {
        m_isCheckingPing = true;
        QString pingTarget = !m_gatewayAddress.isEmpty() ? m_gatewayAddress : "1.1.1.1";
        QProcess *pingProc = new QProcess(this);
        connect(pingProc, &QProcess::finished, this, [this, pingProc](int code, QProcess::ExitStatus) {
            QString out = pingProc->readAllStandardOutput();
            pingProc->deleteLater();

            int newPing = -1;
            if (code == 0) {
                QRegularExpression re("time=([0-9.]+)\\s*ms");
                QRegularExpressionMatch match = re.match(out);
                if (match.hasMatch()) {
                    newPing = qRound(match.captured(1).toDouble());
                }
            }

            if (m_pingMs != newPing) {
                m_pingMs = newPing;
                emit pingMsChanged();
            }
            m_isCheckingPing = false;
        });

        pingProc->start("ping", {"-c", "1", "-w", "2", pingTarget});
    } else if (!m_isConnected && !m_ethernetConnected && m_gatewayAddress.isEmpty()) {
        if (m_pingMs != -1) {
            m_pingMs = -1;
            emit pingMsChanged();
        }
    }
}

void NetworkManager::scanNetworks() {
    if (m_isScanning) return;
    m_isScanning = true;
    emit isScanningChanged();

    QProcess *p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p]() {
        QString out = p->readAllStandardOutput().trimmed();
        p->deleteLater();

        QVariantList newList;
        QStringList plainSsids;
        QSet<QString> seenSsids;

        for (const QString &line : out.split('\n')) {
            if (line.trimmed().isEmpty()) continue;
            // Output format: IN-USE:SSID:SIGNAL:SECURITY
            QStringList parts = line.split(':');
            if (parts.size() < 4) continue;

            bool inUse = (parts[0].trimmed() == "*");
            QString ssid = parts[1].trimmed();
            int signal = parts[2].trimmed().toInt();
            QString security = parts[3].trimmed();

            if (ssid.isEmpty()) continue;
            if (seenSsids.contains(ssid)) continue;
            seenSsids.insert(ssid);

            bool isSaved = m_savedNetworks.contains(ssid);

            QVariantMap netMap;
            netMap["ssid"] = ssid;
            netMap["signal"] = signal;
            netMap["security"] = security;
            netMap["isConnected"] = inUse || (m_isConnected && m_connectedSsid == ssid);
            netMap["isSaved"] = isSaved;

            newList.append(netMap);
        }

        // Sort scanned networks by signal strength descending (highest speed/signal first)
        std::sort(newList.begin(), newList.end(), [](const QVariant &a, const QVariant &b) {
            return a.toMap()["signal"].toInt() > b.toMap()["signal"].toInt();
        });

        // Restrict to Top 10 Speed Networks only
        if (newList.size() > 10) {
            newList = newList.mid(0, 10);
        }

        for (const QVariant &v : newList) {
            plainSsids.append(v.toMap()["ssid"].toString());
        }

        m_scannedNetworks = newList;
        m_availableNetworks = plainSsids;

        emit scannedNetworksChanged();
        emit availableNetworksChanged();

        m_isScanning = false;
        emit isScanningChanged();
    });

    p->start("bash", {"-c", "nmcli -g IN-USE,SSID,SIGNAL,SECURITY dev wifi list --rescan yes 2>/dev/null"});
}

void NetworkManager::connectToNetwork(const QString &ssid, const QString &password) {
    if (ssid.isEmpty() || m_isConnecting) return;

    m_isConnecting = true;
    m_connectingSsid = ssid;
    emit isConnectingChanged();
    emit connectingSsidChanged();

    QProcess *p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p, ssid](int code, QProcess::ExitStatus exitStatus) {
        QString errStr = p->readAllStandardError().trimmed();
        if (errStr.isEmpty()) errStr = p->readAllStandardOutput().trimmed();
        p->deleteLater();

        m_isConnecting = false;
        m_connectingSsid.clear();
        emit isConnectingChanged();
        emit connectingSsidChanged();

        if (code == 0 && exitStatus == QProcess::NormalExit) {
            refreshStatus();
            emit connectionSuccess(ssid);
        } else {
            if (errStr.isEmpty()) errStr = "Failed to connect to " + ssid;
            emit connectionError(errStr);
        }
    });

    if (!password.isEmpty()) {
        p->start("nmcli", {"dev", "wifi", "connect", ssid, "password", password});
    } else {
        p->start("nmcli", {"dev", "wifi", "connect", ssid});
    }
}

void NetworkManager::disconnectNetwork() {
    QProcess::startDetached("nmcli", {"dev", "disconnect", "wlan0"});
    m_connectedSsid.clear();
    m_isConnected = false;
    emit connectedSsidChanged();
    emit isConnectedChanged();
    refreshStatus();
}

void NetworkManager::forgetNetwork(const QString &ssid) {
    if (ssid.isEmpty()) return;
    QProcess *p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p](int code) {
        p->deleteLater();
        refreshStatus();
        scanNetworks();
    });
    p->start("nmcli", {"connection", "delete", "id", ssid});
}

void NetworkManager::copyToClipboard(const QString &text) {
    if (text.isEmpty()) return;
    QClipboard *cb = QGuiApplication::clipboard();
    if (cb) {
        cb->setText(text);
    }
}


