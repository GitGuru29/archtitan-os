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

    // 1. Antigravity IDE
    QVariantMap n1;
    n1["id"] = 1;
    n1["appName"] = "Antigravity IDE";
    n1["dateStr"] = "Now";
    n1["count"] = 755;
    n1["primaryText"] = "Antigravity IDE Project Status Inquiry";
    n1["secondaryText"] = "Requesting your permission in Terminal: Comm...";
    n1["iconType"] = "antigravity";
    n1["expanded"] = false;
    m_notifications.append(n1);

    // 2. notify-send
    QVariantMap n2;
    n2["id"] = 2;
    n2["appName"] = "notify-send";
    n2["dateStr"] = "Yesterday";
    n2["count"] = 13712;
    n2["primaryText"] = "THM: Reclaiming idle workload \"210707\" is idle an...";
    n2["secondaryText"] = "THM: Reclaiming idle workload \"210627\" is idle an...";
    n2["iconType"] = "message";
    n2["expanded"] = false;
    m_notifications.append(n2);

    // 3. Network Management
    QVariantMap n3;
    n3["id"] = 3;
    n3["appName"] = "Network Management";
    n3["dateStr"] = "Yesterday";
    n3["count"] = 25;
    n3["primaryText"] = "moto g24 power Connection 'moto g24 power' a...";
    n3["secondaryText"] = "Limited Connectivity This device appears to be ...";
    n3["iconType"] = "wifi";
    n3["expanded"] = false;
    m_notifications.append(n3);

    // 4. Hyprland
    QVariantMap n4;
    n4["id"] = 4;
    n4["appName"] = "Hyprland";
    n4["dateStr"] = "September 08";
    n4["count"] = 2;
    n4["primaryText"] = "Exited Virtual Machine submap Keybinds re-ena...";
    n4["secondaryText"] = "Entered Virtual Machine submap Keybinds disab...";
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
