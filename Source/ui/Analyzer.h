#pragma once
#include "Widgets.h"

namespace halo::ui
{
//==============================================================================
// 8192-point FFT analyser for the input (pre) and output (post) signal,
// with a 4.5 dB/oct display slope and fast-attack / slow-release smoothing.
class SpectrumEngine
{
public:
    static constexpr int Order = 13, Size = 1 << Order, Bins = Size / 2;

    explicit SpectrumEngine (HaloProcessor& p) : proc (p), fft (Order), window ((size_t) Size, dsp::WindowingFunction<float>::blackmanHarris, false)
    {
        for (int s = 0; s < 2; ++s) { hist[s].assign (Size, 0.0f); db[s].assign (Bins, -120.0f); }
        work.assign (Size * 2, 0.0f);
        scratch.assign (AudioFifo::Size, 0.0f);
    }

    void update()
    {
        pull (proc.preFifo, 0);
        pull (proc.postFifo, 1);
    }

    // smoothed display level (dB incl. slope) at frequency f
    float levelAt (int s, float f) const
    {
        const double fs = proc.currentFs.load();
        const float bin = (float) (f / fs * Size);
        const int i = std::clamp ((int) bin, 1, Bins - 2);
        const float t = std::clamp (bin - (float) i, 0.0f, 1.0f);
        return db[(size_t) s][(size_t) i] * (1 - t) + db[(size_t) s][(size_t) i + 1] * t;
    }
    // max level over a frequency span (for dense high-frequency pixels)
    float levelSpan (int s, float f0, float f1) const
    {
        const double fs = proc.currentFs.load();
        int a = std::clamp ((int) (f0 / fs * Size), 1, Bins - 1), b = std::clamp ((int) (f1 / fs * Size), 1, Bins - 1);
        if (b - a < 2) return levelAt (s, 0.5f * (f0 + f1));
        float m = -200.0f;
        for (int i = a; i <= b; ++i) m = std::max (m, db[(size_t) s][(size_t) i]);
        return m;
    }

private:
    void pull (AudioFifo& f, int s)
    {
        const int n = f.pull (scratch.data(), (int) scratch.size());
        auto& h = hist[(size_t) s];
        if (n <= 0) { decay (s); return; }
        if (n >= Size) std::copy (scratch.begin() + (n - Size), scratch.begin() + n, h.begin());
        else
        {
            std::move (h.begin() + n, h.end(), h.begin());
            std::copy (scratch.begin(), scratch.begin() + n, h.end() - n);
        }
        std::copy (h.begin(), h.end(), work.begin());
        std::fill (work.begin() + Size, work.end(), 0.0f);
        window.multiplyWithWindowingTable (work.data(), (size_t) Size);
        fft.performFrequencyOnlyForwardTransform (work.data(), true);
        const double fs = proc.currentFs.load();
        const float norm = 2.0f / (Size * 0.36f);
        auto& d = db[(size_t) s];
        for (int i = 1; i < Bins; ++i)
        {
            const float freq = (float) (i * fs / Size);
            const float lvl = 20.0f * std::log10 (std::max (work[(size_t) i] * norm, 1.0e-7f)) + 4.5f * std::log2 (std::max (freq, 10.0f) / 1000.0f);
            float& o = d[(size_t) i];
            o = lvl > o ? o + (lvl - o) * 0.65f : o + (lvl - o) * 0.10f;
        }
    }
    void decay (int s) { for (auto& v : db[(size_t) s]) v = v - 0.9f; }

    HaloProcessor& proc;
    dsp::FFT fft;
    dsp::WindowingFunction<float> window;
    std::vector<float> hist[2], db[2], work, scratch;
};

inline float freqToX (float f, float w)  { return std::log (f / 20.0f) / std::log (1000.0f) * w; }
inline float xToFreq (float x, float w)  { return 20.0f * std::pow (1000.0f, x / w); }

//==============================================================================
class SpectrumView : public Component
{
public:
    explicit SpectrumView (SpectrumEngine& e) : eng (e) {}

    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (col::label);
        g.setFont (fontLabel (9.5f));
        g.drawText ("SPECTRUM", b.reduced (16, 10), Justification::topLeft, false);
        g.drawText ("IN / OUT", b.reduced (16, 10), Justification::topRight, false);
        auto plot = b.reduced (16.0f, 10.0f).withTrimmedTop (18.0f).withTrimmedBottom (12.0f);


        // grid
        g.setFont (font (9.0f));
        for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
        {
            const float x = plot.getX() + freqToX (f, plot.getWidth());
            g.setColour (col::cream.withAlpha (0.045f));
            g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
            g.setColour (col::faint);
            g.drawText (f >= 1000 ? String ((int) (f / 1000)) + "k" : String ((int) f), Rectangle<float> (x - 20, plot.getBottom() + 1, 40, 12), Justification::centred, false);
        }
        for (float d : { -60.f, -40.f, -20.f })
        {
            g.setColour (col::cream.withAlpha (0.03f));
            g.drawHorizontalLine ((int) yFor (d, plot), plot.getX(), plot.getRight());
        }

