#pragma once
#include <QObject>

class DisplayManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(int nightLightTemp READ nightLightTemp WRITE setNightLightTemp NOTIFY nightLightTempChanged)
    Q_PROPERTY(bool nightLightEnabled READ nightLightEnabled WRITE setNightLightEnabled NOTIFY nightLightEnabledChanged)
    Q_PROPERTY(QString resolution READ resolution NOTIFY resolutionChanged)
    Q_PROPERTY(double refreshRate READ refreshRate NOTIFY refreshRateChanged)
    Q_PROPERTY(double scaleFactor READ scaleFactor WRITE setScaleFactor NOTIFY scaleFactorChanged)

public:
    explicit DisplayManager(QObject *parent = nullptr);

    int brightness() const;
    void setBrightness(int v);
    int nightLightTemp() const;
    void setNightLightTemp(int v);
    bool nightLightEnabled() const;
    void setNightLightEnabled(bool v);
    QString resolution() const;
    double refreshRate() const;
    double scaleFactor() const;
    void setScaleFactor(double v);

signals:
    void brightnessChanged();
    void nightLightTempChanged();
    void nightLightEnabledChanged();
    void resolutionChanged();
    void refreshRateChanged();
    void scaleFactorChanged();

private:
    int m_brightness = 100;
    int m_nightLightTemp = 4500;
    bool m_nightLightEnabled = false;
    QString m_resolution;
    double m_refreshRate = 60.0;
    double m_scaleFactor = 1.0;
    bool m_initialized = false;  // guards against startup-binding side-effects

    QString findBacklightPath() const;
    int maxBrightness() const;
};
