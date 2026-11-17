#pragma once
#include "Controls.h"
#include "Analyzer.h"
#include "Ear.h"
#include "../DeptPresets.h"

namespace halo::ui
{
//==============================================================================
// Animated department characters
namespace icons
{
    inline Path chevron (float cx, float cy, float w, float h, bool down)
    {
        Path p;
        const float s = down ? 1.0f : -1.0f;
        p.startNewSubPath (cx - w, cy - h * s); p.lineTo (cx, cy + h * s); p.lineTo (cx + w, cy - h * s);
        return p;
    }

    inline void dynamics (Graphics& g, Rectangle<float> r, float t, float act, float a)
    {
        const float s = std::min (r.getWidth(), r.getHeight());
        const float cx = r.getCentreX(), cy = r.getCentreY();
        const float bob = std::sin (t * 2.4f) * 2.0f;
        const float gap = s * (0.36f - 0.16f * act);
        glowStroke (g, chevron (cx, cy - gap + bob, s * 0.15f, s * 0.06f, true), col::accent, 2.4f, a);
        glowStroke (g, chevron (cx, cy + gap - bob, s * 0.15f, s * 0.06f, false), col::accent, 2.4f, a);
        Path wave;
        const float amp = s * 0.12f * (1.0f - 0.7f * act) * (0.75f + 0.25f * std::sin (t * 1.7f));
        for (int i = 0; i <= 60; ++i)
        {
            const float u = (float) i / 60.0f;
            const float x = cx - s * 0.5f + u * s;
            const float y = cy + std::sin (u * 14.0f - t * 5.0f) * amp * std::sin (u * kPiF);
            if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
        }
        glowStroke (g, wave, col::accent, 2.0f, a);
    }

    inline void tone (Graphics& g, Rectangle<float> r, float t, SpectrumEngine& eng, float a, float* smooth)
    {
        const float freqs[7] = { 70, 180, 450, 1100, 2800, 6500, 13000 };
        const float w = std::min (r.getWidth() * 0.86f, r.getHeight() * 1.6f), x0 = r.getCentreX() - w * 0.5f;
        Path curve, caps;
        Point<float> pts[7];
        for (int i = 0; i < 7; ++i)
        {
            float v = std::clamp ((eng.levelAt (1, freqs[i]) + 80.0f) / 70.0f, 0.05f, 1.0f);
            v = v * 0.8f + 0.1f * (0.5f + 0.5f * std::sin (t * 1.4f + (float) i * 0.9f));
            smooth[i] += (v - smooth[i]) * 0.2f;
            pts[i] = { x0 + w * (float) i / 6.0f, r.getBottom() - 10.0f - smooth[i] * (r.getHeight() - 26.0f) };
            g.setColour (col::accent.withAlpha (0.12f * a));
            g.fillRoundedRectangle (pts[i].x - 1.0f, r.getY() + 8.0f, 2.0f, r.getHeight() - 16.0f, 1.0f);
            caps.addRoundedRectangle (pts[i].x - 7.0f, pts[i].y - 2.5f, 14.0f, 5.0f, 2.5f);
        }
        curve.startNewSubPath (pts[0]);
        for (int i = 1; i < 7; ++i)
        {
            const auto m = (pts[i - 1] + pts[i]) * 0.5f;
            curve.quadraticTo (pts[i - 1], m);
        }
        curve.lineTo (pts[6]);
        glowStroke (g, curve, col::accent, 1.6f, a * 0.7f);
        g.setColour (col::accent.withAlpha (a));
        g.fillPath (caps);
    }

    inline void space (Graphics& g, Rectangle<float> r, float t, float act, float decay, float dly, float a)
    {
        const float s = std::min (r.getWidth(), r.getHeight());
        const float cx = r.getCentreX(), cy = r.getCentreY();
        const float speed = 0.10f + 0.12f * (1.0f - decay);
        for (int i = 0; i < 7; ++i)
        {
            const float z = std::fmod ((float) i / 7.0f + t * speed, 1.0f);
            const float rr = s * (0.04f + 0.55f * z * z);
            Path ring;
            ring.addEllipse (cx - rr * 1.5f, cy - rr * 0.62f, rr * 3.0f, rr * 1.24f);
            glowStroke (g, ring, col::accent, 1.0f + 1.2f * z, a * std::sin (z * kPiF) * (0.45f + 0.55f * clamp01 (act + 0.25f)));
        }
        for (int i = 0; i < 4; ++i)
        {
            const float ph = std::fmod (t * 0.5f + (float) i * 0.25f, 1.0f);
            const float x = cx - s * 0.75f + ph * s * 1.5f, y = cy + std::sin (ph * kPiF * 2.0f) * s * 0.05f;
            g.setColour (col::accent.withAlpha ((1.0f - ph) * a * (0.15f + 0.85f * dly)));
            g.fillEllipse (x - 2.5f, y - 2.5f, 5, 5);
        }
    }

