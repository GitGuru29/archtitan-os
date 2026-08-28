#include "audioprofilecontroller.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDebug>

AudioProfileController::AudioProfileController(QObject *parent)
    : QObject(parent)
{
    // ~/.config/archtitan/audio-profile.json
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    m_configPath = configDir + QStringLiteral("/archtitan/audio-profile.json");

    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &AudioProfileController::loadProfile);

    // Watch the file if it exists, otherwise watch the directory
    if (QFile::exists(m_configPath)) {
        m_watcher.addPath(m_configPath);
    } else {
        // Watch parent directory so we detect file creation
        QString dir = QFileInfo(m_configPath).absolutePath();
        QDir().mkpath(dir);
        m_watcher.addPath(dir);
    }

    loadProfile();
}

void AudioProfileController::loadProfile()
{
    QFile file(m_configPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        if (!m_profileName.isEmpty()) {
            m_profileName.clear();
            emit profileChanged();
        }
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString name = obj.value("profile").toString();

    if (name != m_profileName) {
        m_profileName = name;
        emit profileChanged();
    }

    // Re-add path to watcher (QFileSystemWatcher drops it after file change on some systems)
    if (!m_watcher.files().contains(m_configPath)) {
        m_watcher.addPath(m_configPath);
    }
}
