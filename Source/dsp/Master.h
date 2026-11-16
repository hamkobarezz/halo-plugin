#pragma once
#include "Dynamics.h"
#include "Drive.h"

namespace halo
{
//==============================================================================
// Look-ahead brickwall limiter. Sliding-window minimum + box smoothing guarantees
// the gain is already down when a peak arrives (no overshoot), release is exponential.
class LookaheadLimiter
{
public:
    void prepare (int lookahead, double osFs)
    {
        L = std::max (2, lookahead);
        fs = osFs;
        cap = juce::nextPowerOfTwo (L + 2);
        dqVal.assign ((size_t) cap, 1.0f);
        dqIdx.assign ((size_t) cap, 0);
        box.assign ((size_t) L, 1.0f);
        dly[0].assign ((size_t) (L - 1), 0.0f);
        dly[1].assign ((size_t) (L - 1), 0.0f);
        reset();
    }
    void reset()
    {
        head = tail = 0; t = 0; env = 1; boxSum = L; boxPos = 0; dPos = 0;
        std::fill (box.begin(), box.end(), 1.0f);
        std::fill (dly[0].begin(), dly[0].end(), 0.0f);
        std::fill (dly[1].begin(), dly[1].end(), 0.0f);
    }
    void setRelease (float ms) { relC = timeCoef (ms * 0.001, fs); }

    // threshold is linear; returns gain applied
    inline float process (float& l, float& r, float threshold) noexcept
    {
        const float pk = std::max (std::abs (l), std::abs (r));
        const float req = pk > threshold ? threshold / pk : 1.0f;

        // sliding minimum over the last L requirements
        while (head != tail && dqVal[(size_t) ((tail - 1) & (cap - 1))] >= req) --tail;
        dqVal[(size_t) (tail & (cap - 1))] = req;
        dqIdx[(size_t) (tail & (cap - 1))] = t;
        ++tail;
        while (dqIdx[(size_t) (head & (cap - 1))] <= t - L) ++head;
        const float held = dqVal[(size_t) (head & (cap - 1))];
        ++t;

        env = 1.0f - (1.0f - env) * relC;
        env = std::min (env, held);

        boxSum += (double) env - (double) box[(size_t) boxPos];
        box[(size_t) boxPos] = env;
        boxPos = (boxPos + 1) % L;
        const float g = std::min (1.0f, (float) (boxSum / L));

        const float dl = dly[0][(size_t) dPos], dr = dly[1][(size_t) dPos];
        dly[0][(size_t) dPos] = l; dly[1][(size_t) dPos] = r;
        dPos = (dPos + 1) % (L - 1);
        l = dl; r = dr;
        return g;
    }

private:
    int L = 2, cap = 4;
    double fs = 44100;
    std::vector<float> dqVal, box, dly[2];
    std::vector<int64_t> dqIdx;
    int64_t head = 0, tail = 0, t = 0;
    float env = 1, relC = 0.999f;
    double boxSum = 2;
    int boxPos = 0, dPos = 0;
};

//==============================================================================
// MASTER: tilt EQ → bass mono → width → glue compressor → loudness drive →
// oversampled true-peak-safe look-ahead limiter.
class MasterSection
{
public:
    void prepare (double sr, int maxBlock)
    {
        fs = sr;
        bank.prepare (maxBlock);
        glue.prepare (sr);
        widthS.reset (sr, 0.05); driveS.reset (sr, 0.05); enableS.reset (sr, 0.02);
        lookBase = std::max (1, (int) std::round (0.0015 * fs));
        for (int q = 0; q < 4; ++q) lim[q].prepare (lookBase * (1 << q) + 1, fs * (1 << q));
        lastTilt = lastMono = -999;
        reset();
    }
    void reset()
    {
        glue.reset();
        for (auto& x : lim) x.reset();
        for (int c = 0; c < 2; ++c) { loS[c].reset(); hiS[c].reset(); }
        side1.reset(); side2.reset();
    }

