#pragma once
#include "Analyzer.h"

namespace halo::ui
{
//==============================================================================
class ModulePanel : public Component
{
public:
    ModulePanel (HaloProcessor& p, const String& t, Colour c, const String& powerId)
        : proc (p), title (t), colour (c), power (c)
    {
        power.attach (p, powerId);
        addAndMakeVisible (power);
        icon.draw = [this] (Graphics& g, Rectangle<float> r) { drawIcon (g, r); };
        addAndMakeVisible (icon);
    }

    void tick (float dt)
    {
        t += dt;
        const float a = readActivity();
        act = a > act ? act + (a - act) * 0.5f : act + (a - act) * 0.08f;
        const float target = power.getToggleState() ? 1.0f : 0.0f;
        onAmt += (target - onAmt) * 0.18f;
        const bool on = target > 0.5f;
        if (on != lastOn)
        {
            lastOn = on;
            for (auto* c : getChildren())
                if (c != &power && c != &icon) c->setAlpha (on ? 1.0f : 0.38f);
            repaint();
        }
        icon.repaint();
        onTick();
    }

    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto head = b.removeFromTop ((float) headerH);
        g.setColour (colour.withMultipliedAlpha (0.45f + 0.55f * onAmt));
        g.setFont (font (13.0f, true).withExtraKerningFactor (0.09f));
        g.drawText (title, head.withTrimmedLeft (38).withTrimmedRight (10), Justification::centredLeft, false);

        // open dividers: a short stroke under the title, a broken stroke under the icon
        divider (g, b.getX() + 14.0f, b.getY() + 1.5f, b.getX() + 14.0f + std::min (64.0f, b.getWidth() * 0.3f), b.getY() + 1.5f);
        const float y = (float) (headerH + iconH) + 4.0f;
        divider (g, b.getX() + 14.0f, y, b.getCentreX() - 10.0f, y);
        divider (g, b.getCentreX() + 6.0f, y, b.getRight() - b.getWidth() * 0.28f, y);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        auto head = b.removeFromTop (headerH);
        power.setBounds (head.removeFromLeft (38).withSizeKeepingCentre (24, 24).translated (6, 0));
        icon.setBounds (b.removeFromTop (iconH).reduced (6, 2));
        b.removeFromTop (12);
        layoutControls (b.reduced (8, 4));
    }

protected:
    virtual void drawIcon (Graphics&, Rectangle<float>) = 0;
    virtual void layoutControls (Rectangle<int>) = 0;
    virtual float readActivity() = 0;
    virtual void onTick() {}

