#pragma once
#include <QObject>
#include <QString>
#include <QTimer>

class SystemInfo : public QObject {
    Q_OBJECT

    // ── System ─────────────────────────────────────────────────────────
    Q_PROPERTY(QString hostname      READ hostname      NOTIFY hostnameChanged)
    Q_PROPERTY(QString kernelVersion READ kernelVersion CONSTANT)
    Q_PROPERTY(QString osVersion     READ osVersion     CONSTANT)
    Q_PROPERTY(QString cpuModel      READ cpuModel      CONSTANT)
    Q_PROPERTY(QString gpuModel      READ gpuModel      CONSTANT)
    Q_PROPERTY(int     totalRam      READ totalRam      CONSTANT)
    Q_PROPERTY(int     usedRam       READ usedRam       NOTIFY usedRamChanged)
    Q_PROPERTY(double  cpuUsage      READ cpuUsage      NOTIFY cpuUsageChanged)
    Q_PROPERTY(double  diskUsedGb    READ diskUsedGb    NOTIFY diskUsedGbChanged)
    Q_PROPERTY(double  diskTotalGb   READ diskTotalGb   CONSTANT)
    Q_PROPERTY(QString uptime        READ uptime        NOTIFY uptimeChanged)

    // ── Battery basics ─────────────────────────────────────────────────
    Q_PROPERTY(int  batteryLevel    READ batteryLevel    NOTIFY batteryLevelChanged)
    Q_PROPERTY(bool batteryCharging READ batteryCharging NOTIFY batteryChargingChanged)
    Q_PROPERTY(bool acConnected     READ acConnected     NOTIFY acConnectedChanged)
    Q_PROPERTY(int  batteryCycles   READ batteryCycles   NOTIFY batteryCyclesChanged)
    Q_PROPERTY(int  batteryHealth   READ batteryHealth   NOTIFY batteryHealthChanged)
    Q_PROPERTY(int  powerNow        READ powerNow        NOTIFY powerNowChanged)
    Q_PROPERTY(double estimatedRuntime READ estimatedRuntime NOTIFY estimatedRuntimeChanged)

    // ── Battery Protection (charge limit / conservation) ───────────────
    // chargeProtectionSupported : true if ANY protection mechanism found
    // chargeProtectionMode      : "threshold" | "conservation" | "asus_mode" | "none"
    //   "threshold"   → ThinkPad, ASUS, Samsung, Framework, Huawei, MSI
    //                   shows % chips; setChargeLimit(60..100)
    //   "conservation"→ Lenovo IdeaPad binary on/off (caps at ~60%)
    //                   shows toggle; setChargeProtection(bool)
    //   "asus_mode"   → ASUS ROG 3-way: 0=Balanced 1=Full 2=Gaming
    //                   shows 3 chips; setAsusChargeMode(int)
    Q_PROPERTY(bool    chargeProtectionSupported READ chargeProtectionSupported CONSTANT)
    Q_PROPERTY(QString chargeProtectionMode      READ chargeProtectionMode      CONSTANT)
    Q_PROPERTY(bool    chargeProtectionEnabled   READ chargeProtectionEnabled   NOTIFY chargeProtectionEnabledChanged)
    Q_PROPERTY(int     chargeLimit               READ chargeLimit               NOTIFY chargeLimitChanged)
    Q_PROPERTY(int     asusChargeMode            READ asusChargeMode            NOTIFY asusChargeModeChanged)

    // ── Rapid / Fast Charging ──────────────────────────────────────────
    // Supported on: Lenovo IdeaPad (rapid_charge), some ASUS
    Q_PROPERTY(bool rapidChargeSupported READ rapidChargeSupported CONSTANT)
    Q_PROPERTY(bool rapidChargeEnabled   READ rapidChargeEnabled   NOTIFY rapidChargeEnabledChanged)

public:
    explicit SystemInfo(QObject *parent = nullptr);

