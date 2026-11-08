// POSSESSED — Pure Vocal Destruction (voice transformer DSP engine).
// Self-contained (no JUCE) so it can be tested offline.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

namespace poss
{
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

inline float clampf (float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }
inline float semisToRatio (float s) { return std::pow (2.0f, s / 12.0f); }
inline float softClip (float x) { return std::tanh (x); }

struct Rng
{
    uint32_t s = 0x9E3779B9u;
    explicit Rng (uint32_t seed = 0x9E3779B9u) : s (seed ? seed : 1u) {}
    float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s >> 8) * (1.0f / 16777216.0f); }
    float bip() { return next() * 2.0f - 1.0f; }
};

// One-pole parameter smoother
struct Smooth
{
    float v = 0, t = 0, c = 0.001f;
    void setup (double sr, float ms) { c = 1.0f - std::exp (-1.0f / (float) (sr * ms * 0.001)); }
    void reset (float x) { v = t = x; }
    float next() { v += (t - v) * c; return v; }
};

// Smoothed random walk (for organic modulation)
struct Drift
{
    Rng rng; float cur = 0, target = 0, coef = 0.001f; int count = 0, period = 1000;
    void setup (double sr, float rateHz, uint32_t seed)
    {
        rng = Rng (seed); period = std::max (1, (int) (sr / std::max (0.01f, rateHz)));
        coef = 1.0f - std::exp (-1.0f / (float) period);
    }
    float next()
    {
        if (--count <= 0) { count = period; target = rng.bip(); }
        cur += (target - cur) * coef;
        return cur;
    }
};

// Topology-preserving state variable filter
struct SVF
{
    float ic1 = 0, ic2 = 0, g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0;
    void set (double sr, float hz, float q)
    {
        hz = clampf (hz, 10.0f, (float) sr * 0.45f);
        g = std::tan (kPi * hz / (float) sr); k = 1.0f / std::max (0.3f, q);
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    void reset() { ic1 = ic2 = 0; }
    // returns low, writes band/high
    inline void tick (float x, float& lp, float& bp, float& hp)
    {
        const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
    float lp (float x) { float l, b, h; tick (x, l, b, h); return l; }
    float bp (float x) { float l, b, h; tick (x, l, b, h); return b; }
    float hp (float x) { float l, b, h; tick (x, l, b, h); return h; }
};

struct OnePole
{
    float z = 0, a = 0;
    void set (double sr, float hz) { a = std::exp (-kTwoPi * hz / (float) sr); }
    float lp (float x) { z = x + a * (z - x); return z; }
    float hp (float x) { return x - lp (x); }
};

// Circular delay line with fractional (linear) read
struct DelayLine
{
    std::vector<float> buf; int w = 0, mask = 0;
    void prepare (int maxSamples)
    {
        int n = 1; while (n < maxSamples + 4) n <<= 1;
        buf.assign ((size_t) n, 0.0f); mask = n - 1; w = 0;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
    void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    float read (float delay) const // delay in samples, >= 1
    {
        const float rp = (float) w - delay;
        const int i = (int) std::floor (rp);
        const float f = rp - (float) i;
        const float a = buf[(size_t) (i & mask)], b = buf[(size_t) ((i + 1) & mask)];
        return a + (b - a) * f;
    }
    int size() const { return mask + 1; }
};

//==============================================================================
// Radix-2 complex FFT (unnormalised inverse)
class FFT
{
public:
    void init (int n)
    {
        N = n; int bits = 0; while ((1 << bits) < n) ++bits;
        rev.resize ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            int r = 0; for (int b = 0; b < bits; ++b) if (i & (1 << b)) r |= 1 << (bits - 1 - b);
            rev[(size_t) i] = r;
        }
        tw.resize ((size_t) n / 2);
        for (int i = 0; i < n / 2; ++i) tw[(size_t) i] = std::polar (1.0f, -kTwoPi * (float) i / (float) n);
    }
    void run (std::complex<float>* x, bool inverse) const
    {
        for (int i = 0; i < N; ++i) if (i < rev[(size_t) i]) std::swap (x[i], x[rev[(size_t) i]]);
        for (int len = 2; len <= N; len <<= 1)
        {
            const int half = len >> 1, step = N / len;
            for (int i = 0; i < N; i += len)
                for (int j = 0; j < half; ++j)
                {
                    auto w = tw[(size_t) (j * step)]; if (inverse) w = std::conj (w);
                    const auto u = x[i + j], v = x[i + j + half] * w;
                    x[i + j] = u + v; x[i + j + half] = u - v;
                }
        }
    }
private:
    int N = 0; std::vector<int> rev; std::vector<std::complex<float>> tw;
};

//==============================================================================
// Phase-vocoder voice: independent pitch and formant shifting (cepstral envelope),
// two synthesis voices (main + layer) from one analysis, and spectral "whisper".
class SpectralCore
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        N = sr > 60000.0 ? 4096 : 2048;
        osamp = 4; step = N / osamp; latency = N - step; half = N / 2;
        fft.init (N);
        window.resize ((size_t) N);
        for (int i = 0; i < N; ++i) window[(size_t) i] = 0.5f - 0.5f * std::cos (kTwoPi * (float) i / (float) N);
        inFifo.assign ((size_t) N, 0.0f); outFifo.assign ((size_t) N, 0.0f);
        accum.assign ((size_t) N * 2, 0.0f);
        buf.assign ((size_t) N, {}); cep.assign ((size_t) N, {}); work.assign ((size_t) N, {});
        const size_t H = (size_t) half + 1;
        lastPhase.assign (H, 0.0f); anaMag.assign (H, 0.0f); anaFreq.assign (H, 0.0f);
        flat.assign (H, 0.0f); env.assign (H, 0.0f); tmpMag.assign (H, 0.0f); tmpPh.assign (H, 0.0f);
        for (auto& v : voices) { v.synMag.assign (H, 0.0f); v.synFreq.assign (H, 0.0f); v.sumPhase.assign (H, 0.0f); v.agc = 1.0f; }
        expct = kTwoPi * (float) step / (float) N;
        lifter = std::max (12, (int) (sr * 0.0009)); // ~0.9 ms quefrency cut: keeps formants, drops harmonics
        rover = latency;
    }

