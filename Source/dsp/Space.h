#pragma once
#include "Common.h"

namespace halo
{
//==============================================================================
// MULTIPLY: up to 8 drifting, detuned, time-offset voices spread across the stereo field.
// The low end of the wet signal is removed so the bass stays mono and centred.
class Doubler
{
public:
    static constexpr int MaxVoices = 8;

    void prepare (double sr)
    {
        fs = sr;
        line.prepare ((int) (fs * 0.14));
        amt.reset (sr, 0.05);
        for (int v = 0; v < MaxVoices; ++v)
            lfo[v].init ((uint32_t) (v + 11), fs, 0.25f + 0.11f * (float) v);
        lpF = rbj::lowpass (fs, 13500.0, 0.7071);
        lastLow = -1;
        reset();
    }
    void reset() { line.reset(); for (int c = 0; c < 2; ++c) { hp1[c].reset(); hp2[c].reset(); lpS[c].reset(); } }

    void set (float amount01, float spread01, float offset01, float detune01, float lowHz, int voices)
    {
        numVoices = std::clamp (voices, 1, MaxVoices);
        amt.setTargetValue (amount01 * amount01 * 1.1f);
        if (std::abs (lowHz - lastLow) > 0.5f) { lastLow = lowHz; hpF = rbj::highpass (fs, lowHz, 0.7071); }
        const float norm = 1.0f / std::sqrt ((float) numVoices);
        for (int v = 0; v < numVoices; ++v)
        {
            const float rank = (float) (v / 2) / (float) std::max (1, (numVoices - 1) / 2);
            const float side = (v % 2 == 0) ? -1.0f : 1.0f;
            const float pan = side * spread01 * (0.55f + 0.45f * rank);
            const float ang = (pan + 1.0f) * kPiF * 0.25f;
            gL[v] = std::cos (ang) * norm * 1.3f;
            gR[v] = std::sin (ang) * norm * 1.3f;
            base[v] = (float) ((6.0 + (v + 1) * (1.4 + 9.5 * offset01)) * fs * 0.001);
            depth[v] = (float) ((0.08 + 1.9 * detune01) * fs * 0.001) * (0.7f + 0.3f * (float) ((v * 7) % 5) / 4.0f);
        }
    }

    void process (float* l, float* r, int n) noexcept
    {
        float peak = 0;
        for (int i = 0; i < n; ++i)
        {
            line.push (0.5f * (l[i] + r[i]));
            float wl = 0, wr = 0;
            for (int v = 0; v < numVoices; ++v)
            {
                const float s = line.read (base[v] + depth[v] * lfo[v].next());
                wl += s * gL[v];
                wr += s * gR[v];
            }
            wl = lpS[0].process (lpF, hp2[0].process (hpF, hp1[0].process (hpF, wl)));
            wr = lpS[1].process (lpF, hp2[1].process (hpF, hp1[1].process (hpF, wr)));
            const float a = amt.getNextValue();
            l[i] += wl * a;
            r[i] += wr * a;
            peak = std::max (peak, std::abs (wl * a) + std::abs (wr * a));
        }
        activity = peak;
    }

    float activity = 0;

private:
    double fs = 44100;
    DelayLine line;
    DriftLFO lfo[MaxVoices];
    float base[MaxVoices] {}, depth[MaxVoices] {}, gL[MaxVoices] {}, gR[MaxVoices] {};
    int numVoices = 4;
    float lastLow = -1;
    BQ hpF, lpF;
    BQState hp1[2], hp2[2], lpS[2];
    juce::SmoothedValue<float> amt { 0.0f };
};

//==============================================================================
// SPACE / Reverb: 8-channel FDN with a 4-stage Hadamard diffuser, modulated
// Householder feedback loop, in-loop damping and ducking.
class FDNReverb
{
public:
    static constexpr int N = 8, Steps = 4;

