// POSSESSED — FX rack: performance effects with engage switches.
// Stutter (tempo-synced step gate), Repeat (buffer stutter), Tape Stop, Freeze (granular hold), Dive (pitch drop/rise).
// Self-contained (no JUCE).
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace poss
{
// Stereo circular buffer with fractional reads at absolute (wrapping) positions
struct StereoRing
{
    std::vector<float> l, r; int size = 1, w = 0;
    void prepare (int n) { size = std::max (4, n); l.assign ((size_t) size, 0.0f); r.assign ((size_t) size, 0.0f); w = 0; }
    void push (float a, float b) { l[(size_t) w] = a; r[(size_t) w] = b; if (++w >= size) w = 0; }
    int wrap (int i) const { i %= size; return i < 0 ? i + size : i; }
    void read (double pos, float& a, float& b) const
    {
        const double fl = std::floor (pos);
        const int i0 = wrap ((int) fl), i1 = wrap ((int) fl + 1);
        const float f = (float) (pos - fl);
        a = l[(size_t) i0] + (l[(size_t) i1] - l[(size_t) i0]) * f;
        b = r[(size_t) i0] + (r[(size_t) i1] - r[(size_t) i0]) * f;
    }
};

struct FxParams
{
    // Stutter: lit steps CUT the volume
    bool stutterOn = false, stutterTails = false;
    int stutterRate = 2;          // 0 1/4, 1 1/8, 2 1/16, 3 1/32, 4 1/8T, 5 1/16T
    int stutterSteps = 16;        // pattern length 1..16
    float stutterDepth = 1.0f;    // how far the volume drops on a lit step
    float stutterSmooth = 3.0f;   // edge smoothing, ms
    uint16_t stutterPattern = 0b0110110001101100;
    // Repeat
    bool repeatOn = false; int repeatLength = 2; // 0 1/4, 1 1/8, 2 1/16, 3 1/32, 4 1/64
    float repeatDrop = 0.0f;      // semitones per repeat (negative = sinks, positive = rises)
    // Tape stop
    bool tapeOn = false; float tapeTime = 0.8f; // seconds to grind to a halt
    // Freeze
    bool freezeOn = false; float freezeSize = 0.4f, freezeMix = 0.7f;
    // Dive
    bool diveOn = false; float diveAmount = -12.0f, diveTime = 1.0f;
};

class FxRack
{
public:
    FxParams p;
    int currentStep = -1;   // for the UI
    float diveSemis = 0.0f; // current dive offset, applied to the voice by the engine

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        history.prepare ((int) (sr * 3.5));
        tape.prepare ((int) (sr * 6.0));
        repL.assign ((size_t) (sr * 3.1), 0.0f); repR.assign (repL.size(), 0.0f);
        frzL.assign ((size_t) (sr * 1.0), 0.0f); frzR.assign (frzL.size(), 0.0f);
        gate = 1.0f; repMix = tapeMix = frzEnv = 0.0f; wasRepeat = wasTape = wasFreeze = false;
        diveSemis = 0.0f; freeBeat = 0.0;
        xfadeCoef = 1.0f - std::exp (-1.0f / (float) (sr * 0.006));
        frzCoef = 1.0f - std::exp (-1.0f / (float) (sr * 0.06));
        for (auto& g : grains) g.active = false;
    }

    // The host shifts our output earlier by the plugin latency (delay compensation), so the step grid is
    // read 'latency' samples behind the playhead to land exactly on the song's beats.
    void setLatency (int samples) { latency = samples; }

    void setTransport (double bpmIn, double ppqIn, bool playingIn)
    {
        bpm = bpmIn > 1.0 ? bpmIn : 120.0;
        if (playingIn && ppqIn >= 0.0) { playing = true; ppq = ppqIn; }
        else { playing = false; ppq = freeBeat; }
    }

    // Called once per block before the voice is processed: advances the dive.
    void updateDive (int n)
    {
        const float target = p.diveOn ? p.diveAmount : 0.0f;
        const float t = std::max (0.02f, p.diveOn ? p.diveTime : p.diveTime * 0.35f);
        const float c = 1.0f - std::exp (-(float) n / (float) (sr * t * 0.35));
        diveSemis += (target - diveSemis) * c;
        if (std::abs (diveSemis) < 1e-3f && ! p.diveOn) diveSemis = 0.0f;
    }

    // Stutter gain at sample i of the block (also advances the step clock when i == n-1)
    float stutterGain (int i)
    {
        static const double stepBeats[6] = { 1.0, 0.5, 0.25, 0.125, 1.0 / 3.0, 1.0 / 6.0 };
        const double beat = ppq + (double) (i - latency) * bpm / 60.0 / sr;
        const double len = stepBeats[std::clamp (p.stutterRate, 0, 5)];
        const int steps = std::clamp (p.stutterSteps, 1, 16);
        const long long idx = (long long) std::floor (beat / len);
        const int step = (int) (((idx % steps) + steps) % steps);
        currentStep = p.stutterOn ? step : -1;
        const bool cut = p.stutterOn && ((p.stutterPattern >> step) & 1u);
        const float target = cut ? 1.0f - std::clamp (p.stutterDepth, 0.0f, 1.0f) : 1.0f;
        const float c = 1.0f - std::exp (-1.0f / (float) (sr * std::max (0.3f, p.stutterSmooth) * 0.001));
        gate += (target - gate) * c;
        return gate;
    }

    void endBlock (int n)
    {
        freeBeat = ppq + (double) n * bpm / 60.0 / sr;
        if (playing) freeBeat = std::fmod (freeBeat, 4096.0);
    }

    // Post-mix chain for one stereo sample: Freeze -> Repeat -> [Stutter, if not Tails] -> Tape Stop
    void processSample (float& a, float& b, int i)
    {
        history.push (a, b);

        // ---- Freeze: capture the last second on engage, then play drifting, detuned grains of it
        if (p.freezeOn && ! wasFreeze) captureFreeze();
        wasFreeze = p.freezeOn;
        frzEnv += ((p.freezeOn ? 1.0f : 0.0f) - frzEnv) * frzCoef;
        if (frzEnv > 1e-4f)
        {
            float fa = 0, fb = 0;
            renderGrains (fa, fb);
            const float m = std::clamp (p.freezeMix, 0.0f, 1.0f) * frzEnv;
            a = a * (1.0f - 0.5f * m) + fa * m;
            b = b * (1.0f - 0.5f * m) + fb * m;
        }

        // ---- Repeat: loop a tempo-synced slice, optionally sinking/rising in pitch each pass
        if (p.repeatOn && ! wasRepeat) captureRepeat();
        wasRepeat = p.repeatOn;
        repMix += ((p.repeatOn ? 1.0f : 0.0f) - repMix) * xfadeCoef;
        if (repMix > 1e-4f)
        {
            const float pos = repPos;
            const int i0 = std::min ((int) pos, repLen - 1); const float f = pos - (float) i0;
            const int i1 = std::min (i0 + 1, repLen - 1);
            float ra = repL[(size_t) i0] + (repL[(size_t) i1] - repL[(size_t) i0]) * f;
            float rb = repR[(size_t) i0] + (repR[(size_t) i1] - repR[(size_t) i0]) * f;
            const float edge = std::max (1.0f, (float) sr * 0.002f);
            const float fade = std::min ({ 1.0f, pos / edge, ((float) repLen - pos) / edge });
            ra *= fade; rb *= fade;
            a += (ra - a) * repMix; b += (rb - b) * repMix;
            if (p.repeatOn)
            {
                repPos += repRate;
                if (repPos >= (float) repLen)
                {
                    repPos -= (float) repLen;
                    repRate = std::clamp (repRate * std::pow (2.0f, p.repeatDrop / 12.0f), 0.125f, 4.0f);
                }
            }
        }

        // ---- Stutter (post position)
        if (! p.stutterTails) { const float g = stutterGain (i); a *= g; b *= g; }

        // ---- Tape stop: read back ever slower until it grinds to a halt
        tape.push (a, b);
        if (p.tapeOn && ! wasTape) { tapeRead = (double) tape.w - 1.0; tapeRate = 1.0f; }
        wasTape = p.tapeOn;
        tapeMix += ((p.tapeOn ? 1.0f : 0.0f) - tapeMix) * xfadeCoef;
        if (tapeMix > 1e-4f)
        {
            float ta = 0, tb = 0;
            tape.read (tapeRead, ta, tb);
            const float level = std::min (1.0f, tapeRate * 6.0f);
            ta *= level; tb *= level;
            a += (ta - a) * tapeMix; b += (tb - b) * tapeMix;
            if (p.tapeOn)
            {
                tapeRate = std::max (0.0f, tapeRate - 1.0f / (std::max (0.05f, p.tapeTime) * (float) sr));
                tapeRead += tapeRate;
            }
        }
    }

private:
    struct Grain { bool active = false; float pos = 0, start = 0, len = 1, rate = 1, phase = 0, pan = 0; };

    void captureRepeat()
    {
        static const double lenBeats[5] = { 1.0, 0.5, 0.25, 0.125, 0.0625 };
        const double secs = lenBeats[std::clamp (p.repeatLength, 0, 4)] * 60.0 / bpm;
        repLen = std::clamp ((int) (secs * sr), (int) (sr * 0.01), (int) repL.size());
        for (int k = 0; k < repLen; ++k)
        {
            const int src = history.wrap (history.w - repLen + k);
            repL[(size_t) k] = history.l[(size_t) src]; repR[(size_t) k] = history.r[(size_t) src];
        }
        repPos = 0.0f; repRate = 1.0f;
    }

    void captureFreeze()
    {
        const int len = (int) frzL.size();
        for (int k = 0; k < len; ++k)
        {
            const int src = history.wrap (history.w - len + k);
            frzL[(size_t) k] = history.l[(size_t) src]; frzR[(size_t) k] = history.r[(size_t) src];
        }
        for (size_t g = 0; g < grains.size(); ++g) { grains[g].active = false; grains[g].phase = (float) g / (float) grains.size(); }
    }

    float rnd() { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (float) (rs >> 8) * (1.0f / 16777216.0f); }

    void renderGrains (float& outA, float& outB)
    {
        const float glen = (float) sr * (0.06f + 0.54f * std::clamp (p.freezeSize, 0.0f, 1.0f));
        const int capLen = (int) frzL.size();
        for (auto& g : grains)
        {
            if (! g.active || g.phase >= 1.0f)
            {
                // (re)start this grain somewhere in the most recent part of the capture
                g.active = true;
                g.len = glen;
                const float maxStart = std::max (1.0f, (float) capLen - glen * 1.1f - 2.0f);
                g.start = maxStart * (0.35f + 0.65f * rnd());
                g.rate = std::pow (2.0f, (rnd() * 2.0f - 1.0f) * 0.12f / 12.0f); // +-12 cents
                g.pan = rnd() * 1.2f - 0.6f;
                if (g.phase >= 1.0f) g.phase -= 1.0f;
                g.pos = g.phase * g.len;
            }
            const float rp = g.start + g.pos * g.rate;
            const int i0 = std::clamp ((int) rp, 0, capLen - 2); const float f = rp - (float) i0;
            const float sa = frzL[(size_t) i0] + (frzL[(size_t) i0 + 1] - frzL[(size_t) i0]) * f;
            const float sb = frzR[(size_t) i0] + (frzR[(size_t) i0 + 1] - frzR[(size_t) i0]) * f;
            const float win = std::sin (3.14159265f * g.phase); const float w2 = win * win;
            outA += (sa * (1.0f - g.pan * 0.5f)) * w2 * 0.8f; // random-position grains sum incoherently: ~+4 dB makeup
            outB += (sb * (1.0f + g.pan * 0.5f)) * w2 * 0.8f;
            g.pos += 1.0f; g.phase = g.pos / g.len;
        }
    }

    double sr = 48000, bpm = 120, ppq = 0, freeBeat = 0; bool playing = false; int latency = 0;
    float gate = 1.0f, xfadeCoef = 0.01f, frzCoef = 0.001f;
    StereoRing history, tape;
    std::vector<float> repL, repR, frzL, frzR;
    int repLen = 1; float repPos = 0, repRate = 1, repMix = 0;
    double tapeRead = 0; float tapeRate = 1, tapeMix = 0;
    float frzEnv = 0;
    std::array<Grain, 4> grains;
    bool wasRepeat = false, wasTape = false, wasFreeze = false;
    uint32_t rs = 0x5EEDu;
};
} // namespace poss