    int getLatency() const { return N; } // measured: one full frame

    float pitch[2] { 1.0f, 0.5f }, formant[2] { 1.0f, 1.0f }, level[2] { 1.0f, 0.0f };
    float whisper = 0.0f;

    inline float process (float in)
    {
        inFifo[(size_t) rover] = in;
        const float out = outFifo[(size_t) (rover - latency)];
        if (++rover >= N) { rover = latency; frame(); }
        return out;
    }

private:
    struct Voice { std::vector<float> synMag, synFreq, sumPhase; float agc = 1.0f; };

    void frame()
    {
        for (int k = 0; k < N; ++k) buf[(size_t) k] = { inFifo[(size_t) k] * window[(size_t) k], 0.0f };
        fft.run (buf.data(), false);

        // --- analysis: magnitude + true frequency (in bins)
        for (int k = 0; k <= half; ++k)
        {
            const auto c = buf[(size_t) k];
            const float mag = std::abs (c), ph = std::atan2 (c.imag(), c.real());
            float d = ph - lastPhase[(size_t) k]; lastPhase[(size_t) k] = ph;
            d -= (float) k * expct;
            d -= kTwoPi * std::floor (d / kTwoPi + 0.5f);
            anaFreq[(size_t) k] = (float) k + (float) osamp * d / kTwoPi;
            anaMag[(size_t) k] = mag;
        }

        // --- spectral envelope via real cepstrum
        for (int k = 0; k <= half; ++k) cep[(size_t) k] = { std::log (anaMag[(size_t) k] + 1e-7f), 0.0f };
        for (int k = 1; k < half; ++k) cep[(size_t) (N - k)] = cep[(size_t) k];
        fft.run (cep.data(), true);
        const float invN = 1.0f / (float) N;
        for (int q = 0; q < N; ++q)
        {
            const int qq = q <= half ? q : N - q;
            float w = 0.0f;
            if (qq < lifter) w = 1.0f;
            else if (qq < lifter + 6) w = 0.5f + 0.5f * std::cos (kPi * (float) (qq - lifter) / 6.0f);
            cep[(size_t) q] = { cep[(size_t) q].real() * invN * w, 0.0f };
        }
        fft.run (cep.data(), false);
        for (int k = 0; k <= half; ++k)
        {
            env[(size_t) k] = std::exp (clampf (cep[(size_t) k].real(), -30.0f, 30.0f));
            flat[(size_t) k] = anaMag[(size_t) k] / (env[(size_t) k] + 1e-9f);
        }

        // --- synthesis for each voice, summed into one spectrum
        for (auto& c : work) c = {};
        const float w = clampf (whisper, 0.0f, 1.0f);
        for (int vi = 0; vi < 2; ++vi)
        {
            if (level[vi] <= 1e-4f) continue;
            auto& v = voices[(size_t) vi];
            std::fill (v.synMag.begin(), v.synMag.end(), 0.0f);
            std::fill (v.synFreq.begin(), v.synFreq.end(), 0.0f);
            const float p = pitch[vi];
            for (int k = 0; k <= half; ++k)
            {
                const int idx = (int) ((float) k * p + 0.5f);
                if (idx > half) break;
                if (flat[(size_t) k] > v.synMag[(size_t) idx]) // keep the strongest contributor
                {
                    v.synMag[(size_t) idx] = flat[(size_t) k];
                    v.synFreq[(size_t) idx] = anaFreq[(size_t) k] * p;
                }
            }
            const float fr = formant[vi];
            const bool identity = std::abs (p - 1.0f) < 1e-4f; // no pitch shift: keep analysis phases (transparent)
            double inE = 0, outE = 0;
            for (int k = 0; k <= half; ++k)
            {
                // formant-shifted envelope
                const float x = (float) k / fr;
                float e;
                if (x >= (float) half) e = env[(size_t) half] * 0.1f;
                else { const int i0 = (int) x; const float f = x - (float) i0; e = env[(size_t) i0] + (env[(size_t) i0 + 1] - env[(size_t) i0]) * f; }

                float ph;
                if (identity)
                {
                    ph = lastPhase[(size_t) k];
                    v.sumPhase[(size_t) k] = ph;
                }
                else
                {
                    const float dev = v.synFreq[(size_t) k] - (float) k;
                    v.sumPhase[(size_t) k] += kTwoPi * dev / (float) osamp + (float) k * expct;
                    v.sumPhase[(size_t) k] -= kTwoPi * std::floor (v.sumPhase[(size_t) k] / kTwoPi);
                    ph = v.sumPhase[(size_t) k];
                }
                const float m = v.synMag[(size_t) k] * (1.0f - w) + w * (0.5f + rng.next());
                ph += w * kPi * rng.bip();
                const float a = m * e;
                tmpMag[(size_t) k] = a; tmpPh[(size_t) k] = ph;
                inE += (double) anaMag[(size_t) k] * anaMag[(size_t) k];
                outE += (double) a * a;
            }
            // auto-gain: each voice comes out as loud as the input (smoothed across frames)
            const float target = outE > 1e-12 ? (float) std::sqrt (inE / outE) : 1.0f;
            v.agc += (clampf (target, 0.1f, 12.0f) - v.agc) * (inE > 1e-8 ? 0.35f : 0.0f);
            const float gain = level[vi] * v.agc;
            for (int k = 0; k <= half; ++k) work[(size_t) k] += std::polar (tmpMag[(size_t) k] * gain, tmpPh[(size_t) k]);
        }
        work[0] = { work[0].real(), 0.0f };
        work[(size_t) half] = { work[(size_t) half].real(), 0.0f };
        for (int k = 1; k < half; ++k) work[(size_t) (N - k)] = std::conj (work[(size_t) k]);
        fft.run (work.data(), true);

        const float scale = 1.0f / ((float) N * 1.5f); // hann^2 at 4x overlap sums to 1.5
        for (int k = 0; k < N; ++k) accum[(size_t) k] += window[(size_t) k] * work[(size_t) k].real() * scale;
        for (int k = 0; k < step; ++k) outFifo[(size_t) k] = accum[(size_t) k];
        std::move (accum.begin() + step, accum.begin() + step + N, accum.begin());
        std::fill (accum.begin() + N, accum.end(), 0.0f);
        std::move (inFifo.begin() + step, inFifo.end(), inFifo.begin());
    }

