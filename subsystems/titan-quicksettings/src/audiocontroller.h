#pragma once

#include <QObject>
#include <QTimer>

class AudioController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool sinkMuted READ sinkMuted NOTIFY sinkMutedChanged)
    Q_PROPERTY(int sinkVolume READ sinkVolume NOTIFY sinkVolumeChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool sourceMuted READ sourceMuted NOTIFY sourceMutedChanged)

public:
    explicit AudioController(QObject *parent = nullptr);

    bool sinkMuted() const { return m_sinkMuted; }
    int sinkVolume() const { return m_sinkVolume; }
    QString statusText() const { return m_statusText; }
    bool sourceMuted() const { return m_sourceMuted; }

public slots:
    void toggleSinkMute();
    void toggleSourceMute();
    void setVolume(int vol);
    void refresh();

signals:
    void sinkMutedChanged();
    void sinkVolumeChanged();
    void statusTextChanged();
    void sourceMutedChanged();

private:
    void queryStatus();

    bool m_sinkMuted{true};
    int m_sinkVolume{60};
    QString m_statusText{"Muted"};
    bool m_sourceMuted{true};

    QTimer *m_timer;
};
