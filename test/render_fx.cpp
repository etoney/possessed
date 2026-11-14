// Offline test for the FX rack: behaviour checks + click measurement + demo renders.
//   render_fx out_dir
#include "Presets.h"
#include <cstdio>
#include <functional>
#include <string>

using namespace poss;
static const int SR = 48000, BLOCK = 256;

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4, riff = 36 + dataBytes;
    const uint16_t fmt = 1, ch = 2, bits = 16, align = 4; const uint32_t rate = SR, bps = SR * 4, fmtLen = 16;
    std::fwrite ("RIFF", 1, 4, f); std::fwrite (&riff, 4, 1, f); std::fwrite ("WAVEfmt ", 1, 8, f);
    std::fwrite (&fmtLen, 4, 1, f); std::fwrite (&fmt, 2, 1, f); std::fwrite (&ch, 2, 1, f);
    std::fwrite (&rate, 4, 1, f); std::fwrite (&bps, 4, 1, f); std::fwrite (&align, 2, 1, f); std::fwrite (&bits, 2, 1, f);
    std::fwrite ("data", 1, 4, f); std::fwrite (&dataBytes, 4, 1, f);
    for (uint32_t i = 0; i < n; ++i) { int16_t s[2] = { (int16_t) (clampf (L[i], -1, 1) * 32767), (int16_t) (clampf (R[i], -1, 1) * 32767) }; std::fwrite (s, 2, 2, f); }
    std::fclose (f);
}

// talking-ish synthetic voice (same idea as render_presets)
static std::vector<float> makeVoice (float seconds)
{
    struct V { float f1, f2; }; const V vowels[] = { { 730, 1090 }, { 530, 1840 }, { 270, 2290 }, { 570, 840 }, { 300, 870 } };
    std::vector<float> y ((size_t) (SR * seconds)); SVF a, b; Rng rng (7); float ph = 0; OnePole tilt; tilt.set (SR, 900);
    const int syl = (int) (SR * 0.22);
    for (size_t i = 0; i < y.size(); ++i)
    {
        const int s = (int) (i / (size_t) syl); const float sp = (float) (i % (size_t) syl) / (float) syl, t = (float) i / SR;
        const float f0 = 125.0f * (1.0f + 0.12f * std::sin (t * 1.3f));
        ph += f0 / SR; if (ph >= 1) ph -= 1;
        float g = ph < 0.6f ? std::pow (std::sin (kPi * ph / 0.6f), 2.0f) : 0.0f; g = tilt.hp (g) * 3.0f;
        const V& v = vowels[(size_t) (s * 3 + s / 2) % 5];
        if (i % 64 == 0) { a.set (SR, v.f1, 8); b.set (SR, v.f2, 10); }
        y[i] = (s % 7 == 6) ? 0.0f : (a.bp (g) + 0.6f * b.bp (g)) * std::sqrt (std::sin (kPi * sp)) * 0.35f;
    }
    return y;
}

// run audio through an engine; 'control' is called before each block with the block's start time (s)
static void run (Engine& e, std::vector<float>& L, std::vector<float>& R, const std::function<void (double)>& control, double bpm = 120.0)
{
    for (size_t b = 0; b + BLOCK <= L.size(); b += BLOCK)
    {
        const double t = (double) b / SR;
        control (t);
        e.fx.setTransport (bpm, t * bpm / 60.0, true);
        e.process (&L[b], &R[b], BLOCK);
    }
}

// largest sample-to-sample jump relative to the typical jump (high = clicks)
static double stepCrest (const std::vector<float>& v, size_t a, size_t b)
{
    double mx = 0, s = 0; for (size_t i = a + 1; i < b; ++i) { const double d = std::abs (v[i] - v[i - 1]); mx = std::max (mx, d); s += d * d; }
    return mx / std::sqrt (s / (double) (b - a) + 1e-20);
}
static double rmsOf (const std::vector<float>& v, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += v[i] * (double) v[i]; return std::sqrt (s / (double) (b - a)); }
static float zcFreq (const std::vector<float>& v, size_t a, size_t b) { int c = 0; for (size_t i = a + 1; i < b; ++i) if (v[i - 1] < 0 && v[i] >= 0) ++c; return (float) c * SR / (float) (b - a); }