    struct Type { float minMs, maxMs, diffMs, rtMin, rtMax, modMs, early; };
    static const Type& type (int t)
    {
        static const Type types[6] = {
            { 7,  22,  10, 0.15f, 1.4f,  0.10f, 0.55f },   // Room
            { 11, 34,  18, 0.30f, 2.4f,  0.14f, 0.45f },   // Studio
            { 9,  29,  32, 0.50f, 5.0f,  0.22f, 0.20f },   // Plate
            { 27, 83,  44, 1.00f, 7.5f,  0.32f, 0.35f },   // Hall
            { 46, 139, 70, 2.50f, 14.0f, 0.42f, 0.30f },   // Cathedral
            { 61, 191, 105, 4.0f, 45.0f, 0.60f, 0.18f } }; // Ambient
        return types[std::clamp (t, 0, 5)];
    }

    void prepare (double sr)
    {
        fs = sr;
        pre[0].prepare ((int) (fs * 0.30)); pre[1].prepare ((int) (fs * 0.30));
        for (int s = 0; s < Steps; ++s)
            for (int j = 0; j < N; ++j) diff[s][j].prepare ((int) (fs * 0.12));
        for (int j = 0; j < N; ++j) loop[j].prepare ((int) (fs * 0.22));
        juce::Random rng (8417);
        for (int s = 0; s < Steps; ++s)
            for (int j = 0; j < N; ++j)
            {
                diffFrac[s][j] = ((float) j + rng.nextFloat()) / (float) N;
                flip[s][j] = rng.nextBool() ? -1.0f : 1.0f;
            }
        for (int j = 0; j < N; ++j)
        {
            lfoPh[j] = (float) j / N;
            lfoInc[j] = (float) ((0.07 + 0.09 * j) / fs);
        }
        duckEnv.set (fs, 0.004, 0.25);
        mixS.reset (sr, 0.05);
        curType = -1;
        lastDecay = lastDamp = lastLow = -1;
        reset();
    }

    void reset()
    {
        pre[0].reset(); pre[1].reset();
        for (auto& s : diff) for (auto& d : s) d.reset();
        for (auto& d : loop) d.reset();
        for (int j = 0; j < N; ++j) { damp[j].reset(); dcHp[j].reset(); }
        for (int c = 0; c < 2; ++c) { lowS[c].reset(); lowS2[c].reset(); }
        duckEnv.reset();
    }

    void set (int typeIdx, float mix01, float decay01, float predelayMs, float damp01, float width01, float duck01, float lowCutHz)
    {
        if (typeIdx != curType)
        {
            curType = typeIdx;
            const auto& t = type (typeIdx);
            for (int s = 0; s < Steps; ++s)
            {
                const float stepMs = t.diffMs * (float) (1 << s) / 15.0f;
                for (int j = 0; j < N; ++j)
                    diffDelay[s][j] = std::max (1, (int) (stepMs * 0.001f * (float) fs * diffFrac[s][j]));
            }
            for (int j = 0; j < N; ++j)
            {
                const float r = (float) j / (N - 1);
                const float ms = t.minMs * std::pow (t.maxMs / t.minMs, r) * (1.0f + 0.031f * std::sin ((float) j * 2.3f));
                loopLen[j] = ms * 0.001f * (float) fs;
            }
            modDepth = t.modMs * 0.001f * (float) fs;
            earlyMix = t.early;
            lastDecay = -1;
        }
        if (std::abs (decay01 - lastDecay) > 1.0e-4f)
        {
            lastDecay = decay01;
            const auto& t = type (curType);
            const float rt = t.rtMin * std::pow (t.rtMax / t.rtMin, decay01);
            for (int j = 0; j < N; ++j)
                loopGain[j] = std::pow (10.0f, -3.0f * loopLen[j] / (rt * (float) fs));
        }
        if (std::abs (damp01 - lastDamp) > 1.0e-4f)
        {
            lastDamp = damp01;
            const double cut = 18000.0 * std::pow (1500.0 / 18000.0, (double) damp01);
            for (int j = 0; j < N; ++j) { damp[j].setCutoff (fs, cut); dcHp[j].setCutoff (fs, 40.0); }
        }
        if (std::abs (lowCutHz - lastLow) > 0.5f) { lastLow = lowCutHz; lowF = rbj::highpass (fs, lowCutHz, 0.7071); }
        preSamples = std::max (1.0f, predelayMs * 0.001f * (float) fs);
        width = width01;
        duck = duck01;
        mixS.setTargetValue (mix01 * mix01 * 0.9f);
    }

