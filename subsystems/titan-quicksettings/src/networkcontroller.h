#pragma once

#include <QObject>
#include <QTimer>

class NetworkController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool wifiEnabled READ wifiEnabled NOTIFY wifiEnabledChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString ssid READ ssid NOTIFY ssidChanged)

public:
    explicit NetworkController(QObject *parent = nullptr);

    bool wifiEnabled() const { return m_wifiEnabled; }
    bool connected() const { return m_connected; }
    QString ssid() const { return m_ssid; }

public slots:
    void toggleWifi();
    void refresh();

signals:
    void wifiEnabledChanged();
    void connectedChanged();
    void ssidChanged();

private:
    void queryStatus();

    bool m_wifiEnabled{true};
    bool m_connected{true};
    QString m_ssid{"moto g24 power"};

    QTimer *m_timer;
};