    // System getters
    QString hostname()      const;
    QString kernelVersion() const;
    QString osVersion()     const;
    QString cpuModel()      const;
    QString gpuModel()      const;
    int     totalRam()      const;
    int     usedRam()       const;
    double  cpuUsage()      const;
    double  diskUsedGb()    const;
    double  diskTotalGb()   const;
    QString uptime()        const;

    // Battery getters
    int    batteryLevel()       const;
    bool   batteryCharging()    const;
    bool   acConnected()        const;
    int    batteryCycles()      const;
    int    batteryHealth()      const;
    int    powerNow()           const;
    double estimatedRuntime()   const;

    // Protection getters
    bool    chargeProtectionSupported() const;
    QString chargeProtectionMode()      const;
    bool    chargeProtectionEnabled()   const;
    int     chargeLimit()               const;
    int     asusChargeMode()            const;

    // Rapid charge
    bool rapidChargeSupported() const;
    bool rapidChargeEnabled()   const;

    // Invokables — QML calls these
    Q_INVOKABLE void setChargeProtection(bool enabled);   // conservation mode on/off
    Q_INVOKABLE void setChargeLimit(int percent);         // threshold: 60-100
    Q_INVOKABLE void setAsusChargeMode(int mode);         // ASUS ROG: 0/1/2
    Q_INVOKABLE void setRapidCharge(bool enabled);        // Lenovo rapid charge

signals:
    void hostnameChanged();
    void usedRamChanged();
    void cpuUsageChanged();
    void diskUsedGbChanged();
    void uptimeChanged();
    void batteryLevelChanged();
    void batteryChargingChanged();
    void acConnectedChanged();
    void batteryCyclesChanged();
    void batteryHealthChanged();
    void powerNowChanged();
    void estimatedRuntimeChanged();
    void chargeProtectionEnabledChanged();
    void chargeLimitChanged();
    void asusChargeModeChanged();
    void rapidChargeEnabledChanged();

private slots:
    void refresh();

private:
    QTimer m_timer;
    mutable QString m_cpuModel;
    mutable QString m_gpuModel;
    mutable int    m_totalRam    = 0;
    mutable double m_diskTotalGb = 0.0;

    // Runtime state
    double m_cpuUsage      = 0.0;
    int    m_usedRam       = 0;
    double m_diskUsedGb    = 0.0;
    int    m_batteryLevel  = 100;
    bool   m_batteryCharging = false;
    bool   m_acConnected   = false;
    int    m_batteryCycles = -1;
    int    m_batteryHealth = -1;
    int    m_powerNow      = 0;
    double m_estimatedRuntime = -1.0;

    // Battery protection state
    bool    m_chargeProtectionEnabled = false;
    int     m_chargeLimit             = 100;
    int     m_asusChargeMode          = -1;
    bool    m_rapidChargeEnabled      = false;
    bool    m_usesAcpiCallForRapidCharge = false;

    // Discovered hardware paths (empty = not available on this device)
    QString m_batPath;               // /sys/class/power_supply/BAT0
    QString m_acPath;                // /sys/class/power_supply/ACAD (or ADP1, etc)
    QString m_thresholdEndPath;      // BAT*/charge_control_end_threshold  (ThinkPad,ASUS,Samsung,Framework...)
    QString m_thresholdStartPath;    // BAT*/charge_control_start_threshold (ThinkPad)
    QString m_conservationPath;      // Lenovo IdeaPad conservation_mode
    QString m_rapidChargePath;       // Lenovo IdeaPad rapid_charge
    QString m_asusChargeModePath;    // ASUS ROG charge_mode
    QString m_chargeProtectionMode;  // detected mode string

    // CPU delta
    long long m_prevIdle = 0, m_prevTotal = 0;

    // Helpers
    void initCpuModel();
    void initTotalRam();
    void initDiskTotal();
    void initBatteryStatics();    // hardware detection — runs once at startup
    void readCpuUsage();
    void readMemory();
    void readDisk();
    void readBattery();

    static QString readSysFile(const QString &path);
    static bool    writeSysFile(const QString &path, const QString &value);
    static QString findSysPath(const QStringList &candidates);  // returns first existing path
};
