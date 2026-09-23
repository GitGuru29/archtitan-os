#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>

class SystemController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString uptime READ uptime NOTIFY uptimeChanged)
    Q_PROPERTY(QString currentDateString READ currentDateString NOTIFY currentDateStringChanged)
    Q_PROPERTY(bool caffeineActive READ caffeineActive WRITE setCaffeineActive NOTIFY caffeineActiveChanged)
    Q_PROPERTY(bool themeDark READ themeDark WRITE setThemeDark NOTIFY themeDarkChanged)
    Q_PROPERTY(bool visualizerActive READ visualizerActive WRITE setVisualizerActive NOTIFY visualizerActiveChanged)
    Q_PROPERTY(bool gameModeActive READ gameModeActive WRITE setGameModeActive NOTIFY gameModeActiveChanged)
    Q_PROPERTY(bool vpnActive READ vpnActive WRITE setVpnActive NOTIFY vpnActiveChanged)

public:
    explicit SystemController(QObject *parent = nullptr);

    QString uptime() const { return m_uptime; }
    QString currentDateString() const { return m_currentDateString; }
    bool caffeineActive() const { return m_caffeineActive; }
    bool themeDark() const { return m_themeDark; }
    bool visualizerActive() const { return m_visualizerActive; }
    bool gameModeActive() const { return m_gameModeActive; }
    bool vpnActive() const { return m_vpnActive; }

    void setCaffeineActive(bool active);
    void setThemeDark(bool dark);
    void setVisualizerActive(bool active);
    void setGameModeActive(bool active);
    void setVpnActive(bool active);

public slots:
    void refresh();
    void openSettings();
    void openPowerMenu();
    void takeScreenshot();
    void pickColor();
    void toggleVirtualKeyboard();
    void openMediaWidget();

signals:
    void uptimeChanged();
    void currentDateStringChanged();
    void caffeineActiveChanged();
    void themeDarkChanged();
    void visualizerActiveChanged();
    void gameModeActiveChanged();
    void vpnActiveChanged();

private:
    void updateUptime();
    void updateDate();

    QString m_uptime{"Up 0m"};
    QString m_currentDateString;
    bool m_caffeineActive{false};
    bool m_themeDark{true};
    bool m_visualizerActive{true};
    bool m_gameModeActive{false};
    bool m_vpnActive{false};

    QTimer *m_timer;
};
