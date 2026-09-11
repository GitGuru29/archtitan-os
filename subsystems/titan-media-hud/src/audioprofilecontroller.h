#pragma once

#include <QObject>
#include <QString>
#include <QFileSystemWatcher>

/**
 * AudioProfileController — Reads the active ArchTitan audio profile.
 *
 * Watches ~/.config/archtitan/audio-profile.json for changes.
 * When Titan Settings writes a new profile, this picks it up.
 */
class AudioProfileController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString profileName READ profileName NOTIFY profileChanged)
    Q_PROPERTY(bool hasProfile READ hasProfile NOTIFY profileChanged)

public:
    explicit AudioProfileController(QObject *parent = nullptr);

    QString profileName() const { return m_profileName; }
    bool hasProfile() const { return !m_profileName.isEmpty(); }

signals:
    void profileChanged();

private slots:
    void loadProfile();

private:
    QString m_configPath;
    QString m_profileName;
    QFileSystemWatcher m_watcher;
};
