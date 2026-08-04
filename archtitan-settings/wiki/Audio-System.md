# Audio System

[← Home](Home.md)

The audio subsystem is the most complex part of ArchTitan Settings. It evolved from a basic `wpctl` volume wrapper to a full native PipeWire effects pipeline with real-time visualization, six EQ presets, a custom 10-band equalizer, and Haas-effect spatial stereo widening.

**Backend:** `src/audiobackend.*` · **UI:** `qml/pages/AudioPage.qml`

---

## Pipeline Architecture

```
Media Player ──► [Spatial Sink] ──► [EQ Sink] ──► Hardware Output
                  (optional)         (always)
                     │                   │
              archtitan-spatial    archtitan-eq
              filter-chain         filter-chain
              .conf                .conf
```

When spatial audio is **disabled**, streams route directly to the EQ sink. When **enabled**, streams route through the spatial sink first, then into the EQ sink downstream.

---

## Volume & Mute Control

| Operation | Command |
|---|---|
| Read output volume | `wpctl get-volume @DEFAULT_AUDIO_SINK@` |
| Write output volume | `wpctl set-volume @DEFAULT_AUDIO_SINK@ N%` |
| Read mic volume | `wpctl get-volume @DEFAULT_AUDIO_SOURCE@` |
| Write mic volume | `wpctl set-volume @DEFAULT_AUDIO_SOURCE@ N%` |
| Mute/unmute | `wpctl set-mute @DEFAULT_*@ 0\|1` |

### External Sync

Instead of polling, `AudioBackend` runs `pactl subscribe` in a background process. Volume changes from keyboard media keys or other apps trigger a 50 ms debounced `sync()` call, keeping the UI accurate without manual refresh.

On startup, `sync()` queries the real system state so microphone volume and mute are not stuck at defaults.

---

## Real-Time Visualizer (Cava)

On startup, a temporary Cava config is written to `/tmp/archtitan-cava.conf`:

```ini
[general]
bars = 24
framerate = 60
[output]
method = raw
raw_target = /dev/stdout
data_format = ascii
ascii_max_range = 100
```

Cava output is parsed line-by-line into the `eqLevels` property (24 integer values), driving the animated bar chart in `AudioPage.qml`.

**Auto-restart:** If Cava exits (e.g., after a PipeWire service restart), it is relaunched after a 1-second delay.

---

## Native PipeWire Equalizer

The equalizer was migrated from **EasyEffects** to native **PipeWire filter-chain** modules, removing a heavy dependency and enabling live parameter updates.

### EQ Profiles

| Profile | Description | Key Adjustments |
|---|---|---|
| Flat | Neutral | All bands at 0 dB |
| Bass Boost | Enhanced low end | +6 dB @ 32 Hz, +5 dB @ 64 Hz |
| Vocal | Mid-range emphasis | +4 dB @ 500–2000 Hz, cut lows/highs |
| Electronic | V-shaped | Boost lows and highs, cut mids |
| Acoustic | Natural warmth | Gentle mid/high boost |
| Custom | User-defined | 10 adjustable bands, persisted in QSettings |

### Band Configuration

Each profile defines 10 bands at standard ISO frequencies:

```
32 Hz · 64 Hz · 125 Hz · 250 Hz · 500 Hz · 1 kHz · 2 kHz · 4 kHz · 8 kHz · 16 kHz
```

Filter types: **lowshelf** (band 1), **peaking** (bands 2–9), **highshelf** (band 10), Q = 0.707.

### Config Generation

`applyEqProfile()` generates SPA-JSON filter-chain configuration and writes it to:

```
~/.config/pipewire/filter-chain.conf.d/archtitan-eq.conf
```

The active profile name is stored as a comment header (`# Profile: Bass Boost`) for persistence across reboots.

### Live Gain Updates

Instead of restarting PipeWire (which caused 1–3 second audio gaps), gain changes are applied atomically:

```bash
pw-cli set-param "$NODE_ID" Props '{ params: [ "eq_band_N:Gain" X.X ... ] }'
```

Node ID is resolved via `pactl list sinks` (< 100 ms). Active streams are re-routed with `pactl move-sink-input` to prevent playback gaps.

When spatial audio is active, EQ profiles are forced to **Flat** to avoid interference with the spatial processing chain.

---

## Spatial Audio (Haas Effect)

Spatial audio implements stereo widening using a **Haas-effect cross-feed** built entirely from PipeWire builtin filters — no external impulse response files.

### Signal Flow

```
Input L ──► copyL ──┬──► mixL (direct) ──► Output L
                     └──► delayL ──► mixR (cross) ──► Output R

Input R ──► copyR ──┬──► mixR (direct) ──► Output R
                     └──► delayR ──► mixL (cross) ──► Output L
```