    inline void fx (Graphics& g, Rectangle<float> r, float t, float act, float drive, float dbl, float a)
    {
        const float s = std::min (r.getWidth(), r.getHeight());
        const float cx = r.getCentreX(), cy = r.getCentreY();
        // doubled ghost rings behind
        for (int i = 2; i >= 1; --i)
        {
            const float off = (s * 0.06f + s * 0.12f * dbl) * (float) i * std::sin (t * 0.8f + (float) i);
            Path ring; const float rr = s * 0.34f;
            ring.addEllipse (cx - rr + off, cy - rr, rr * 2, rr * 2);
            glowStroke (g, ring, col::accent, 1.0f, a * (0.12f + 0.3f * dbl));
        }
        const float A = s * 0.28f * (1.0f + 2.2f * drive) * (0.85f + 0.3f * act);
        const float ceil = s * 0.24f;
        Path wave;
        for (int i = 0; i <= 80; ++i)
        {
            const float u = (float) i / 80.0f;
            const float x = cx - s * 0.5f + u * s;
            const float v = std::clamp (std::sin (u * kPiF * 3.0f - t * 3.2f) * A, -ceil, ceil);
            if (i == 0) wave.startNewSubPath (x, cy - v); else wave.lineTo (x, cy - v);
        }
        glowStroke (g, wave, col::accent, 2.2f, a);
    }
}

//==============================================================================
// Top-row card: power, title, animated character, department preset name with arrows + dots, list menu
class DeptCard : public Component
{
public:
    DeptCard (HaloProcessor& p, int deptId) : proc (p), d (deptId), power ("power"), menu ("menu"), prev ("<"), next (">")
    {
        for (auto* b : { &power, &menu, &prev, &next }) addAndMakeVisible (b);
        power.drawGlyph = [this] (Graphics& g, Rectangle<float> r, Colour)
        {
            const float cx = r.getCentreX(), cy = r.getCentreY(), rr = 6.5f;
            Path pp;
            pp.addCentredArc (cx, cy, rr, rr, 0.0f, 0.7f, kPiF * 2.0f - 0.7f, true);
            pp.startNewSubPath (cx, cy - rr - 1.5f); pp.lineTo (cx, cy - 1.5f);
            g.setColour (on ? col::accent : col::dim);
            g.strokePath (pp, PathStrokeType (1.5f, PathStrokeType::curved, PathStrokeType::rounded));
        };
        menu.drawGlyph = [] (Graphics& g, Rectangle<float> r, Colour c)
        {
            g.setColour (c);
            for (int i = -1; i <= 1; ++i) g.fillRoundedRectangle (r.getCentreX() - 6.0f, r.getCentreY() + i * 4.0f - 0.75f, 12.0f, 1.5f, 0.75f);
        };
        auto arrow = [] (bool left)
        {
            return [left] (Graphics& g, Rectangle<float> r, Colour c)
            {
                Path p; const float cx = r.getCentreX(), cy = r.getCentreY(), s = left ? 1.0f : -1.0f;
                p.startNewSubPath (cx + 3 * s, cy - 6); p.lineTo (cx - 3 * s, cy); p.lineTo (cx + 3 * s, cy + 6);
                g.setColour (c);
                g.strokePath (p, PathStrokeType (1.5f, PathStrokeType::curved, PathStrokeType::rounded));
            };
        };
        prev.drawGlyph = arrow (true);
        next.drawGlyph = arrow (false);
        for (auto* b : { &power, &menu, &prev, &next }) b->setPaintFrame (false);
        power.onClick = [this] { proc.undo.beginNewTransaction(); dept::setOn (proc.apvts, d, ! dept::isOn (proc.apvts, d)); };
        prev.onClick = [this] { step (-1); };
        next.onClick = [this] { step (1); };
        menu.onClick = [this] { showMenu(); };
        power.setTooltip ("Department on/off");
        menu.setTooltip ("All " + String (dept::title (d)).toLowerCase() + " presets");
        icon.draw = [this] (Graphics& g, Rectangle<float> r) { if (drawIcon) drawIcon (g, r, t, act, 0.3f + 0.7f * onAmt); };
        addAndMakeVisible (icon);
    }

