#pragma once
#include <QObject>
#include <QTimer>
#include <QProcess>

class AudioBackend : public QObject {
    Q_OBJECT

    Q_PROPERTY(int masterVolume READ masterVolume WRITE setMasterVolume NOTIFY masterVolumeChanged)
    Q_PROPERTY(bool masterMuted READ masterMuted WRITE setMasterMuted NOTIFY masterMutedChanged)
    Q_PROPERTY(int micVolume READ micVolume WRITE setMicVolume NOTIFY micVolumeChanged)
    Q_PROPERTY(bool micMuted READ micMuted WRITE setMicMuted NOTIFY micMutedChanged)
    Q_PROPERTY(QString activeOutput READ activeOutput NOTIFY activeOutputChanged)
    Q_PROPERTY(QVariantList eqLevels READ eqLevels NOTIFY eqLevelsChanged)
    Q_PROPERTY(QString activeEqProfile READ activeEqProfile WRITE setActiveEqProfile NOTIFY activeEqProfileChanged)
    Q_PROPERTY(QVariantList customGains READ customGains WRITE setCustomGains NOTIFY customGainsChanged)
    Q_PROPERTY(bool spatialAudio READ spatialAudio WRITE setSpatialAudio NOTIFY spatialAudioChanged)
    Q_PROPERTY(int spatialWidth READ spatialWidth WRITE setSpatialWidth NOTIFY spatialWidthChanged)

public:
    explicit AudioBackend(QObject *parent = nullptr);
    int masterVolume() const;
    void setMasterVolume(int v);
    bool masterMuted() const;
    void setMasterMuted(bool v);
    int micVolume() const;
    void setMicVolume(int v);
    bool micMuted() const;
    void setMicMuted(bool v);
    QString activeOutput() const;
    QVariantList eqLevels() const;
    QString activeEqProfile() const;
    void setActiveEqProfile(const QString &profile);
    Q_INVOKABLE void openMixer();

    QVariantList customGains() const;
    void setCustomGains(const QVariantList &v);
    Q_INVOKABLE void setCustomBandGain(int index, double gain);
    Q_INVOKABLE void resetCustomGains();

    bool spatialAudio() const;
    void setSpatialAudio(bool enabled);
    int spatialWidth() const;
    void setSpatialWidth(int width);

signals:
    void masterVolumeChanged();
    void masterMutedChanged();
    void micVolumeChanged();
    void micMutedChanged();
    void activeOutputChanged();
    void eqLevelsChanged();
    void activeEqProfileChanged();
    void customGainsChanged();
    void spatialAudioChanged();
    void spatialWidthChanged();

private slots:
    void sync();

private:
    void installEqPresets();
    void applyEqProfile(const QString &profile);
    void applySpatialAudio();

    QProcess m_monitorProcess;
    QProcess m_cavaProcess;
    QTimer m_debounceTimer;
    int m_masterVolume = 70;
    bool m_masterMuted = false;
    int m_micVolume = 80;
    bool m_micMuted = false;
    QString m_activeOutput;
    QVariantList m_eqLevels;
    QString m_activeEqProfile = "Flat";
    QVariantList m_customGains;
    bool m_spatialAudio = false;
    int m_spatialWidth = 80;
};
