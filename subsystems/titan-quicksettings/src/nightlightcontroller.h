#pragma once

#include <QObject>

class NightLightController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit NightLightController(QObject *parent = nullptr);

    bool active() const { return m_active; }
    QString statusText() const { return m_active ? "Active" : "Inactive"; }

public slots:
    void toggleNightLight();

signals:
    void activeChanged();
    void statusTextChanged();

private:
    bool m_active{false};
};
