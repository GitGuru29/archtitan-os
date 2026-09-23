#pragma once

#include <QObject>
#include <QTimer>

class BluetoothController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool powered READ powered NOTIFY poweredChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit BluetoothController(QObject *parent = nullptr);

    bool powered() const { return m_powered; }
    bool connected() const { return m_connected; }
    QString statusText() const { return m_statusText; }

public slots:
    void toggleBluetooth();
    void refresh();

signals:
    void poweredChanged();
    void connectedChanged();
    void statusTextChanged();

private:
    void queryStatus();

    bool m_powered{false};
    bool m_connected{false};
    QString m_statusText{"Not connected"};

    QTimer *m_timer;
};
