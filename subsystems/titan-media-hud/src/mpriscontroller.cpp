#include "mpriscontroller.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QCryptographicHash>
#include <QUrl>

MprisController::MprisController(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_positionTimer(new QTimer(this))
    , m_driftTimer(new QTimer(this))
    , m_netManager(new QNetworkAccessManager(this))
{
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MprisController::onPlayerctlOutput);
    connect(m_positionTimer, &QTimer::timeout, this, &MprisController::onPositionTick);
    connect(m_driftTimer, &QTimer::timeout, this, &MprisController::onDriftCorrection);
    connect(m_netManager, &QNetworkAccessManager::finished, this, &MprisController::onArtworkDownloaded);

    // Position interpolation: update every 250ms for smooth progress
    m_positionTimer->setInterval(250);

    // Drift correction: sync with real D-Bus position every 5 seconds
    m_driftTimer->setInterval(5000);

    startPlayerctlProcess();
    refresh();
}

MprisController::~MprisController()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(500);
    }
}

void MprisController::startPlayerctlProcess()
{
    if (m_process->state() != QProcess::NotRunning) {
        return;
    }

    // Follow metadata events with structured delimiter
    QStringList args;
    args << "--follow"
         << "metadata"
         << "--format"
         << "{{playerName}};;{{status}};;{{title}};;{{artist}};;{{album}};;{{mpris:artUrl}};;{{position}};;{{mpris:length}}";

    // Connect restart handler only once
    static bool connectedRestart = false;
    if (!connectedRestart) {
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int, QProcess::ExitStatus) {
                    QTimer::singleShot(2000, this, &MprisController::startPlayerctlProcess);
                });
        connectedRestart = true;
    }

    m_process->start("playerctl", args);
}

void MprisController::onPlayerctlOutput()
{
    while (m_process->canReadLine()) {
        QString line = QString::fromUtf8(m_process->readLine()).trimmed();
        if (!line.isEmpty()) {
            parseMetadataLine(line);
        }
    }
}

void MprisController::parseMetadataLine(const QString &line)
{
    QStringList parts = line.split(";;");
    if (parts.size() < 8) {
        return;
    }

    QString pName   = parts[0].trimmed();
    QString status  = parts[1].trimmed();
    QString title   = parts[2].trimmed();
    QString artist  = parts[3].trimmed();
    QString album   = parts[4].trimmed();
    QString artUrl  = parts[5].trimmed();
    qint64 pos      = parts[6].trimmed().toLongLong();
    qint64 len      = parts[7].trimmed().toLongLong();

    bool hasTrack = !title.isEmpty() && (status.compare("Playing", Qt::CaseInsensitive) == 0 ||
                                         status.compare("Paused", Qt::CaseInsensitive) == 0);

    bool mediaChanged = (m_hasMedia != hasTrack);
    m_hasMedia = hasTrack;

    // Detect actual track change (title+artist combo changed)
    QString trackId = title + "||" + artist;
    bool isNewTrack = (!trackId.isEmpty() && trackId != m_lastTrackId && !title.isEmpty());

    if (m_playerName != pName || m_title != title || m_artist != artist || m_album != album || m_length != len) {
        m_playerName = pName;
        m_title = title.isEmpty() ? "Unknown Track" : title;
        m_artist = artist.isEmpty() ? "Unknown Artist" : artist;
        m_album = album;
        m_length = len;
        emit metadataChanged();
    }

    if (isNewTrack) {
        m_lastTrackId = trackId;
        emit trackChanged();
    }

    if (m_playbackStatus != status) {
        m_playbackStatus = status;
        emit playbackStatusChanged();

        if (isPlaying()) {
            if (!m_positionTimer->isActive()) m_positionTimer->start();
            if (!m_driftTimer->isActive()) m_driftTimer->start();
        } else {
            if (m_positionTimer->isActive()) m_positionTimer->stop();
            if (m_driftTimer->isActive()) m_driftTimer->stop();
        }
    }

    if (pos > 0) {
        m_position = pos;
        emit positionChanged();
    }

    if (m_rawArtUrl != artUrl) {
        m_rawArtUrl = artUrl;
        if (artUrl.startsWith("file://")) {
            m_artUrl = artUrl;
            emit artUrlChanged();
        } else if (artUrl.startsWith("http://") || artUrl.startsWith("https://")) {
            downloadArtwork(QUrl(artUrl));
        } else if (!artUrl.isEmpty() && QFile::exists(artUrl)) {
            m_artUrl = QUrl::fromLocalFile(artUrl).toString();
            emit artUrlChanged();
        } else {
            m_artUrl = "";
            emit artUrlChanged();
        }
    }

    if (mediaChanged) {
        emit mediaStateChanged();
    }

    // Query extended properties on metadata change
    queryVolume();
    queryShuffle();
    queryLoop();
}