        auto makePath = [&] (int s, bool close)
        {
            Path p;
            const int steps = (int) plot.getWidth() / 2;
            for (int i = 0; i <= steps; ++i)
            {
                const float x0 = (float) i / steps * plot.getWidth();
                const float f0 = xToFreq (x0, plot.getWidth()), f1 = xToFreq (x0 + 2.0f, plot.getWidth());
                const float y = yFor (eng.levelSpan (s, f0, f1), plot);
                if (i == 0) p.startNewSubPath (plot.getX(), y); else p.lineTo (plot.getX() + x0, y);
            }
            if (close) { p.lineTo (plot.getRight(), plot.getBottom()); p.lineTo (plot.getX(), plot.getBottom()); p.closeSubPath(); }
            return p;
        };

        // pre (input) – ghost
        g.setColour (col::cream.withAlpha (0.06f));
        g.fillPath (makePath (0, true));

        // post (output) – gradient neon
        auto post = makePath (1, true);
        ColourGradient grad (moduleColour[0], plot.getX(), 0, moduleColour[4], plot.getRight(), 0, false);
        grad.addColour (0.35, moduleColour[2]);
        grad.addColour (0.65, moduleColour[3]);
        auto gFill = grad;
        gFill.multiplyOpacity (0.16f);
        g.setGradientFill (gFill);
        g.fillPath (post);
        auto line = makePath (1, false);
        g.setColour (col::accent);
        g.strokePath (line, PathStrokeType (1.3f, PathStrokeType::curved));
    }

private:
    static float yFor (float dbv, Rectangle<float> r)
    {
        const float t = std::clamp ((dbv + 90.0f) / 96.0f, 0.0f, 1.0f);
        return r.getBottom() - t * r.getHeight();
    }
    SpectrumEngine& eng;
};

//==============================================================================
// Interactive EQ curve: drag nodes (freq/gain), mouse-wheel for Q, double-click resets.
class EqDisplay : public Component
{
public:
    struct Band { const char* f; const char* g; const char* q; const char* name; };
    static const Band& band (int i)
    {
        static const Band b[6] = {
            { "eq_hpf", nullptr, nullptr, "Low Cut" }, { "eq_low_freq", "eq_low_gain", nullptr, "Low Shelf" },
            { "eq_m1_freq", "eq_m1_gain", "eq_m1_q", "Mid 1" }, { "eq_m2_freq", "eq_m2_gain", "eq_m2_q", "Mid 2" },
            { "eq_high_freq", "eq_high_gain", nullptr, "High Shelf" }, { "eq_lpf", nullptr, nullptr, "High Cut" } };
        return b[i];
    }

    EqDisplay (HaloProcessor& p, SpectrumEngine& e, Colour c) : proc (p), eng (e), colour (c) {}
    std::function<void (int)> onSelect;
    int selected = 2;
    float alphaMul = 1.0f;

    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (Colour (0xff0e0e10));
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (Colour (0xff1e1e1e));
        g.drawRoundedRectangle (b.reduced (0.5f), 3.0f, 1.0f);
        auto plot = b.reduced (4.0f);
        g.saveState();
        g.reduceClipRegion (plot.toNearestInt());

        for (float f : { 100.f, 1000.f, 10000.f })
        {
            g.setColour (col::cream.withAlpha (0.05f));
            g.drawVerticalLine ((int) (plot.getX() + freqToX (f, plot.getWidth())), plot.getY(), plot.getBottom());
        }
        g.setColour (col::cream.withAlpha (0.08f));
        g.drawHorizontalLine ((int) gainToY (0, plot), plot.getX(), plot.getRight());

        // live spectrum behind the curve
        {
            Path sp;
            const int steps = (int) plot.getWidth() / 2;
            for (int i = 0; i <= steps; ++i)
            {
                const float x = (float) i / steps * plot.getWidth();
                const float lv = eng.levelSpan (1, xToFreq (x, plot.getWidth()), xToFreq (x + 2, plot.getWidth()));
                const float y = plot.getBottom() - std::clamp ((lv + 90.0f) / 96.0f, 0.0f, 1.0f) * plot.getHeight();
                if (i == 0) sp.startNewSubPath (plot.getX(), y); else sp.lineTo (plot.getX() + x, y);
            }
            sp.lineTo (plot.getRight(), plot.getBottom()); sp.lineTo (plot.getX(), plot.getBottom()); sp.closeSubPath();
            g.setColour (colour.withAlpha (0.10f * alphaMul));
            g.fillPath (sp);
        }

        // response curve (designed with the exact same code the DSP uses)
        Equalizer::Params ep = readParams();
        BQ co[8];
        const double fs = proc.currentFs.load();
        Equalizer::design (ep, fs, co);
        Path curve, fill;
        const int steps = (int) plot.getWidth();
        for (int i = 0; i <= steps; ++i)
        {
            const float x = plot.getX() + (float) i;
            const float f = xToFreq ((float) i, plot.getWidth());
            const float y = gainToY ((float) Equalizer::responseDb (ep, co, fs, f), plot);
            if (i == 0) { curve.startNewSubPath (x, y); fill.startNewSubPath (x, gainToY (0, plot)); }
            curve.lineTo (x, y); fill.lineTo (x, y);
        }
        fill.lineTo (plot.getRight(), gainToY (0, plot)); fill.closeSubPath();
        g.setColour (colour.withAlpha (0.13f * alphaMul));
        g.fillPath (fill);
        glowStroke (g, curve, colour, 1.8f, alphaMul);

