#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <complex>
#include <array>
#include <atomic>
#include <vector>
#include <algorithm>

namespace halo
{
constexpr double kPi = 3.14159265358979323846;
constexpr float kPiF = 3.14159265358979f;

inline float dbToGain (float db) noexcept { return std::exp (db * 0.11512925465f); }
inline float gainToDb (float g) noexcept  { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }
inline float clamp01 (float x) noexcept   { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }
inline float timeCoef (double seconds, double fs) noexcept
{
    return seconds <= 0.0 ? 0.0f : (float) std::exp (-1.0 / (seconds * fs));
}

//==============================================================================
// Biquad (TDF-II, double state for clean low-frequency filtering)
struct BQ { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

struct BQState
{
    double z1 = 0, z2 = 0;
    inline float process (const BQ& c, float xin) noexcept
    {
        const double x = xin;
        const double y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return (float) y;
    }
    void reset() noexcept { z1 = z2 = 0; }
};

namespace rbj
{
    inline BQ norm (double b0, double b1, double b2, double a0, double a1, double a2)
    {
        BQ c;
        c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0;
        c.a1 = a1 / a0; c.a2 = a2 / a0;
        return c;
    }
    inline double omega (double fs, double f) { f = std::clamp (f, 1.0, fs * 0.4999); return 2.0 * kPi * f / fs; }

    inline BQ lowpass (double fs, double f, double q)
    {
        const double w = omega (fs, f), cw = std::cos (w), al = std::sin (w) / (2.0 * q);
        return norm ((1 - cw) * 0.5, 1 - cw, (1 - cw) * 0.5, 1 + al, -2 * cw, 1 - al);
    }
    inline BQ highpass (double fs, double f, double q)
    {
        const double w = omega (fs, f), cw = std::cos (w), al = std::sin (w) / (2.0 * q);
        return norm ((1 + cw) * 0.5, -(1 + cw), (1 + cw) * 0.5, 1 + al, -2 * cw, 1 - al);
    }
    inline BQ bandpass (double fs, double f, double q)
    {
        const double w = omega (fs, f), cw = std::cos (w), al = std::sin (w) / (2.0 * q);
        return norm (al, 0, -al, 1 + al, -2 * cw, 1 - al);
    }
    inline BQ bell (double fs, double f, double q, double db)
    {
        const double A = std::pow (10.0, db / 40.0);
        const double w = omega (fs, f), cw = std::cos (w), al = std::sin (w) / (2.0 * q);
        return norm (1 + al * A, -2 * cw, 1 - al * A, 1 + al / A, -2 * cw, 1 - al / A);
    }
    inline BQ lowShelf (double fs, double f, double q, double db)
    {
        const double A = std::pow (10.0, db / 40.0);
        const double w = omega (fs, f), cw = std::cos (w), al = std::sin (w) / (2.0 * q);
        const double s = 2.0 * std::sqrt (A) * al;
        return norm (A * ((A + 1) - (A - 1) * cw + s), 2 * A * ((A - 1) - (A + 1) * cw), A * ((A + 1) - (A - 1) * cw - s),
                     (A + 1) + (A - 1) * cw + s, -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - s);
    }
    inline BQ highShelf (double fs, double f, double q, double db)
    {
        const double A = std::pow (10.0, db / 40.0);
        const double w = omega (fs, f), cw = std::cos (w), al = std::sin (w) / (2.0 * q);
        const double s = 2.0 * std::sqrt (A) * al;
        return norm (A * ((A + 1) + (A - 1) * cw + s), -2 * A * ((A - 1) + (A + 1) * cw), A * ((A + 1) + (A - 1) * cw - s),
                     (A + 1) - (A - 1) * cw + s, 2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - s);
    }
    inline double magnitude (const BQ& c, double fs, double f)
    {
        const double w = 2.0 * kPi * f / fs;
        const std::complex<double> z = std::polar (1.0, -w);
        const auto num = c.b0 + c.b1 * z + c.b2 * z * z;
        const auto den = 1.0 + c.a1 * z + c.a2 * z * z;
        return std::abs (num / den);
    }
}

//==============================================================================
struct OnePole
{
    float a = 0.0f, z = 0.0f;
    void setCutoff (double fs, double f) { a = (float) std::exp (-2.0 * kPi * std::min (f, fs * 0.49) / fs); }
    inline float lp (float x) noexcept { z = x + (z - x) * a; return z; }
    inline float hp (float x) noexcept { return x - lp (x); }
    void reset() noexcept { z = 0.0f; }
};

struct DCBlocker
{
    float x1 = 0, y1 = 0, R = 0.9995f;
    void prepare (double fs) { R = (float) std::exp (-2.0 * kPi * 7.0 / fs); }
    inline float process (float x) noexcept { const float y = x - x1 + R * y1; x1 = x; y1 = y; return y; }
    void reset() noexcept { x1 = y1 = 0; }
};

struct EnvFollower
{
    float att = 0, rel = 0, v = 0;
    void set (double fs, double attS, double relS) { att = timeCoef (attS, fs); rel = timeCoef (relS, fs); }
    inline float process (float x) noexcept { const float c = x > v ? att : rel; v = x + (v - x) * c; return v; }
    void reset() noexcept { v = 0; }
};

// Linkwitz-Riley 4th order crossover (LP^2 + HP^2 sums to an allpass)
struct LR4
{
    BQ lp, hp;
    BQState l1[2], l2[2], h1[2], h2[2];
    void set (double fs, double f) { lp = rbj::lowpass (fs, f, 0.70710678); hp = rbj::highpass (fs, f, 0.70710678); }
    inline void split (int ch, float x, float& lo, float& hi) noexcept
    {
        lo = l2[ch].process (lp, l1[ch].process (lp, x));
        hi = h2[ch].process (hp, h1[ch].process (hp, x));
    }
    void reset() { for (int c = 0; c < 2; ++c) { l1[c].reset(); l2[c].reset(); h1[c].reset(); h2[c].reset(); } }
};

//==============================================================================
// Power-of-two ring buffer delay with Hermite interpolation
struct DelayLine
{
    std::vector<float> buf;
    int mask = 0, w = 0;

