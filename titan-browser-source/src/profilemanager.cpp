#include "profilemanager.h"
#include "adblocker.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QWebEngineSettings>
#include <QWebEngineCookieStore>
#include <QNetworkCookie>
#include <QDateTime>
#include <QUuid>

ProfileManager &ProfileManager::instance()
{
    static ProfileManager s_instance;
    return s_instance;
}

ProfileManager::ProfileManager(QObject *parent)
    : QObject(parent)
{
    m_configDir = QDir::homePath() + QStringLiteral("/.config/titanbrowser");
    QDir().mkpath(m_configDir);
    QDir().mkpath(m_configDir + QStringLiteral("/profiles"));
}

void ProfileManager::initialize(AdBlocker *adBlocker)
{
    m_adBlocker = adBlocker;
    loadFromDisk();

    // Ensure at least one profile exists
    if (m_profiles.isEmpty()) {
        UserProfile def;
        def.id = QStringLiteral("default");
        def.name = QStringLiteral("Personal");
        def.avatarColor = QStringLiteral("#38bdf8");
        def.avatarIcon = QStringLiteral("user");
        def.email = QStringLiteral("");
        def.isDefault = true;
        def.createdAt = QDateTime::currentDateTime();
        def.lastActive = QDateTime::currentDateTime();
        m_profiles.append(def);
        m_activeProfileId = def.id;
        saveToDisk();
    }

    if (m_activeProfileId.isEmpty() || !m_profiles.isEmpty()) {
        bool found = false;
        for (const auto &p : m_profiles) {
            if (p.id == m_activeProfileId) {
                found = true;
                break;
            }
        }
        if (!found && !m_profiles.isEmpty()) {
            m_activeProfileId = m_profiles.first().id;
        }
    }
}

void ProfileManager::setAdBlocker(AdBlocker *adBlocker)
{
    m_adBlocker = adBlocker;
    for (auto *prof : m_engineProfiles) {
        if (prof && m_adBlocker) {
            prof->setUrlRequestInterceptor(m_adBlocker);
            m_adBlocker->installContentScript(prof);
        }
    }
}

QList<UserProfile> ProfileManager::profiles() const
{
    return m_profiles;
}

UserProfile ProfileManager::activeProfile() const
{
    for (const auto &p : m_profiles) {
        if (p.id == m_activeProfileId)
            return p;
    }
    if (!m_profiles.isEmpty())
        return m_profiles.first();

    UserProfile fallback;
    fallback.id = QStringLiteral("default");
    fallback.name = QStringLiteral("Personal");
    fallback.avatarColor = QStringLiteral("#38bdf8");
    fallback.avatarIcon = QStringLiteral("user");
    return fallback;
}

bool ProfileManager::setActiveProfile(const QString &id)
{
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].id == id) {
            m_profiles[i].lastActive = QDateTime::currentDateTime();
            m_activeProfileId = id;
            saveToDisk();
            emit activeProfileChanged(m_profiles[i]);
            return true;
        }
    }
    return false;
}

UserProfile ProfileManager::createProfile(const QString &name, const QString &color, const QString &icon, const QString &email)
{
    UserProfile p;
    p.id = QStringLiteral("profile_%1").arg(QDateTime::currentMSecsSinceEpoch());
    p.name = name.trimmed().isEmpty() ? QStringLiteral("New Profile") : name.trimmed();
    p.avatarColor = color.isEmpty() ? QStringLiteral("#38bdf8") : color;
    p.avatarIcon = icon.isEmpty() ? QStringLiteral("user") : icon;
    p.email = email.trimmed();
    p.isDefault = false;
    p.createdAt = QDateTime::currentDateTime();
    p.lastActive = QDateTime::currentDateTime();

    // Create profile storage directory
    QString profDir = m_configDir + QStringLiteral("/profiles/") + p.id;
    QDir().mkpath(profDir + QStringLiteral("/storage"));
    QDir().mkpath(profDir + QStringLiteral("/cache"));

    m_profiles.append(p);
    saveToDisk();
    emit profilesListChanged();
    return p;
}

bool ProfileManager::updateProfile(const QString &id, const QString &name, const QString &color, const QString &icon, const QString &email)
{
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].id == id) {
            if (!name.trimmed().isEmpty()) m_profiles[i].name = name.trimmed();
            if (!color.isEmpty()) m_profiles[i].avatarColor = color;
            if (!icon.isEmpty()) m_profiles[i].avatarIcon = icon;
            m_profiles[i].email = email.trimmed();
            saveToDisk();
            emit profilesListChanged();
            if (id == m_activeProfileId) {
                emit activeProfileChanged(m_profiles[i]);
            }
            return true;
        }
    }
    return false;
}

bool ProfileManager::deleteProfile(const QString &id)
{
    if (m_profiles.size() <= 1)
        return false; // Cannot delete the only profile

    int targetIdx = -1;
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].id == id) {
            targetIdx = i;
            break;
        }
    }
    if (targetIdx == -1) return false;

    // If deleting the active profile, switch to first available other profile
    if (id == m_activeProfileId) {
        int nextIdx = (targetIdx == 0) ? 1 : 0;
        setActiveProfile(m_profiles[nextIdx].id);
    }

    m_profiles.removeAt(targetIdx);

    if (m_engineProfiles.contains(id)) {
        auto *ep = m_engineProfiles.take(id);
        if (ep != QWebEngineProfile::defaultProfile()) {
            ep->deleteLater();
        }
    }

    // Remove profile directory
    QString profDir = m_configDir + QStringLiteral("/profiles/") + id;
    QDir(profDir).removeRecursively();

    saveToDisk();
    emit profilesListChanged();
    return true;
}