    double sr = 48000;
    int N = 2048, osamp = 4, step = 512, latency = 1536, half = 1024, rover = 1536, lifter = 40;
    float expct = 0;
    FFT fft; Rng rng { 0xC0FFEEu };
    std::vector<float> window, inFifo, outFifo, accum, lastPhase, anaMag, anaFreq, flat, env, tmpMag, tmpPh;
    std::vector<std::complex<float>> buf, cep, work;
    std::array<Voice, 2> voices;
};

//==============================================================================
struct Params
{
    float inputDb = 0;
    float pitch = 0, formant = 0, whisper = 0;
    float layerPitch = -12, layerFormant = 0, layerLevel = 0;
    float growl = 0, rasp = 0;
    float ringMix = 0, ringFreq = 80;
    int distType = 0; float distDrive = 0;
    int filterType = 0; float cutoff = 2000, resonance = 0.2f;
    float wobble = 0;
    float legion = 0, legionSpread = 0.5f;
    float reverseMix = 0, reverseLength = 0.5f;
    float delayMix = 0, delayTime = 350, delayFeedback = 0.4f; int delayType = 1; // Digital, Ping-Pong, Analog, Tape
    float reverbMix = 0, reverbSize = 0.6f;
    float lofi = 0, lofiNoise = 0;
    float mix = 1, outputDb = 0;
};

class Engine
{
public:
    Params params;

    int getLatency() const { return core.getLatency(); }

