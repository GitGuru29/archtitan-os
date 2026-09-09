#include "wallpapermanager.h"
#include <QProcess>
#include <QDir>
#include <QStandardPaths>
#include <QImage>
#include <QColor>
#include <QFile>

WallpaperManager::WallpaperManager(QObject *parent) : QObject(parent) {
    // Try to read current wallpaper from swww or config file
    QProcess p;
    p.start("bash", {"-c", "cat ~/.config/swww/current 2>/dev/null || echo ''"});
    p.waitForFinished(500);
    m_currentWallpaper = p.readAllStandardOutput().trimmed();
    calculateBrightness();
    scanWallpapers();
}

QString WallpaperManager::currentWallpaper() const { return m_currentWallpaper; }
void WallpaperManager::setCurrentWallpaper(const QString &path) {
    if (m_currentWallpaper == path) return;
    m_currentWallpaper = path;
    calculateBrightness();
    // Apply with swww
    QProcess::startDetached("bash", {"-c",
        QString("swww img '%1' --transition-type wipe --transition-duration 1 2>/dev/null; "
                "mkdir -p ~/.config/swww && echo '%1' > ~/.config/swww/current").arg(path)});
    emit currentWallpaperChanged();
    emit isDarkChanged();
}

bool WallpaperManager::isDark() const { return m_isDark; }

void WallpaperManager::calculateBrightness() {
    if (m_currentWallpaper.isEmpty() || !QFile::exists(m_currentWallpaper)) {
        m_isDark = true;
        return;
    }
    QImage img(m_currentWallpaper);
    if (img.isNull()) {
        m_isDark = true;
        return;
    }
    QImage scaled = img.scaled(1, 1, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QColor avgColor = scaled.pixelColor(0, 0);
    double luminance = 0.299 * avgColor.red() + 0.587 * avgColor.green() + 0.114 * avgColor.blue();
    m_isDark = luminance < 128.0;
}

QStringList WallpaperManager::wallpaperList() const { return m_wallpaperList; }

void WallpaperManager::scanWallpapers() {
    m_wallpaperList.clear();
    QStringList searchDirs = {
        "/usr/share/backgrounds/archtitan",
        "/usr/share/wallpapers",
        QDir::homePath() + "/Pictures/Wallpapers",
        QDir::homePath() + "/Pictures"
    };

    QStringList exts = {"*.jpg","*.jpeg","*.png","*.webp"};
    for (const QString &dir : searchDirs) {
        QDir d(dir);
        if (!d.exists()) continue;
        for (const QString &f : d.entryList(exts, QDir::Files))
            m_wallpaperList.append(d.absoluteFilePath(f));
    }
    emit wallpaperListChanged();
}

void WallpaperManager::openFilePicker() {
    // This must be called from a non-GUI thread via QFileDialog signal
    // Since QML can invoke this, we use QProcess to open a portal
    QProcess p;
    p.start("bash", {"-c", "zenity --file-selection --file-filter='Images | *.png *.jpg *.jpeg *.webp' 2>/dev/null"});
    p.waitForFinished(30000);
    QString chosen = p.readAllStandardOutput().trimmed();
    if (!chosen.isEmpty()) setCurrentWallpaper(chosen);
}
