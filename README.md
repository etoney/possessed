# Possessed: evil voice transformer

A Castle Danger MC insert effect (VST3 / AU / Standalone, built with JUCE). It turns a recorded vocal into demons, possessed children, ghosts and worse. Put it on a vocal track, pick a preset, then shape it like a synth.

## Signal chain
| Section | What it does |
|---|---|
| **Voice** | Phase-vocoder **Pitch** shift (±24 st) with independent **Formant** shift (±12 st, cepstral envelope). Low formants give a huge throat, high formants give a small child. **Whisper** scrambles the phase so the voice turns into a breathy, pitchless whisper |
| **Layer** | A second voice from the same analysis, with its own pitch, formant and level. Use it for a demon an octave below or a ghost an octave above |
| **Demon** | **Growl**: zero-crossing sub-octave dividers, one and then two octaves down. **Rasp**: jittered vocal-fry pulses and rough modulation |
| **Ring Mod** | Metallic, inhuman sidebands (5 Hz – 2 kHz) |
| **Distort** | Tube, Fuzz, Bitcrush, or Radio (band-limited, clipped, crackling) |
| **Filter** | Low-, band- or high-pass resonant filter |
| **Wobble** | Seasick tape warble and flutter |
| **Legion** | Six drifting, detuned copies spread across the stereo field ("we are many") |
| **Reverse** | Backwards echoes that swell into each word |
| **Delay / Reverb** | Ping-pong delay and an 8-line FDN reverb (tails up to ~14 s) |
| **In / Out** | Input gain, Dry/Wet, Output, meters |

The voice engine auto-matches loudness, so pitch, formant and whisper changes don't make the level jump. Latency is 2048 samples at 44.1/48 kHz (4096 above 60 kHz) and is reported to the host for delay compensation.

**14 factory presets:** Init (Clean), Demon, Disturbed Child, The Haunting, Possessed, Legion, Hellhound, Ghost Whisper, The Entity, Backwards Prayer, Static From Beyond, Old Witch, The Abyss, Siren of the Dead. Changing presets keeps your Input gain.

The header's demon eyes react to the signal. They glow, dilate, look around and blink.

## Building
Requirements: CMake 3.22+, Git, and a C++17 compiler (Xcode on macOS, Visual Studio 2022 on Windows). JUCE is downloaded automatically.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Possessed_VST3 Possessed_Standalone   # add Possessed_AU on macOS
```
Output goes to `build/Possessed_artefacts/Release/`. See `readme.txt` for installing by hand.

**No compiler?** The GitHub Actions workflow (`.github/workflows/build.yml`) builds macOS and Windows versions on every push. Download them from the run's Artifacts on the Actions tab.

## Offline DSP test
`Source/PossessedEngine.h` and `Source/Presets.h` don't depend on JUCE.

```bash
cmake --build build --target possessed_render
./build/possessed_render out_dir [your_vocal.wav]
```
This checks pitch accuracy, neutral transparency, NaNs and levels, then renders every preset to WAV. Pass a 16-bit WAV of your own voice to hear the presets on it.