    std::function<void (Graphics&, Rectangle<float>, float, float, float)> drawIcon;
    std::function<float()> activity;

    // use a live component (the ear) as the character instead of a drawn icon
    void setCustomIcon (Component* c) { custom = c; addAndMakeVisible (c); icon.setVisible (false); resized(); }

    void tick (float dt)
    {
        t += dt;
        const float a = activity ? activity() : 0.0f;
        act = a > act ? act + (a - act) * 0.5f : act + (a - act) * 0.08f;
        on = dept::isOn (proc.apvts, d);
        onAmt += ((on ? 1.0f : 0.0f) - onAmt) * 0.18f;
        if ((matchTimer -= dt) <= 0.0f) { matchTimer = 0.25f; current = dept::match (proc.apvts, d); }
        icon.repaint();
        repaint (getLocalBounds().removeFromBottom (64));
        power.repaint();
    }
    bool isOn() const { return on; }

    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (col::accent.withMultipliedAlpha (0.45f + 0.55f * onAmt));
        g.setFont (font (12.5f, true).withExtraKerningFactor (0.12f));
        g.drawText (dept::title (d), b.removeFromTop (30.0f).withTrimmedLeft (36), Justification::centredLeft, false);

        const auto& list = dept::presets (d);
        auto nameRow = Rectangle<float> (b.getX() + 30, (float) getHeight() - 58.0f, b.getWidth() - 60, 26.0f);
        const String name = current >= 0 ? String (list[(size_t) current].name) : String ("CUSTOM");
        g.setFont (font (15.0f, true).withExtraKerningFactor (0.08f));
        g.setColour ((current >= 0 ? col::accent : col::label).withMultipliedAlpha (0.45f + 0.55f * onAmt));
        g.drawText (name, nameRow, Justification::centred, true);

        // page dots
        const int n = (int) list.size();
        const float dx = 9.0f, y = (float) getHeight() - 22.0f;
        const float x0 = b.getCentreX() - dx * (float) (n - 1) * 0.5f;
        for (int i = 0; i < n; ++i)
        {
            const bool sel = i == current;
            g.setColour (sel ? col::accent : Colour (0xff3a3a3a));
            const float rr = sel ? 3.0f : 2.0f;
            g.fillEllipse (x0 + dx * (float) i - rr, y - rr, rr * 2, rr * 2);
        }
    }

    void resized() override
    {
        auto b = getLocalBounds();
        auto head = b.removeFromTop (30);
        power.setBounds (head.removeFromLeft (32).withSizeKeepingCentre (26, 26).translated (4, 0));
        menu.setBounds (head.removeFromRight (32).withSizeKeepingCentre (26, 26));
        b.removeFromBottom (64);
        (custom != nullptr ? custom : (Component*) &icon)->setBounds (b.reduced (8, 2));
        const int ny = getHeight() - 58;
        prev.setBounds (6, ny, 26, 26);
        next.setBounds (getWidth() - 32, ny, 26, 26);
    }

private:
    void step (int delta)
    {
        const int n = (int) dept::presets (d).size();
        const int base = current < 0 ? (delta > 0 ? -1 : 0) : current;
        apply (((base + delta) % n + n) % n);
    }
    void apply (int i)
    {
        proc.undo.beginNewTransaction();
        dept::apply (proc.apvts, d, i);
        if (! dept::isOn (proc.apvts, d)) dept::setOn (proc.apvts, d, true);
        current = i;
        matchTimer = 0.3f;
        repaint();
    }
    void showMenu()
    {
        PopupMenu m;
        m.addSectionHeader (dept::title (d));
        const auto& list = dept::presets (d);
        for (int i = 0; i < (int) list.size(); ++i) m.addItem (i + 1, list[(size_t) i].name, true, i == current);
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&menu), [this] (int r) { if (r > 0) apply (r - 1); });
    }

    HaloProcessor& proc;
    int d;
    GlyphButton power, menu, prev, next;
    Canvas icon;
    Component* custom = nullptr;
    float t = 0, act = 0, onAmt = 1, matchTimer = 0;
    bool on = true;
    int current = -1;
};

