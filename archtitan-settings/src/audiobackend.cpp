#include "audiobackend.h"
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QVariant>
#include <QDir>
#include <QSettings>

AudioBackend::AudioBackend(QObject *parent) : QObject(parent) {
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(50); // 50ms debounce
    connect(&m_debounceTimer, &QTimer::timeout, this, &AudioBackend::sync);

    // Load custom gains from QSettings
    QSettings settings("ArchTitan", "archtitan-settings");
    m_customGains = settings.value("audio/customGains").toList();
    if (m_customGains.size() != 10) {
        m_customGains.clear();
        for (int i = 0; i < 10; ++i) {
            m_customGains.append(0.0);
        }
    }

    // Load spatial audio settings from QSettings
    m_spatialAudio = settings.value("audio/spatialAudio", false).toBool();
    m_spatialWidth = settings.value("audio/spatialWidth", 80).toInt();

    installEqPresets();

    connect(&m_monitorProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        m_monitorProcess.readAllStandardOutput(); // Clear buffer
        m_debounceTimer.start(); // Restart debounce timer
    });
    
    // Subscribe to PulseAudio/PipeWire events so we don't have to poll
    m_monitorProcess.start("pactl", {"subscribe"});

    // Start/Restart cava for real-time equalizer
    auto startCava = [this]() {
        m_cavaProcess.kill();
        m_cavaProcess.waitForFinished(500);

        QFile cavaConf("/tmp/archtitan-cava.conf");
        if (cavaConf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream ts(&cavaConf);
            ts << "[general]\nbars = 24\nframerate = 60\n"
               << "[output]\nmethod = raw\nraw_target = /dev/stdout\ndata_format = ascii\nascii_max_range = 100\n";
            cavaConf.close();
        }
        m_cavaProcess.start("cava", {"-p", "/tmp/archtitan-cava.conf"});
    };

    connect(&m_cavaProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        while (m_cavaProcess.canReadLine()) {
            QString line = QString::fromUtf8(m_cavaProcess.readLine()).trimmed();
            if (line.isEmpty()) continue;
            QStringList parts = line.split(';', Qt::SkipEmptyParts);
            QVariantList levels;
            for (const QString &p : parts) {
                levels.append(p.toInt());
            }
            if (levels.size() == 24) {
                m_eqLevels = levels;
                emit eqLevelsChanged();
            }
        }
    });

    // Auto-restart Cava if it exits (e.g., when PipeWire service is restarted)
    connect(&m_cavaProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, startCava]() {
        QTimer::singleShot(1000, this, startCava);
    });

    startCava();


    // Initialize 24 empty bars
    for (int i=0; i<24; i++) m_eqLevels.append(0);

    sync();
}

void AudioBackend::sync() {
    // Master volume
    QProcess p;
    p.start("bash", {"-c", "wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null"});
    p.waitForFinished(1000);
    QString out = p.readAllStandardOutput().trimmed();
    QStringList parts = out.split(' ');
    if (parts.size() >= 2) {
        int vol = (int)(parts[1].toDouble() * 100);
        if (m_masterVolume != vol) { m_masterVolume = vol; emit masterVolumeChanged(); }
    }
    bool muted = out.contains("[MUTED]");
    if (m_masterMuted != muted) { m_masterMuted = muted; emit masterMutedChanged(); }

    // Microphone volume
    QProcess p2;
    p2.start("bash", {"-c", "wpctl get-volume @DEFAULT_AUDIO_SOURCE@ 2>/dev/null"});
    p2.waitForFinished(1000);
    QString out2 = p2.readAllStandardOutput().trimmed();
    QStringList parts2 = out2.split(' ');
    if (parts2.size() >= 2) {
        int mvol = (int)(parts2[1].toDouble() * 100);
        if (m_micVolume != mvol) { m_micVolume = mvol; emit micVolumeChanged(); }
    }
    bool mmuted = out2.contains("[MUTED]");
    if (m_micMuted != mmuted) { m_micMuted = mmuted; emit micMutedChanged(); }

    // Active sink name
    QProcess sink;
    sink.start("bash", {"-c", "wpctl inspect @DEFAULT_AUDIO_SINK@ 2>/dev/null | grep 'node.nick\\|node.name' | head -1 | awk -F'\"' '{print $2}'"});
    sink.waitForFinished(1000);
    QString act = sink.readAllStandardOutput().trimmed();
    if (act.isEmpty()) act = "Default Output";
    if (m_activeOutput != act) { m_activeOutput = act; emit activeOutputChanged(); }
}