    void prepare (double sampleRate, int /*maxBlock*/)
    {
        sr = sampleRate;
        core.prepare (sr);
        const int lat = core.getLatency();
        dryL.prepare (lat + 8); dryR.prepare (lat + 8);
        agcCoef = 1.0f - std::exp (-1.0f / (float) (sr * 0.12)); agcIn = agcOut = 0; agcGain = 1;

        for (auto* s : { &sIn, &sOut, &sMix, &sGrowl, &sRasp, &sRing, &sDrive, &sCut, &sRes, &sWob,
                         &sLeg, &sSpread, &sRev, &sDelMix, &sDelTime, &sDelFb, &sVerbMix, &sLayer, &sLofi, &sNoise })
            s->setup (sr, 30.0f);
        sDelTime.setup (sr, 250.0f);
        snapSmoothers();

        trackLP.set (sr, 280.0f, 0.7f); trackLP.reset();
        trackLP2.set (sr, 280.0f, 0.7f); trackLP2.reset();
        subLP.set (sr, 1100.0f, 0.7f); subLP.reset();
        sub2LP.set (sr, 700.0f, 0.7f); sub2LP.reset();
        envAttack = 1.0f - std::exp (-1.0f / (float) (sr * 0.002));
        envRelease = 1.0f - std::exp (-1.0f / (float) (sr * 0.06));
        burstDecay = std::exp (-1.0f / (float) (sr * 0.004));
        creakAttack = 1.0f - std::exp (-1.0f / (float) (sr * 0.0007));
        raspLP.set (sr, 7500.0f); creakKick = burst = 0;
        raspNoiseLP.set (sr, 90.0f);

        radioHP.set (sr, 450.0f, 0.7f); radioLP.set (sr, 2800.0f, 0.9f);
        fuzzLP.set (sr, 6500.0f); satLP.set (sr, 11000.0f);
        mainFilter.reset();

        wobDelay.prepare ((int) (sr * 0.05));
        wobDrift.setup (sr, 6.0f, 0x1234u); wobDrift2.setup (sr, 0.9f, 0x4321u);

        legionDelay.prepare ((int) (sr * 0.12));
        for (int i = 0; i < kLegion; ++i) legionDrift[(size_t) i].setup (sr, 0.25f + 0.11f * (float) i, 0x51u + (uint32_t) i * 977u);

        const int revMax = (int) (sr * 1.6 * 2.2) + 16;
        revBufL.prepare (revMax); revBufR.prepare (revMax);
        revPhase = 0.0f;

        delL.prepare ((int) (sr * 1.6)); delR.prepare ((int) (sr * 1.6));
        delLP_L.set (sr, 4500.0f); delLP_R.set (sr, 4500.0f);
        delHP_L.set (sr, 120.0f); delHP_R.set (sr, 120.0f);
        delWow.setup (sr, 0.7f, 0x77u); delFlutter.setup (sr, 9.0f, 0x99u); delDrift.setup (sr, 0.35f, 0x55u);
        lastDelayType = -1;

        lofiDelL.prepare ((int) (sr * 0.02)); lofiDelR.prepare ((int) (sr * 0.02));
        lofiWow.setup (sr, 0.55f, 0x3141u); lofiFlutter.setup (sr, 7.0f, 0x2718u);
        for (auto* f : { &lofiHP[0], &lofiHP[1], &lofiLP[0], &lofiLP[1] }) f->reset();
        hissLP.set (sr, 5000.0f); crackleEnv = 0; lofiHold = 0; lofiHeld[0] = lofiHeld[1] = 0;

        static const int baseLens[8] = { 1559, 1867, 2053, 2251, 2399, 2617, 2797, 3061 };
        for (int i = 0; i < 8; ++i)
        {
            fdn[(size_t) i].prepare ((int) (baseLens[i] * 2.2 * sr / 48000.0) + 64);
            fdnLen[(size_t) i] = (float) baseLens[i] * (float) (sr / 48000.0);
            fdnDamp[(size_t) i].set (sr, 6000.0f);
            fdnDrift[(size_t) i].setup (sr, 0.3f + 0.07f * (float) i, 0xABCu + (uint32_t) i * 131u);
        }
        verbPre.prepare ((int) (sr * 0.05));
        verbHP.set (sr, 120.0f);
    }

    void snapSmoothers()
    {
        const auto& p = params;
        sIn.reset (dbToGain (p.inputDb)); sOut.reset (dbToGain (p.outputDb)); sMix.reset (p.mix);
        sGrowl.reset (p.growl); sRasp.reset (p.rasp); sRing.reset (p.ringMix); sDrive.reset (p.distDrive);
        sCut.reset (p.cutoff); sRes.reset (p.resonance); sWob.reset (p.wobble); sLeg.reset (p.legion);
        sSpread.reset (p.legionSpread); sRev.reset (p.reverseMix); sDelMix.reset (p.delayMix);
        sDelTime.reset (p.delayTime); sDelFb.reset (p.delayFeedback); sVerbMix.reset (p.reverbMix); sLayer.reset (p.layerLevel);
        sLofi.reset (p.lofi); sNoise.reset (p.lofiNoise);
    }

