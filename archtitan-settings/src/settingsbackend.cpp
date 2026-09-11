#include "settingsbackend.h"
#include <QProcess>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>

SettingsBackend::SettingsBackend(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral("ArchTitan"), QStringLiteral("archtitan-settings"))
{
    loadSettings();

    // Listen to external D-Bus changes for power profile (try both common service names)
    QDBusConnection::systemBus().connect(
        QStringLiteral("net.hadess.PowerProfiles"),
        QStringLiteral("/net/hadess/PowerProfiles"),
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("PropertiesChanged"),
        this,
        SLOT(onDBusPropertiesChanged(QString,QVariantMap,QStringList))
    );
    QDBusConnection::systemBus().connect(
        QStringLiteral("org.freedesktop.UPower.PowerProfiles"),
        QStringLiteral("/org/freedesktop/UPower/PowerProfiles"),
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("PropertiesChanged"),
        this,
        SLOT(onDBusPropertiesChanged(QString,QVariantMap,QStringList))
    );
}

void SettingsBackend::loadSettings() {
    m_colorTheme   = m_settings.value("appearance/colorTheme",   "Dark").toString();
    m_accentColor  = m_settings.value("appearance/accentColor",  "#7AA2F7").toString();
    m_glassmorphism= m_settings.value("appearance/glassmorphism", true).toBool();
    m_panelOpacity = m_settings.value("appearance/panelOpacity",  0.88).toDouble();
    m_iconTheme    = m_settings.value("appearance/iconTheme",    "Papirus-Dark").toString();
    m_fontFamily   = m_settings.value("appearance/fontFamily",   "Inter").toString();
    m_fontSize     = m_settings.value("appearance/fontSize",     13).toInt();

    m_screenTimeout  = m_settings.value("power/screenTimeout",  300).toInt();
    m_suspendTimeout = m_settings.value("power/suspendTimeout", 600).toInt();
    m_powerProfile   = m_settings.value("power/profile",        "Balanced").toString();

    m_autolockEnabled = m_settings.value("security/autolockEnabled", true).toBool();
    m_autolockDelay   = m_settings.value("security/autolockDelay",   300).toInt();
    m_lockOnScreenOff = m_settings.value("security/lockOnScreenOff", false).toBool();
    m_lockscreenBlur  = m_settings.value("security/lockscreenBlur", false).toBool();
    m_lockscreenRingColor = m_settings.value("security/lockscreenRingColor", "default").toString();
}

void SettingsBackend::applyAppearance() {
    // 1. Persist to QSettings
    m_settings.setValue("appearance/colorTheme",   m_colorTheme);
    m_settings.setValue("appearance/accentColor",  m_accentColor);
    m_settings.setValue("appearance/glassmorphism",m_glassmorphism);
    m_settings.setValue("appearance/panelOpacity", m_panelOpacity);
    m_settings.setValue("appearance/iconTheme",    m_iconTheme);
    m_settings.setValue("appearance/fontFamily",   m_fontFamily);
    m_settings.setValue("appearance/fontSize",     m_fontSize);
    m_settings.sync();

    // 2. Apply accent color to Hyprland (border colors)
    QString hex = m_accentColor;
    hex.remove('#');
    QString hyprColor = "0xff" + hex;
    QProcess::startDetached("hyprctl", {"keyword", "general:col.active_border", hyprColor + " 0xff444444 45deg"});
    QProcess::startDetached("hyprctl", {"keyword", "general:col.inactive_border", "0xff333333"});

    // 3. Apply icon theme via gsettings (GTK apps pick this up)
    QProcess::startDetached("gsettings", {"set", "org.gnome.desktop.interface", "icon-theme", m_iconTheme});

    // 4. Apply font family + size via gsettings
    QString fontSpec = m_fontFamily + " " + QString::number(m_fontSize);
    QProcess::startDetached("gsettings", {"set", "org.gnome.desktop.interface", "font-name", fontSpec});
    QProcess::startDetached("gsettings", {"set", "org.gnome.desktop.interface", "document-font-name", fontSpec});
    QProcess::startDetached("gsettings", {"set", "org.gnome.desktop.interface", "monospace-font-name",
                                          m_fontFamily + " " + QString::number(m_fontSize)});

    qDebug() << "[SettingsBackend] Appearance Applied:" << m_colorTheme << m_accentColor << m_iconTheme << fontSpec;
    emit settingsSaved();
}

