#pragma once
#include "Common.h"

namespace halo
{
//==============================================================================
// Stereo-linked feed-forward compressor core with soft knee, RMS/peak detection
// and optional program-dependent (opto style) release.
class CompressorCore
{
public:
    void prepare (double sr) { fs = sr; lastHpf = -1; rmsC = timeCoef (0.008, fs); reset(); }
    void reset() { env = 0; rms = 0; sc[0].reset(); sc[1].reset(); }

    void configure (float thrDb, float ratio, float kneeDb, float attMs, float relMs,
                    bool useRms, bool programRelease, float scHpfHz)
    {
        thr = thrDb;
        slope = 1.0f / std::max (1.0f, ratio) - 1.0f;
        knee = std::max (0.0f, kneeDb);
        attC = timeCoef (attMs * 0.001, fs);
        relFast = timeCoef (relMs * 0.001, fs);
        relSlow = timeCoef (relMs * 0.0045, fs);
        rmsMode = useRms;
        prog = programRelease;
        if (std::abs (scHpfHz - lastHpf) > 0.01f)
        {
            lastHpf = scHpfHz;
            hp = rbj::highpass (fs, scHpfHz, 0.7071);
        }
    }

    inline float curve (float x) const noexcept
    {
        const float over = x - thr;
        if (knee > 0.0f && 2.0f * std::abs (over) <= knee)
        {
            const float t = over + knee * 0.5f;
            return slope * t * t / (2.0f * knee);
        }
        return over > 0.0f ? slope * over : 0.0f;
    }

    // returns the smoothed gain change in dB (<= 0)
    inline float process (float l, float r) noexcept
    {
        const float sl = sc[0].process (hp, l), sr = sc[1].process (hp, r);
        float det;
        if (rmsMode)
        {
            const float p = std::max (sl * sl, sr * sr);
            rms = p + (rms - p) * rmsC;
            det = std::sqrt (rms) * 1.41421356f;
        }
        else
            det = std::max (std::abs (sl), std::abs (sr));

        const float target = curve (gainToDb (det));
        if (target < env)
            env = target + (env - target) * attC;
        else
        {
            const float rc = prog ? lerp (relFast, relSlow, clamp01 (-env / 12.0f)) : relFast;
            env = target + (env - target) * rc;
        }
        return env;
    }

private:
    double fs = 44100;
    float thr = 0, slope = 0, knee = 0, attC = 0, relFast = 0, relSlow = 0, rmsC = 0;
    bool rmsMode = false, prog = false;
    float env = 0, rms = 0, lastHpf = -1;
    BQ hp;
    BQState sc[2];
};

//==============================================================================
// One-knob vocal compressor with four character models.
class VocalCompressor
{
public:
    enum Style { Clean = 0, Opto, FET, VariMu };

    void prepare (double sr)
    {
        fs = sr;
        core.prepare (sr);
        inG.reset (sr, 0.03); outG.reset (sr, 0.03); mixS.reset (sr, 0.03);
        dc[0].prepare (sr); dc[1].prepare (sr);
        reset();
    }
    void reset() { core.reset(); dc[0].reset(); dc[1].reset(); }

    void set (float inDb, float amount01, int style, float speed01, float outDb, bool autoGain, float mix01, float hpfHz)
    {
        struct S { float att, rel, knee, ratioMax; bool rms; float color; };
        static constexpr S styles[4] = {
            { 3.0f,   120.0f, 6.0f,  6.0f,  false, 0.0f  },   // Clean  : transparent VCA
            { 10.0f,  260.0f, 12.0f, 4.0f,  true,  0.0f  },   // Opto   : smooth, program dependent
            { 0.25f,  70.0f,  2.0f,  12.0f, false, 0.18f },   // FET    : fast & aggressive, adds bite
            { 25.0f,  420.0f, 18.0f, 5.0f,  true,  0.10f } }; // Vari-Mu: thick, glue, 2nd harmonic
        styleIdx = std::clamp (style, 0, 3);
        const auto& s = styles[styleIdx];
        const float speedMul = std::pow (4.0f, 0.5f - speed01);
        const float ratio = 1.0f + (s.ratioMax - 1.0f) * std::pow (amount01, 0.7f);
        const float thrDb = -amount01 * 32.0f;
        core.configure (thrDb, ratio, s.knee, s.att * speedMul, s.rel * speedMul, s.rms,
                        styleIdx == Opto || styleIdx == VariMu, hpfHz);
        const float makeup = autoGain ? -core.curve (-12.0f) * 0.75f : 0.0f;
        color = s.color * amount01;
        inG.setTargetValue (dbToGain (inDb));
        outG.setTargetValue (dbToGain (outDb + makeup));
        mixS.setTargetValue (mix01);
    }