//==============================================================================
class DetailPanel : public Component
{
public:
    explicit DetailPanel (HaloProcessor& p) : proc (p) {}
    virtual void tick (float) {}

protected:
    ValueChip& chip (const char* id, const String& label, bool big = false)
    {
        auto c = std::make_unique<ValueChip> (proc, id, label, big);
        addAndMakeVisible (*c);
        auto& r = *c; owned.push_back (std::move (c)); return r;
    }
    Knob& knob (const String& label, const char* id)
    {
        auto k = std::make_unique<Knob> (label, col::accent);
        k->attach (proc, id);
        addAndMakeVisible (*k);
        auto& r = *k; owned.push_back (std::move (k)); return r;
    }
    FaderBox& fader (const char* id, const String& label)
    {
        auto f = std::make_unique<FaderBox> (proc, id, label);
        addAndMakeVisible (*f);
        auto& r = *f; owned.push_back (std::move (f)); return r;
    }
    static void row (Rectangle<int> r, std::initializer_list<Component*> cs, int gap = 6)
    {
        const int n = (int) cs.size();
        const int w = (r.getWidth() - gap * (n - 1)) / n;
        int i = 0;
        for (auto* c : cs) { if (c) c->setBounds (r.getX() + i * (w + gap), r.getY(), w, r.getHeight()); ++i; }
    }
    static void stack (Rectangle<int>& r, std::initializer_list<Component*> cs, int h = 26, int gap = 5)
    {
        for (auto* c : cs) { if (c) c->setBounds (r.removeFromTop (h)); r.removeFromTop (gap); }
    }
    float param (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }

    HaloProcessor& proc;
    std::vector<std::unique_ptr<Component>> owned;
};

//==============================================================================
class DynamicsDetail : public DetailPanel
{
public:
    explicit DynamicsDetail (HaloProcessor& p) : DetailPanel (p),
        gate ("GATE", 24, true), gr ("GR", 24, false), ds ("DS", 24, false),
        comp (knob ("Comp", "comp_amount")), style (chip ("comp_style", "Color")), speed (chip ("comp_speed", "Speed")),
        deess (knob ("De-Ess", "enh_deess")), focus (chip ("enh_deess_freq", "Focus")), denoise (chip ("enh_denoise", "Gate")),
        input (chip ("comp_input", "Input")), output (chip ("comp_output", "Output")), mix (chip ("comp_mix", "Mix")),
        sc (chip ("comp_hpf", "SC HPF")), autoG (chip ("comp_auto", "Auto Gain")), pre (chip ("enh_pre", "DS Pre"))
    {
        for (auto* m : { &gate, &gr, &ds }) addAndMakeVisible (m);
    }
    void tick (float dt) override
    {
        gate.setValue (proc.meters.noiseGr.exchange (0.0f), dt);
        gr.setValue (proc.meters.uiComp, dt);
        ds.setValue (proc.meters.essGr.exchange (0.0f), dt);
    }
    void resized() override
    {
        auto b = getLocalBounds().reduced (10, 6);
        auto bottom = b.removeFromBottom (3 * 26 + 2 * 6);
        b.removeFromBottom (14);
        auto meters = b.removeFromLeft (102);
        auto mw = meters.getWidth();
        gate.setBounds (meters.removeFromLeft (mw * 46 / 100));
        gr.setBounds (meters.removeFromLeft (meters.getWidth() / 2));
        ds.setBounds (meters);
        b.removeFromLeft (12);
        const int kh = (b.getHeight() - 2 * (26 + 5) * 2 - 12) / 2;
        comp.setBounds (b.removeFromTop (kh)); b.removeFromTop (4);
        stack (b, { &style, &speed });
        b.removeFromTop (8);
        deess.setBounds (b.removeFromTop (kh)); b.removeFromTop (4);
        stack (b, { &focus, &denoise });
        row (bottom.removeFromTop (26), { &input, &output }); bottom.removeFromTop (6);
        row (bottom.removeFromTop (26), { &mix, &sc }); bottom.removeFromTop (6);
        row (bottom.removeFromTop (26), { &autoG, &pre });
    }

private:
    ReductionMeter gate, gr, ds;
    Knob& comp; ValueChip &style, &speed;
    Knob& deess; ValueChip &focus, &denoise, &input, &output, &mix, &sc, &autoG, &pre;
};