void SettingsBackend::applySecurity() {
    m_settings.setValue("security/autolockEnabled",m_autolockEnabled);
    m_settings.setValue("security/autolockDelay",  m_autolockDelay);
    m_settings.setValue("security/lockOnScreenOff", m_lockOnScreenOff);
    m_settings.setValue("security/lockscreenBlur", m_lockscreenBlur);
    m_settings.setValue("security/lockscreenRingColor", m_lockscreenRingColor);
    m_settings.sync();

    updateSwayidleConfig();

    qDebug() << "[SettingsBackend] Security Applied";
    emit settingsSaved();
}

void SettingsBackend::updateSwayidleConfig() {
    QProcess::startDetached("bash", {"-c", "hyprctl keyword decoration:screen_shader '' 2>/dev/null"});

    QString swaylockCmd = "swaylock -f";
    if (m_lockscreenBlur) {
        swaylockCmd += " --effect-blur 7x5";
    }
    if (m_lockscreenRingColor != "default") {
        swaylockCmd += " --ring-color " + m_lockscreenRingColor + " --inside-color 00000000 --line-color 00000000 --separator-color 00000000";
    }

    QString swayidleConfig = QString(
        "timeout %1 'hyprctl dispatch dpmsoff%2' resume 'hyprctl dispatch dpmson'\n"
        "timeout %3 'systemctl suspend'\n"
        "before-sleep '%4'\n"
    ).arg(m_screenTimeout).arg(m_lockOnScreenOff ? QString("; ") + swaylockCmd : "").arg(m_suspendTimeout).arg(swaylockCmd);

    if (m_autolockEnabled) {
        swayidleConfig += QString("timeout %1 '%2'\n").arg(m_autolockDelay).arg(swaylockCmd);
    }

    QFile idleConf(QDir::homePath() + "/.config/swayidle/config");
    if (idleConf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&idleConf) << swayidleConfig;
        idleConf.close();
    }
    QProcess::startDetached("bash", {"-c", "pkill swayidle 2>/dev/null; command -v swayidle >/dev/null && swayidle -w &"});

    if (!m_autolockEnabled) {
        QProcess::startDetached("pkill", {"swayidle"});
    }
}

void SettingsBackend::resetToDefaults() {
    m_settings.clear();
    loadSettings();
    emit colorThemeChanged();
    emit accentColorChanged();
    emit glassmorphismChanged();
    emit panelOpacityChanged();
    emit iconThemeChanged();
    emit fontFamilyChanged();
    emit fontSizeChanged();
    emit screenTimeoutChanged();
    emit suspendTimeoutChanged();
    emit powerProfileChanged();
    emit autolockEnabledChanged();
    emit autolockDelayChanged();
    emit lockOnScreenOffChanged();
    emit lockscreenBlurChanged();
    emit lockscreenRingColorChanged();
}

// Apply & persist the power profile immediately — no full applyAndSave() needed.
// Called directly from QML when the user clicks a profile card.
void SettingsBackend::applyPowerProfileNow(const QString &profile)
{
    // 1. Update in-memory state and emit so UI updates instantly
    if (m_powerProfile != profile) {
        m_powerProfile = profile;
        emit powerProfileChanged();
    }

    // 2. Persist just the profile key
    m_settings.setValue(QStringLiteral("power/profile"), profile);
    m_settings.sync();

    // 3. Map display name → daemon ID  ("Power Saver" → "power-saver")
    QString prof = profile.toLower();
    prof.replace(QLatin1Char(' '), QLatin1Char('-'));

    // 4. Try D-Bus first; synchronous fallback if daemon unavailable
    bool ok = applyPowerProfileDBus(prof);
    if (!ok) {
        qWarning() << "[SettingsBackend] D-Bus failed, falling back to powerprofilesctl";
        QProcess::execute(QStringLiteral("powerprofilesctl"), {QStringLiteral("set"), prof});
    }
}