    // Adds the wet reverb into l/r (send style: the dry signal stays untouched)
    void process (float* l, float* r, int n) noexcept
    {
        float peak = 0;
        for (int i = 0; i < n; ++i)
        {
            const float inL = l[i], inR = r[i];
            pre[0].push (inL); pre[1].push (inR);
            const float pl = pre[0].read (preSamples), pr = pre[1].read (preSamples);

            float v[N];
            for (int j = 0; j < N; ++j) v[j] = ((j & 1) ? pr : pl) * ((j & 2) ? -0.5f : 0.5f);

            for (int s = 0; s < Steps; ++s)
            {
                float t[N];
                for (int j = 0; j < N; ++j)
                {
                    diff[s][j].push (v[j]);
                    t[j] = diff[s][j].tap (diffDelay[s][j]) * flip[s][j];
                }
                for (int j = 0; j < N; ++j) v[j] = t[(j + 3) % N];
                hadamard (v);
            }

            float o[N], sum = 0;
            for (int j = 0; j < N; ++j)
            {
                lfoPh[j] += lfoInc[j];
                if (lfoPh[j] >= 1.0f) lfoPh[j] -= 1.0f;
                const float m = modDepth * std::sin (lfoPh[j] * 2.0f * kPiF);
                float y = loop[j].read (loopLen[j] + m);
                y = dcHp[j].hp (damp[j].lp (y));
                o[j] = y;
                sum += y * loopGain[j];
            }
            const float hh = sum * (2.0f / N);
            for (int j = 0; j < N; ++j)
                loop[j].push (v[j] + o[j] * loopGain[j] - hh);

            float wl = 0, wr = 0;
            for (int j = 0; j < N; j += 2) { wl += o[j] + v[j] * earlyMix; wr += o[j + 1] + v[j + 1] * earlyMix; }
            wl *= 0.55f; wr *= 0.55f;
            wl = lowS2[0].process (lowF, lowS[0].process (lowF, wl));
            wr = lowS2[1].process (lowF, lowS[1].process (lowF, wr));

            const float mid = 0.5f * (wl + wr), side = 0.5f * (wl - wr) * width;
            wl = mid + side; wr = mid - side;

            const float e = duckEnv.process (std::max (std::abs (inL), std::abs (inR)));
            const float dg = dbToGain (-duck * 18.0f * clamp01 ((gainToDb (e) + 42.0f) / 30.0f));
            const float g = mixS.getNextValue() * dg;
            l[i] = inL + wl * g;
            r[i] = inR + wr * g;
            peak = std::max (peak, std::abs (wl * g) + std::abs (wr * g));
        }
        activity = peak;
    }

    float activity = 0;

private:
    static inline void hadamard (float* x) noexcept
    {
        for (int h = 1; h < N; h *= 2)
            for (int i = 0; i < N; i += 2 * h)
                for (int j = i; j < i + h; ++j)
                {
                    const float a = x[j], b = x[j + h];
                    x[j] = a + b; x[j + h] = a - b;
                }
        for (int i = 0; i < N; ++i) x[i] *= 0.35355339f;
    }

    double fs = 44100;
    DelayLine pre[2], diff[Steps][N], loop[N];
    int diffDelay[Steps][N] {};
    float diffFrac[Steps][N] {}, flip[Steps][N] {};
    float loopLen[N] {}, loopGain[N] {}, lfoPh[N] {}, lfoInc[N] {};
    OnePole damp[N], dcHp[N];
    float modDepth = 0, earlyMix = 0.3f, preSamples = 1, width = 1, duck = 0;
    int curType = -1;
    float lastDecay = -1, lastDamp = -1, lastLow = -1;
    BQ lowF;
    BQState lowS[2], lowS2[2];
    EnvFollower duckEnv;
    juce::SmoothedValue<float> mixS { 0.0f };
};

//==============================================================================
// SPACE / Delay: Mono, Ping-Pong, Wide, Tape (wow/flutter + saturation), Slap.
class StereoDelay
{
public:
    enum Type { Mono = 0, PingPong, Wide, Tape, Slap };

