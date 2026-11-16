#pragma once
#include "Common.h"

namespace halo
{
// Six-band mix EQ: 24 dB/oct low cut, low shelf, two bells, high shelf, 24 dB/oct high cut.
// Parameters glide smoothly (coefficients re-designed every 16 samples while moving).
class Equalizer
{
public:
    struct Params
    {
        float hpf = 10, lpf = 22000;
        float lowF = 120, lowG = 0;
        float m1F = 400, m1G = 0, m1Q = 1;
        float m2F = 3000, m2G = 0, m2Q = 1;
        float highF = 10000, highG = 0;
    };

    static bool hpfActive (float f) { return f > 10.5f; }
    static bool lpfActive (float f) { return f < 21500.0f; }

    // Shared with the UI so the drawn curve is exactly what is heard
    static void design (const Params& p, double fs, BQ out[8])
    {
        out[0] = rbj::highpass (fs, p.hpf, 0.54119610);
        out[1] = rbj::highpass (fs, p.hpf, 1.30656296);
        out[2] = rbj::lowShelf (fs, p.lowF, 0.7071, p.lowG);
        out[3] = rbj::bell (fs, p.m1F, p.m1Q, p.m1G);
        out[4] = rbj::bell (fs, p.m2F, p.m2Q, p.m2G);
        out[5] = rbj::highShelf (fs, p.highF, 0.7071, p.highG);
        out[6] = rbj::lowpass (fs, p.lpf, 0.54119610);
        out[7] = rbj::lowpass (fs, p.lpf, 1.30656296);
    }

    static double responseDb (const Params& p, const BQ c[8], double fs, double f)
    {
        double m = 1.0;
        if (hpfActive (p.hpf)) m *= rbj::magnitude (c[0], fs, f) * rbj::magnitude (c[1], fs, f);
        for (int i = 2; i < 6; ++i) m *= rbj::magnitude (c[i], fs, f);
        if (lpfActive (p.lpf)) m *= rbj::magnitude (c[6], fs, f) * rbj::magnitude (c[7], fs, f);
        return 20.0 * std::log10 (std::max (m, 1.0e-9));
    }

    void prepare (double sr) { fs = sr; first = true; reset(); }
    void reset() { for (auto& b : st) for (auto& s : b) s.reset(); }

    void set (const Params& p) { target = p; if (first) { cur = p; first = false; redesign(); } }

    void process (float* l, float* r, int n) noexcept
    {
        int i = 0;
        while (i < n)
        {
            if (counter <= 0) { glide(); counter = 16; }
            const int todo = std::min (n - i, counter);
            const bool hp = hpfActive (cur.hpf), lp = lpfActive (cur.lpf);
            for (int k = 0; k < todo; ++k)
            {
                float x[2] = { l[i + k], r[i + k] };
                for (int c = 0; c < 2; ++c)
                {
                    float y = x[c];
                    if (hp) { y = st[0][c].process (co[0], y); y = st[1][c].process (co[1], y); }
                    for (int b = 0; b < 4; ++b) y = st[2 + b][c].process (co[2 + b], y);
                    if (lp) { y = st[6][c].process (co[6], y); y = st[7][c].process (co[7], y); }
                    x[c] = y;
                }
                l[i + k] = x[0]; r[i + k] = x[1];
            }
            i += todo;
            counter -= todo;
        }
    }

private:
    static float glideLog (float c, float t, bool& moved)
    {
        if (std::abs (c - t) < c * 0.0005f) return t;
        moved = true;
        return std::exp (std::log (c) + (std::log (t) - std::log (c)) * 0.12f);
    }
    static float glideLin (float c, float t, bool& moved)
    {
        if (std::abs (c - t) < 0.002f) return t;
        moved = true;
        return c + (t - c) * 0.12f;
    }

    void glide()
    {
        bool moved = false;
        cur.hpf = glideLog (cur.hpf, target.hpf, moved);
        cur.lpf = glideLog (cur.lpf, target.lpf, moved);
        cur.lowF = glideLog (cur.lowF, target.lowF, moved);
        cur.m1F = glideLog (cur.m1F, target.m1F, moved);
        cur.m2F = glideLog (cur.m2F, target.m2F, moved);
        cur.highF = glideLog (cur.highF, target.highF, moved);
        cur.m1Q = glideLog (cur.m1Q, target.m1Q, moved);
        cur.m2Q = glideLog (cur.m2Q, target.m2Q, moved);
        cur.lowG = glideLin (cur.lowG, target.lowG, moved);
        cur.m1G = glideLin (cur.m1G, target.m1G, moved);
        cur.m2G = glideLin (cur.m2G, target.m2G, moved);
        cur.highG = glideLin (cur.highG, target.highG, moved);
        if (moved) redesign();
    }
    void redesign() { design (cur, fs, co); }

    double fs = 44100;
    bool first = true;
    int counter = 0;
    Params target, cur;
    BQ co[8];
    BQState st[8][2];
};
} // namespace halo