//==============================================================================
class ToneDetail : public DetailPanel
{
public:
    ToneDetail (HaloProcessor& p, SpectrumEngine& e) : DetailPanel (p), display (p, e, col::accent),
        low (fader ("eq_low_gain", "Low")), mid (fader ("eq_m1_gain", "Mid")), pres (fader ("eq_m2_gain", "Pres")),
        high (fader ("eq_high_gain", "High")), body (fader ("enh_body", "Body")), air (fader ("enh_air", "Air+")),
        lowCut (chip ("eq_hpf", "Low Cut")), highCut (chip ("eq_lpf", "High Cut")), pre (chip ("eq_pre", "Pre Comp"))
    {
        addAndMakeVisible (display);
        for (int i = 0; i < 6; ++i)
        {
            static const char* n[6] = { "LC", "LO", "M1", "M2", "HI", "HC" };
            auto b = std::make_unique<GlyphButton> (n[i], col::accent);
            b->onClick = [this, i] { select (i); };
            addAndMakeVisible (*b);
            bands.push_back (std::move (b));
        }
        display.onSelect = [this] (int i) { select (i); };
        select (2);
    }
    void tick (float) override { display.repaint(); }
    void resized() override
    {
        auto b = getLocalBounds().reduced (10, 6);
        display.setBounds (b.removeFromTop (164)); b.removeFromTop (6);
        auto br = b.removeFromTop (22);
        const int w = br.getWidth() / 6;
        for (int i = 0; i < 6; ++i) bands[(size_t) i]->setBounds (br.getX() + i * w, br.getY(), w - 3, 22);
        b.removeFromTop (6);
        selRow = b.removeFromTop (26);
        layoutSel();
        b.removeFromTop (12);
        auto bottom = b.removeFromBottom (2 * 26 + 6);
        b.removeFromBottom (8);
        row (b, { &low, &mid, &pres, &high, &body, &air }, 2);
        row (bottom.removeFromTop (26), { &lowCut, &highCut }); bottom.removeFromTop (6);
        pre.setBounds (bottom.removeFromTop (26));
    }

private:
    void select (int i)
    {
        display.selected = i;
        for (int k = 0; k < 6; ++k) { bands[(size_t) k]->selected = k == i; bands[(size_t) k]->repaint(); }
        const auto& bd = EqDisplay::band (i);
        selF.reset (new ValueChip (proc, bd.f, "Freq"));
        addAndMakeVisible (*selF);
        selQ.reset (bd.q != nullptr ? new ValueChip (proc, bd.q, "Q") : bd.g != nullptr ? new ValueChip (proc, bd.g, "Gain") : nullptr);
        if (selQ) addAndMakeVisible (*selQ);
        layoutSel();
        display.repaint();
    }
    void layoutSel()
    {
        if (selF == nullptr || selRow.isEmpty()) return;
        if (selQ) row (selRow, { selF.get(), selQ.get() }); else selF->setBounds (selRow);
    }

    EqDisplay display;
    FaderBox &low, &mid, &pres, &high, &body, &air;
    ValueChip &lowCut, &highCut, &pre;
    std::vector<std::unique_ptr<GlyphButton>> bands;
    std::unique_ptr<ValueChip> selF, selQ;
    Rectangle<int> selRow;
};

//==============================================================================
class SpaceDetail : public DetailPanel
{
public:
    explicit SpaceDetail (HaloProcessor& p) : DetailPanel (p),
        vrbDuck (chip ("rev_duck", "Vrb Duck")), dlyDuck (chip ("dly_duck", "Dly Duck")),
        reverb (knob ("Reverb", "rev_mix")), delay (knob ("Delay", "dly_mix")),
        rType (chip ("rev_type", "")), rDecay (chip ("rev_decay", "Time")), rPre (chip ("rev_predelay", "Pre")),
        rDamp (chip ("rev_damp", "Damp")), rLow (chip ("rev_lowcut", "Low Cut")), rWidth (chip ("rev_width", "Width")),
        dType (chip ("dly_type", "")), dRate (chip ("dly_rate", "Rate")), dTime (chip ("dly_time", "Time")),
        dFb (chip ("dly_feedback", "Fdbk")), dTone (chip ("dly_tone", "Tone")), dSync (chip ("dly_sync", "Sync")) {}

