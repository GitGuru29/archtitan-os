#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QDateTime>
#include <QLocalSocket>
#include <QProcess>

/**
 * SystemController — Unified system telemetry, power management,
 * Hyprland active window tracker, hardware sensors, and HUD carousel manager.
 */
class SystemController : public QObject
{
    Q_OBJECT

    // ── Carousel properties ──
    Q_PROPERTY(int currentPageIndex READ currentPageIndex NOTIFY currentPageIndexChanged)
    Q_PROPERTY(QString currentPageName READ currentPageName NOTIFY currentPageIndexChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY availablePagesChanged)
    Q_PROPERTY(QStringList availablePages READ availablePages NOTIFY availablePagesChanged)
    Q_PROPERTY(int autoScrollInterval READ autoScrollInterval WRITE setAutoScrollInterval NOTIFY autoScrollIntervalChanged)
    Q_PROPERTY(bool autoScrollEnabled READ autoScrollEnabled WRITE setAutoScrollEnabled NOTIFY autoScrollEnabledChanged)

    // ── Power Profile properties ──
    Q_PROPERTY(QString powerProfile READ powerProfile NOTIFY powerProfileChanged)
    Q_PROPERTY(QString powerProfileDesc READ powerProfileDesc NOTIFY powerProfileChanged)
    Q_PROPERTY(QString powerProfileIcon READ powerProfileIcon NOTIFY powerProfileChanged)

    // ── Active Window properties ──
    Q_PROPERTY(QString activeWindowTitle READ activeWindowTitle NOTIFY activeWindowChanged)
    Q_PROPERTY(QString activeWindowClass READ activeWindowClass NOTIFY activeWindowChanged)
    Q_PROPERTY(QString activeWindowApp READ activeWindowApp NOTIFY activeWindowChanged)

    // ── CPU Temperature properties ──
    Q_PROPERTY(int cpuTemp READ cpuTemp NOTIFY cpuTempChanged)
    Q_PROPERTY(QString cpuTempFormatted READ cpuTempFormatted NOTIFY cpuTempChanged)
    Q_PROPERTY(QString cpuTempStatus READ cpuTempStatus NOTIFY cpuTempChanged)

    // ── Battery properties ──
    Q_PROPERTY(bool hasBattery READ hasBattery NOTIFY batteryChanged)
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY batteryChanged)
    Q_PROPERTY(QString batteryStatus READ batteryStatus NOTIFY batteryChanged)
    Q_PROPERTY(int batteryTemp READ batteryTemp NOTIFY batteryChanged)
    Q_PROPERTY(QString batteryTempFormatted READ batteryTempFormatted NOTIFY batteryChanged)
    Q_PROPERTY(QString batteryHealth READ batteryHealth NOTIFY batteryChanged)

    // ── Date & Time properties ──
    Q_PROPERTY(QString dateFormatted READ dateFormatted NOTIFY timeChanged)
    Q_PROPERTY(QString timeFormatted READ timeFormatted NOTIFY timeChanged)
    Q_PROPERTY(QString greetingText READ greetingText NOTIFY timeChanged)

    // ── GPU properties ──
    Q_PROPERTY(bool hasGpu READ hasGpu NOTIFY gpuChanged)
    Q_PROPERTY(QString gpuModel READ gpuModel NOTIFY gpuChanged)
    Q_PROPERTY(int gpuTemp READ gpuTemp NOTIFY gpuChanged)
    Q_PROPERTY(QString gpuTempFormatted READ gpuTempFormatted NOTIFY gpuChanged)
    Q_PROPERTY(int gpuUsage READ gpuUsage NOTIFY gpuChanged)

