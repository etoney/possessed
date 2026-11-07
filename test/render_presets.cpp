// Offline test: renders a synthetic voice (or a WAV you pass) through every preset.
//   render_presets out_dir [input.wav]
#include "Presets.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

using namespace poss;

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4, riff = 36 + dataBytes;
    const uint16_t fmt = 1, ch = 2, bits = 16, align = 4; const uint32_t rate = (uint32_t) sr, bps = rate * 4, fmtLen = 16;
    std::fwrite ("RIFF", 1, 4, f); std::fwrite (&riff, 4, 1, f); std::fwrite ("WAVEfmt ", 1, 8, f);
    std::fwrite (&fmtLen, 4, 1, f); std::fwrite (&fmt, 2, 1, f); std::fwrite (&ch, 2, 1, f);
    std::fwrite (&rate, 4, 1, f); std::fwrite (&bps, 4, 1, f); std::fwrite (&align, 2, 1, f); std::fwrite (&bits, 2, 1, f);
    std::fwrite ("data", 1, 4, f); std::fwrite (&dataBytes, 4, 1, f);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t s[2] = { (int16_t) (clampf (L[i], -1, 1) * 32767), (int16_t) (clampf (R[i], -1, 1) * 32767) };
        std::fwrite (s, 2, 2, f);
    }
    std::fclose (f);
}

// minimal 16-bit PCM wav reader (mono or stereo), returns mono
static bool readWav (const char* path, std::vector<float>& out, int& sr)
{
    FILE* f = std::fopen (path, "rb"); if (! f) return false;
    char id[4]; uint32_t sz; std::fread (id, 1, 4, f); std::fread (&sz, 4, 1, f); std::fread (id, 1, 4, f);
    uint16_t ch = 1, bits = 16; uint32_t rate = 48000;
    while (std::fread (id, 1, 4, f) == 4 && std::fread (&sz, 4, 1, f) == 1)
    {
        if (! std::memcmp (id, "fmt ", 4)) { uint16_t fmt; std::fread (&fmt, 2, 1, f); std::fread (&ch, 2, 1, f); std::fread (&rate, 4, 1, f);
                                             uint32_t b; std::fread (&b, 4, 1, f); uint16_t a; std::fread (&a, 2, 1, f); std::fread (&bits, 2, 1, f);
                                             std::fseek (f, (long) sz - 16, SEEK_CUR); }
        else if (! std::memcmp (id, "data", 4))
        {
            const uint32_t frames = sz / (2 * ch);
            out.resize (frames);
            for (uint32_t i = 0; i < frames; ++i) { float acc = 0; for (int c = 0; c < ch; ++c) { int16_t s; std::fread (&s, 2, 1, f); acc += s / 32768.0f; } out[i] = acc / ch; }
            break;
        }
        else std::fseek (f, (long) sz, SEEK_CUR);
    }
    std::fclose (f); sr = (int) rate; return ! out.empty();
}

// Synthetic talking voice: glottal pulses with intonation + 2 formant pairs per vowel + fricatives
static std::vector<float> makeVoice (int sr, float seconds)
{
    struct V { float f1, f2, f3; };
    const V vowels[] = { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 }, { 570, 840, 2410 }, { 300, 870, 2240 } };
    std::vector<float> y ((size_t) (sr * seconds), 0.0f);
    SVF f1, f2, f3; Rng rng (7); float ph = 0; OnePole tilt; tilt.set (sr, 900);
    const int syl = (int) (sr * 0.22);
    for (size_t i = 0; i < y.size(); ++i)
    {
        const int s = (int) (i / (size_t) syl); const float sp = (float) (i % (size_t) syl) / (float) syl;
        const float t = (float) i / (float) sr;
        const bool pause = (s % 7) == 6;
        const float f0 = 125.0f * (1.0f + 0.12f * std::sin (t * 1.3f) + 0.05f * std::sin (s * 2.1f)) * (1.0f + 0.006f * std::sin (t * 5.5f * kTwoPi));
        ph += f0 / (float) sr; if (ph >= 1) ph -= 1;
        float glot = ph < 0.6f ? std::sin (kPi * ph / 0.6f) * std::sin (kPi * ph / 0.6f) : 0.0f; // Rosenberg-ish
        glot = tilt.hp (glot) * 3.0f + 0.02f * rng.bip();
        const V& v = vowels[(size_t) (s * 3 + s / 2) % 5];
        if (i % 64 == 0) { f1.set (sr, v.f1, 8); f2.set (sr, v.f2, 10); f3.set (sr, v.f3, 12); }
        float voiced = f1.bp (glot) * 1.0f + f2.bp (glot) * 0.6f + f3.bp (glot) * 0.3f;
        float envl = std::sin (kPi * sp); envl = std::sqrt (std::max (0.0f, envl));
        float fric = (sp < 0.15f && (s % 3 == 0)) ? rng.bip() * 0.15f * (1.0f - sp / 0.15f) : 0.0f;
        y[i] = pause ? 0.0f : (voiced * envl * 0.35f + fric);
    }
    return y;
}

