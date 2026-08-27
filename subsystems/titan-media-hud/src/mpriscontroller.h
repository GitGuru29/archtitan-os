#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QProcess>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

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

public slots:
    void playPause();
    void next();
    void previous();
    void seek(qreal normalized);
    void refresh();

signals:
    void mediaStateChanged();
    void playbackStatusChanged();
    void metadataChanged();
    void artUrlChanged();
    void positionChanged();

private slots:
    void onPlayerctlOutput();
    void onPositionTick();
    void onArtworkDownloaded(QNetworkReply *reply);

private:
    void startPlayerctlProcess();
    void parseMetadataLine(const QString &line);
    void queryPositionDirect();
    void downloadArtwork(const QUrl &url);
    static QString formatTime(qint64 microseconds);

    QProcess *m_process{nullptr};
    QTimer *m_positionTimer{nullptr};
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
};