QWebEngineProfile *ProfileManager::webEngineProfile(const QString &id)
{
    if (m_engineProfiles.contains(id) && m_engineProfiles[id]) {
        return m_engineProfiles[id];
    }

    QWebEngineProfile *profile = nullptr;
    if (id == QStringLiteral("default") || id.isEmpty()) {
        // Can wrap or configure default profile
        profile = QWebEngineProfile::defaultProfile();
    } else {
        profile = new QWebEngineProfile(id, this);
    }

    setupWebEngineProfile(profile, id);
    m_engineProfiles[id] = profile;
    return profile;
}

QWebEngineProfile *ProfileManager::activeWebEngineProfile()
{
    return webEngineProfile(m_activeProfileId);
}

QString ProfileManager::downloadPath() const
{
    if (m_downloadPath.isEmpty()) {
        return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    }
    return m_downloadPath;
}

void ProfileManager::setDownloadPath(const QString &path)
{
    if (path.isEmpty()) return;
    m_downloadPath = path;
    saveToDisk();

    for (auto *ep : m_engineProfiles.values()) {
        if (ep) {
            ep->setDownloadPath(m_downloadPath);
        }
    }
    if (QWebEngineProfile::defaultProfile()) {
        QWebEngineProfile::defaultProfile()->setDownloadPath(m_downloadPath);
    }
}

void ProfileManager::setupWebEngineProfile(QWebEngineProfile *profile, const QString &profileId)
{
    if (!profile) return;

    profile->setDownloadPath(downloadPath());

    QString storagePath = m_configDir + QStringLiteral("/profiles/") + profileId + QStringLiteral("/storage");
    QString cachePath   = m_configDir + QStringLiteral("/profiles/") + profileId + QStringLiteral("/cache");
    QDir().mkpath(storagePath);
    QDir().mkpath(cachePath);

    profile->setPersistentStoragePath(storagePath);
    profile->setCachePath(cachePath);
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setHttpCacheMaximumSize(512 * 1024 * 1024); // 512 MB disk cache
    profile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
    profile->setHttpAcceptLanguage(QStringLiteral("en-US,en;q=0.9"));

    // Profile-level WebEngine Settings for smoothness and isolated security
    auto *settings = profile->settings();
    settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);
    settings->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);

    // Set default English preferences on YouTube & Google for this profile
    auto *cookieStore = profile->cookieStore();
    QNetworkCookie ytPref(QByteArrayLiteral("PREF"), QByteArrayLiteral("f6=40000000&hl=en&gl=US"));
    ytPref.setDomain(QStringLiteral(".youtube.com"));
    ytPref.setPath(QStringLiteral("/"));
    ytPref.setExpirationDate(QDateTime::currentDateTime().addYears(5));
    cookieStore->setCookie(ytPref, QUrl(QStringLiteral("https://www.youtube.com")));

    QNetworkCookie googlePref(QByteArrayLiteral("PREF"), QByteArrayLiteral("hl=en&gl=US"));
    googlePref.setDomain(QStringLiteral(".google.com"));
    googlePref.setPath(QStringLiteral("/"));
    googlePref.setExpirationDate(QDateTime::currentDateTime().addYears(5));
    cookieStore->setCookie(googlePref, QUrl(QStringLiteral("https://www.google.com")));

    if (m_adBlocker) {
        profile->setUrlRequestInterceptor(m_adBlocker);
        m_adBlocker->installContentScript(profile);
    }
}

void ProfileManager::loadFromDisk()
{
    m_profiles.clear();
    QString filePath = m_configDir + QStringLiteral("/profiles.json");
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    m_activeProfileId = root.value(QStringLiteral("activeProfileId")).toString();
    m_downloadPath = root.value(QStringLiteral("downloadPath")).toString();
    if (m_downloadPath.isEmpty()) {
        m_downloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    }

    QJsonArray array = root.value(QStringLiteral("profiles")).toArray();
    for (const auto &val : array) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();
        UserProfile p;
        p.id = obj.value(QStringLiteral("id")).toString();
        p.name = obj.value(QStringLiteral("name")).toString();
        p.avatarColor = obj.value(QStringLiteral("avatarColor")).toString(QStringLiteral("#38bdf8"));
        p.avatarIcon = obj.value(QStringLiteral("avatarIcon")).toString(QStringLiteral("user"));
        p.email = obj.value(QStringLiteral("email")).toString();
        p.isDefault = obj.value(QStringLiteral("isDefault")).toBool(false);
        p.createdAt = QDateTime::fromString(obj.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
        p.lastActive = QDateTime::fromString(obj.value(QStringLiteral("lastActive")).toString(), Qt::ISODate);
        if (!p.id.isEmpty()) {
            m_profiles.append(p);
        }
    }
}

void ProfileManager::saveToDisk()
{
    QJsonObject root;
    root.insert(QStringLiteral("activeProfileId"), m_activeProfileId);
    root.insert(QStringLiteral("downloadPath"), m_downloadPath);

    QJsonArray array;
    for (const auto &p : m_profiles) {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), p.id);
        obj.insert(QStringLiteral("name"), p.name);
        obj.insert(QStringLiteral("avatarColor"), p.avatarColor);
        obj.insert(QStringLiteral("avatarIcon"), p.avatarIcon);
        obj.insert(QStringLiteral("email"), p.email);
        obj.insert(QStringLiteral("isDefault"), p.isDefault);
        obj.insert(QStringLiteral("createdAt"), p.createdAt.toString(Qt::ISODate));
        obj.insert(QStringLiteral("lastActive"), p.lastActive.toString(Qt::ISODate));
        array.append(obj);
    }
    root.insert(QStringLiteral("profiles"), array);

    QJsonDocument doc(root);
    QString filePath = m_configDir + QStringLiteral("/profiles.json");
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}