    void prepare (int maxSamples)
    {
        const int size = juce::nextPowerOfTwo (std::max (16, maxSamples + 8));
        buf.assign ((size_t) size, 0.0f);
        mask = size - 1;
        w = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    int capacity() const { return mask + 1; }

    inline void push (float x) noexcept { buf[(size_t) w] = x; w = (w + 1) & mask; }
    // d = 0 → most recently pushed sample
    inline float tap (int d) const noexcept { return buf[(size_t) ((w - 1 - d) & mask)]; }

    inline float read (float d) const noexcept
    {
        d = std::clamp (d, 1.0f, (float) (mask - 4));
        const int i = (int) d;
        const float f = d - (float) i;
        const float xm1 = tap (i - 1), x0 = tap (i), x1 = tap (i + 1), x2 = tap (i + 2);
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }
    inline float readLinear (float d) const noexcept
    {
        d = std::clamp (d, 0.0f, (float) (mask - 2));
        const int i = (int) d;
        const float f = d - (float) i;
        return tap (i) + (tap (i + 1) - tap (i)) * f;
    }
};

// Smoothly interpolated random LFO (-1..1)
struct DriftLFO
{
    float phase = 0, inc = 0, cur = 0, tgt = 0;
    uint32_t seed = 1;
    inline float rnd() noexcept { seed = seed * 1664525u + 1013904223u; return (float) (seed >> 8) / 8388608.0f - 1.0f; }
    void init (uint32_t s, double fs, float rateHz) { seed = s * 2654435761u + 7u; cur = rnd(); tgt = rnd(); inc = (float) (rateHz / fs); phase = 0; }
    inline float next() noexcept
    {
        phase += inc;
        if (phase >= 1.0f) { phase -= 1.0f; cur = tgt; tgt = rnd(); }
        const float s = phase * phase * (3.0f - 2.0f * phase);
        return cur + (tgt - cur) * s;
    }
};

//==============================================================================
// Lock-free mono analyser feed, read by the UI
struct AudioFifo
{
    static constexpr int Size = 32768;
    juce::AbstractFifo fifo { Size };
    std::vector<float> data = std::vector<float> (Size, 0.0f);

    void push (const float* l, const float* r, int n) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (n, s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i) data[(size_t) (s1 + i)] = 0.5f * (l[i] + r[i]);
        for (int i = 0; i < n2; ++i) data[(size_t) (s2 + i)] = 0.5f * (l[n1 + i] + r[n1 + i]);
        fifo.finishedWrite (n1 + n2);
    }
    int pull (float* dest, int maxN) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (std::min (maxN, fifo.getNumReady()), s1, n1, s2, n2);
        std::copy (data.begin() + s1, data.begin() + s1 + n1, dest);
        std::copy (data.begin() + s2, data.begin() + s2 + n2, dest + n1);
        fifo.finishedRead (n1 + n2);
        return n1 + n2;
    }
};
} // namespace halo