    void prepare (double sr)
    {
        fs = sr;
        dl[0].prepare ((int) (fs * 4.2)); dl[1].prepare ((int) (fs * 4.2));
        duckEnv.set (fs, 0.004, 0.3);
        mixS.reset (sr, 0.05);
        timeC = timeCoef (0.09, fs);
        cur[0] = cur[1] = -1;
        lastTone = -1;
        reset();
    }
    void reset()
    {
        dl[0].reset(); dl[1].reset();
        for (int c = 0; c < 2; ++c) { fbLp[c].reset(); fbHp[c].reset(); }
        duckEnv.reset();
    }

    void set (int typeIdx, float mix01, float timeSec, float feedback01, float tone01, float duck01)
    {
        type = std::clamp (typeIdx, 0, 4);
        float t = type == Slap ? 0.095f : timeSec;
        t = std::clamp (t, 0.001f, 2.0f);
        tgt[0] = t * (float) fs;
        tgt[1] = (type == Wide ? t * 1.5f : t) * (float) fs;
        if (cur[0] < 0) { cur[0] = tgt[0]; cur[1] = tgt[1]; }
        fb = std::min (0.95f, feedback01 * 0.95f);
        if (type == Slap) fb = std::min (fb, 0.18f);
        if (std::abs (tone01 - lastTone) > 1.0e-4f || type != lastType)
        {
            lastTone = tone01; lastType = type;
            double cut = 1800.0 * std::pow (18000.0 / 1800.0, (double) tone01);
            if (type == Tape) cut = std::min (cut, 7000.0);
            for (int c = 0; c < 2; ++c) { fbLp[c].setCutoff (fs, cut); fbHp[c].setCutoff (fs, 160.0); }
        }
        duck = duck01;
        mixS.setTargetValue (mix01 * mix01);
    }

    void process (float* l, float* r, int n) noexcept
    {
        float peak = 0;
        const float wowInc = (float) (0.55 / fs), flInc = (float) (6.3 / fs);
        for (int i = 0; i < n; ++i)
        {
            const float inL = l[i], inR = r[i];
            cur[0] = tgt[0] + (cur[0] - tgt[0]) * timeC;
            cur[1] = tgt[1] + (cur[1] - tgt[1]) * timeC;
            float mod = 0;
            if (type == Tape)
            {
                wow += wowInc; if (wow >= 1) wow -= 1;
                flut += flInc; if (flut >= 1) flut -= 1;
                mod = (float) fs * (0.0016f * std::sin (wow * 2 * kPiF) + 0.00018f * std::sin (flut * 2 * kPiF));
            }
            const float rl = dl[0].read (cur[0] + mod), rr = dl[1].read (cur[1] + mod * 0.9f);
            float fl = fbHp[0].hp (fbLp[0].lp (rl)), fr = fbHp[1].hp (fbLp[1].lp (rr));
            if (type == Tape) { fl = std::tanh (fl * 1.4f) / 1.4f; fr = std::tanh (fr * 1.4f) / 1.4f; }

            const float m = 0.5f * (inL + inR);
            switch (type)
            {
                case Mono:     dl[0].push (m + fl * fb);   dl[1].push (m + fr * fb); break;
                case PingPong: dl[0].push (m + fr * fb);   dl[1].push (fl * fb);     break;
                default:       dl[0].push (inL + fl * fb); dl[1].push (inR + fr * fb); break;
            }

            const float e = duckEnv.process (std::max (std::abs (inL), std::abs (inR)));
            const float dg = dbToGain (-duck * 20.0f * clamp01 ((gainToDb (e) + 42.0f) / 30.0f));
            const float g = mixS.getNextValue() * dg;
            l[i] = inL + rl * g;
            r[i] = inR + rr * g;
            peak = std::max (peak, std::abs (rl * g) + std::abs (rr * g));
        }
        activity = peak;
    }

    float activity = 0;

private:
    double fs = 44100;
    DelayLine dl[2];
    float cur[2] { -1, -1 }, tgt[2] { 1, 1 }, timeC = 0, fb = 0, duck = 0, wow = 0, flut = 0, lastTone = -1;
    int type = PingPong, lastType = -1;
    OnePole fbLp[2], fbHp[2];
    EnvFollower duckEnv;
    juce::SmoothedValue<float> mixS { 0.0f };
};
} // namespace halo