int AudioBackend::masterVolume() const { return m_masterVolume; }
void AudioBackend::setMasterVolume(int v) {
    v = qBound(0, v, 100);
    if (m_masterVolume == v) return;
    m_masterVolume = v;
    QProcess::startDetached("wpctl", {"set-volume", "@DEFAULT_AUDIO_SINK@", QString("%1%").arg(v)});
    emit masterVolumeChanged();
}

bool AudioBackend::masterMuted() const { return m_masterMuted; }
void AudioBackend::setMasterMuted(bool v) {
    if (m_masterMuted == v) return;
    m_masterMuted = v;
    QProcess::startDetached("wpctl", {"set-mute", "@DEFAULT_AUDIO_SINK@", v ? "1" : "0"});
    emit masterMutedChanged();
}

int AudioBackend::micVolume() const { return m_micVolume; }
void AudioBackend::setMicVolume(int v) {
    v = qBound(0, v, 100);
    if (m_micVolume == v) return;
    m_micVolume = v;
    QProcess::startDetached("wpctl", {"set-volume", "@DEFAULT_AUDIO_SOURCE@", QString("%1%").arg(v)});
    emit micVolumeChanged();
}

bool AudioBackend::micMuted() const { return m_micMuted; }
void AudioBackend::setMicMuted(bool v) {
    if (m_micMuted == v) return;
    m_micMuted = v;
    QProcess::startDetached("wpctl", {"set-mute", "@DEFAULT_AUDIO_SOURCE@", v ? "1" : "0"});
    emit micMutedChanged();
}

QString AudioBackend::activeOutput() const { return m_activeOutput; }

QVariantList AudioBackend::eqLevels() const { return m_eqLevels; }

QString AudioBackend::activeEqProfile() const { return m_activeEqProfile; }

void AudioBackend::setActiveEqProfile(const QString &profile) {
    if (m_activeEqProfile == profile) return;
    m_activeEqProfile = profile;
    emit activeEqProfileChanged();
    applyEqProfile(profile);
}