int main (int argc, char** argv)
{
    const std::string dir = argc > 1 ? argv[1] : ".";
    bool ok = true;
    auto check = [&] (bool c, const char* what) { std::printf ("  %-58s %s\n", what, c ? "ok" : "FAIL"); ok &= c; };
    std::vector<float> sine ((size_t) SR * 4);
    for (size_t i = 0; i < sine.size(); ++i) sine[i] = 0.3f * std::sin (kTwoPi * 220.0f * (float) i / SR);

    // ---- Stutter: 1/16 at 120 bpm = 125 ms steps; pattern cuts every other step
    {
        std::printf ("STUTTER\n");
        Engine e; e.prepare (SR, BLOCK); e.snapSmoothers();
        e.fx.p.stutterOn = true; e.fx.p.stutterRate = 2; e.fx.p.stutterSteps = 16; e.fx.p.stutterPattern = 0xAAAA; e.fx.p.stutterSmooth = 3.0f;
        auto L = sine, R = sine; run (e, L, R, [] (double) {});
        const int lat = e.getLatency(); const size_t step = SR / 8;
        // step k spans [k*step, (k+1)*step) in host time; output is delayed by 'lat'
        double onR = 0, offR = 0;
        for (int k = 4; k < 24; ++k)
        {
            const size_t a = (size_t) k * step + (size_t) lat + 600, b = a + step - 1200;
            (k % 2 ? offR : onR) += rmsOf (L, a, b);
        }
        std::printf ("  open-step rms %.3f, cut-step rms %.4f\n", onR / 10, offR / 10);
        check (offR / 10 < 0.01 * onR / 10, "lit steps cut the volume (>40 dB)");
        check (stepCrest (L, SR, L.size()) < 12.0, "no clicks at step edges");
    }

    // ---- Repeat: 1/8 at 120 bpm = 250 ms slice, dropping 2 semitones per repeat
    {
        std::printf ("REPEAT\n");
        Engine e; e.prepare (SR, BLOCK); e.snapSmoothers(); e.fx.p.repeatLength = 1; e.fx.p.repeatDrop = 0.0f;
        std::vector<float> L ((size_t) SR * 3), R; Rng rr (3); for (auto& x : L) x = rr.bip() * 0.2f; R = L; // noise so every slice is unique
        run (e, L, R, [&] (double t) { e.fx.p.repeatOn = t >= 1.0; });
        // once engaged, output should repeat every 250 ms
        const size_t a = (size_t) (SR * 1.6), P = SR / 4; double c = 0, n1 = 0, n2 = 0;
        for (size_t i = a; i < a + P - 400; ++i) { c += L[i] * L[i + P]; n1 += L[i] * L[i]; n2 += L[i + P] * L[i + P]; }
        const double corr = c / std::sqrt (n1 * n2);
        std::printf ("  correlation between consecutive repeats %.3f\n", corr);
        check (corr > 0.98, "slice loops exactly at the tempo-synced length");
        check (stepCrest (L, SR / 2, L.size()) < 12.0, "no clicks on engage / loop points");
    }

    // ---- Tape stop on a sine: pitch falls, then silence; disengage returns
    {
        std::printf ("TAPE STOP\n");
        Engine e; e.prepare (SR, BLOCK); e.snapSmoothers(); e.fx.p.tapeTime = 1.0f;
        auto L = sine, R = sine; run (e, L, R, [&] (double t) { e.fx.p.tapeOn = t >= 1.0 && t < 3.0; });
        const float f0 = zcFreq (L, SR / 2, SR), fMid = zcFreq (L, (size_t) (SR * 1.45), (size_t) (SR * 1.55));
        const double after = rmsOf (L, (size_t) (SR * 2.2), (size_t) (SR * 2.9)), back = rmsOf (L, (size_t) (SR * 3.3), (size_t) (SR * 3.9));
        std::printf ("  before %.0f Hz, halfway %.0f Hz, stopped rms %.4f, released rms %.3f\n", f0, fMid, after, back);
        check (fMid < 0.65f * f0 && fMid > 0.35f * f0, "pitch slides down as the tape slows");
        check (after < 0.002 && back > 0.15, "grinds to silence, comes back on release");
        check (stepCrest (L, SR / 2, L.size()) < 12.0, "no clicks on engage / release");
    }

    // ---- Freeze: hold the voice, then the input goes silent -> drone continues
    {
        std::printf ("FREEZE\n");
        Engine e; e.prepare (SR, BLOCK); e.snapSmoothers(); e.fx.p.freezeMix = 1.0f; e.fx.p.freezeSize = 0.4f;
        auto v = makeVoice (5.0f); for (size_t i = (size_t) SR * 2; i < v.size(); ++i) v[i] = 0.0f; // voice stops at 2 s
        auto L = v, R = v; run (e, L, R, [&] (double t) { e.fx.p.freezeOn = t >= 1.9 && t < 4.0; });
        const double held = rmsOf (L, (size_t) (SR * 2.6), (size_t) (SR * 3.8)), released = rmsOf (L, (size_t) (SR * 4.4), (size_t) (SR * 4.9));
        std::printf ("  held rms %.3f (voice was %.3f), after release %.4f\n", held, rmsOf (v, SR / 2, SR + SR / 2), released);
        check (held > 0.03 && released < 0.002, "sustains after the voice stops, fades out on release");
        check (stepCrest (L, SR / 2, L.size()) < 12.0, "no clicks");
    }

    // ---- Dive on a sine: -12 st lands an octave down
    {
        std::printf ("DIVE\n");
        Engine e; e.prepare (SR, BLOCK); e.snapSmoothers(); e.fx.p.diveAmount = -12.0f; e.fx.p.diveTime = 0.5f;
        auto L = sine, R = sine; run (e, L, R, [&] (double t) { e.fx.p.diveOn = t >= 1.0; });
        const float f = zcFreq (L, (size_t) (SR * 2.8), (size_t) (SR * 3.9));
        std::printf ("  after dive: %.1f Hz (want 110)\n", f);
        check (std::abs (f - 110.0f) < 4.0f, "dives an octave");
    }

    // ---- Torture: every effect toggled at random times, extreme settings
    {
        std::printf ("TORTURE\n");
        Engine e; e.prepare (SR, BLOCK); e.snapSmoothers(); Rng rr (11);
        e.fx.p.stutterSmooth = 0.3f; e.fx.p.repeatLength = 4; e.fx.p.repeatDrop = -12; e.fx.p.tapeTime = 0.1f; e.fx.p.freezeSize = 0.0f;
        auto v = makeVoice (8.0f); auto L = v, R = v; float peak = 0; bool bad = false;
        run (e, L, R, [&] (double) {
            if (rr.next() < 0.05f) e.fx.p.stutterOn = ! e.fx.p.stutterOn;
            if (rr.next() < 0.05f) e.fx.p.stutterTails = ! e.fx.p.stutterTails;
            if (rr.next() < 0.05f) e.fx.p.repeatOn = ! e.fx.p.repeatOn;
            if (rr.next() < 0.03f) e.fx.p.tapeOn = ! e.fx.p.tapeOn;
            if (rr.next() < 0.03f) e.fx.p.freezeOn = ! e.fx.p.freezeOn;
            if (rr.next() < 0.03f) e.fx.p.diveOn = ! e.fx.p.diveOn;
        }, 200.0);
        for (size_t i = 0; i < L.size(); ++i) { bad |= ! std::isfinite (L[i]) || ! std::isfinite (R[i]); peak = std::max (peak, std::abs (L[i])); }
        check (! bad && peak < 1.25f, "random switching: no NaN, no blow-ups");
    }

    // ---- Demo renders on the synthetic voice
    auto demo = [&] (const char* name, const std::function<void (Engine&, double)>& ctl, const char* preset = nullptr)
    {
        Engine e;
        if (preset) for (auto& pr : factoryPresets()) if (std::string (pr.name) == preset) for (auto& [id, val] : pr.values) setParam (e.params, id, val);
        e.prepare (SR, BLOCK); e.snapSmoothers();
        auto v = makeVoice (8.0f); auto L = v, R = v;
        run (e, L, R, [&] (double t) { ctl (e, t); }, 100.0);
        writeWav (dir + "/" + name + ".wav", L, R);
    };
    demo ("fx_stutter", [] (Engine& e, double t) { e.fx.p.stutterPattern = 0b0101101001011010; e.fx.p.stutterOn = t > 2.0; });
    demo ("fx_stutter_tails_haunting", [] (Engine& e, double t) { e.fx.p.stutterPattern = 0b1110111011101110; e.fx.p.stutterTails = true; e.fx.p.stutterOn = t > 2.0; }, "The Haunting");
    demo ("fx_repeat_sinking", [] (Engine& e, double t) { e.fx.p.repeatLength = 2; e.fx.p.repeatDrop = -1.5f; e.fx.p.repeatOn = (t > 2.0 && t < 4.0) || (t > 5.5 && t < 7.0); });
    demo ("fx_tapestop_demon", [] (Engine& e, double t) { e.fx.p.tapeTime = 1.2f; e.fx.p.tapeOn = (t > 2.5 && t < 4.5) || t > 6.0; }, "Demon");
    demo ("fx_freeze_cultchant", [] (Engine& e, double t) { e.fx.p.freezeMix = 0.8f; e.fx.p.freezeOn = t > 1.5 && t < 6.5; }, "Cult Chant");
    demo ("fx_dive", [] (Engine& e, double t) { e.fx.p.diveAmount = -14; e.fx.p.diveTime = 1.5f; e.fx.p.diveOn = (t > 2.0 && t < 4.5) || t > 6.0; });

    std::printf ("\n%s\n", ok ? "ALL OK" : "SOME CHECKS FAILED");
    return ok ? 0 : 1;
}
