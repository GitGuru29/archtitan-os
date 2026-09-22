#include "audiocontroller.h"
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

AudioController::AudioController(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    queryStatus();
    connect(m_timer, &QTimer::timeout, this, &AudioController::queryStatus);
    m_timer->start(3000);
}

void AudioController::queryStatus()
{
    // Query default sink volume and mute
    QProcess sinkProc;
    sinkProc.start("bash", {"-c", "wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null"});
    if (sinkProc.waitForFinished(800)) {
        QString out = sinkProc.readAllStandardOutput().trimmed();
        // Example output: "Volume: 0.65 [MUTED]" or "Volume: 0.65"
        bool isMuted = out.contains("[MUTED]");
        if (isMuted != m_sinkMuted) {
            m_sinkMuted = isMuted;
            emit sinkMutedChanged();
        }

        static QRegularExpression re("Volume:\\s+([0-9.]+)");
        auto match = re.match(out);
        if (match.hasMatch()) {
            double volFrac = match.captured(1).toDouble();
            int vol = static_cast<int>(volFrac * 100.0 + 0.5);
            if (vol != m_sinkVolume) {
                m_sinkVolume = vol;
                emit sinkVolumeChanged();
            }
        }

        QString text = m_sinkMuted ? "Muted" : QString("%1%").arg(m_sinkVolume);
        if (text != m_statusText) {
            m_statusText = text;
            emit statusTextChanged();
        }
    }

    // Query default source (microphone)
    QProcess srcProc;
    srcProc.start("bash", {"-c", "wpctl get-volume @DEFAULT_AUDIO_SOURCE@ 2>/dev/null"});
    if (srcProc.waitForFinished(800)) {
        QString out = srcProc.readAllStandardOutput().trimmed();
        bool isMuted = out.contains("[MUTED]");
        if (isMuted != m_sourceMuted) {
            m_sourceMuted = isMuted;
            emit sourceMutedChanged();
        }
    }
}

void AudioController::toggleSinkMute()
{
    m_sinkMuted = !m_sinkMuted;
    emit sinkMutedChanged();
    m_statusText = m_sinkMuted ? "Muted" : QString("%1%").arg(m_sinkVolume);
    emit statusTextChanged();
    QProcess::startDetached("wpctl", {"set-mute", "@DEFAULT_AUDIO_SINK@", "toggle"});
}

void AudioController::toggleSourceMute()
{
    m_sourceMuted = !m_sourceMuted;
    emit sourceMutedChanged();
    QProcess::startDetached("wpctl", {"set-mute", "@DEFAULT_AUDIO_SOURCE@", "toggle"});
}

void AudioController::setVolume(int vol)
{
    m_sinkVolume = qBound(0, vol, 150);
    emit sinkVolumeChanged();
    if (m_sinkMuted) {
        m_sinkMuted = false;
        emit sinkMutedChanged();
    }
    m_statusText = QString("%1%").arg(m_sinkVolume);
    emit statusTextChanged();
    QProcess::startDetached("wpctl", {"set-volume", "@DEFAULT_AUDIO_SINK@", QString("%1%").arg(m_sinkVolume)});
}

void AudioController::refresh()
{
    queryStatus();
}