void MprisController::downloadArtwork(const QUrl &url)
{
    QString cacheDir = "/tmp/titan_media_hud";
    QDir().mkpath(cacheDir);

    QByteArray hash = QCryptographicHash::hash(url.toString().toUtf8(), QCryptographicHash::Md5).toHex();
    QString localFile = cacheDir + "/art_" + QString::fromLatin1(hash) + ".png";

    if (QFile::exists(localFile)) {
        m_artUrl = QUrl::fromLocalFile(localFile).toString();
        emit artUrlChanged();
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = m_netManager->get(request);
    reply->setProperty("destFile", localFile);
}

void MprisController::onArtworkDownloaded(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() == QNetworkReply::NoError) {
        QString destFile = reply->property("destFile").toString();
        if (!destFile.isEmpty()) {
            QFile file(destFile);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                m_artUrl = QUrl::fromLocalFile(destFile).toString();
                emit artUrlChanged();
            }
        }
    }
}

void MprisController::onPositionTick()
{
    if (!isPlaying()) return;

    // Advance position by 250ms (250,000 microseconds)
    m_position += 250000;
    if (m_length > 0 && m_position > m_length) {
        m_position = m_length;
    }
    emit positionChanged();
}

void MprisController::onDriftCorrection()
{
    if (!isPlaying()) return;

    // Query real position from playerctl to correct drift
    QProcess queryProc;
    queryProc.start("playerctl", QStringList() << "position");
    if (queryProc.waitForFinished(500)) {
        QString out = QString::fromUtf8(queryProc.readAllStandardOutput()).trimmed();
        bool ok = false;
        double seconds = out.toDouble(&ok);
        if (ok) {
            qint64 realPos = static_cast<qint64>(seconds * 1000000.0);
            // Only correct if drift > 1 second
            if (qAbs(realPos - m_position) > 1000000) {
                m_position = realPos;
                emit positionChanged();
            }
        }
    }
}

void MprisController::playPause()
{
    QProcess::startDetached("playerctl", QStringList() << "play-pause");
    QTimer::singleShot(200, this, &MprisController::refresh);
}

void MprisController::next()
{
    QProcess::startDetached("playerctl", QStringList() << "next");
    QTimer::singleShot(200, this, &MprisController::refresh);
}

void MprisController::previous()
{
    QProcess::startDetached("playerctl", QStringList() << "previous");
    QTimer::singleShot(200, this, &MprisController::refresh);
}

void MprisController::seek(qreal normalized)
{
    if (m_length <= 0) return;
    qint64 targetMicroseconds = static_cast<qint64>(normalized * m_length);
    qreal targetSeconds = static_cast<qreal>(targetMicroseconds) / 1000000.0;

    m_position = targetMicroseconds;
    emit positionChanged();

    QProcess::startDetached("playerctl", QStringList() << "position" << QString::number(targetSeconds, 'f', 2));
}

void MprisController::toggleShuffle()
{
    QProcess::startDetached("playerctl", QStringList() << "shuffle" << "toggle");
    QTimer::singleShot(300, this, &MprisController::queryShuffle);
}

void MprisController::cycleLoop()
{
    // Cycle: None → Track → Playlist → None
    QString next;
    if (m_loopStatus == "None") next = "Track";
    else if (m_loopStatus == "Track") next = "Playlist";
    else next = "None";

    QProcess::startDetached("playerctl", QStringList() << "loop" << next);
    QTimer::singleShot(300, this, &MprisController::queryLoop);
}

void MprisController::queryVolume()
{
    QProcess proc;
    proc.start("playerctl", QStringList() << "volume");
    if (proc.waitForFinished(500)) {
        bool ok = false;
        qreal vol = QString::fromUtf8(proc.readAllStandardOutput()).trimmed().toDouble(&ok);
        if (ok && vol != m_volume) {
            m_volume = vol;
            emit volumeChanged();
        }
    }
}

void MprisController::queryShuffle()
{
    QProcess proc;
    proc.start("playerctl", QStringList() << "shuffle");
    if (proc.waitForFinished(500)) {
        QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed().toLower();
        bool shuf = (out == "on" || out == "true");
        if (shuf != m_shuffle) {
            m_shuffle = shuf;
            emit shuffleChanged();
        }
    }
}

void MprisController::queryLoop()
{
    QProcess proc;
    proc.start("playerctl", QStringList() << "loop");
    if (proc.waitForFinished(500)) {
        QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (!out.isEmpty() && out != m_loopStatus) {
            m_loopStatus = out;
            emit loopStatusChanged();
        }
    }
}

void MprisController::refresh()
{
    QProcess queryProc;
    queryProc.start("playerctl", QStringList() << "metadata" << "--format"
                    << "{{playerName}};;{{status}};;{{title}};;{{artist}};;{{album}};;{{mpris:artUrl}};;{{position}};;{{mpris:length}}");
    if (queryProc.waitForFinished(1000)) {
        QString out = QString::fromUtf8(queryProc.readAllStandardOutput()).trimmed();
        if (!out.isEmpty()) {
            parseMetadataLine(out);
        } else {
            if (m_hasMedia) {
                m_hasMedia = false;
                m_playbackStatus = "Stopped";
                emit mediaStateChanged();
                emit playbackStatusChanged();
            }
        }
    }
}

QString MprisController::formatTime(qint64 microseconds)
{
    if (microseconds <= 0) return "0:00";
    qint64 totalSeconds = microseconds / 1000000;
    qint64 hours = totalSeconds / 3600;
    qint64 minutes = (totalSeconds % 3600) / 60;
    qint64 seconds = totalSeconds % 60;

    if (hours > 0) {
        return QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    }
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}