// Apply screen-off timeout immediately — persists and rewrites swayidle config.
void SettingsBackend::applyScreenTimeoutNow(int seconds)
{
    if (m_screenTimeout != seconds) {
        m_screenTimeout = seconds;
        emit screenTimeoutChanged();
    }
    m_settings.setValue(QStringLiteral("power/screenTimeout"), seconds);
    m_settings.sync();

    // Rewrite swayidle config with the new screen timeout
    updateSwayidleConfig();

    qDebug() << "[SettingsBackend] Screen timeout applied:" << seconds << "s";
}

// Apply suspend timeout immediately — persists and rewrites swayidle config.
void SettingsBackend::applySuspendTimeoutNow(int seconds)
{
    if (m_suspendTimeout != seconds) {
        m_suspendTimeout = seconds;
        emit suspendTimeoutChanged();
    }
    m_settings.setValue(QStringLiteral("power/suspendTimeout"), seconds);
    m_settings.sync();

    // "Never" sentinel (99999) — just kill swayidle so suspend never fires
    if (seconds >= 99999) {
        QProcess::startDetached(QStringLiteral("pkill"), {QStringLiteral("swayidle")});
        qDebug() << "[SettingsBackend] Suspend disabled (Never)";
        return;
    }

    updateSwayidleConfig();

    qDebug() << "[SettingsBackend] Suspend timeout applied:" << seconds << "s";
}

// ── Getters / Setters ──────────────────────────────────────────────────────

QString SettingsBackend::colorTheme() const { return m_colorTheme; }
void SettingsBackend::setColorTheme(const QString &v) {
    if (m_colorTheme == v) return;
    m_colorTheme = v;
    emit colorThemeChanged();
}

QString SettingsBackend::accentColor() const { return m_accentColor; }
void SettingsBackend::setAccentColor(const QString &v) {
    if (m_accentColor == v) return;
    m_accentColor = v;
    emit accentColorChanged();
}

bool SettingsBackend::glassmorphism() const { return m_glassmorphism; }
void SettingsBackend::setGlassmorphism(bool v) {
    if (m_glassmorphism == v) return;
    m_glassmorphism = v;
    emit glassmorphismChanged();
}

double SettingsBackend::panelOpacity() const { return m_panelOpacity; }
void SettingsBackend::setPanelOpacity(double v) {
    if (qFuzzyCompare(m_panelOpacity, v)) return;
    m_panelOpacity = v;
    emit panelOpacityChanged();
}

QString SettingsBackend::iconTheme() const { return m_iconTheme; }
void SettingsBackend::setIconTheme(const QString &v) {
    if (m_iconTheme == v) return;
    m_iconTheme = v;
    emit iconThemeChanged();
}

QString SettingsBackend::fontFamily() const { return m_fontFamily; }
void SettingsBackend::setFontFamily(const QString &v) {
    if (m_fontFamily == v) return;
    m_fontFamily = v;
    emit fontFamilyChanged();
}

int SettingsBackend::fontSize() const { return m_fontSize; }
void SettingsBackend::setFontSize(int v) {
    if (m_fontSize == v) return;
    m_fontSize = v;
    emit fontSizeChanged();
}

int SettingsBackend::screenTimeout() const { return m_screenTimeout; }
void SettingsBackend::setScreenTimeout(int v) {
    if (m_screenTimeout == v) return;
    m_screenTimeout = v;
    emit screenTimeoutChanged();
}

int SettingsBackend::suspendTimeout() const { return m_suspendTimeout; }
void SettingsBackend::setSuspendTimeout(int v) {
    if (m_suspendTimeout == v) return;
    m_suspendTimeout = v;
    emit suspendTimeoutChanged();
}

QString SettingsBackend::powerProfile() const { return m_powerProfile; }
void SettingsBackend::setPowerProfile(const QString &v) {
    if (m_powerProfile == v) return;
    m_powerProfile = v;
    emit powerProfileChanged();
}

bool SettingsBackend::autolockEnabled() const { return m_autolockEnabled; }
void SettingsBackend::setAutolockEnabled(bool v) {
    if (m_autolockEnabled == v) return;
    m_autolockEnabled = v;
    emit autolockEnabledChanged();
}

