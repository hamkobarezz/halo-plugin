#pragma once
#include "Common.h"

namespace halo
{
//==============================================================================
// Bank of linear-phase FIR oversamplers (2x / 4x / 8x) so quality can be switched live.
class OversamplerBank
{
public:
    void prepare (int maxBlock)
    {
        for (int i = 0; i < 3; ++i)
        {
            // Fractional FIR latency is padded to a whole base-rate sample with an exact
            // integer delay at the oversampled rate (JUCE's own integer mode uses a lossy
            // fractional-delay filter that dulls the top end).
            os[i] = std::make_unique<juce::dsp::Oversampling<float>> (
                2, (size_t) (i + 1), juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, false);
            os[i]->initProcessing ((size_t) maxBlock);
            os[i]->reset();
            const double lat = os[i]->getLatencyInSamples();
            const int f = 1 << (i + 1);
            latInt[i] = (int) std::ceil (lat - 1.0e-6);
            pad[i] = (int) std::lround ((latInt[i] - lat) * f);
            for (auto& d : padBuf[i]) d.assign ((size_t) std::max (1, pad[i]), 0.0f);
            padPos[i] = 0;
        }
    }
    void setQuality (int q) // 0 = off, 1 = 2x, 2 = 4x, 3 = 8x
    {
        q = std::clamp (q, 0, 3);
        if (q != quality)
        {
            quality = q;
            if (q > 0) { os[q - 1]->reset(); for (auto& d : padBuf[q - 1]) std::fill (d.begin(), d.end(), 0.0f); }
        }
    }
    int factor() const { return 1 << quality; }
    float latency() const { return latencyFor (*this, quality); }
    static float latencyFor (const OversamplerBank& b, int q) { return q == 0 ? 0.0f : (float) b.latInt[q - 1]; }

    // Runs fn(float* l, float* r, int numOversampledSamples, int factor)
    template <typename Fn>
    void process (float* l, float* r, int n, Fn&& fn)
    {
        if (quality == 0) { fn (l, r, n, 1); return; }
        const int qi = quality - 1;
        float* chans[2] = { l, r };
        juce::dsp::AudioBlock<float> block (chans, 2, (size_t) n);
        auto up = os[qi]->processSamplesUp (block);
        const int un = (int) up.getNumSamples();
        if (pad[qi] > 0)
        {
            int pos = 0;
            for (int c = 0; c < 2; ++c)
            {
                auto* x = up.getChannelPointer ((size_t) c);
                auto& d = padBuf[qi][c];
                pos = padPos[qi];
                for (int i = 0; i < un; ++i)
                {
                    const float t = d[(size_t) pos];
                    d[(size_t) pos] = x[i];
                    x[i] = t;
                    if (++pos == pad[qi]) pos = 0;
                }
            }
            padPos[qi] = pos;
        }
        fn (up.getChannelPointer (0), up.getChannelPointer (1), un, factor());
        os[qi]->processSamplesDown (block);
    }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> os[3];
    std::vector<float> padBuf[3][2];
    int pad[3] {}, padPos[3] {}, latInt[3] {};
    int quality = 0;
};

//==============================================================================
// DRIVE: six analogue-flavoured shapers running oversampled, with auto level
// compensation so the drive knob changes tone rather than volume.
class Saturator
{
public:
    enum Type { TapeT = 0, Tube, BrokenTube, Transistor, Diode, Fold };

    void prepare (double sr, int maxBlock)
    {
        fs = sr;
        bank.prepare (maxBlock);
        outG.reset (sr, 0.03);
        dc[0].prepare (sr); dc[1].prepare (sr);
        lastType = -1;
        reset();
    }
    void reset() { dc[0].reset(); dc[1].reset(); for (int c = 0; c < 2; ++c) { postLp[c].reset(); bp1[c].reset(); bp2[c].reset(); } }
    void setQuality (int q) { bank.setQuality (q); }
    float latency() const { return bank.latency(); }
    OversamplerBank& getBank() { return bank; }

