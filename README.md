# Possessed: Pure Vocal Destruction

A Castle Danger Music Company insert effect (VST3 / AU / Standalone, built with JUCE). It turns a recorded vocal into demons, possessed children, ghosts and worse. Put it on a vocal track, pick a preset, then shape it like a synth.

## Signal chain
| Section | What it does |
|---|---|
| **Voice** | Phase-vocoder **Pitch** shift (±24 st) with independent **Formant** shift (±12 st, cepstral envelope). Low formants give a huge throat, high formants give a small child. **Whisper** scrambles the phase so the voice turns into a breathy, pitchless whisper |
| **Layer** | A second voice from the same analysis, with its own pitch, formant and level. Use it for a demon an octave below or a ghost an octave above |
| **Demon** | **Growl**: zero-crossing sub-octave dividers, one and then two octaves down. **Rasp**: jittered vocal-fry pulses and rough modulation |
| **Ring Mod** | Metallic, inhuman sidebands (5 Hz – 2 kHz) |
| **Distort** | Tube, Fuzz, Bitcrush, Radio (band-limited, clipped, crackling), or Saturate (warm tape-style grit that keeps the words clear) |
| **Filter** | Low-, band- or high-pass resonant filter |
| **Wobble** | Seasick tape warble and flutter |
| **Legion** | Six drifting, detuned copies spread across the stereo field ("we are many") |
| **Reverse** | Backwards echoes that swell into each word |
| **Delay** | Four types: **Digital** (clean), **Ping-Pong**, **Analog** (darker, saturating repeats with clock drift), **Tape** (wow and flutter, warm and degrading) |
| **Reverb** | 8-line FDN reverb (tails up to ~14 s) |
| **Lo-Fi** | Global, applied to the whole output: **Lo-Fi** narrows the bandwidth, adds wobble and saturation, then crushes the bits and sample rate as you turn it up. **Noise** adds tape hiss and crackle |
| **In / Out** | Input gain, Dry/Wet, Output, meters |

## FX rack (bottom row)
Performance effects for programmed or live mayhem. Each has a glowing **ON** switch. The switches are normal plugin parameters, so you can automate them in your DAW to hit an effect on an exact word or beat. Factory presets leave the FX rack alone, so a pattern you've programmed survives preset browsing. User presets save it.

| Effect | What it does |
|---|---|
| **Stutter** | A 16-step sequencer locked to your song's tempo. **Lit steps cut the volume.** **Rate** sets the step size (1/4 to 1/32, plus 1/8T and 1/16T triplets). **Steps** sets the pattern length (1–16, for odd loops); steps past it dim. **Depth** sets how far the volume drops. **Smooth** rounds the edges (low = hard chop, high = pulsing). **Tails** lets delay and reverb ring through the cuts instead of chopping them. The current step glows as it plays |
| **Repeat** | The classic "s-s-s-stutter": grabs a tempo-synced slice (1/4 to 1/64) the moment you switch it on and loops it. **Drop** shifts the pitch each repeat (negative = sinking and dying, positive = rising panic) |
| **Tape Stop** | Everything slows and drops in pitch until it grinds to silence. **Time** sets how long that takes. Switch off to snap back |
| **Freeze** | Holds the last moment as a drifting, slightly detuned drone (a frozen scream) under the vocal. **Size** = grain length (short = buzzy, long = smeared), **Mix** = how loud the frozen sound is |
| **Dive** | Slides the transformed voice down (or up) by **Amount** semitones over **Time**: a demonic drop at the end of a phrase, or a rising shriek |

Stutter's grid is offset for the plugin's latency, so the cuts land exactly on your song's beats. When the DAW is stopped, it free-runs at the session tempo. Every switch and step edge is crossfaded to avoid clicks.

The voice engine auto-matches loudness, so pitch, formant and whisper changes don't make the level jump. Latency is 2048 samples at 44.1/48 kHz (4096 above 60 kHz) and is reported to the host for delay compensation.

**17 factory presets:** Init (Clean), Demon, Lizzy's Playhouse, The Haunting, Possessed, Legion, Hellhound, The Entity, Static From Beyond, Old Witch, Siren of the Dead, Ventriloquist Dummy, Behind You, The Drowned, Found Footage, Cult Chant, Wendigo. Every preset is level-matched and tuned so the words still come through. Changing presets keeps your Input gain.

**Your own presets:** dial in a sound and click **Save Preset...**, then type a name. Saved presets show up under **User Presets** in the preset menu (and in the < > arrows) in every project and every DAW. **Delete** removes the selected user preset. They're stored as small `.possessed` files you can back up or share:
- macOS: `~/Library/Application Support/Castle Danger/Possessed/Presets/`  (open Finder → Go → Go to Folder…)
- Windows: `%APPDATA%\Castle Danger\Possessed\Presets\`

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
This checks pitch accuracy, neutral transparency, NaNs, levels and a clarity score, then renders every preset to WAV. `possessed_render_fx out_dir` tests the FX rack (stutter timing, repeat loop accuracy, tape stop, freeze, dive, clicks, random switching) and renders demos. Pass a 16-bit WAV of your own voice to hear the presets on it.