void AudioBackend::applyEqProfile(const QString &profile) {
    // Band definitions: { frequency, gain_dB }
    struct Band { double freq; double gain; };

    QList<Band> bands;

    if (m_spatialAudio) {
        // Paused: force Flat EQ when Spatial Audio is active to avoid interference
        bands = { {32,0},{64,0},{125,0},{250,0},{500,0},{1000,0},{2000,0},{4000,0},{8000,0},{16000,0} };
    } else if (profile == "Flat") {
        bands = { {32,0},{64,0},{125,0},{250,0},{500,0},{1000,0},{2000,0},{4000,0},{8000,0},{16000,0} };

    } else if (profile == "Bass Boost") {
        bands = { {32,6},{64,5},{125,3},{250,1},{500,0},{1000,0},{2000,0},{4000,0},{8000,0},{16000,0} };
    } else if (profile == "Vocal") {
        bands = { {32,-2},{64,-2},{125,0},{250,2},{500,4},{1000,4},{2000,4},{4000,2},{8000,0},{16000,-2} };
    } else if (profile == "Electronic") {
        bands = { {32,4},{64,4},{125,2},{250,0},{500,-2},{1000,-2},{2000,0},{4000,2},{8000,4},{16000,4} };
    } else if (profile == "Acoustic") {
        bands = { {32,0},{64,2},{125,2},{250,0},{500,0},{1000,0},{2000,2},{4000,2},{8000,2},{16000,0} };
    } else if (profile == "Custom") {
        bands = {
            {32, m_customGains[0].toDouble()},
            {64, m_customGains[1].toDouble()},
            {125, m_customGains[2].toDouble()},
            {250, m_customGains[3].toDouble()},
            {500, m_customGains[4].toDouble()},
            {1000, m_customGains[5].toDouble()},
            {2000, m_customGains[6].toDouble()},
            {4000, m_customGains[7].toDouble()},
            {8000, m_customGains[8].toDouble()},
            {16000, m_customGains[9].toDouble()}
        };
    } else {
        return;
    }

    // Build PipeWire filter-chain SPA-JSON config
    QString config;
    QTextStream ts(&config);

    ts << "# ArchTitan EQ — generated by ArchTitan Settings\n";
    ts << "# Profile: " << profile << "\n\n";
    ts << "context.modules = [\n";
    ts << "    { name = libpipewire-module-filter-chain\n";
    ts << "        args = {\n";
    ts << "            node.description = \"ArchTitan Equalizer\"\n";
    ts << "            media.name       = \"ArchTitan Equalizer\"\n";
    ts << "            filter.graph = {\n";
    ts << "                nodes = [\n";

    for (int i = 0; i < bands.size(); ++i) {
        const auto &b = bands[i];
        QString label = (i == 0) ? "bq_lowshelf" : (i == bands.size()-1) ? "bq_highshelf" : "bq_peaking";
        ts << "                    {\n";
        ts << "                        type  = builtin\n";
        ts << QString("                        name  = eq_band_%1\n").arg(i + 1);
        ts << "                        label = " << label << "\n";
        ts << QString("                        control = { \"Freq\" = %1 \"Q\" = 0.707 \"Gain\" = %2 }\n")
              .arg(b.freq, 0, 'f', 1)
              .arg(b.gain, 0, 'f', 1);
        ts << "                    }\n";
    }

    ts << "                ]\n";
    ts << "                links = [\n";
    for (int i = 0; i < bands.size() - 1; ++i) {
        ts << QString("                    { output = \"eq_band_%1:Out\" input = \"eq_band_%2:In\" }\n")
              .arg(i + 1).arg(i + 2);
    }
    ts << "                ]\n";
    ts << "            }\n";
    ts << "            audio.channels = 2\n";
    ts << "            audio.position = [ FL FR ]\n";
    ts << "            capture.props = {\n";
    ts << "                node.name   = \"effect_input.archtitan_eq\"\n";
    ts << "                media.class = Audio/Sink\n";
    ts << "            }\n";
    ts << "            playback.props = {\n";
    ts << "                node.name   = \"effect_output.archtitan_eq\"\n";
    ts << "                node.passive = true\n";
    ts << "            }\n";
    ts << "        }\n";
    ts << "    }\n";
    ts << "]\n";

    // Write config (for persistence on reboot)
    QString confDir = QDir::homePath() + "/.config/pipewire/filter-chain.conf.d";
    QDir().mkpath(confDir);
    QFile file(confDir + "/archtitan-eq.conf");
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(config.toUtf8());
        file.close();
    }

    // ── Live gain update via pw-cli set-param ─────────────────────────────
    // Replaces the slow pw-dump approach (which froze audio for 1-3s) with a
    // fast pactl-based lookup: pactl list sinks only reads audio sinks, not the
    // full PipeWire graph, so the ID lookup takes < 100ms.
    QString paramsList;
    for (int i = 0; i < bands.size(); ++i) {
        paramsList += QString(" \"eq_band_%1:Gain\" %2").arg(i + 1).arg(bands[i].gain, 0, 'f', 1);
    }

    // Which sink should active streams ultimately play through?
    // If spatial audio is ON, keep them on the spatial sink (which feeds into
    // the EQ node downstream). If OFF, route directly to the EQ sink.
    QString targetSink = m_spatialAudio
        ? "effect_input.archtitan_spatial"
        : "effect_input.archtitan_eq";

    QString liveScript = QString(
        // Fast node-ID lookup via pactl (reads only sink properties, not full graph)
        "ID=$(pactl list sinks 2>/dev/null | "
        "awk '/archtitan_eq/{f=1} f && /object\\.id/{gsub(/[^0-9]/,\"\"); print; exit}'); "
        "if [ -n \"$ID\" ]; then "
        // Apply new gains atomically — no audio gap
        "  pw-cli set-param \"$ID\" Props '{ params: [%1 ] }' 2>/dev/null; "
        "  pactl list short sink-inputs 2>/dev/null | awk '{print $1}' | "
        "  xargs -I{} pactl move-sink-input {} %2 2>/dev/null; "
        "else "
        // EQ sink not loaded yet — start filter-chain and route
        "  systemctl --user start filter-chain 2>/dev/null; "
        "  sleep 0.8; "
        "  pactl list short sink-inputs 2>/dev/null | awk '{print $1}' | "
        "  xargs -I{} pactl move-sink-input {} %2 2>/dev/null; "
        "fi; "
        "true"
    ).arg(paramsList, targetSink);

    QProcess::startDetached("bash", {"-c", liveScript});

}