    void process (float* l, float* r, int n) noexcept
    {
        float minEnv = 0;
        for (int i = 0; i < n; ++i)
        {
            const float gi = inG.getNextValue();
            const float xl = l[i] * gi, xr = r[i] * gi;
            const float e = core.process (xl, xr);
            minEnv = std::min (minEnv, e);
            const float g = dbToGain (e);
            float wl = xl * g, wr = xr * g;

            if (color > 0.0f)
            {
                if (styleIdx == FET)
                {
                    const float d = 1.0f + 2.5f * color;
                    wl = std::tanh (wl * d) / d;
                    wr = std::tanh (wr * d) / d;
                }
                else if (styleIdx == VariMu)
                {
                    wl = dc[0].process (wl + color * wl * wl);
                    wr = dc[1].process (wr + color * wr * wr);
                }
            }
            const float m = mixS.getNextValue(), go = outG.getNextValue();
            l[i] = (xl + (wl - xl) * m) * go;
            r[i] = (xr + (wr - xr) * m) * go;
        }
        grDb = -minEnv;
    }

    float grDb = 0;

private:
    double fs = 44100;
    CompressorCore core;
    int styleIdx = 1;
    float color = 0;
    juce::SmoothedValue<float> inG { 1.0f }, outG { 1.0f }, mixS { 1.0f };
    DCBlocker dc[2];
};

//==============================================================================
// Enhance: two-band de-noise expander, relative split-band de-esser,
// body (low-mid weight) and air (shelf + harmonic exciter).
class Enhancer
{
public:
    void prepare (double sr)
    {
        fs = sr;
        noiseX.set (fs, 3200.0);
        nEnvLo.set (fs, 0.001, 0.05); nEnvHi.set (fs, 0.001, 0.05);
        essHi.set (fs, 0.0004, 0.05); essLo.set (fs, 0.002, 0.08);
        gOpen = timeCoef (0.0008, fs); gClose = timeCoef (0.07, fs);
        essAtt = timeCoef (0.0007, fs); essRel = timeCoef (0.045, fs);
        exHpF = rbj::highpass (fs, 3800.0, 0.7071);
        lastFreq = lastBody = lastAir = -1;
        reset();
    }
    void reset()
    {
        noiseX.reset(); essX.reset();
        nEnvLo.reset(); nEnvHi.reset(); essHi.reset(); essLo.reset();
        gLo = gHi = 0; essG = 0;
        for (int c = 0; c < 2; ++c) { bodyS[c].reset(); cutS[c].reset(); airS[c].reset(); ex1[c].reset(); ex2[c].reset(); }
    }

    void set (float deess01, float deessFreq, float denoise01, float air01, float body01)
    {
        deess = deess01; denoise = denoise01; air = air01; body = body01;
        if (std::abs (deessFreq - lastFreq) > 1.0f) { lastFreq = deessFreq; essX.set (fs, deessFreq); }
        if (std::abs (body01 - lastBody) > 1.0e-4f)
        {
            lastBody = body01;
            bodyBell = rbj::bell (fs, 190.0, 0.8, 5.0 * body01);
            boxCut = rbj::bell (fs, 480.0, 1.3, -2.5 * body01);
        }
        if (std::abs (air01 - lastAir) > 1.0e-4f)
        {
            lastAir = air01;
            airShelf = rbj::highShelf (fs, 10500.0, 0.6, 8.0 * air01);
        }
        nThr = -82.0f + 42.0f * denoise;
        nRange = 6.0f + 34.0f * denoise;
        essThr = -8.0f - 34.0f * deess;
        essSlope = 1.0f - 1.0f / (2.0f + 8.0f * deess);
    }