    void setQuality (int q) { if (q != quality) { quality = q; bank.setQuality (q); lim[q].reset(); } }
    float latency() const { return bank.latency() + (float) lookBase; }
    float latencyFor (int q) const { return OversamplerBank::latencyFor (bank, q) + (float) lookBase; }

    void set (bool enabled, float tiltDb, float width01x2, float monoHz, float glue01, float loudDb, float ceilingDb, float releaseMs)
    {
        on = enabled;
        if (! enabled) { tiltDb = 0; width01x2 = 1; monoHz = 20; glue01 = 0; loudDb = 0; }
        if (std::abs (tiltDb - lastTilt) > 0.001f)
        {
            lastTilt = tiltDb;
            loF = rbj::lowShelf (fs, 650.0, 0.5, -tiltDb * 0.5);
            hiF = rbj::highShelf (fs, 650.0, 0.5, tiltDb * 0.5);
        }
        monoOn = monoHz > 20.5f;
        if (monoOn && std::abs (monoHz - lastMono) > 0.5f) { lastMono = monoHz; sideHp = rbj::highpass (fs, monoHz, 0.7071); }
        glueAmt = glue01;
        glue.configure (-glue01 * 20.0f, 2.0f + 2.0f * glue01, 6.0f, 10.0f, 160.0f, false, true, 90.0f);
        glueMakeup = dbToGain (-glue.curve (-10.0f) * 0.6f);
        widthS.setTargetValue (width01x2);
        driveS.setTargetValue (dbToGain (loudDb));
        enableS.setTargetValue (enabled ? 1.0f : 0.0f);
        ceiling = dbToGain (ceilingDb);
        for (auto& x : lim) x.setRelease (releaseMs);
    }

    void process (float* l, float* r, int n) noexcept
    {
        float minG = 0;
        for (int i = 0; i < n; ++i)
        {
            float a = l[i], b = r[i];
            if (std::abs (lastTilt) > 0.001f)
            {
                a = hiS[0].process (hiF, loS[0].process (loF, a));
                b = hiS[1].process (hiF, loS[1].process (loF, b));
            }
            float mid = 0.5f * (a + b), side = 0.5f * (a - b);
            if (monoOn) side = side2.process (sideHp, side1.process (sideHp, side));
            side *= widthS.getNextValue();
            a = mid + side; b = mid - side;

            if (glueAmt > 0.0005f)
            {
                const float e = glue.process (a, b);
                minG = std::min (minG, e);
                const float g = dbToGain (e) * glueMakeup;
                a *= g; b *= g;
            }
            const float d = driveS.getNextValue();
            l[i] = a * d; r[i] = b * d;
        }
        glueGr = -minG;

        // limiter in the oversampled domain (catches inter-sample peaks)
        float minLim = 1.0f, pk = 0.0f;
        const float ceil = ceiling;
        auto& lm = lim[quality];
        const float e0 = enableS.getCurrentValue();
        for (int i = 0; i < n; ++i) enableS.getNextValue();
        const float e1 = enableS.getCurrentValue();
        bank.process (l, r, n, [&] (float* ul, float* ur, int un, int)
        {
            const float inv = 1.0f / (float) un;
            for (int i = 0; i < un; ++i)
            {
                const float en = e0 + (e1 - e0) * (float) i * inv;
                float a = ul[i], b = ur[i];
                float g = lm.process (a, b, ceil);
                g = 1.0f + (g - 1.0f) * en;
                minLim = std::min (minLim, g);
                a *= g; b *= g;
                if (en > 0.5f) { a = std::clamp (a, -ceil, ceil); b = std::clamp (b, -ceil, ceil); }
                pk = std::max (pk, std::max (std::abs (a), std::abs (b)));
                ul[i] = a; ur[i] = b;
            }
        });
        if (on)
            for (int i = 0; i < n; ++i) { l[i] = std::clamp (l[i], -ceil, ceil); r[i] = std::clamp (r[i], -ceil, ceil); }
        limGr = -gainToDb (minLim);
        osPeak = pk;
    }