void AudioBackend::openMixer() {
    QProcess::startDetached("bash", {"-c", "pavucontrol &"});
}

void AudioBackend::installEqPresets() {
    // Try to load the previously active profile from the written config file
    QString confPath = QDir::homePath() + "/.config/pipewire/filter-chain.conf.d/archtitan-eq.conf";
    QFile file(confPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("# Profile: ")) {
                QString profile = line.mid(11).trimmed();
                if (profile == "Flat" || profile == "Bass Boost" || profile == "Vocal" || profile == "Electronic" || profile == "Acoustic" || profile == "Custom") {
                    m_activeEqProfile = profile;
                }
                break;
            }
        }
        file.close();
    }

    // Apply the profile on startup to ensure filter-chain is loaded
    applyEqProfile(m_activeEqProfile);
}

QVariantList AudioBackend::customGains() const {
    return m_customGains;
}

void AudioBackend::setCustomGains(const QVariantList &v) {
    if (m_customGains == v) return;
    m_customGains = v;
    emit customGainsChanged();
    if (m_activeEqProfile == "Custom") {
        applyEqProfile("Custom");
    }
}

void AudioBackend::setCustomBandGain(int index, double gain) {
    if (index < 0 || index >= m_customGains.size()) return;
    if (qFuzzyCompare(m_customGains[index].toDouble(), gain)) return;
    
    m_customGains[index] = gain;
    emit customGainsChanged();
    
    // Save to settings
    QSettings settings("ArchTitan", "archtitan-settings");
    settings.setValue("audio/customGains", m_customGains);
    settings.sync();

    // If custom is selected, apply it live
    if (m_activeEqProfile == "Custom") {
        applyEqProfile("Custom");
    }
}

void AudioBackend::resetCustomGains() {
    bool changed = false;
    for (int i = 0; i < m_customGains.size(); ++i) {
        if (!qFuzzyCompare(m_customGains[i].toDouble(), 0.0)) {
            m_customGains[i] = 0.0;
            changed = true;
        }
    }
    
    if (changed) {
        emit customGainsChanged();
        
        QSettings settings("ArchTitan", "archtitan-settings");
        settings.setValue("audio/customGains", m_customGains);
        settings.sync();

        if (m_activeEqProfile == "Custom") {
            applyEqProfile("Custom");
        }
    }
}

// ─── Spatial Audio ────────────────────────────────────────────────────────────