int SettingsBackend::autolockDelay() const { return m_autolockDelay; }
void SettingsBackend::setAutolockDelay(int v) {
    if (m_autolockDelay == v) return;
    m_autolockDelay = v;
    emit autolockDelayChanged();
}

bool SettingsBackend::lockOnScreenOff() const { return m_lockOnScreenOff; }
void SettingsBackend::setLockOnScreenOff(bool v) {
    if (m_lockOnScreenOff == v) return;
    m_lockOnScreenOff = v;
    emit lockOnScreenOffChanged();
}

bool SettingsBackend::lockscreenBlur() const { return m_lockscreenBlur; }
void SettingsBackend::setLockscreenBlur(bool v) {
    if (m_lockscreenBlur == v) return;
    m_lockscreenBlur = v;
    emit lockscreenBlurChanged();
}

QString SettingsBackend::lockscreenRingColor() const { return m_lockscreenRingColor; }
void SettingsBackend::setLockscreenRingColor(const QString &v) {
    if (m_lockscreenRingColor == v) return;
    m_lockscreenRingColor = v;
    emit lockscreenRingColorChanged();
}

// ── D-Bus power profile helper ─────────────────────────────────────────────
// Talks directly to power-profiles-daemon over the system bus.
// Service:    org.freedesktop.UPower.PowerProfiles
// Object:     /org/freedesktop/UPower/PowerProfiles
// Interface:  org.freedesktop.DBus.Properties
// Method:     Set("org.freedesktop.UPower.PowerProfiles", "ActiveProfile", variant)
// Profile IDs: "power-saver" | "balanced" | "performance"
bool SettingsBackend::applyPowerProfileDBus(const QString &profile)
{
    static const QString service   = QStringLiteral("org.freedesktop.UPower.PowerProfiles");
    static const QString path      = QStringLiteral("/org/freedesktop/UPower/PowerProfiles");
    static const QString propIface = QStringLiteral("org.freedesktop.DBus.Properties");
    static const QString ppIface   = QStringLiteral("org.freedesktop.UPower.PowerProfiles");

    QDBusInterface iface(service, path, propIface, QDBusConnection::systemBus());
    if (!iface.isValid()) {
        qWarning() << "[SettingsBackend] power-profiles-daemon D-Bus interface not available:"
                   << iface.lastError().message();
        return false;
    }

    // org.freedesktop.DBus.Properties.Set(interface, property, value)
    QDBusReply<void> reply = iface.call(
        QStringLiteral("Set"),
        ppIface,
        QStringLiteral("ActiveProfile"),
        QVariant::fromValue(QDBusVariant(profile))
    );

    if (!reply.isValid()) {
        qWarning() << "[SettingsBackend] D-Bus Set(ActiveProfile) failed:"
                   << reply.error().message();
        return false;
    }

    qDebug() << "[SettingsBackend] Power profile set via D-Bus:" << profile;
    return true;
}

void SettingsBackend::onDBusPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties);

    // Log what interface name the daemon actually sends so we can debug
    qDebug() << "[SettingsBackend] PropertiesChanged from interface:" << interface
             << "keys:" << changedProperties.keys();

    // Accept from any power profiles interface — covers both
    // net.hadess.PowerProfiles and org.freedesktop.UPower.PowerProfiles
    if (!changedProperties.contains(QStringLiteral("ActiveProfile"))) return;

    QString newProfile = changedProperties.value(QStringLiteral("ActiveProfile")).toString();

    // Map dbus value ("power-saver") back to UI value ("Power Saver")
    QString uiProfile = QStringLiteral("Balanced");
    if (newProfile == QStringLiteral("performance"))  uiProfile = QStringLiteral("Performance");
    else if (newProfile == QStringLiteral("power-saver")) uiProfile = QStringLiteral("Power Saver");

    qDebug() << "[SettingsBackend] External profile change detected:" << newProfile << "->" << uiProfile;

    if (m_powerProfile != uiProfile) {
        m_powerProfile = uiProfile;
        m_settings.setValue("power/profile", m_powerProfile);
        m_settings.sync();
        emit powerProfileChanged();
    }
}