    // In-place stereo processing. R may equal L for mono.
    void process (float* L, float* R, int n)
    {
        const auto& p = params;
        core.pitch[0] = semisToRatio (clampf (p.pitch, -24, 24));
        core.pitch[1] = semisToRatio (clampf (p.layerPitch, -24, 24));
        core.formant[0] = semisToRatio (clampf (p.formant, -12, 12));
        core.formant[1] = semisToRatio (clampf (p.layerFormant, -12, 12));
        core.whisper = p.whisper;

        sIn.t = dbToGain (p.inputDb); sOut.t = dbToGain (p.outputDb); sMix.t = p.mix;
        sGrowl.t = p.growl; sRasp.t = p.rasp; sRing.t = p.ringMix; sDrive.t = p.distDrive;
        sCut.t = p.cutoff; sRes.t = p.resonance; sWob.t = p.wobble; sLeg.t = p.legion; sSpread.t = p.legionSpread;
        sRev.t = p.reverseMix; sDelMix.t = p.delayMix; sDelTime.t = p.delayTime; sDelFb.t = p.delayFeedback;
        sVerbMix.t = p.reverbMix; sLayer.t = p.layerLevel; sLofi.t = p.lofi; sNoise.t = p.lofiNoise;

        const float ringInc = kTwoPi * clampf (p.ringFreq, 1.0f, 5000.0f) / (float) sr;
        const float revLen = clampf (p.reverseLength, 0.1f, 1.5f) * (float) sr;
        const float revInc = 1.0f / revLen;
        // reverb decay from size: RT60 ~ 0.6 s .. 14 s
        const float rt60 = 0.6f * std::pow (23.0f, clampf (p.reverbSize, 0, 1));
        const float sizeScale = 0.55f + 1.5f * clampf (p.reverbSize, 0, 1);
        std::array<float, 8> fdnGain;
        for (int i = 0; i < 8; ++i)
            fdnGain[(size_t) i] = std::pow (10.0f, -3.0f * fdnLen[(size_t) i] * sizeScale / ((float) sr * rt60));

        if (p.delayType != lastDelayType)
        {
            lastDelayType = p.delayType;
            const float lpHz = p.delayType == 2 ? 2300.0f : p.delayType == 3 ? 4200.0f : p.delayType == 1 ? 6500.0f : 12000.0f;
            delLP_L.set (sr, lpHz); delLP_R.set (sr, lpHz);
        }
        const float lofiAmt = clampf (p.lofi, 0, 1);
        for (int c = 0; c < 2; ++c)
        {
            lofiHP[c].set (sr, 20.0f * std::pow (17.0f, lofiAmt), 0.7f);
            lofiLP[c].set (sr, 19000.0f * std::pow (2600.0f / 19000.0f, lofiAmt), 0.75f + 0.5f * lofiAmt);
        }
        const float lofiHoldLen = 1.0f + ((float) sr / 6500.0f - 1.0f) * std::pow (clampf ((lofiAmt - 0.3f) / 0.7f, 0, 1), 1.6f);
        const float lofiLevels = std::pow (2.0f, 15.0f - 9.5f * std::pow (lofiAmt, 1.3f));

        const bool stereo = (R != L);
        for (int i = 0; i < n; ++i)
        {
            const float inGain = sIn.next();
            const float xL = L[i] * inGain, xR = (stereo ? R[i] : L[i]) * inGain;
            dryL.push (xL); dryR.push (xR);
            const float lat = (float) core.getLatency();
            const float dL = dryL.read (lat + 1.0f), dR = dryR.read (lat + 1.0f);

            // ---------------- spectral voice (mono)
            core.level[1] = sLayer.next();
            const float monoIn = 0.5f * (xL + xR);
            float w = core.process (monoIn);
            // loudness match: core output follows the (latency-aligned) input level
            {
                const float ref = 0.5f * (dL + dR);
                agcIn += (ref * ref - agcIn) * agcCoef;
                agcOut += (w * w - agcOut) * agcCoef;
                if (agcIn > 1e-9f && agcOut > 1e-12f)
                {
                    const float target = clampf (std::sqrt (agcIn / agcOut), 0.3f, 4.0f);
                    agcGain += (target - agcGain) * agcCoef * 0.5f;
                }
                w *= agcGain;
            }

            // ---------------- growl: sub-octave dividers
            const float g = sGrowl.next();
            {
                const float a = std::abs (w);
                envF += (a - envF) * (a > envF ? envAttack : envRelease);
                const float t = trackLP.lp (trackLP2.lp (w));
                const float thr = 0.05f * envF + 1e-5f;
                if (! zcHigh && t > thr) { zcHigh = true; flip1 = -flip1; if (flip1 > 0) flip2 = -flip2; }
                else if (zcHigh && t < -thr) zcHigh = false;
                if (g > 1e-4f)
                {
                    const float sub1 = subLP.lp (w * flip1);
                    const float sub2 = sub2LP.lp (w * flip2);
                    const float g2 = clampf ((g - 0.45f) / 0.55f, 0, 1);
                    w = w * (1.0f - 0.35f * g) + g * 1.6f * (sub1 + g2 * 0.9f * sub2);
                }
            }

            // ---------------- rasp: creaky vocal-fry pulses + rough AM
            // Each creak is a smooth bump (fast swell, short decay) rather than an instant jump, so it
            // sounds like vocal cords rattling instead of clicks.
            const float r = sRasp.next();
            {
                if (--creakCount <= 0)
                {
                    creakCount = (int) (sr / (38.0f + 34.0f * rng.next()));
                    creakKick = 0.6f + 0.4f * rng.next();
                }
                creakKick *= burstDecay;
                burst += (creakKick - burst) * creakAttack;
                if (r > 1e-4f)
                {
                    const float rough = raspNoiseLP.lp (rng.bip()) * 4.0f;
                    const float creak = w * (0.15f + 2.6f * burst) * (1.0f + 0.5f * rough);
                    float wet = softClip (creak * (1.3f + 1.6f * r)) * (0.6f + 0.2f * r);
                    wet = raspLP.lp (wet);
                    w = w + (wet - w) * r;
                }
            }

            // ---------------- ring modulator
            const float rm = sRing.next();
            ringPh += ringInc; if (ringPh > kTwoPi) ringPh -= kTwoPi;
            if (rm > 1e-4f) w = w * (1.0f - rm) + w * std::sin (ringPh) * rm * 1.3f;

            // ---------------- distortion
            const float dr = sDrive.next();
            if (dr > 1e-4f) w = distort (w, dr, p.distType);

            // ---------------- filter
            {
                const float cut = sCut.next(), res = sRes.next();
                if (p.filterType > 0)
                {
                    if (++filterCount >= 16) { filterCount = 0; mainFilter.set (sr, cut, 0.6f + res * 11.0f); }
                    float lo, bp, hi; mainFilter.tick (w, lo, bp, hi);
                    w = p.filterType == 1 ? lo : p.filterType == 2 ? bp * (1.0f + res) : hi;
                }
            }

            // ---------------- wobble (tape warble / seasick pitch)
            const float wob = sWob.next();
            wobDelay.push (w);
            const float wd1 = wobDrift.next(), wd2 = wobDrift2.next();
            if (wob > 1e-4f)
            {
                const float depth = wob * (float) sr * 0.006f;
                const float d = (float) sr * 0.008f + depth * (0.55f + 0.3f * wd2 + 0.15f * wd1);
                const float wv = wobDelay.read (std::max (2.0f, d));
                w = w + (wv - w) * clampf (wob * 2.0f, 0, 1);
            }

            // ---------------- legion (crowd of detuned voices) -> stereo
            float sL = w, sR = w;
            const float leg = sLeg.next(), spread = sSpread.next();
            legionDelay.push (w);
            if (leg > 1e-4f)
            {
                float accL = 0, accR = 0;
                for (int v = 0; v < kLegion; ++v)
                {
                    const float base = (0.011f + 0.0085f * (float) v) * (float) sr;
                    const float mod = legionDrift[(size_t) v].next() * (0.0015f + 0.009f * spread) * (float) sr;
                    const float tap = legionDelay.read (std::max (2.0f, base + mod));
                    const float pan = (v % 2 == 0 ? -1.0f : 1.0f) * (0.4f + 0.6f * (float) (v / 2) / 2.0f);
                    accL += tap * (0.5f - 0.5f * pan); accR += tap * (0.5f + 0.5f * pan);
                }
                const float lg = leg * 0.55f;
                sL = w * (1.0f - 0.3f * leg) + accL * lg;
                sR = w * (1.0f - 0.3f * leg) + accR * lg;
            }

            // ---------------- reverse echoes (two windowed backward-reading grains)
            const float rv = sRev.next();
            {
                const float fb = 0.35f * rv;
                revBufL.push (sL + lastRevL * fb); revBufR.push (sR + lastRevR * fb);
                revPhase += revInc; if (revPhase >= 1.0f) revPhase -= 1.0f;
                float oL = 0, oR = 0;
                for (int gI = 0; gI < 2; ++gI)
                {
                    float ph = revPhase + 0.5f * (float) gI; if (ph >= 1.0f) ph -= 1.0f;
                    const float win = std::sin (kPi * ph);
                    const float d = 2.0f * ph * revLen + 2.0f;
                    oL += revBufL.read (d) * win;
                    oR += revBufR.read (d * 1.03f + 3.0f) * win;
                }
                lastRevL = oL * 0.7071f; lastRevR = oR * 0.7071f;
                if (rv > 1e-4f) { sL += lastRevL * rv * 1.2f; sR += lastRevR * rv * 1.2f; }
            }

            // ---------------- delay (Digital / Ping-Pong / Analog / Tape)
            {
                const float dm = sDelMix.next(), dt = sDelTime.next(), dfb = sDelFb.next();
                const float ds = std::max (2.0f, dt * 0.001f * (float) sr);
                const int type = p.delayType;
                float mod = 0.0f;
                const float wow = delWow.next(), flut = delFlutter.next(), drift = delDrift.next();
                if (type == 2) mod = drift * 0.0007f * (float) sr;                                  // BBD clock drift
                if (type == 3) mod = (wow * 0.0024f + flut * 0.00025f) * (float) sr;               // tape wow + flutter
                const float tL = delL.read (std::max (2.0f, ds + mod));
                const float tR = delR.read (std::max (2.0f, ds + mod * 1.07f + (type == 0 ? 0.013f * (float) sr : 0.0f)));
                const float in = 0.5f * (sL + sR);
                if (type == 1) // ping-pong
                {
                    delL.push (in + delLP_R.lp (tR) * dfb);
                    delR.push (delLP_L.lp (tL) * dfb);
                }
                else if (type == 0) // clean digital, slightly widened
                {
                    delL.push (in + delLP_L.lp (tL) * dfb);
                    delR.push (in + delLP_R.lp (tR) * dfb);
                }
                else // analog / tape: darker each repeat and saturating as it builds up
                {
                    const float drive = type == 2 ? 1.4f : 1.9f;
                    float fL = delLP_L.lp (tL) * dfb, fR = delLP_R.lp (tR) * dfb;
                    if (type == 3) { fL = delHP_L.hp (fL); fR = delHP_R.hp (fR); }
                    delL.push (std::tanh ((in + fL) * drive) / drive + (type == 2 ? rng.bip() * 0.0004f : 0.0f));
                    delR.push (std::tanh ((in + fR) * drive) / drive);
                }
                sL += tL * dm; sR += tR * dm;
            }

            // ---------------- FDN reverb
            {
                const float vm = sVerbMix.next();
                verbPre.push (verbHP.hp (0.5f * (sL + sR)));
                if (vm > 1e-4f || verbActive > 0)
                {
                    const float inV = verbPre.read ((float) sr * 0.02f);
                    std::array<float, 8> o;
                    for (int k = 0; k < 8; ++k)
                    {
                        const float len = fdnLen[(size_t) k] * sizeScale + fdnDrift[(size_t) k].next() * 12.0f;
                        o[(size_t) k] = fdnDamp[(size_t) k].lp (fdn[(size_t) k].read (len)) * fdnGain[(size_t) k];
                    }
                    // 8x8 Hadamard (fast)
                    float h[8]; for (int k = 0; k < 8; ++k) h[k] = o[(size_t) k];
                    for (int len = 1; len < 8; len <<= 1)
                        for (int a = 0; a < 8; a += len * 2)
                            for (int b = a; b < a + len; ++b) { const float u = h[b], v = h[b + len]; h[b] = u + v; h[b + len] = u - v; }
                    const float norm = 0.35355339f;
                    for (int k = 0; k < 8; ++k) fdn[(size_t) k].push (h[k] * norm + inV * ((k & 1) ? 0.5f : -0.5f));
                    const float vL = (o[0] + o[2] + o[4] + o[6]) * 0.6f, vR = (o[1] + o[3] + o[5] + o[7]) * 0.6f;
                    sL = sL * (1.0f - 0.35f * vm) + vL * vm * 1.2f;
                    sR = sR * (1.0f - 0.35f * vm) + vR * vm * 1.2f;
                    verbActive = vm > 1e-4f ? (int) (sr * rt60) : verbActive - 1;
                }
            }

            // ---------------- mix + output
            const float m = sMix.next(), og = sOut.next();
            float outL = (dL * (1.0f - m) + sL * m) * og;
            float outR = (dR * (1.0f - m) + sR * m) * og;
            // ---------------- global Lo-Fi (worn-out tape / cheap recorder) + noise
            const float lf = sLofi.next(), nz = sNoise.next();
            if (lf > 1e-4f)
            {
                lofiDelL.push (outL); lofiDelR.push (outR);
                const float d = (0.004f + lf * (0.0018f * lofiWow.next() + 0.00025f * lofiFlutter.next())) * (float) sr;
                float y[2] = { lofiDelL.read (d), lofiDelR.read (d) };
                const float lg = 1.0f + 2.5f * lf;
                lofiHold += 1.0f;
                const bool sample = lofiHold >= lofiHoldLen;
                if (sample) lofiHold -= lofiHoldLen;
                for (int c = 0; c < 2; ++c)
                {
                    float v = lofiLP[c].lp (lofiHP[c].hp (y[c]));
                    v = std::tanh (v * lg) / lg * (1.0f + 0.6f * lf);
                    if (sample) lofiHeld[c] = std::round (v * lofiLevels) / lofiLevels;
                    y[c] = lofiHeld[c];
                }
                const float wet = clampf (lf * 4.0f, 0, 1); // fade in so tiny amounts don't click
                outL += (y[0] - outL) * wet; outR += (y[1] - outR) * wet;
            }
            if (nz > 1e-4f)
            {
                const float hiss = hissLP.lp (rng.bip()) * 0.012f * nz;
                if (rng.next() < 0.00025f * nz) crackleEnv = (0.3f + 0.5f * rng.next()) * (rng.next() < 0.5f ? -1.0f : 1.0f);
                crackleEnv *= 0.8f;
                const float noiseS = hiss + crackleEnv * 0.25f * nz;
                outL += noiseS; outR += noiseS * 0.9f + hissLP.z * 0.002f * nz;
            }
            outL = safety (outL); outR = safety (outR);
            L[i] = outL; if (stereo) R[i] = outR; else L[i] = 0.5f * (outL + outR);
        }
    }

private:
    static float safety (float x)
    {
        if (! std::isfinite (x)) return 0.0f;
        // transparent below ~0.9, gently rounds peaks to stay under ~1.2
        const float a = std::abs (x);
        if (a < 0.9f) return x;
        const float y = 0.9f + 0.3f * std::tanh ((a - 0.9f) / 0.3f);
        return x < 0 ? -y : y;
    }