    float param (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    float iconAlpha() const { return 0.25f + 0.75f * onAmt; }

    Knob& knob (const String& label, const char* id)
    {
        auto k = std::make_unique<Knob> (label, colour);
        k->attach (proc, id);
        addAndMakeVisible (*k);
        auto& ref = *k;
        owned.push_back (std::move (k));
        return ref;
    }
    ChoiceSelector& selector (const char* id, const String& caption = {})
    {
        auto s = std::make_unique<ChoiceSelector> (proc, id, colour, caption);
        addAndMakeVisible (*s);
        auto& ref = *s;
        owned.push_back (std::move (s));
        return ref;
    }
    NeonToggle& toggle (const String& text, const char* id)
    {
        auto s = std::make_unique<NeonToggle> (text, colour);
        s->attach (proc, id);
        addAndMakeVisible (*s);
        auto& ref = *s;
        owned.push_back (std::move (s));
        return ref;
    }
    static void row (Rectangle<int> r, std::initializer_list<Component*> cs, int gap = 4)
    {
        const int n = (int) cs.size();
        const int w = (r.getWidth() - gap * (n - 1)) / n;
        int i = 0;
        for (auto* c : cs) { if (c) c->setBounds (r.getX() + i * (w + gap), r.getY(), w, r.getHeight()); ++i; }
    }

    static Path chevron (float cx, float cy, float w, float h, bool down)
    {
        Path p;
        const float s = down ? 1.0f : -1.0f;
        p.startNewSubPath (cx - w, cy - h * s);
        p.lineTo (cx, cy + h * s);
        p.lineTo (cx + w, cy - h * s);
        return p;
    }

    HaloProcessor& proc;
    String title;
    Colour colour;
    PowerButton power;
    Canvas icon;
    float t = 0, act = 0, onAmt = 1;
    bool lastOn = true, tinted = false;
    int headerH = 44, iconH = 150;
    std::vector<std::unique_ptr<Component>> owned;
};

//==============================================================================
class CompressPanel : public ModulePanel
{
public:
    CompressPanel (HaloProcessor& p) : ModulePanel (p, "COMPRESS", moduleColour[0], "comp_on"),
        style (selector ("comp_style")), input (knob ("Input", "comp_input")), amount (knob ("Compress", "comp_amount")),
        speed (knob ("Speed", "comp_speed")), output (knob ("Output", "comp_output")), mix (knob ("Mix", "comp_mix")),
        hpf (knob ("SC Filter", "comp_hpf")), autoG (toggle ("Auto Gain", "comp_auto")), gr (colour, "GR", 18.0f)
    { addAndMakeVisible (gr); }

protected:
    float readActivity() override { return clamp01 (proc.meters.compGr.load() / 14.0f); }
    void onTick() override { gr.setValue (act * 14.0f); }
    void layoutControls (Rectangle<int> b) override
    {
        style.setBounds (b.removeFromTop (28)); b.removeFromTop (8);
        gr.setBounds (b.removeFromTop (14).reduced (4, 0)); b.removeFromTop (6);
        const int kh = (b.getHeight() - 34) / 3;
        row (b.removeFromTop (kh), { &input, &amount });
        row (b.removeFromTop (kh), { &speed, &output });
        row (b.removeFromTop (kh), { &mix, &hpf });
        b.removeFromTop (6);
        autoG.setBounds (b.removeFromTop (26).reduced (20, 0));
    }
    void drawIcon (Graphics& g, Rectangle<float> r) override
    {
        const float s = std::min (r.getWidth(), r.getHeight());
        const float cx = r.getCentreX(), cy = r.getCentreY();
        const float a = iconAlpha();
        const float sq = act;
        const float bob = std::sin (t * 2.4f) * 2.0f;
        const float gap = s * (0.34f - 0.15f * sq);
        glowStroke (g, chevron (cx, cy - gap + bob, s * 0.17f, s * 0.07f, true), colour, 2.6f, a);
        glowStroke (g, chevron (cx, cy + gap - bob, s * 0.17f, s * 0.07f, false), colour, 2.6f, a);
        // arrow shafts
        Path shafts;
        shafts.startNewSubPath (cx, cy - gap - s * 0.2f + bob); shafts.lineTo (cx, cy - gap + s * 0.06f + bob);
        shafts.startNewSubPath (cx, cy + gap + s * 0.2f - bob); shafts.lineTo (cx, cy + gap - s * 0.06f - bob);
        glowStroke (g, shafts, colour, 2.2f, a * 0.8f);
        // squeezed wave
        Path wave;
        const float amp = s * 0.11f * (1.0f - 0.7f * sq) * (0.75f + 0.25f * std::sin (t * 1.7f));
        for (int i = 0; i <= 60; ++i)
        {
            const float u = (float) i / 60.0f;
            const float x = cx - s * 0.42f + u * s * 0.84f;
            const float env = std::sin (u * kPiF);
            const float y = cy + std::sin (u * 14.0f - t * 5.0f) * amp * env;
            if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
        }
        glowStroke (g, wave, colour, 2.0f, a);
    }

private:
    ChoiceSelector& style;
    Knob &input, &amount, &speed, &output, &mix, &hpf;
    NeonToggle& autoG;
    MiniBar gr;
};

//==============================================================================
class EnhancePanel : public ModulePanel
{
public:
    EnhancePanel (HaloProcessor& p) : ModulePanel (p, "ENHANCE", moduleColour[1], "enh_on"),
        deess (knob ("De-Ess", "enh_deess")), freq (knob ("Ess Freq", "enh_deess_freq")), denoise (knob ("De-Noise", "enh_denoise")),
        body (knob ("Body", "enh_body")), air (knob ("Air", "enh_air")), pre (toggle ("Pre Comp", "enh_pre")),
        essBar (colour, "ESS", 12.0f), noiseBar (colour, "GATE", 24.0f)
    { addAndMakeVisible (essBar); addAndMakeVisible (noiseBar); }

protected:
    float readActivity() override { return clamp01 (param ("enh_air") * 0.01f * 0.6f + proc.meters.essGr.load() / 10.0f); }
    void onTick() override
    {
        essS += (proc.meters.essGr.exchange (0.0f) - essS) * 0.3f;
        nS += (proc.meters.noiseGr.exchange (0.0f) - nS) * 0.3f;
        essBar.setValue (essS); noiseBar.setValue (nS);
    }
    void layoutControls (Rectangle<int> b) override
    {
        const int kh = (b.getHeight() - 34 - 36) / 3;
        row (b.removeFromTop (kh), { &deess, &freq });
        essBar.setBounds (b.removeFromTop (14).reduced (4, 0)); b.removeFromTop (4);
        row (b.removeFromTop (kh), { &denoise, &body });
        noiseBar.setBounds (b.removeFromTop (14).reduced (4, 0)); b.removeFromTop (4);
        air.setBounds (b.removeFromTop (kh).withSizeKeepingCentre (b.getWidth() / 2, kh));
        b.removeFromTop (6);
        pre.setBounds (b.removeFromTop (26).reduced (20, 0));
    }
    void drawIcon (Graphics& g, Rectangle<float> r) override
    {
        const float s = std::min (r.getWidth(), r.getHeight());
        const float cx = r.getCentreX(), cy = r.getCentreY();
        const float a = iconAlpha();
        const float airAmt = param ("enh_air") * 0.01f;
        const float R = s * (0.30f + 0.04f * std::sin (t * 2.0f) + 0.10f * act);
        const float rot = t * 0.35f;
        Path star;
        for (int i = 0; i < 4; ++i)
        {
            const float ang = rot + (float) i * kPiF * 0.5f;
            const float nx = cx + std::cos (ang) * R, ny = cy + std::sin (ang) * R;
            const float next = ang + kPiF * 0.5f;
            const float mx = cx + std::cos (ang + kPiF * 0.25f) * R * 0.16f, my = cy + std::sin (ang + kPiF * 0.25f) * R * 0.16f;
            if (i == 0) star.startNewSubPath (nx, ny);
            star.quadraticTo (mx, my, cx + std::cos (next) * R, cy + std::sin (next) * R);
        }
        star.closeSubPath();
        g.setColour (colour.withAlpha (0.06f * a));
        g.fillPath (star);
        glowStroke (g, star, colour, 2.2f, a);
        // orbiting sparkles (more & brighter with Air)
        const int n = 3 + (int) (airAmt * 4.0f);
        for (int i = 0; i < n; ++i)
        {
            const float ang = -t * 0.8f + (float) i * kPiF * 2.0f / (float) n;
            const float rr = s * 0.42f;
            const float px = cx + std::cos (ang) * rr, py = cy + std::sin (ang) * rr * 0.55f;
            const float tw = 0.5f + 0.5f * std::sin (t * 4.0f + (float) i * 1.7f);
            const float ss = 2.0f + 4.0f * tw;
            Path sp;
            sp.startNewSubPath (px - ss, py); sp.lineTo (px + ss, py);
            sp.startNewSubPath (px, py - ss); sp.lineTo (px, py + ss);
            glowStroke (g, sp, colour, 1.3f, a * (0.35f + 0.65f * tw));
        }
        // inner core
        g.setColour (colour.interpolatedWith (col::cream, 0.6f).withAlpha (0.8f * a));
        g.fillEllipse (cx - 3, cy - 3, 6, 6);
    }

private:
    Knob &deess, &freq, &denoise, &body, &air;
    NeonToggle& pre;
    MiniBar essBar, noiseBar;
    float essS = 0, nS = 0;
};

//==============================================================================
class EqPanel : public ModulePanel
{
public:
    EqPanel (HaloProcessor& p, SpectrumEngine& e) : ModulePanel (p, "EQ", moduleColour[2], "eq_on"),
        eng (e), display (p, e, colour), fK ("Freq", colour), gK ("Gain", colour), qK ("Q", colour), pre (toggle ("Pre Comp", "eq_pre"))
    {
        iconH = 96;
        addAndMakeVisible (display);
        for (int i = 0; i < 6; ++i)
        {
            auto b = std::make_unique<GlyphButton> (shortName (i), colour);
            b->onClick = [this, i] { select (i); };
            addAndMakeVisible (*b);
            bandButtons.push_back (std::move (b));
        }
        addAndMakeVisible (fK); addAndMakeVisible (gK); addAndMakeVisible (qK);
        display.onSelect = [this] (int i) { select (i); };
        select (2);
    }

protected:
    float readActivity() override { return clamp01 (proc.meters.eqAct.exchange (0.0f) * 1.5f); }
    void onTick() override { display.alphaMul = 0.35f + 0.65f * onAmt; display.repaint(); }
    void layoutControls (Rectangle<int> b) override
    {
        display.setBounds (b.removeFromTop (150)); b.removeFromTop (8);
        auto br = b.removeFromTop (24);
        const int w = br.getWidth() / 6;
        for (int i = 0; i < 6; ++i) bandButtons[(size_t) i]->setBounds (br.getX() + i * w, br.getY(), w - 2, 24);
        b.removeFromTop (8);
        const int kh = std::min (104, (b.getHeight() - 40) / 2);
        row (b.removeFromTop (kh), { &fK, &gK });
        qK.setBounds (b.removeFromTop (kh).withSizeKeepingCentre (b.getWidth() / 2, kh));
        b.removeFromTop (6);
        pre.setBounds (b.removeFromTop (26).reduced (20, 0));
    }
    void drawIcon (Graphics& g, Rectangle<float> r) override
    {
        // live "fader bank" driven by the output spectrum
        const float a = iconAlpha();
        const float freqs[9] = { 50, 110, 250, 500, 1000, 2200, 4500, 9000, 15000 };
        const int n = 9;
        const float w = r.getWidth() * 0.86f, x0 = r.getCentreX() - w * 0.5f;
        Path caps, link;
        for (int i = 0; i < n; ++i)
        {
            const float x = x0 + w * (float) i / (float) (n - 1);
            const float lv = eng.levelAt (1, freqs[i]);
            float v = std::clamp ((lv + 80.0f) / 70.0f, 0.03f, 1.0f) * onAmt;
            v = v * 0.85f + 0.06f * (0.5f + 0.5f * std::sin (t * 1.3f + (float) i));
            smooth[i] += (v - smooth[i]) * 0.25f;
            const float y = r.getBottom() - 8.0f - smooth[i] * (r.getHeight() - 22.0f);
            g.setColour (col::cream.withAlpha (0.06f));
            g.fillRoundedRectangle (x - 1.0f, r.getY() + 8.0f, 2.0f, r.getHeight() - 16.0f, 1.0f);
            caps.addRoundedRectangle (x - 7.0f, y - 2.5f, 14.0f, 5.0f, 2.5f);
            if (i == 0) link.startNewSubPath (x, y); else link.lineTo (x, y);
        }
        glowStroke (g, link, colour, 1.2f, a * 0.55f);
        g.setColour (colour.withAlpha (a));
        g.fillPath (caps);
        glowStroke (g, caps, colour, 1.0f, a * 0.7f);
    }

private:
    static String shortName (int i) { static const char* n[6] = { "LC", "LO", "M1", "M2", "HI", "HC" }; return n[i]; }
    void select (int i)
    {
        sel = i;
        display.selected = i;
        for (int k = 0; k < 6; ++k) { bandButtons[(size_t) k]->selected = k == i; bandButtons[(size_t) k]->repaint(); }
        const auto& bd = EqDisplay::band (i);
        fK.attach (proc, bd.f);
        fK.setLabel (i == 0 ? "Low Cut" : i == 5 ? "High Cut" : "Freq");
        gK.setVisible (bd.g != nullptr);
        if (bd.g) gK.attach (proc, bd.g);
        qK.setVisible (bd.q != nullptr);
        if (bd.q) qK.attach (proc, bd.q);
        if (lastOn == false) { gK.setAlpha (0.38f); qK.setAlpha (0.38f); }
        display.repaint();
    }