    void tick (float dt) override
    {
        auto upd = [dt] (float raw, float& v) { const float x = clamp01 (raw * 2.2f); v = x > v ? x : std::max (0.0f, v - 1.4f * dt); };
        upd (proc.meters.revAct.exchange (0.0f), rv);
        upd (proc.meters.dlyAct.exchange (0.0f), dv);
        const bool sync = param ("dly_sync") > 0.5f;
        if (dRate.isVisible() != sync) { dRate.setVisible (sync); dTime.setVisible (! sync); }
        repaint (screen);
    }
    void paint (Graphics& g) override
    {
        auto s = screen.toFloat();
        g.setColour (Colour (0xff0e0e10));
        g.fillRoundedRectangle (s, 4.0f);
        g.setColour (col::border);
        g.drawRoundedRectangle (s.reduced (0.5f), 4.0f, 1.0f);
        auto half = s.reduced (10, 8);
        auto left = half.removeFromLeft (half.getWidth() / 2).withTrimmedRight (6), right = half.withTrimmedLeft (6);
        drawScreen (g, left, "REVERB", rv, param ("rev_mix") * 0.01f);
        drawScreen (g, right, "DELAY", dv, param ("dly_mix") * 0.01f);
    }
    void resized() override
    {
        auto b = getLocalBounds().reduced (10, 6);
        screen = b.removeFromTop (112);
        b.removeFromTop (6);
        row (b.removeFromTop (26), { &vrbDuck, &dlyDuck });
        b.removeFromTop (12);
        auto l = b.removeFromLeft (b.getWidth() / 2).withTrimmedRight (5), r = b.withTrimmedLeft (5);
        const int kh = 104;
        reverb.setBounds (l.removeFromTop (kh)); l.removeFromTop (6);
        delay.setBounds (r.removeFromTop (kh)); r.removeFromTop (6);
        stack (l, { &rType, &rDecay, &rPre, &rDamp, &rLow, &rWidth });
        auto rr = r;
        stack (r, { &dType, &dRate, &dFb, &dTone, &dSync });
        rr.removeFromTop (31);
        dTime.setBounds (rr.removeFromTop (26));
    }

private:
    void drawScreen (Graphics& g, Rectangle<float> r, const String& name, float act, float amount)
    {
        g.setColour (col::label);
        g.setFont (fontLabel (8.8f));
        g.drawText (name, r.removeFromTop (14), Justification::centredLeft, false);
        // two animated level columns + an amount column, segmented
        const int segs = 16;
        const float colW = 9.0f, gapW = 5.0f;
        const float values[3] = { act, act * (0.85f + 0.15f * std::sin ((float) Time::getMillisecondCounter() * 0.004f)), amount };
        for (int c = 0; c < 3; ++c)
        {
            auto col = Rectangle<float> (r.getX() + (float) c * (colW + gapW) + (c == 2 ? 8.0f : 0.0f), r.getY() + 4, colW, r.getHeight() - 6);
            const float sh = col.getHeight() / segs;
            const int lit = (int) std::round (std::clamp (values[c], 0.0f, 1.0f) * segs);
            for (int i = 0; i < segs; ++i)
            {
                const bool l = (segs - 1 - i) < lit;
                g.setColour (l ? (c == 2 ? col::cream.withAlpha (0.8f) : col::accent) : Colour (0xff1c1c1c));
                g.fillRect (col.getX(), col.getY() + i * sh + 0.75f, col.getWidth(), sh - 1.5f);
            }
        }
        g.setColour (col::text);
        g.setFont (fontBold (16.0f));
        g.drawText (String ((int) std::round (amount * 100.0f)) + "%", r.withTrimmedLeft (50), Justification::centredRight, false);
    }

    ValueChip &vrbDuck, &dlyDuck;
    Knob &reverb, &delay;
    ValueChip &rType, &rDecay, &rPre, &rDamp, &rLow, &rWidth, &dType, &dRate, &dTime, &dFb, &dTone, &dSync;
    Rectangle<int> screen;
    float rv = 0, dv = 0;
};