    float glueGr = 0, limGr = 0, osPeak = 0;

private:
    double fs = 44100;
    OversamplerBank bank;
    LookaheadLimiter lim[4];
    CompressorCore glue;
    int quality = 0, lookBase = 64;
    bool on = true, monoOn = false;
    float lastTilt = -999, lastMono = -999, glueAmt = 0, glueMakeup = 1, ceiling = 1;
    BQ loF, hiF, sideHp;
    BQState loS[2], hiS[2], side1, side2;
    juce::SmoothedValue<float> widthS { 1.0f }, driveS { 1.0f }, enableS { 1.0f };
};

//==============================================================================
// ITU-R BS.1770 / EBU R128 loudness: momentary, short-term, gated integrated.
class LoudnessMeter
{
public:
    void prepare (double sr)
    {
        fs = sr;
        shelf = rbj::highShelf (fs, 1681.97, 0.7071, 3.99984);
        hp = rbj::highpass (fs, 38.135, 0.5003);
        blockLen = std::max (1, (int) std::round (fs * 0.1));
        reset();
    }
    void reset()
    {
        for (int c = 0; c < 2; ++c) { s[c].reset(); h[c].reset(); }
        acc = 0; counter = 0; subPos = 0; subFill = 0;
        std::fill (std::begin (sub), std::end (sub), 0.0);
        std::fill (std::begin (histCount), std::end (histCount), 0u);
        std::fill (std::begin (histEnergy), std::end (histEnergy), 0.0);
        momentary = shortTerm = integrated = -70.0f;
    }

    void process (const float* l, const float* r, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float a = h[0].process (hp, s[0].process (shelf, l[i]));
            const float b = h[1].process (hp, s[1].process (shelf, r[i]));
            acc += (double) a * a + (double) b * b;
            if (++counter >= blockLen)
            {
                sub[subPos] = acc / blockLen;
                subPos = (subPos + 1) % 30;
                subFill = std::min (subFill + 1, 30);
                acc = 0; counter = 0;
                const double m = mean (4), st = mean (30);
                momentary = toLufs (m);
                shortTerm = toLufs (st);
                if (subFill >= 4 && momentary > -70.0f)
                {
                    const int bin = std::clamp ((int) ((momentary + 70.0f) * 10.0f), 0, Bins - 1);
                    histCount[bin]++;
                    histEnergy[bin] += m;
                    updateIntegrated();
                }
            }
        }
    }

    float momentary = -70, shortTerm = -70, integrated = -70;

private:
    static constexpr int Bins = 800;
    static float toLufs (double ms) { return (float) (-0.691 + 10.0 * std::log10 (std::max (ms, 1.0e-12))); }
    double mean (int k) const
    {
        k = std::min (k, std::max (1, subFill));
        double sum = 0;
        for (int i = 0; i < k; ++i) sum += sub[(subPos - 1 - i + 30) % 30];
        return sum / k;
    }
    void updateIntegrated()
    {
        double e = 0; uint64_t c = 0;
        for (int b = 0; b < Bins; ++b) { e += histEnergy[b]; c += histCount[b]; }
        if (c == 0) return;
        const float rel = toLufs (e / (double) c) - 10.0f;
        const int start = std::clamp ((int) ((rel + 70.0f) * 10.0f), 0, Bins - 1);
        e = 0; c = 0;
        for (int b = start; b < Bins; ++b) { e += histEnergy[b]; c += histCount[b]; }
        if (c > 0) integrated = toLufs (e / (double) c);
    }

    double fs = 44100;
    BQ shelf, hp;
    BQState s[2], h[2];
    int blockLen = 4410, counter = 0, subPos = 0, subFill = 0;
    double acc = 0, sub[30] {};
    uint32_t histCount[Bins] {};
    double histEnergy[Bins] {};
};
} // namespace halo