bool AudioBackend::spatialAudio() const { return m_spatialAudio; }
int  AudioBackend::spatialWidth()  const { return m_spatialWidth;  }

void AudioBackend::setSpatialAudio(bool enabled) {
    if (m_spatialAudio == enabled) return;
    m_spatialAudio = enabled;
    emit spatialAudioChanged();

    QSettings settings("ArchTitan", "archtitan-settings");
    settings.setValue("audio/spatialAudio", enabled);
    settings.sync();

    applyEqProfile(m_activeEqProfile);
    applySpatialAudio();
}

void AudioBackend::setSpatialWidth(int width) {
    width = qBound(0, width, 100);
    if (m_spatialWidth == width) return;
    m_spatialWidth = width;
    emit spatialWidthChanged();

    QSettings settings("ArchTitan", "archtitan-settings");
    settings.setValue("audio/spatialWidth", width);
    settings.sync();

    if (m_spatialAudio) applySpatialAudio();
}

void AudioBackend::applySpatialAudio() {
    const QString confDir  = QDir::homePath() + "/.config/pipewire/filter-chain.conf.d";
    const QString confPath = confDir + "/archtitan-spatial.conf";

    // ── Shared auto-resume snippet ──────────────────────────────────────────
    // After PipeWire restarts, media players that paused are resumed via MPRIS.
    const QString resumeScript =
        "sleep 0.1; "
        "for p in $(playerctl -l 2>/dev/null); do "
        "  playerctl -p \"$p\" play 2>/dev/null; "
        "done; ";

    if (!m_spatialAudio) {
        // Remove conf so the spatial sink is gone on next (and this) PipeWire start.
        QFile::remove(confPath);
        QProcess::startDetached("bash", {"-c",
            "systemctl --user restart pipewire pipewire-pulse 2>/dev/null; "
            "sleep 0.9; "
            // Route back to EQ sink
            "pactl list short sink-inputs 2>/dev/null | awk '{print $1}' | "
            "xargs -I{} pactl move-sink-input {} effect_input.archtitan_eq 2>/dev/null; "
            + resumeScript +
            "true"});
        return;
    }

    // ── Compute gains ───────────────────────────────────────────────────────
    // Width 0–100 → crossGain 0.0–0.65  (strong, clearly audible Haas cross-feed)
    // At Width=80 (default): crossGain ≈ 0.52, directGain ≈ 0.79
    double crossGain  = (m_spatialWidth / 100.0) * 0.65;
    double directGain = 1.0 - crossGain * 0.32;
    // 15 ms Haas delay (0.015 seconds) — clearly noticeable spatial cue
    const double delaySec = 0.015;

    // ── Write filter-chain conf ─────────────────────────────────────────────
    {
        QString conf;
        QTextStream tc(&conf);
        tc << "# ArchTitan Spatial Audio — generated by ArchTitan Settings\n";
        tc << "# Width: " << m_spatialWidth << "\n\n";
        tc << "context.modules = [\n";
        tc << "    { name = libpipewire-module-filter-chain\n";
        tc << "        flags = [ nofail ]\n";
        tc << "        args = {\n";
        tc << "            node.description = \"ArchTitan Spatial\"\n";
        tc << "            media.name       = \"ArchTitan Spatial\"\n";
        tc << "            filter.graph = {\n";
        tc << "                nodes = [\n";
        // Pass-through copies so we can fan-out L to both direct and delay paths
        tc << "                    { type = builtin label = copy  name = copyL }\n";
        tc << "                    { type = builtin label = copy  name = copyR }\n";
        // Haas delay: cross-channel — R delayed injected into L output and vice versa
        tc << "                    { type = builtin label = delay name = delayR\n"
              "                        config = { \"max-delay\" = 1.0 }\n"
              "                        control = { \"Delay (s)\" = " << delaySec << " } }\n";
        tc << "                    { type = builtin label = delay name = delayL\n"
              "                        config = { \"max-delay\" = 1.0 }\n"
              "                        control = { \"Delay (s)\" = " << delaySec << " } }\n";
        // Mixers: Out_L = direct_L + cross_R_delayed,  Out_R = direct_R + cross_L_delayed
        tc << QString("                    { type = builtin label = mixer name = mixL\n"
                      "                        control = { \"Gain 1\" = %1 \"Gain 2\" = %2 } }\n")
              .arg(directGain, 0, 'f', 4).arg(crossGain, 0, 'f', 4);
        tc << QString("                    { type = builtin label = mixer name = mixR\n"
                      "                        control = { \"Gain 1\" = %1 \"Gain 2\" = %2 } }\n")
              .arg(directGain, 0, 'f', 4).arg(crossGain, 0, 'f', 4);
        tc << "                ]\n";
        tc << "                links = [\n";
        // Direct path
        tc << "                    { output = \"copyL:Out\"  input = \"mixL:In 1\" }\n";
        tc << "                    { output = \"copyR:Out\"  input = \"mixR:In 1\" }\n";
        // Cross-feed path: R → delay → into L mixer (creates width on left ear)
        tc << "                    { output = \"copyR:Out\"  input = \"delayR:In\" }\n";
        tc << "                    { output = \"delayR:Out\" input = \"mixL:In 2\" }\n";
        // Cross-feed path: L → delay → into R mixer (creates width on right ear)
        tc << "                    { output = \"copyL:Out\"  input = \"delayL:In\" }\n";
        tc << "                    { output = \"delayL:Out\" input = \"mixR:In 2\" }\n";
        tc << "                ]\n";
        tc << "                inputs  = [ \"copyL:In\" \"copyR:In\" ]\n";
        tc << "                outputs = [ \"mixL:Out\" \"mixR:Out\" ]\n";
        tc << "            }\n";
        tc << "            audio.channels = 2\n";
        tc << "            audio.position = [ FL FR ]\n";
        tc << "            capture.props = {\n";
        tc << "                node.name   = \"effect_input.archtitan_spatial\"\n";
        tc << "                media.class = Audio/Sink\n";
        tc << "            }\n";
        tc << "            playback.props = {\n";
        tc << "                node.name    = \"effect_output.archtitan_spatial\"\n";
        tc << "                node.passive = true\n";
        tc << "            }\n";
        tc << "        }\n";
        tc << "    }\n";
        tc << "]\n";

        QDir().mkpath(confDir);
        QFile f(confPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write(conf.toUtf8());
            f.close();
        }
    }

    // ── Restart PipeWire (only if node not active) or update parameters live ──
    // Replaces the PipeWire service restart with a fast live update when just
    // changing width/presets, making preset switching and slider moves gapless!
    QString liveScript = QString(
        "ID=$(pactl list sinks 2>/dev/null | "
        "awk '/archtitan_spatial/{f=1} f && /object\\.id/{gsub(/[^0-9]/,\"\"); print; exit}'); "
        "if [ -n \"$ID\" ]; then "
        "  pw-cli set-param \"$ID\" Props '{ params: [ \"mixL:Gain 1\" %1 \"mixL:Gain 2\" %2 \"mixR:Gain 1\" %1 \"mixR:Gain 2\" %2 ] }' 2>/dev/null; "
        "else "
        "  systemctl --user restart pipewire pipewire-pulse 2>/dev/null; "
        "  sleep 0.9; "
        "  pactl list short sink-inputs 2>/dev/null | awk '{print $1}' | "
        "  xargs -I{} pactl move-sink-input {} effect_input.archtitan_spatial 2>/dev/null; "
        + resumeScript +
        "fi; "
        "true"
    ).arg(directGain, 0, 'f', 4).arg(crossGain, 0, 'f', 4);

    QProcess::startDetached("bash", {"-c", liveScript});
}
