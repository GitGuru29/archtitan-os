#pragma once
#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QDBusInterface>
#include <QDBusReply>

class SettingsBackend : public QObject {
    Q_OBJECT

    // Appearance
    Q_PROPERTY(QString colorTheme READ colorTheme WRITE setColorTheme NOTIFY colorThemeChanged)
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(bool glassmorphism READ glassmorphism WRITE setGlassmorphism NOTIFY glassmorphismChanged)
    Q_PROPERTY(double panelOpacity READ panelOpacity WRITE setPanelOpacity NOTIFY panelOpacityChanged)
    Q_PROPERTY(QString iconTheme READ iconTheme WRITE setIconTheme NOTIFY iconThemeChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)

    // Power
    Q_PROPERTY(int screenTimeout READ screenTimeout WRITE setScreenTimeout NOTIFY screenTimeoutChanged)
    Q_PROPERTY(int suspendTimeout READ suspendTimeout WRITE setSuspendTimeout NOTIFY suspendTimeoutChanged)
    Q_PROPERTY(QString powerProfile READ powerProfile WRITE setPowerProfile NOTIFY powerProfileChanged)

    // Security
    Q_PROPERTY(bool autolockEnabled READ autolockEnabled WRITE setAutolockEnabled NOTIFY autolockEnabledChanged)
    Q_PROPERTY(int autolockDelay READ autolockDelay WRITE setAutolockDelay NOTIFY autolockDelayChanged)
    Q_PROPERTY(bool lockOnScreenOff READ lockOnScreenOff WRITE setLockOnScreenOff NOTIFY lockOnScreenOffChanged)
    Q_PROPERTY(bool lockscreenBlur READ lockscreenBlur WRITE setLockscreenBlur NOTIFY lockscreenBlurChanged)
    Q_PROPERTY(QString lockscreenRingColor READ lockscreenRingColor WRITE setLockscreenRingColor NOTIFY lockscreenRingColorChanged)

public:
    explicit SettingsBackend(QObject *parent = nullptr);

    QString colorTheme() const;
    void setColorTheme(const QString &v);

    QString accentColor() const;
    void setAccentColor(const QString &v);

    bool glassmorphism() const;
    void setGlassmorphism(bool v);

    double panelOpacity() const;
    void setPanelOpacity(double v);

    QString iconTheme() const;
    void setIconTheme(const QString &v);

    QString fontFamily() const;
    void setFontFamily(const QString &v);

    int fontSize() const;
    void setFontSize(int v);

    int screenTimeout() const;
    void setScreenTimeout(int v);

    int suspendTimeout() const;
    void setSuspendTimeout(int v);

    QString powerProfile() const;
    void setPowerProfile(const QString &v);

    bool autolockEnabled() const;
    void setAutolockEnabled(bool v);

    int autolockDelay() const;
    void setAutolockDelay(int v);

    bool lockOnScreenOff() const;
    void setLockOnScreenOff(bool v);

    bool lockscreenBlur() const;
    void setLockscreenBlur(bool v);

    QString lockscreenRingColor() const;
    void setLockscreenRingColor(const QString &v);

    Q_INVOKABLE void applyAppearance();
    Q_INVOKABLE void applySecurity();
    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE void applyPowerProfileNow(const QString &profile);
    Q_INVOKABLE void applyScreenTimeoutNow(int seconds);
    Q_INVOKABLE void applySuspendTimeoutNow(int seconds);
    Q_INVOKABLE QString version() const { return QStringLiteral("1.0.0"); }

signals:
    void colorThemeChanged();
    void accentColorChanged();
    void glassmorphismChanged();
    void panelOpacityChanged();
    void iconThemeChanged();
    void fontFamilyChanged();
    void fontSizeChanged();
    void screenTimeoutChanged();
    void suspendTimeoutChanged();
    void powerProfileChanged();
    void autolockEnabledChanged();
    void autolockDelayChanged();
    void lockOnScreenOffChanged();
    void lockscreenBlurChanged();
    void lockscreenRingColorChanged();
    void settingsSaved();

private:
    QSettings m_settings;
    QString m_colorTheme;
    QString m_accentColor;
    bool m_glassmorphism;
    double m_panelOpacity;
    QString m_iconTheme;
    QString m_fontFamily;
    int m_fontSize;
    int m_screenTimeout;
    int m_suspendTimeout;
    QString m_powerProfile;
    bool m_autolockEnabled;
    int m_autolockDelay;
    bool m_lockOnScreenOff;
    bool m_lockscreenBlur;
    QString m_lockscreenRingColor;

    void loadSettings();
    void updateSwayidleConfig();
    bool applyPowerProfileDBus(const QString &profile);

private slots:
    void onDBusPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
};