    void process (float* l, float* r, int n) noexcept
    {
        float maxEss = 0, maxNoise = 0;
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { l[i], r[i] };

            // ---- De-noise (two band downward expander) ----
            {
                float lo[2], hi[2];
                noiseX.split (0, x[0], lo[0], hi[0]);
                noiseX.split (1, x[1], lo[1], hi[1]);
                if (denoise > 0.0005f)
                {
                    const float eLo = gainToDb (nEnvLo.process (std::max (std::abs (lo[0]), std::abs (lo[1]))));
                    const float eHi = gainToDb (nEnvHi.process (std::max (std::abs (hi[0]), std::abs (hi[1]))));
                    const float tLo = expander (eLo, nThr);
                    const float tHi = expander (eHi, nThr - 6.0f);
                    gLo = tLo + (gLo - tLo) * (tLo > gLo ? gOpen : gClose);
                    gHi = tHi + (gHi - tHi) * (tHi > gHi ? gOpen : gClose);
                }
                else
                {
                    gLo *= 0.999f; gHi *= 0.999f;
                }
                const float a = dbToGain (gLo), b = dbToGain (gHi);
                x[0] = lo[0] * a + hi[0] * b;
                x[1] = lo[1] * a + hi[1] * b;
                maxNoise = std::min (maxNoise, std::min (gLo, gHi));
            }

            // ---- De-ess (relative split band) ----
            {
                float lo[2], hi[2];
                essX.split (0, x[0], lo[0], hi[0]);
                essX.split (1, x[1], lo[1], hi[1]);
                float target = 0;
                const float hDb = gainToDb (essHi.process (std::max (std::abs (hi[0]), std::abs (hi[1]))));
                const float lDb = gainToDb (essLo.process (std::max (std::abs (lo[0]), std::abs (lo[1]))));
                if (deess > 0.0005f)
                {
                    const float weight = clamp01 ((hDb - lDb + 12.0f) / 12.0f);
                    const float over = std::max (0.0f, hDb - essThr) * weight;
                    target = -std::min (20.0f, over * essSlope);
                }
                essG = target + (essG - target) * (target < essG ? essAtt : essRel);
                const float g = dbToGain (essG);
                x[0] = lo[0] + hi[0] * g;
                x[1] = lo[1] + hi[1] * g;
                maxEss = std::min (maxEss, essG);
            }

            // ---- Body + Air ----
            for (int c = 0; c < 2; ++c)
            {
                float y = x[c];
                if (lastBody > 0.0005f)
                    y = cutS[c].process (boxCut, bodyS[c].process (bodyBell, y));
                if (lastAir > 0.0005f)
                {
                    y = airS[c].process (airShelf, y);
                    const float h = ex1[c].process (exHpF, y);
                    const float e = std::tanh (h * 5.0f) * 0.2f;
                    y += ex2[c].process (exHpF, e) * 0.9f * air;
                }
                x[c] = y;
            }
            l[i] = x[0]; r[i] = x[1];
        }
        essGr = -maxEss;
        noiseGr = -maxNoise;
    }

    float essGr = 0, noiseGr = 0;

private:
    inline float expander (float lvlDb, float thrDb) const noexcept
    {
        const float below = thrDb - lvlDb;
        return below > 0.0f ? -std::min (nRange, below * 2.0f) : 0.0f;
    }

    double fs = 44100;
    float deess = 0, denoise = 0, air = 0, body = 0;
    float lastFreq = -1, lastBody = -1, lastAir = -1;
    float nThr = -80, nRange = 10, essThr = -20, essSlope = 0.5f;
    float gLo = 0, gHi = 0, essG = 0, gOpen = 0, gClose = 0, essAtt = 0, essRel = 0;
    LR4 noiseX, essX;
    EnvFollower nEnvLo, nEnvHi, essHi, essLo;
    BQ bodyBell, boxCut, airShelf, exHpF;
    BQState bodyS[2], cutS[2], airS[2], ex1[2], ex2[2];
};
} // namespace halo