    void set (bool enabled, int typeIdx, float drive01, float mix01, float gainDb, bool clip)
    {
        type = std::clamp (typeIdx, 0, 5);
        tDrive = dbToGain (drive01 * 36.0f);
        tMix = enabled ? mix01 : 0.0f;
        clipOn = clip && enabled;
        drive01Cur = enabled ? drive01 : 0.0f;
        const float ref = 0.2f;
        const float s = std::abs (shape (ref * tDrive));
        tComp = std::clamp (ref / std::max (s, 1.0e-4f), 0.02f, 1.0f);
        outG.setTargetValue (dbToGain (enabled ? gainDb : 0.0f));
        if (type != lastType)
        {
            lastType = type;
            postLpF = rbj::lowpass (fs, type == TapeT ? 15000.0 : 9000.0, 0.6);
            bpLo = rbj::highpass (fs, 280.0, 0.7);
            bpHi = rbj::lowpass (fs, 4200.0, 0.9);
        }
    }

    void process (float* l, float* r, int n) noexcept
    {
        const float d0 = curDrive, d1 = tDrive, m0 = curMix, m1 = tMix, c0 = curComp, c1 = tComp;
        float peak = 0;
        bank.process (l, r, n, [&] (float* ul, float* ur, int un, int)
        {
            const float inv = 1.0f / (float) un;
            for (int i = 0; i < un; ++i)
            {
                const float t = (float) i * inv;
                const float d = d0 + (d1 - d0) * t, m = m0 + (m1 - m0) * t, cp = c0 + (c1 - c0) * t;
                float a = ul[i], b = ur[i];
                float ya = a + (shape (a * d) * cp - a) * m;
                float yb = b + (shape (b * d) * cp - b) * m;
                if (clipOn) { ya = softClip (ya); yb = softClip (yb); }
                ul[i] = ya; ur[i] = yb;
            }
        });
        curDrive = d1; curMix = m1; curComp = c1;

        const float colour = clamp01 (drive01Cur * 1.6f) * curMix;
        const bool idle = m0 == 0.0f && m1 == 0.0f && ! clipOn;
        if (idle) { dc[0].reset(); dc[1].reset(); }
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { l[i], r[i] };
            for (int c = 0; c < 2; ++c)
            {
                float y = x[c];
                if (colour > 0.0005f)
                {
                    if (type == BrokenTube)
                        y += (bp2[c].process (bpHi, bp1[c].process (bpLo, y)) * 1.6f - y) * colour;
                    else if (type == TapeT || type == Tube)
                        y += (postLp[c].process (postLpF, y) - y) * colour * 0.6f;
                }
                x[c] = idle ? y : dc[c].process (y);
            }
            const float g = outG.getNextValue();
            l[i] = x[0] * g; r[i] = x[1] * g;
            peak = std::max (peak, std::abs (l[i]));
        }
        activity = peak * drive01Cur;
    }

    float activity = 0;

private:
    inline float shape (float x) const noexcept
    {
        switch (type)
        {
            case TapeT:
            {
                constexpr float b = 0.06f, tb = 0.0599281f, k = 1.0f / (1.0f - tb * tb);
                return (std::tanh (x + b) - tb) * k;
            }
            case Tube:
            {
                constexpr float b = 0.3f, tb = 0.2913126f, k = 1.0f / (1.0f - tb * tb);
                return (std::tanh (x + b) - tb) * k;
            }
            case BrokenTube:
            {
                const float u = x * 1.4f + 0.7f;
                const float c = u / std::cbrt (1.0f + std::abs (u * u * u));
                constexpr float c0 = 0.6345f; // value at u = 0.7
                const float y = (c - c0) * 1.2f;
                return y + 0.35f * std::abs (y);
            }
            case Transistor: return x / std::sqrt (std::sqrt (1.0f + x * x * x * x));
            case Diode:      return x >= 0.0f ? 1.0f - std::exp (-x) : -(1.0f - std::exp (x * 1.8f)) / 1.8f;
            case Fold:       return std::sin (x);
            default:         return x;
        }
    }
    static inline float softClip (float x) noexcept
    {
        constexpr float c = 0.966f;
        const float u = x / c, u2 = u * u, u6 = u2 * u2 * u2;
        return c * u / std::pow (1.0f + u6, 1.0f / 6.0f);
    }

    double fs = 44100;
    OversamplerBank bank;
    int type = Tube, lastType = -1;
    float tDrive = 1, tMix = 0, tComp = 1, curDrive = 1, curMix = 0, curComp = 1, drive01Cur = 0;
    bool clipOn = false;
    juce::SmoothedValue<float> outG { 1.0f };
    DCBlocker dc[2];
    BQ postLpF, bpLo, bpHi;
    BQState postLp[2], bp1[2], bp2[2];
};
} // namespace halo
