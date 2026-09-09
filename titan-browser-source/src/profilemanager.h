#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QMap>
#include <QWebEngineProfile>

class AdBlocker;

struct UserProfile
{
    QString   id;
    QString   name;
    QString   avatarColor; // e.g. "#38bdf8", "#22c55e", "#ef4444", "#a78bfa", "#f59e0b"
    QString   avatarIcon;  // e.g. "user", "briefcase", "code", "gamepad", "globe"
    QString   email;       // e.g. "user@gmail.com" (optional)
    bool      isDefault = false;
    QDateTime createdAt;
    QDateTime lastActive;
};

class ProfileManager : public QObject
{
    Q_OBJECT
public:
    static ProfileManager &instance();

    void initialize(AdBlocker *adBlocker = nullptr);

    QList<UserProfile> profiles() const;
    UserProfile activeProfile() const;
    QString activeProfileId() const { return m_activeProfileId; }

    bool setActiveProfile(const QString &id);
    UserProfile createProfile(const QString &name, const QString &color = QString(), const QString &icon = QString(), const QString &email = QString());
    bool updateProfile(const QString &id, const QString &name, const QString &color, const QString &icon, const QString &email);
    bool deleteProfile(const QString &id);

    QWebEngineProfile *webEngineProfile(const QString &id);
    QWebEngineProfile *activeWebEngineProfile();

    QString downloadPath() const;
    void setDownloadPath(const QString &path);

    void setAdBlocker(AdBlocker *adBlocker);

signals:
    void activeProfileChanged(const UserProfile &profile);
    void profilesListChanged();

private:
    explicit ProfileManager(QObject *parent = nullptr);
    ~ProfileManager() override = default;
    ProfileManager(const ProfileManager &) = delete;
    ProfileManager &operator=(const ProfileManager &) = delete;

    void loadFromDisk();
    void saveToDisk();
    void setupWebEngineProfile(QWebEngineProfile *profile, const QString &profileId);

    QString                             m_activeProfileId;
    QList<UserProfile>                  m_profiles;
    QMap<QString, QWebEngineProfile *>  m_engineProfiles;
    AdBlocker                          *m_adBlocker = nullptr;
    QString                             m_configDir;
    QString                             m_downloadPath;
};