static double rms (const std::vector<float>& v, size_t a = 0) { double s = 0; for (size_t i = a; i < v.size(); ++i) s += v[i] * (double) v[i]; return std::sqrt (s / std::max<size_t> (1, v.size() - a)); }

// estimate fundamental via zero crossings of a lowpassed signal
static float zcFreq (const std::vector<float>& v, int sr, size_t from)
{
    int crossings = 0; for (size_t i = from + 1; i < v.size(); ++i) if (v[i - 1] < 0 && v[i] >= 0) ++crossings;
    return (float) crossings * (float) sr / (float) (v.size() - from);
}

int main (int argc, char** argv)
{
    const std::string outDir = argc > 1 ? argv[1] : ".";
    int sr = 48000; std::vector<float> voice;
    if (argc > 2 && readWav (argv[2], voice, sr)) std::printf ("input: %s (%d Hz, %.1f s)\n", argv[2], sr, voice.size() / (double) sr);
    else voice = makeVoice (sr, 6.0f);
    const int block = 256; bool allOk = true;

    // --- 1) Pitch accuracy on a sine
    for (float semis : { -12.0f, 7.0f, -24.0f })
    {
        Engine e; e.prepare (sr, block); e.params.pitch = semis; e.snapSmoothers();
        std::vector<float> L (sr * 2), R;
        for (size_t i = 0; i < L.size(); ++i) L[i] = 0.3f * std::sin (kTwoPi * 220.0f * (float) i / (float) sr);
        R = L;
        for (size_t b = 0; b + block <= L.size(); b += block) e.process (&L[b], &R[b], block);
        const float f = zcFreq (L, sr, (size_t) sr / 2), want = 220.0f * semisToRatio (semis);
        const bool ok = std::abs (f / want - 1.0f) < 0.03f;
        allOk &= ok;
        std::printf ("pitch %+5.1f st: got %7.1f Hz, want %7.1f Hz %s\n", semis, f, want, ok ? "ok" : "FAIL");
    }

    // --- 2) Neutral settings ~ transparent (after latency)
    {
        Engine e; e.prepare (sr, block); e.snapSmoothers();
        std::vector<float> L = voice, R = voice;
        for (size_t b = 0; b + block <= L.size(); b += block) e.process (&L[b], &R[b], block);
        const int lat = e.getLatency(); double err = 0, ref = 0;
        for (size_t i = (size_t) lat + 4800; i + 4800 < L.size(); ++i) { const double d = L[i] - voice[i - (size_t) lat]; err += d * d; ref += voice[i - (size_t) lat] * (double) voice[i - (size_t) lat]; }
        std::printf ("neutral: latency %d samples, residual %.1f dB below signal\n", lat, -10 * std::log10 (err / ref + 1e-12));
    }

    // --- 3) every preset
    writeWav (outDir + "/00_dry_input.wav", voice, voice, sr);
    const auto& ps = factoryPresets();
    std::printf ("\n%-22s %8s %8s %8s %6s\n", "preset", "in rms", "out rms", "peak", "x RT");
    for (size_t pi = 0; pi < ps.size(); ++pi)
    {
        Engine e; for (auto& [id, v] : ps[pi].values) setParam (e.params, id, v);
        e.prepare (sr, block); e.snapSmoothers();
        std::vector<float> L = voice, R = voice;
        L.resize (voice.size() + (size_t) sr * 2, 0.0f); R.resize (L.size(), 0.0f); // tail
        const auto t0 = std::chrono::steady_clock::now();
        for (size_t b = 0; b + block <= L.size(); b += block) e.process (&L[b], &R[b], block);
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        float peak = 0; bool bad = false;
        for (size_t i = 0; i < L.size(); ++i) { if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) bad = true; peak = std::max ({ peak, std::abs (L[i]), std::abs (R[i]) }); }
        std::vector<float> mono (L.size()); for (size_t i = 0; i < L.size(); ++i) mono[i] = 0.5f * (L[i] + R[i]);
        const double rt = (L.size() / (double) sr) / secs;
        std::printf ("%-22s %8.3f %8.3f %8.3f %6.0f %s\n", ps[pi].name, rms (voice), rms (mono), peak, rt, bad ? "NaN!" : "");
        allOk &= ! bad && peak < 1.25f && rms (mono) > 0.01;
        char fn[256]; std::snprintf (fn, sizeof fn, "%s/%02zu_%s.wav", outDir.c_str(), pi, ps[pi].name);
        std::string s = fn; for (auto& c : s) if (c == ' ' || c == '(' || c == ')') c = '_';
        writeWav (s, L, R, sr);
    }
    std::printf ("\n%s\n", allOk ? "ALL OK" : "SOME CHECKS FAILED");
    return allOk ? 0 : 1;
}
