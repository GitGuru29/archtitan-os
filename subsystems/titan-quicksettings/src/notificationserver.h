#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QDBusContext>

class NotificationServer : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")
    Q_PROPERTY(QVariantList notifications READ notifications NOTIFY notificationsChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
    Q_PROPERTY(bool dndActive READ dndActive WRITE setDndActive NOTIFY dndActiveChanged)

public:
    explicit NotificationServer(QObject *parent = nullptr);

    QVariantList notifications() const { return m_notifications; }
    int totalCount() const { return m_totalCount; }
    bool dndActive() const { return m_dndActive; }
    void setDndActive(bool dnd);

public slots:
    void clearAll();
    void dismissAt(int index);
    void toggleDnd();
    void toggleExpanded(int index);
    void addNotification(const QString &appName, const QString &summary, const QString &body, const QString &icon = "");

    // org.freedesktop.Notifications D-Bus Methods
    uint Notify(const QString &app_name, uint replaces_id, const QString &app_icon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int timeout);
    void CloseNotification(uint id);
    QStringList GetCapabilities();
    void GetServerInformation(QString &name, QString &vendor, QString &version, QString &spec_version);

signals:
    void notificationsChanged();
    void totalCountChanged();
    void dndActiveChanged();
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &action_key);

private:
    void initDefaultNotifications();
    void recalculateCount();

    QVariantList m_notifications;
    int m_totalCount{8967};
    bool m_dndActive{false};
    uint m_nextId{100};
};