public:
    explicit SystemController(QObject *parent = nullptr);
    ~SystemController();

    // Getters - Carousel
    int currentPageIndex() const { return m_currentPageIndex; }
    QString currentPageName() const;
    int pageCount() const { return m_availablePages.size(); }
    QStringList availablePages() const { return m_availablePages; }
    int autoScrollInterval() const { return m_autoScrollInterval; }
    void setAutoScrollInterval(int msec);
    bool autoScrollEnabled() const { return m_autoScrollEnabled; }
    void setAutoScrollEnabled(bool enabled);

    // Getters - Power Profile
    QString powerProfile() const { return m_powerProfile; }
    QString powerProfileDesc() const { return m_powerProfileDesc; }
    QString powerProfileIcon() const { return m_powerProfileIcon; }

    // Getters - Active Window
    QString activeWindowTitle() const { return m_activeWindowTitle; }
    QString activeWindowClass() const { return m_activeWindowClass; }
    QString activeWindowApp() const { return m_activeWindowApp; }

    // Getters - CPU Temperature
    int cpuTemp() const { return m_cpuTemp; }
    QString cpuTempFormatted() const { return QString::number(m_cpuTemp) + QStringLiteral("°C"); }
    QString cpuTempStatus() const;

    // Getters - Battery
    bool hasBattery() const { return m_hasBattery; }
    int batteryPercent() const { return m_batteryPercent; }
    QString batteryStatus() const { return m_batteryStatus; }
    int batteryTemp() const { return m_batteryTemp; }
    QString batteryTempFormatted() const { return m_batteryTemp > 0 ? (QString::number(m_batteryTemp) + QStringLiteral("°C")) : QStringLiteral("Normal"); }
    QString batteryHealth() const { return m_batteryHealth; }

    // Getters - Date & Time
    QString dateFormatted() const { return m_dateFormatted; }
    QString timeFormatted() const { return m_timeFormatted; }
    QString greetingText() const { return m_greetingText; }

    // Getters - GPU
    bool hasGpu() const { return m_hasGpu; }
    QString gpuModel() const { return m_gpuModel; }
    int gpuTemp() const { return m_gpuTemp; }
    QString gpuTempFormatted() const { return QString::number(m_gpuTemp) + QStringLiteral("°C"); }
    int gpuUsage() const { return m_gpuUsage; }

public slots:
    void nextPage();
    void previousPage();
    void setPage(int index);
    void userScrolled(int delta);
    void refreshAll();

signals:
    void currentPageIndexChanged();
    void availablePagesChanged();
    void autoScrollIntervalChanged();
    void autoScrollEnabledChanged();
    void powerProfileChanged();
    void activeWindowChanged();
    void cpuTempChanged();
    void batteryChanged();
    void timeChanged();
    void gpuChanged();

private slots:
    void onAutoScrollTimer();
    void onSensorsTimer();
    void onTimeTimer();
    void onHyprlandSocketReadyRead();
    void onHyprlandSocketError();
    void onDBusPropertiesChanged(const QString &interface, const QVariantMap &changedProps, const QStringList &invalidatedProps);

private:
    void initAvailablePages();
    void initHyprlandIpc();
    void initPowerProfilesDBus();
    void updatePowerProfile();
    void updateCpuTemperature();
    void updateBattery();
    void updateDateTime();
    void updateGpu();
    void updateActiveWindowFromHyprctl();
    void parseHyprlandEvent(const QString &line);

    // Carousel state
    int m_currentPageIndex{0};
    QStringList m_availablePages;
    int m_autoScrollInterval{4000}; // 4 seconds default
    bool m_autoScrollEnabled{true};
    QTimer *m_autoScrollTimer{nullptr};
    QTimer *m_sensorsTimer{nullptr};
    QTimer *m_timeTimer{nullptr};

    // Power Profile
    QString m_powerProfile{"Balanced"};
    QString m_powerProfileDesc{"Optimized for performance and efficiency"};
    QString m_powerProfileIcon{"⚡"};

    // Active Window
    QString m_activeWindowTitle{"ArchTitan Desktop"};
    QString m_activeWindowClass{"Desktop"};
    QString m_activeWindowApp{"ArchTitan OS"};
    QLocalSocket *m_hyprlandSocket{nullptr};

    // CPU Temperature
    int m_cpuTemp{45};

    // Battery
    bool m_hasBattery{false};
    int m_batteryPercent{100};
    QString m_batteryStatus{"Discharging"};
    int m_batteryTemp{35};
    QString m_batteryHealth{"Good"};

    // Date & Time
    QString m_dateFormatted;
    QString m_timeFormatted;
    QString m_greetingText{"Have a productive day! ☀️"};

    // GPU
    bool m_hasGpu{false};
    QString m_gpuModel{"Discrete GPU"};
    int m_gpuTemp{45};
    int m_gpuUsage{20};
};