        // nodes
        for (int i = 0; i < 6; ++i)
        {
            const auto pt = nodePos (i, plot, ep);
            const bool sel = i == selected;
            const float r = sel ? 6.5f : 4.5f;
            g.setColour (colour.withAlpha ((sel ? 0.35f : 0.15f) * alphaMul));
            g.fillEllipse (pt.x - r * 2, pt.y - r * 2, r * 4, r * 4);
            g.setColour ((sel ? colour.interpolatedWith (col::cream, 0.6f) : colour).withAlpha (alphaMul));
            g.fillEllipse (pt.x - r, pt.y - r, r * 2, r * 2);
            g.setColour (Colours::black.withAlpha (0.7f));
            g.setFont (font (9.0f, true));
            g.drawText (String (i + 1), Rectangle<float> (pt.x - r, pt.y - r, r * 2, r * 2), Justification::centred, false);
        }
        g.restoreState();
    }

    void mouseDown (const MouseEvent& e) override
    {
        auto plot = getLocalBounds().toFloat().reduced (4.0f);
        const auto ep = readParams();
        float best = 1.0e9f; int bi = -1;
        for (int i = 0; i < 6; ++i)
        {
            const float d = nodePos (i, plot, ep).getDistanceFrom (e.position);
            if (d < best) { best = d; bi = i; }
        }
        if (bi < 0 || best > 28.0f) { dragging = -1; return; }
        dragging = bi;
        selected = bi;
        if (onSelect) onSelect (bi);
        proc.undo.beginNewTransaction();
        gesture (true);
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragging < 0) return;
        auto plot = getLocalBounds().toFloat().reduced (4.0f);
        const auto& bd = band (dragging);
        const float f = xToFreq (std::clamp (e.position.x - plot.getX(), 0.0f, plot.getWidth()), plot.getWidth());
        set (bd.f, f);
        if (bd.g != nullptr) set (bd.g, yToGain (e.position.y, plot));
        repaint();
    }
    void mouseUp (const MouseEvent&) override { if (dragging >= 0) gesture (false); dragging = -1; }
    void mouseDoubleClick (const MouseEvent&) override
    {
        const auto& bd = band (selected);
        auto reset = [&] (const char* id) { if (id) if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->getDefaultValue()); };
        reset (bd.g); reset (bd.q);
        if (bd.g == nullptr) reset (bd.f);
    }
    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) override
    {
        const auto& bd = band (selected);
        const char* id = bd.q != nullptr ? bd.q : bd.f;
        if (auto* p = proc.apvts.getParameter (id))
            p->setValueNotifyingHost (std::clamp (p->getValue() + w.deltaY * 0.06f, 0.0f, 1.0f));
    }

private:
    Equalizer::Params readParams() const
    {
        auto v = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
        Equalizer::Params p;
        p.hpf = v ("eq_hpf"); p.lpf = v ("eq_lpf");
        p.lowF = v ("eq_low_freq"); p.lowG = v ("eq_low_gain");
        p.m1F = v ("eq_m1_freq"); p.m1G = v ("eq_m1_gain"); p.m1Q = v ("eq_m1_q");
        p.m2F = v ("eq_m2_freq"); p.m2G = v ("eq_m2_gain"); p.m2Q = v ("eq_m2_q");
        p.highF = v ("eq_high_freq"); p.highG = v ("eq_high_gain");
        return p;
    }
    Point<float> nodePos (int i, Rectangle<float> plot, const Equalizer::Params& p) const
    {
        const float fv[6] = { p.hpf, p.lowF, p.m1F, p.m2F, p.highF, p.lpf };
        const float gv[6] = { 0, p.lowG, p.m1G, p.m2G, p.highG, 0 };
        float x = plot.getX() + freqToX (fv[i], plot.getWidth());
        x = std::clamp (x, plot.getX() + 6.0f, plot.getRight() - 6.0f);
        return { x, gainToY (gv[i], plot) };
    }
    static float gainToY (float g, Rectangle<float> r) { return r.getCentreY() - g / 20.0f * r.getHeight() * 0.5f; }
    static float yToGain (float y, Rectangle<float> r) { return std::clamp ((r.getCentreY() - y) / (r.getHeight() * 0.5f) * 20.0f, -18.0f, 18.0f); }

    void set (const char* id, float v)
    {
        if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
    }
    void gesture (bool begin)
    {
        const auto& bd = band (dragging);
        for (auto id : { bd.f, bd.g })
            if (id != nullptr)
                if (auto* p = proc.apvts.getParameter (id)) { if (begin) p->beginChangeGesture(); else p->endChangeGesture(); }
    }

    HaloProcessor& proc;
    SpectrumEngine& eng;
    Colour colour;
    int dragging = -1;
};
} // namespace halo::ui