| Haas delay | 15 ms on cross-channel paths |
| Cross gain | 0.0–0.65 (mapped from width slider 0–100) |
| Direct gain | `1.0 - crossGain × 0.32` |
| Config file | `~/.config/pipewire/filter-chain.conf.d/archtitan-spatial.conf` |

### Configuration Template

The spatial audio node is defined in `archtitan-spatial.conf` using PipeWire's built-in `delay` and `mixer` filters. To prevent initialization crashes, the delay node configuration must separate the static maximum buffer size (`max-delay` in `config`) from the active delay value (`Delay (s)` in `control` in seconds):

```spa-json
context.modules = [
    { name = libpipewire-module-filter-chain
        flags = [ nofail ]
        args = {
            node.description = "ArchTitan Spatial"
            media.name       = "ArchTitan Spatial"
            filter.graph = {
                nodes = [
                    { type = builtin label = copy  name = copyL }
                    { type = builtin label = copy  name = copyR }
                    { type = builtin label = delay name = delayR
                        config = { "max-delay" = 1.0 }
                        control = { "Delay (s)" = 0.015 } }
                    { type = builtin label = delay name = delayL
                        config = { "max-delay" = 1.0 }
                        control = { "Delay (s)" = 0.015 } }
                    { type = builtin label = mixer name = mixL
                        control = { "Gain 1" = 0.7900 "Gain 2" = 0.5200 } }
                    { type = builtin label = mixer name = mixR
                        control = { "Gain 1" = 0.7900 "Gain 2" = 0.5200 } }
                ]
                links = [
                    { output = "copyL:Out"  input = "mixL:In 1" }
                    { output = "copyR:Out"  input = "mixR:In 1" }
                    { output = "copyR:Out"  input = "delayR:In" }
                    { output = "delayR:Out" input = "mixL:In 2" }
                    { output = "copyL:Out"  input = "delayL:In" }
                    { output = "delayL:Out" input = "mixR:In 2" }
                ]
                inputs  = [ "copyL:In" "copyR:In" ]
                outputs = [ "mixL:Out" "mixR:Out" ]
            }
            audio.channels = 2
            audio.position = [ FL FR ]
            capture.props = {
                node.name   = "effect_input.archtitan_spatial"
                media.class = Audio/Sink
            }
            playback.props = {
                node.name    = "effect_output.archtitan_spatial"
                node.passive = true
            }
        }
    }
]
```

### Gapless Operation

| Action | Behavior | Implementation / Command |
|---|---|---|
| Width/preset change | Live `pw-cli set-param` — no PipeWire restart | `pw-cli set-param $NODE_ID Props '{ params: [ "mixL:Gain 1" X.X ... ] }'` |
| Toggle on/off | PipeWire restart only when spatial node doesn't exist yet | `systemctl --user restart pipewire pipewire-pulse` |
| Playback interruption | MPRIS auto-resume via `playerctl play` after restart | Shell script triggers `playerctl -p $PLAYER play` after 0.9s |
| EQ interaction | Spatial ON forces Flat EQ; streams route spatial → EQ | EQ profiles and custom sliders are disabled/greyed out with a `(Paused)` suffix |


---

## Custom Equalizer

Users can define a 10-band custom profile:

- Gains stored in QSettings (`audio/customGains`)
- Per-band live update via `setCustomBandGain(index, gain)`
- Reset to flat via `resetCustomGains()`
- Selecting "Custom" profile applies user gains through the same filter-chain pipeline

---

## Evolution Timeline

| Phase | Approach | Issue |
|---|---|---|
| v0 | Random QML bar animation | No real audio data |
| v1 | EasyEffects + D-Bus preset loading | Heavy dependency, slow switching |
| v2 | Native PipeWire filter-chain + file generation | 1–3 s gaps on profile switch |
| v3 | `pw-cli set-param` live updates | Fast EQ, but spatial caused pauses |
| v4 (current) | Gapless spatial + Cava auto-restart + MPRIS resume | Production-ready |

---

## Glossary

| Term | Definition |
|---|---|
| **Haas Effect** | Psychoacoustic phenomenon where a delayed copy of a signal creates spatial width |
| **Filter-chain** | PipeWire module that chains audio processing filters in a graph |
| **SPA-JSON** | PipeWire's JSON-based configuration format for filter nodes |
| **MPRIS** | D-Bus interface for media player control (play, pause, stop) |
| **Cava** | Console-based Audio Visualizer for ALSA/PulseAudio/PipeWire |

---

## Related Pages

- [Backend Modules](Backend-Modules.md) — AudioBackend property reference
- [Known Issues](Known-Issues.md) — resolved audio bugs
- [Roadmap](Roadmap.md) — pending audio output device selector

[← Home](Home.md)