    SpectrumEngine& eng;
    EqDisplay display;
    Knob fK, gK, qK;
    NeonToggle& pre;
    std::vector<std::unique_ptr<GlyphButton>> bandButtons;
    int sel = 2;
    float smooth[9] {};
};

//==============================================================================
class MultiplyPanel : public ModulePanel
{
public:
    MultiplyPanel (HaloProcessor& p) : ModulePanel (p, "MULTIPLY", moduleColour[3], "mul_on"),
        voices (selector ("mul_voices")), amount (knob ("Amount", "mul_amount")), spread (knob ("Spread", "mul_spread")),
        offset (knob ("Offset", "mul_offset")), detune (knob ("Detune", "mul_detune")), low (knob ("Low Cut", "mul_low")) {}

protected:
    float readActivity() override { return clamp01 (proc.meters.mulAct.exchange (0.0f) * 2.5f + param ("mul_amount") * 0.004f); }
    void layoutControls (Rectangle<int> b) override
    {
        voices.setBounds (b.removeFromTop (28)); b.removeFromTop (10);
        const int kh = (b.getHeight() - 8) / 3;
        row (b.removeFromTop (kh), { &amount, &spread });
        row (b.removeFromTop (kh), { &offset, &detune });
        low.setBounds (b.removeFromTop (kh).withSizeKeepingCentre (b.getWidth() / 2, kh));
    }
    void drawIcon (Graphics& g, Rectangle<float> r) override
    {
        const float s = std::min (r.getWidth(), r.getHeight());
        const float cx = r.getCentreX(), cy = r.getCentreY();
        const float a = iconAlpha();
        const int nv = ((int) param ("mul_voices") + 1) * 2;
        const float spreadV = param ("mul_spread") * 0.01f;
        const float amt = param ("mul_amount") * 0.01f;
        const int shown = std::max (2, std::min (nv, 6));
        const float orbit = s * (0.05f + 0.16f * spreadV) * (0.4f + 0.6f * amt + 0.3f * act);
        for (int i = shown - 1; i >= 0; --i)
        {
            const float ph = t * (0.5f + 0.07f * (float) i) + (float) i * kPiF * 2.0f / (float) shown;
            const float x = cx + std::cos (ph) * orbit * 1.6f;
            const float y = cy + std::sin (ph * 1.3f) * orbit * 0.5f;
            const float rr = s * (0.25f - 0.012f * (float) i);
            Path ring;
            ring.addEllipse (x - rr, y - rr, rr * 2, rr * 2);
            glowStroke (g, ring, colour, 1.8f, a * (i == 0 ? 1.0f : 0.35f + 0.4f * amt));
        }
        g.setColour (colour.withAlpha (0.10f * a));
        g.fillEllipse (cx - s * 0.08f, cy - s * 0.08f, s * 0.16f, s * 0.16f);
    }

private:
    ChoiceSelector& voices;
    Knob &amount, &spread, &offset, &detune, &low;
};

//==============================================================================
class SpacePanel : public ModulePanel
{
public:
    SpacePanel (HaloProcessor& p) : ModulePanel (p, "SPACE", moduleColour[4], "space_on"),
        revType (selector ("rev_type", "Reverb")), revMix (knob ("Reverb", "rev_mix")), decay (knob ("Decay", "rev_decay")),
        preDly (knob ("Pre-Delay", "rev_predelay")), damp (knob ("Damping", "rev_damp")), revLow (knob ("Low Cut", "rev_lowcut")),
        width (knob ("Width", "rev_width")), revDuck (knob ("Duck", "rev_duck")),
        dlyType (selector ("dly_type", "Delay")), dlyMix (knob ("Delay", "dly_mix")), rate (selector ("dly_rate")),
        timeMs (knob ("Time", "dly_time")), feedback (knob ("Feedback", "dly_feedback")), tone (knob ("Tone", "dly_tone")),
        dlyDuck (knob ("Duck", "dly_duck")), sync (toggle ("Sync", "dly_sync")) {}

protected:
    float readActivity() override
    {
        revA += (proc.meters.revAct.exchange (0.0f) - revA) * 0.2f;
        dlyA += (proc.meters.dlyAct.exchange (0.0f) - dlyA) * 0.2f;
        return clamp01 ((revA + dlyA) * 3.0f);
    }
    void onTick() override
    {
        const int s = param ("dly_sync") > 0.5f ? 1 : 0;
        if (s != syncState) { syncState = s; rate.setVisible (s == 1); timeMs.setVisible (s == 0); }
    }
    void layoutControls (Rectangle<int> b) override
    {
        auto left = b.removeFromLeft (b.getWidth() / 2).withTrimmedRight (6);
        auto right = b.withTrimmedLeft (6);
        const int kh = (left.getHeight() - 44 - 8) / 4;

        revType.setBounds (left.removeFromTop (44)); left.removeFromTop (8);
        row (left.removeFromTop (kh), { &revMix, &decay });
        row (left.removeFromTop (kh), { &preDly, &damp });
        row (left.removeFromTop (kh), { &revLow, &width });
        revDuck.setBounds (left.removeFromTop (kh).withSizeKeepingCentre (left.getWidth() / 2, kh));

        dlyType.setBounds (right.removeFromTop (44)); right.removeFromTop (8);
        auto r1 = right.removeFromTop (kh);
        dlyMix.setBounds (r1.removeFromLeft (r1.getWidth() / 2));
        auto timeArea = r1.withTrimmedLeft (4);
        timeMs.setBounds (timeArea);
        rate.setBounds (timeArea.withSizeKeepingCentre (timeArea.getWidth(), 30).translated (0, -8));
        row (right.removeFromTop (kh), { &feedback, &tone });
        auto r3 = right.removeFromTop (kh);
        dlyDuck.setBounds (r3.removeFromLeft (r3.getWidth() / 2));
        sync.setBounds (r3.withSizeKeepingCentre (r3.getWidth() - 12, 26));
    }
    void drawIcon (Graphics& g, Rectangle<float> r) override
    {
        const float s = std::min (r.getWidth(), r.getHeight());
        const float cx = r.getCentreX(), cy = r.getCentreY();
        const float a = iconAlpha();
        const float decayV = param ("rev_decay") * 0.01f;
        const float speed = 0.10f + 0.12f * (1.0f - decayV);
        // receding tunnel of rings
        const int n = 7;
        for (int i = 0; i < n; ++i)
        {
            float z = std::fmod ((float) i / (float) n + t * speed, 1.0f);
            const float rr = s * (0.04f + 0.50f * z * z);
            const float fade = std::sin (z * kPiF);
            Path ring;
            ring.addEllipse (cx - rr * 1.5f, cy - rr * 0.62f, rr * 3.0f, rr * 1.24f);
            glowStroke (g, ring, colour, 1.0f + 1.2f * z, a * fade * (0.45f + 0.55f * clamp01 (act + 0.25f)));
        }
        // delay echo dots
        const float dlyMixV = param ("dly_mix") * 0.01f;
        for (int i = 0; i < 4; ++i)
        {
            const float ph = std::fmod (t * 0.5f + (float) i * 0.25f, 1.0f);
            const float x = cx - s * 0.75f + ph * s * 1.5f;
            const float y = cy + std::sin (ph * kPiF * 2.0f) * s * 0.05f;
            const float al = (1.0f - ph) * a * (0.15f + 0.85f * dlyMixV);
            g.setColour (colour.withAlpha (0.25f * al));
            g.fillEllipse (x - 6, y - 6, 12, 12);
            g.setColour (colour.interpolatedWith (col::cream, 0.5f).withAlpha (al));
            g.fillEllipse (x - 2.5f, y - 2.5f, 5, 5);
        }
    }

private:
    ChoiceSelector& revType;
    Knob &revMix, &decay, &preDly, &damp, &revLow, &width, &revDuck;
    ChoiceSelector& dlyType;
    Knob& dlyMix;
    ChoiceSelector& rate;
    Knob &timeMs, &feedback, &tone, &dlyDuck;
    NeonToggle& sync;
    float revA = 0, dlyA = 0;
    int syncState = -1;
};

//==============================================================================
class DrivePanel : public ModulePanel
{
public:
    DrivePanel (HaloProcessor& p) : ModulePanel (p, "DRIVE", moduleColour[5], "sat_on"),
        type (selector ("sat_type")), drive (knob ("Drive", "sat_drive")), mix (knob ("Dry/Wet", "sat_mix")),
        gain (knob ("Gain", "sat_gain")), preFx (toggle ("Pre FX", "sat_prefx")), clip (toggle ("Clip", "sat_clip")) {}

protected:
    float readActivity() override { return clamp01 (proc.meters.satAct.exchange (0.0f) * 3.0f); }
    void layoutControls (Rectangle<int> b) override
    {
        type.setBounds (b.removeFromTop (28)); b.removeFromTop (10);
        const int big = (int) (b.getHeight() * 0.42f);
        drive.setBounds (b.removeFromTop (big).reduced (14, 0));
        drive.setLabelHeight (19);
        const int kh = b.getHeight() - 40;
        row (b.removeFromTop (kh), { &mix, &gain });
        b.removeFromTop (8);
        row (b.removeFromTop (26), { &preFx, &clip }, 8);
    }
    void drawIcon (Graphics& g, Rectangle<float> r) override
    {
        const float s = std::min (r.getWidth(), r.getHeight());
        const float cx = r.getCentreX(), cy = r.getCentreY();
        const float a = iconAlpha();
        const float d = param ("sat_drive") * 0.01f;
        const float A = s * 0.30f * (1.0f + 2.2f * d) * (0.85f + 0.3f * act);
        const float ceil = s * 0.28f;
        // ceiling lines
        Path lines;
        for (float y : { cy - ceil, cy + ceil })
            for (float x = cx - s * 0.62f; x < cx + s * 0.62f; x += 12.0f) { lines.startNewSubPath (x, y); lines.lineTo (x + 6.0f, y); }
        g.setColour (colour.withAlpha (0.25f * a));
        g.strokePath (lines, PathStrokeType (1.0f));
        // clipped wave
        Path wave;
        std::vector<Point<float>> clipPts;
        for (int i = 0; i <= 90; ++i)
        {
            const float u = (float) i / 90.0f;
            const float x = cx - s * 0.62f + u * s * 1.24f;
            float v = std::sin (u * kPiF * 4.0f - t * 3.2f) * A;
            v = std::clamp (v, -ceil, ceil);
            if (std::abs (v) >= ceil - 0.01f && (i % 9 == 0)) clipPts.push_back ({ x, cy - v });
            if (i == 0) wave.startNewSubPath (x, cy - v); else wave.lineTo (x, cy - v);
        }
        glowStroke (g, wave, colour, 2.3f, a);
        // sparks where it clips
        juce::Random rng ((int64) (t * 20.0f));
        for (auto& p : clipPts)
        {
            const int n = 1 + (int) (d * 3);
            for (int k = 0; k < n; ++k)
            {
                const float sx = p.x + (rng.nextFloat() - 0.5f) * 14.0f;
                const float sy = p.y + (p.y < cy ? -1.0f : 1.0f) * rng.nextFloat() * 14.0f * (0.4f + d);
                g.setColour (colour.interpolatedWith (col::cream, 0.4f).withAlpha (a * rng.nextFloat() * (0.3f + 0.7f * d)));
                g.fillEllipse (sx - 1.3f, sy - 1.3f, 2.6f, 2.6f);
            }
        }
    }

private:
    ChoiceSelector& type;
    Knob &drive, &mix, &gain;
    NeonToggle &preFx, &clip;
};

//==============================================================================
class MasterPanel : public ModulePanel
{
public:
    MasterPanel (HaloProcessor& p) : ModulePanel (p, "MASTER", moduleColour[6], "mst_on"),
        tilt (knob ("Tilt", "mst_tilt")), width (knob ("Width", "mst_width")), mono (knob ("Bass Mono", "mst_mono")),
        glue (knob ("Glue", "mst_glue")), loud (knob ("Loudness", "mst_loud")), ceiling (knob ("Ceiling", "mst_ceiling")),
        release (knob ("Release", "mst_release")), limBar (colour, "LIM", 12.0f) { addAndMakeVisible (limBar); tinted = true; }

protected:
    float readActivity() override { return clamp01 (lim / 8.0f); }
    void onTick() override
    {
        lim += (proc.meters.uiLim - lim) * 0.35f;
        const float s = proc.meters.lufsS.load();
        lufs += (s - lufs) * 0.15f;
        limBar.setValue (lim);
    }
    void layoutControls (Rectangle<int> b) override
    {
        limBar.setBounds (b.removeFromTop (14).reduced (4, 0)); b.removeFromTop (6);
        const int kh = b.getHeight() / 4;
        row (b.removeFromTop (kh), { &tilt, &width });
        row (b.removeFromTop (kh), { &mono, &glue });
        row (b.removeFromTop (kh), { &loud, &ceiling });
        release.setBounds (b.removeFromTop (kh).withSizeKeepingCentre (b.getWidth() / 2, kh));
    }
    void drawIcon (Graphics& g, Rectangle<float> r) override
    {
        const float s = std::min (r.getWidth(), r.getHeight());
        const float cx = r.getCentreX(), cy = r.getCentreY() + 4.0f;
        const float a = iconAlpha();
        const float R = s * 0.42f;
        const float a0 = -kPiF * 0.75f, a1 = kPiF * 0.75f;
        // rotating outer ticks
        Path ticks;
        for (int i = 0; i < 48; ++i)
        {
            const float ang = (float) i / 48.0f * kPiF * 2.0f + t * 0.15f;
            const float r0 = R + 6.0f, r1 = R + ((i % 4 == 0) ? 12.0f : 9.0f);
            ticks.startNewSubPath (cx + std::sin (ang) * r0, cy - std::cos (ang) * r0);
            ticks.lineTo (cx + std::sin (ang) * r1, cy - std::cos (ang) * r1);
        }
        g.setColour (colour.withAlpha (0.22f * a));
        g.strokePath (ticks, PathStrokeType (1.0f));
        // track + loudness arc (-30 … 0 LUFS)
        Path track; track.addCentredArc (cx, cy, R, R, 0, a0, a1, true);
        g.setColour (col::cream.withAlpha (0.07f));
        g.strokePath (track, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));
        const float v = std::clamp ((lufs + 30.0f) / 30.0f, 0.0f, 1.0f);
        if (v > 0.002f)
        {
            Path arc; arc.addCentredArc (cx, cy, R, R, 0, a0, a0 + (a1 - a0) * v, true);
            glowStroke (g, arc, colour, 3.6f, a);
        }
        // limiter reduction (inner, red)
        const float lv = std::clamp (lim / 12.0f, 0.0f, 1.0f);
        if (lv > 0.002f)
        {
            Path la; la.addCentredArc (cx, cy, R - 10, R - 10, 0, a1 - (a1 - a0) * lv, a1, true);
            glowStroke (g, la, moduleColour[5], 2.0f, a);
        }
        g.setColour (col::text.withAlpha (a));
        g.setFont (fontBold (s * 0.2f));
        g.drawText (lufs > -69.0f ? String (lufs, 1) : String ("--"), Rectangle<float> (cx - R, cy - s * 0.14f, R * 2, s * 0.22f), Justification::centred, false);
        g.setColour (colour.withAlpha (0.8f * a));
        g.setFont (fontLabel (9.5f));
        g.drawText ("LUFS S", Rectangle<float> (cx - R, cy + s * 0.08f, R * 2, 14), Justification::centred, false);
    }

private:
    Knob &tilt, &width, &mono, &glue, &loud, &ceiling, &release;
    MiniBar limBar;
    float lim = 0, lufs = -70;
};
} // namespace halo::ui