//==============================================================================
class FxDetail : public DetailPanel
{
public:
    explicit FxDetail (HaloProcessor& p) : DetailPanel (p),
        drive (knob ("Drive", "sat_drive")), type (chip ("sat_type", "")), mix (chip ("sat_mix", "Mix")), gain (chip ("sat_gain", "Gain")),
        preFx (chip ("sat_prefx", "Pre FX")), clip (chip ("sat_clip", "Clip")),
        dbl (knob ("Doubler", "mul_amount")), voices (chip ("mul_voices", "")), spread (chip ("mul_spread", "Spread")),
        detune (chip ("mul_detune", "Detune")), offset (chip ("mul_offset", "Offset")), low (chip ("mul_low", "Low Cut")) {}

    void resized() override
    {
        auto b = getLocalBounds().reduced (10, 6);
        const int h = (b.getHeight() - 16) / 2;
        auto top = b.removeFromTop (h), bot = b.withTrimmedTop (16);
        { auto k = top.removeFromTop (top.getHeight() - 3 * 31); drive.setBounds (k.withSizeKeepingCentre (std::min (k.getWidth(), 150), k.getHeight())); }
        top.removeFromTop (5);
        row (top.removeFromTop (26), { &type, &mix }); top.removeFromTop (5);
        row (top.removeFromTop (26), { &gain, &clip }); top.removeFromTop (5);
        preFx.setBounds (top.removeFromTop (26));
        { auto k = bot.removeFromTop (bot.getHeight() - 3 * 31); dbl.setBounds (k.withSizeKeepingCentre (std::min (k.getWidth(), 150), k.getHeight())); }
        bot.removeFromTop (5);
        row (bot.removeFromTop (26), { &voices, &spread }); bot.removeFromTop (5);
        row (bot.removeFromTop (26), { &detune, &offset }); bot.removeFromTop (5);
        low.setBounds (bot.removeFromTop (26));
    }

private:
    Knob& drive; ValueChip &type, &mix, &gain, &preFx, &clip;
    Knob& dbl; ValueChip &voices, &spread, &detune, &offset, &low;
};

