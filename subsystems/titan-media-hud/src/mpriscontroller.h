#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QProcess>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

/**
 * MprisController — MPRIS media player integration via playerctl.
 *
 * Follows the active media player and exposes real-time metadata,
 * playback state, position, duration, volume, shuffle, and loop status.
 */
class MprisController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY mediaStateChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playbackStatusChanged)
    Q_PROPERTY(QString playbackStatus READ playbackStatus NOTIFY playbackStatusChanged)
    Q_PROPERTY(QString title READ title NOTIFY metadataChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY metadataChanged)
    Q_PROPERTY(QString album READ album NOTIFY metadataChanged)
    Q_PROPERTY(QString playerName READ playerName NOTIFY metadataChanged)
    Q_PROPERTY(QString artUrl READ artUrl NOTIFY artUrlChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 length READ length NOTIFY metadataChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY positionChanged)
    Q_PROPERTY(QString positionText READ positionText NOTIFY positionChanged)
    Q_PROPERTY(QString lengthText READ lengthText NOTIFY metadataChanged)
    Q_PROPERTY(qreal volume READ volume NOTIFY volumeChanged)
    Q_PROPERTY(bool shuffle READ shuffle NOTIFY shuffleChanged)
    Q_PROPERTY(QString loopStatus READ loopStatus NOTIFY loopStatusChanged)

public:
    explicit MprisController(QObject *parent = nullptr);
    ~MprisController();

    bool hasMedia() const { return m_hasMedia; }
    bool isPlaying() const { return m_playbackStatus.compare("Playing", Qt::CaseInsensitive) == 0; }
    QString playbackStatus() const { return m_playbackStatus; }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString playerName() const { return m_playerName; }
    QString artUrl() const { return m_artUrl; }
    qint64 position() const { return m_position; }
    qint64 length() const { return m_length; }
    qreal progress() const { return m_length > 0 ? (qreal)m_position / (qreal)m_length : 0.0; }
    QString positionText() const { return formatTime(m_position); }
    QString lengthText() const { return formatTime(m_length); }
    qreal volume() const { return m_volume; }
    bool shuffle() const { return m_shuffle; }
    QString loopStatus() const { return m_loopStatus; }

public slots:
    void playPause();
    void next();
    void previous();
    void seek(qreal normalized);
    void refresh();
    void toggleShuffle();
    void cycleLoop();

signals:
    void mediaStateChanged();
    void playbackStatusChanged();
    void metadataChanged();
    void artUrlChanged();
    void positionChanged();
    void volumeChanged();
    void shuffleChanged();
    void loopStatusChanged();
    void trackChanged();  // Distinct signal for track transitions (debounced)

private slots:
    void onPlayerctlOutput();
    void onPositionTick();
    void onArtworkDownloaded(QNetworkReply *reply);
    void onDriftCorrection();

private:
    void startPlayerctlProcess();
    void parseMetadataLine(const QString &line);
    void downloadArtwork(const QUrl &url);
    void queryVolume();
    void queryShuffle();
    void queryLoop();
    static QString formatTime(qint64 microseconds);

    QProcess *m_process{nullptr};
    QTimer *m_positionTimer{nullptr};
    QTimer *m_driftTimer{nullptr};
    QNetworkAccessManager *m_netManager{nullptr};

    bool m_hasMedia{false};
    QString m_playbackStatus{"Stopped"};
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_playerName;
    QString m_artUrl;
    QString m_rawArtUrl;
    qint64 m_position{0};
    qint64 m_length{0};
    qreal m_volume{1.0};
    bool m_shuffle{false};
    QString m_loopStatus{"None"};
    QString m_lastTrackId; // For detecting actual track changes vs metadata updates
};
