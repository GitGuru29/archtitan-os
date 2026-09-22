#include "notificationserver.h"
#include <QDBusConnection>
#include <QDateTime>
#include <QDebug>

NotificationServer::NotificationServer(QObject *parent)
    : QObject(parent)
{
    initDefaultNotifications();

    // Register on session bus for desktop notification specification
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.registerService("org.freedesktop.Notifications")) {
        bus.registerObject("/org/freedesktop/Notifications", this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);
        qInfo() << "Registered org.freedesktop.Notifications D-Bus service.";
    } else {
        qDebug() << "Could not acquire org.freedesktop.Notifications (another notification daemon may be running).";
    }
}

void NotificationServer::initDefaultNotifications()
{
    m_notifications.clear();

    // 1. Network Management
    QVariantMap n1;
    n1["id"] = 1;
    n1["appName"] = "Network Management";
    n1["dateStr"] = "September 12";
    n1["count"] = 21;
    n1["primaryText"] = "No Network Connection";
    n1["secondaryText"] = "You are no longer connected to the network.";
    n1["iconType"] = "wifi";
    n1["expanded"] = false;
    m_notifications.append(n1);

    // 2. Antigravity IDE
    QVariantMap n2;
    n2["id"] = 2;
    n2["appName"] = "Antigravity IDE";
    n2["dateStr"] = "September 11";
    n2["count"] = 441;
    n2["primaryText"] = "Antigravity IDE";
    n2["secondaryText"] = "Host System Troubleshooting Assistant\nRequesting your permission in Terminal: Command...";
    n2["iconType"] = "antigravity";
    n2["expanded"] = false;
    m_notifications.append(n2);

    // 3. notify-send
    QVariantMap n3;
    n3["id"] = 3;
    n3["appName"] = "notify-send";
    n3["dateStr"] = "September 10";
    n3["count"] = 8503;
    n3["primaryText"] = "THM: Reclaiming idle workload";
    n3["secondaryText"] = "\"1387655\" is idle ...\nTHM: Reclaiming idle workload \"1387637\" is idle ...";
    n3["iconType"] = "message";
    n3["expanded"] = false;
    m_notifications.append(n3);

    // 4. Hyprland
    QVariantMap n4;
    n4["id"] = 4;
    n4["appName"] = "Hyprland";
    n4["dateStr"] = "September 08";
    n4["count"] = 2;
    n4["primaryText"] = "Exited Virtual Machine submap";
    n4["secondaryText"] = "Keybinds re-enabled.\nEntered Virtual Machine submap Keybinds disabled.";
    n4["iconType"] = "terminal";
    n4["expanded"] = false;
    m_notifications.append(n4);

    recalculateCount();
    emit notificationsChanged();
}

void NotificationServer::recalculateCount()
{
    int total = 0;
    for (const QVariant &item : m_notifications) {
        QVariantMap map = item.toMap();
        total += map.value("count", 1).toInt();
    }
    if (total != m_totalCount) {
        m_totalCount = total;
        emit totalCountChanged();
    }
}

void NotificationServer::clearAll()
{
    m_notifications.clear();
    m_totalCount = 0;
    emit notificationsChanged();
    emit totalCountChanged();
}

void NotificationServer::dismissAt(int index)
{
    if (index >= 0 && index < m_notifications.size()) {
        m_notifications.removeAt(index);
        recalculateCount();
        emit notificationsChanged();
    }
}

void NotificationServer::toggleExpanded(int index)
{
    if (index >= 0 && index < m_notifications.size()) {
        QVariantMap map = m_notifications[index].toMap();
        map["expanded"] = !map.value("expanded", false).toBool();
        m_notifications[index] = map;
        emit notificationsChanged();
    }
}

void NotificationServer::setDndActive(bool dnd)
{
    if (m_dndActive != dnd) {
        m_dndActive = dnd;
        emit dndActiveChanged();
    }
}

void NotificationServer::toggleDnd()
{
    setDndActive(!m_dndActive);
}

void NotificationServer::addNotification(const QString &appName, const QString &summary, const QString &body, const QString &icon)
{
    if (m_dndActive) {
        return; // Muted by DND
    }

    QVariantMap n;
    n["id"] = ++m_nextId;
    n["appName"] = appName.isEmpty() ? "System Notification" : appName;
    n["dateStr"] = QDateTime::currentDateTime().toString("MMMM d");
    n["count"] = 1;
    n["primaryText"] = summary;
    n["secondaryText"] = body;
    n["iconType"] = icon.isEmpty() ? "message" : icon;
    n["expanded"] = false;

    // Check if an existing card with same app name exists to increment count
    bool merged = false;
    for (int i = 0; i < m_notifications.size(); ++i) {
        QVariantMap existing = m_notifications[i].toMap();
        if (existing.value("appName").toString().compare(n["appName"].toString(), Qt::CaseInsensitive) == 0) {
            existing["count"] = existing.value("count", 1).toInt() + 1;
            existing["dateStr"] = n["dateStr"];
            existing["primaryText"] = n["primaryText"];
            existing["secondaryText"] = n["secondaryText"];
            m_notifications[i] = existing;
            merged = true;
            break;
        }
    }

    if (!merged) {
        m_notifications.prepend(n);
    }

    recalculateCount();
    emit notificationsChanged();
}

uint NotificationServer::Notify(const QString &app_name, uint replaces_id, const QString &app_icon,
                                const QString &summary, const QString &body, const QStringList &actions,
                                const QVariantMap &hints, int timeout)
{
    Q_UNUSED(replaces_id);
    Q_UNUSED(actions);
    Q_UNUSED(hints);
    Q_UNUSED(timeout);

    addNotification(app_name, summary, body, app_icon);
    return m_nextId;
}

void NotificationServer::CloseNotification(uint id)
{
    for (int i = 0; i < m_notifications.size(); ++i) {
        if (m_notifications[i].toMap().value("id").toUInt() == id) {
            dismissAt(i);
            emit NotificationClosed(id, 3); // 3 = closed by call
            break;
        }
    }
}

QStringList NotificationServer::GetCapabilities()
{
    return {"actions", "body", "body-markup", "icon-static", "persistence"};
}

void NotificationServer::GetServerInformation(QString &name, QString &vendor, QString &version, QString &spec_version)
{
    name = "ArchTitan Quick Settings & Notifications";
    vendor = "ArchTitan";
    version = "1.0.0";
    spec_version = "1.2";
}