//==============================================================================
// Master: typeable limiter ceiling + typeable loudness target with auto gain inside a
// tolerance window, LUFS gauge, glue/limiter reduction, finishing controls.
class MasterDetail : public DetailPanel
{
public:
    explicit MasterDetail (HaloProcessor& p) : DetailPanel (p),
        target (chip ("mst_target", "Target", true)), ceiling (chip ("mst_ceiling", "Limit", true)),
        autoOn (chip ("mst_auto", "Auto")), tol (chip ("mst_tol", "Range")),
        glue (knob ("Glue", "mst_glue")), loud (knob ("Drive", "mst_loud")),
        tilt (chip ("mst_tilt", "Tilt")), width (chip ("mst_width", "Width")), mono (chip ("mst_mono", "Mono")), rel (chip ("mst_release", "Release"))
    {
        target.setTooltip ("Loudness target: double-click and type e.g. -14. With AUTO on, HALO rides the gain to stay within the range.");
        ceiling.setTooltip ("Limiter ceiling: double-click and type e.g. -1. Nothing passes above this level.");
    }
    void tick (float dt) override
    {
        lufsS += (proc.meters.lufsS.load() - lufsS) * 0.15f;
        lufsI = proc.meters.lufsI.load();
        autoDb += (proc.meters.autoGain.load() - autoDb) * 0.2f;
        glueGr = std::max (proc.meters.glueGr.exchange (0.0f), glueGr - 14.0f * dt);
        limGr = std::max (proc.meters.uiLim, limGr - 14.0f * dt);
        repaint (gauge.expanded (4));
        repaint (bars);
    }
    void mouseDown (const MouseEvent& e) override { if (gauge.contains (e.getPosition())) proc.meters.resetLoudness = true; }
    void paint (Graphics& g) override
    {
        // loudness gauge with target window
        auto gr = gauge.toFloat();
        const float cx = gr.getCentreX(), cy = gr.getCentreY() + 6.0f, R = std::min (gr.getWidth(), gr.getHeight()) * 0.44f;
        const float a0 = -kPiF * 0.75f, a1 = kPiF * 0.75f;
        auto angFor = [&] (float lufs) { return a0 + (a1 - a0) * std::clamp ((lufs + 36.0f) / 36.0f, 0.0f, 1.0f); };
        Path track; track.addCentredArc (cx, cy, R, R, 0, a0, a1, true);
        g.setColour (Colour (0xff262626));
        g.strokePath (track, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));
        const float tgt = param ("mst_target"), tw = param ("mst_tol");
        const bool autoMode = param ("mst_auto") > 0.5f;
        Path window; window.addCentredArc (cx, cy, R, R, 0, angFor (tgt - tw), angFor (tgt + tw), true);
        g.setColour (col::accent.withAlpha (autoMode ? 0.35f : 0.15f));
        g.strokePath (window, PathStrokeType (10.0f, PathStrokeType::curved, PathStrokeType::butt));
        if (lufsS > -69.0f)
        {
            Path v; v.addCentredArc (cx, cy, R, R, 0, a0, angFor (lufsS), true);
            g.setColour (col::accent);
            g.strokePath (v, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        const float ta = angFor (tgt);
        g.setColour (col::cream);
        g.drawLine (cx + (R - 9) * std::sin (ta), cy - (R - 9) * std::cos (ta), cx + (R + 9) * std::sin (ta), cy - (R + 9) * std::cos (ta), 2.0f);
        g.setColour (col::text);
        g.setFont (fontBold (R * 0.42f));
        g.drawText (lufsS > -69.0f ? String (lufsS, 1) : String ("--"), Rectangle<float> (cx - R, cy - R * 0.36f, R * 2, R * 0.48f), Justification::centred, false);
        g.setColour (col::label);
        g.setFont (fontLabel (8.5f));
        g.drawText ("LUFS S   I " + (lufsI > -69.0f ? String (lufsI, 1) : String ("--")), Rectangle<float> (cx - R, cy + R * 0.14f, R * 2, 12), Justification::centred, false);
        if (autoMode)
        {
            g.setColour (col::accent);
            g.setFont (font (10.0f, true));
            g.drawText ("AUTO " + String (autoDb >= 0 ? "+" : "") + String (autoDb, 1) + " dB", Rectangle<float> (cx - R, cy + R * 0.34f, R * 2, 14), Justification::centred, false);
        }

        // reduction bars
        auto br = bars.toFloat();
        const char* names[2] = { "GLUE", "LIMIT" };
        const float vals[2] = { glueGr, limGr };
        for (int i = 0; i < 2; ++i)
        {
            auto rr = br.removeFromTop (br.getHeight() / (2 - i));
            g.setColour (col::label);
            g.setFont (fontLabel (8.8f));
            g.drawText (names[i], rr.removeFromLeft (40), Justification::centredLeft, false);
            g.setColour (col::text);
            g.setFont (font (10.0f, true));
            g.drawText (String (-vals[i], 1), rr.removeFromRight (36), Justification::centredRight, false);
            auto bar = rr.withSizeKeepingCentre (rr.getWidth() - 8, 3.0f);
            g.setColour (Colour (0xff262626));
            g.fillRoundedRectangle (bar, 1.5f);
            g.setColour (col::accent);
            g.fillRoundedRectangle (bar.withLeft (bar.getRight() - bar.getWidth() * std::clamp (vals[i] / 12.0f, 0.0f, 1.0f)), 1.5f);
        }
    }
    void resized() override
    {
        auto b = getLocalBounds().reduced (10, 6);
        gauge = b.removeFromTop (150);
        b.removeFromTop (6);
        row (b.removeFromTop (58), { &target, &ceiling });
        b.removeFromTop (6);
        row (b.removeFromTop (26), { &autoOn, &tol });
        b.removeFromTop (8);
        bars = b.removeFromTop (34);
        b.removeFromTop (8);
        auto bottom = b.removeFromBottom (2 * 26 + 6);
        b.removeFromBottom (8);
        row (b.withSizeKeepingCentre (b.getWidth(), std::min (b.getHeight(), 124)), { &glue, &loud });
        row (bottom.removeFromTop (26), { &tilt, &width }); bottom.removeFromTop (6);
        row (bottom.removeFromTop (26), { &mono, &rel });
    }

private:
    ValueChip &target, &ceiling, &autoOn, &tol;
    Knob &glue, &loud;
    ValueChip &tilt, &width, &mono, &rel;
    Rectangle<int> gauge, bars;
    float lufsS = -70, lufsI = -70, autoDb = 0, glueGr = 0, limGr = 0;
};
} // namespace halo::ui