    float distort (float x, float drive, int type)
    {
        const float gain = std::pow (40.0f, drive);
        switch (type)
        {
            default:
            case 0: // Tube: asymmetric tanh
            {
                const float bias = 0.25f * drive;
                const float y = std::tanh (x * gain + bias) - std::tanh (bias);
                return y / std::tanh (1.0f + gain * 0.25f) * 0.8f;
            }
            case 1: // Fuzz: hard clip + gated octave-ish rectification
            {
                float y = clampf (x * gain * 2.0f, -1.0f, 1.0f);
                y = y * (1.0f - 0.35f * drive) + std::abs (y) * 0.35f * drive;
                return fuzzLP.lp (y) * 0.6f;
            }
            case 2: // Bitcrush
            {
                const float hold = 1.0f + drive * drive * 40.0f;
                crushCount += 1.0f;
                if (crushCount >= hold) { crushCount -= hold; const float levels = std::pow (2.0f, 12.0f - drive * 9.0f); crushHeld = std::round (x * levels) / levels; }
                return crushHeld;
            }
            case 4: // Saturate: warm tape-style saturation that keeps the voice intact
            {
                const float g2 = 1.0f + drive * 7.0f;
                const float y = std::tanh (x * g2 + 0.08f * drive) - std::tanh (0.08f * drive);
                return satLP.lp (y / std::tanh (g2) * (0.9f + 0.4f * drive) * 0.75f);
            }
            case 3: // Radio from beyond: band-limited, clipped, crackling
            {
                float y = radioLP.lp (radioHP.hp (x));
                y = std::tanh (y * gain * 1.5f) * 0.6f;
                const float voiced = clampf (envF * 25.0f, 0.0f, 1.0f); // static rides on the voice, silence stays clean
                if (rng.next() < 0.0006f * drive * voiced) crackle = rng.bip() * 0.4f;
                crackle *= 0.97f;
                return y + (crackle + rng.bip() * 0.004f * voiced) * drive;
            }
        }
    }

    double sr = 48000;
    SpectralCore core;
    float agcIn = 0, agcOut = 0, agcGain = 1, agcCoef = 0.0005f;
    DelayLine dryL, dryR;
    Smooth sLofi, sNoise, sIn, sOut, sMix, sGrowl, sRasp, sRing, sDrive, sCut, sRes, sWob, sLeg, sSpread, sRev, sDelMix, sDelTime, sDelFb, sVerbMix, sLayer;
    Rng rng { 0xDEADBEEFu };

    // growl
    SVF trackLP, trackLP2, subLP, sub2LP;
    float envF = 0, envAttack = 0.01f, envRelease = 0.001f, flip1 = 1, flip2 = 1; bool zcHigh = false;
    // rasp
    int creakCount = 0; float burst = 0, creakKick = 0, creakAttack = 0.05f, burstDecay = 0.99f; OnePole raspNoiseLP, raspLP;
    // ring
    float ringPh = 0;
    // distortion
    SVF radioHP, radioLP; OnePole fuzzLP, satLP; float crushCount = 0, crushHeld = 0, crackle = 0;
    // filter
    SVF mainFilter; int filterCount = 99;
    // wobble
    DelayLine wobDelay; Drift wobDrift, wobDrift2;
    // legion
    static constexpr int kLegion = 6;
    DelayLine legionDelay; std::array<Drift, kLegion> legionDrift;
    // reverse
    DelayLine revBufL, revBufR; float revPhase = 0, lastRevL = 0, lastRevR = 0;
    // delay
    DelayLine delL, delR; OnePole delLP_L, delLP_R, delHP_L, delHP_R; Drift delWow, delFlutter, delDrift; int lastDelayType = -1;
    // lo-fi
    DelayLine lofiDelL, lofiDelR; Drift lofiWow, lofiFlutter; SVF lofiHP[2], lofiLP[2]; OnePole hissLP;
    float crackleEnv = 0, lofiHold = 0, lofiHeld[2] {};
    // reverb
    std::array<DelayLine, 8> fdn; std::array<float, 8> fdnLen {}; std::array<OnePole, 8> fdnDamp; std::array<Drift, 8> fdnDrift;
    DelayLine verbPre; OnePole verbHP; int verbActive = 0;
};
} // namespace poss
