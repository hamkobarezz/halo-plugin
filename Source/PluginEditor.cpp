#include "PluginEditor.h"

namespace halo
{
using namespace juce;
using namespace ui;

//==============================================================================
class TopBar : public Component
{
public:
    explicit TopBar (HaloProcessor& p)
        : proc (p), prev ("<"), next (">"), presetBtn ("preset"), save ("SAVE"), a ("A"), b ("B"), copy ("A > B"),
          undoB ("undo"), redoB ("redo"), quality (p, "quality", col::accent),
          inK ("Input", col::accent), outK ("Output", col::accent), mixK ("Dry/Wet", col::accent), bypass ("Bypass", col::accent)
    {
        for (auto* c : std::initializer_list<Component*> { &prev, &next, &presetBtn, &save, &a, &b, &copy, &undoB, &redoB, &quality, &inK, &outK, &mixK, &bypass })
            addAndMakeVisible (c);

        auto arrow = [] (bool left)
        {
            return [left] (Graphics& g, Rectangle<float> r, Colour c)
            {
                Path p;
                const float cx = r.getCentreX(), cy = r.getCentreY();
                const float s = left ? 1.0f : -1.0f;
                p.startNewSubPath (cx + 3 * s, cy - 6); p.lineTo (cx - 3 * s, cy); p.lineTo (cx + 3 * s, cy + 6);
                g.setColour (c);
                g.strokePath (p, PathStrokeType (1.3f, PathStrokeType::curved, PathStrokeType::rounded));
            };
        };
        prev.drawGlyph = arrow (true);
        next.drawGlyph = arrow (false);
        auto curl = [] (bool redo)
        {
            return [redo] (Graphics& g, Rectangle<float> r, Colour c)
            {
                Path p;
                const float cx = r.getCentreX(), cy = r.getCentreY() + 1;
                p.addCentredArc (cx, cy, 6, 6, 0, redo ? -kPiF * 0.9f : kPiF * 0.9f, redo ? kPiF * 0.6f : -kPiF * 0.6f, true);
                const float ex = cx + 6 * std::sin (redo ? -kPiF * 0.9f : kPiF * 0.9f), ey = cy - 6 * std::cos (redo ? -kPiF * 0.9f : kPiF * 0.9f);
                p.startNewSubPath (ex + (redo ? -4.0f : 4.0f), ey - 1); p.lineTo (ex, ey); p.lineTo (ex + (redo ? 1.0f : -1.0f), ey - 5);
                g.setColour (c);
                g.strokePath (p, PathStrokeType (1.3f, PathStrokeType::curved, PathStrokeType::rounded));
            };
        };
        undoB.drawGlyph = curl (false);
        redoB.drawGlyph = curl (true);

        prev.onClick = [this] { proc.presets.step (-1); };
        next.onClick = [this] { proc.presets.step (1); };
        presetBtn.onClick = [this] { showPresetMenu(); };
        save.onClick = [this] { savePreset(); };
        a.onClick = [this] { proc.selectSlot (0); };
        b.onClick = [this] { proc.selectSlot (1); };
        copy.onClick = [this] { proc.copyToOtherSlot(); };
        undoB.onClick = [this] { proc.undo.undo(); };
        redoB.onClick = [this] { proc.undo.redo(); };
        prev.setTooltip ("Previous preset");
        next.setTooltip ("Next preset");
        save.setTooltip ("Save current settings as a user preset");
        a.setTooltip ("Compare: settings A");
        b.setTooltip ("Compare: settings B");
        copy.setTooltip ("Copy the current settings into the other slot");
        undoB.setTooltip ("Undo");
        redoB.setTooltip ("Redo");

        presetBtn.drawGlyph = [this] (Graphics& g, Rectangle<float> r, Colour)
        {
            g.setColour (col::text);
            g.setFont (font (12.5f, true));
            g.drawText (proc.presets.currentName, r.reduced (12, 0), Justification::centredLeft, true);
            Path p;
            const float x = r.getRight() - 16, y = r.getCentreY();
            p.startNewSubPath (x - 4, y - 2); p.lineTo (x, y + 2); p.lineTo (x + 4, y - 2);
            g.setColour (col::accent);
            g.strokePath (p, PathStrokeType (1.5f));
        };

        inK.attach (p, "in_gain");
        outK.attach (p, "out_gain");
        mixK.attach (p, "mix");
        bypass.attach (p, "bypass");
        for (auto* k : { &inK, &outK, &mixK }) k->setLabelHeight (16);
    }

    void tick()
    {
        a.selected = proc.getSlot() == 0;
        b.selected = proc.getSlot() == 1;
        if (proc.presets.currentName != lastName) { lastName = proc.presets.currentName; presetBtn.repaint(); }
        a.repaint(); b.repaint();
    }

    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);

        // HALO Studios logo
        if (logo != nullptr)
            logo->drawWithin (g, r.removeFromLeft (190).reduced (22, 13), RectanglePlacement::xLeft | RectanglePlacement::yMid, 1.0f);

        g.setColour (col::label);
        g.setFont (fontLabel (9.0f));
        g.drawText ("QUALITY", quality.getBounds().toFloat().translated (0, -16).withHeight (14), Justification::centred, false);
        // divider before bypass
        divider (g, (float) bypass.getX() - 16.0f, r.getY() + 20.0f, (float) bypass.getX() - 16.0f, r.getBottom() - 20.0f);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (0, 14);
        r.removeFromLeft (190);
        auto row = r.withSizeKeepingCentre (r.getWidth(), 30);
        prev.setBounds (row.removeFromLeft (30)); row.removeFromLeft (6);
        presetBtn.setBounds (row.removeFromLeft (290)); row.removeFromLeft (6);
        next.setBounds (row.removeFromLeft (30)); row.removeFromLeft (20);
        save.setBounds (row.removeFromLeft (64)); row.removeFromLeft (24);
        a.setBounds (row.removeFromLeft (40));
        b.setBounds (row.removeFromLeft (40));
        copy.setBounds (row.removeFromLeft (60)); row.removeFromLeft (16);
        undoB.setBounds (row.removeFromLeft (30)); row.removeFromLeft (4);
        redoB.setBounds (row.removeFromLeft (30)); row.removeFromLeft (30);
        quality.setBounds (row.removeFromLeft (150).withTrimmedTop (2).translated (0, 8));

        auto right = getLocalBounds().reduced (16, 6);
        bypass.setBounds (right.removeFromRight (104).withSizeKeepingCentre (104, 36));
        right.removeFromRight (30);
        mixK.setBounds (right.removeFromRight (66));
        outK.setBounds (right.removeFromRight (66));
        inK.setBounds (right.removeFromRight (66));
    }

private:
    void showPresetMenu()
    {
        PopupMenu m;
        const auto& f = PresetManager::factory();
        std::map<String, PopupMenu> cats;
        StringArray order;
        for (int i = 0; i < (int) f.size(); ++i)
        {
            const auto& fp = f[(size_t) i];
            if (! order.contains (fp.category)) order.add (fp.category);
            cats[fp.category].addItem (i + 1, fp.name, true, proc.presets.currentIndex == i);
        }
        for (auto& c : order) m.addSubMenu (c, cats[c]);

        auto users = proc.presets.userPresets();
        PopupMenu um;
        for (int i = 0; i < users.size(); ++i)
            um.addItem (1000 + i, users[i].getFileNameWithoutExtension(), true, proc.presets.currentIndex == (int) f.size() + i);
        if (users.isEmpty()) um.addItem (-1, "(none yet - use Save)", false);
        m.addSubMenu ("User", um);
        m.addSeparator();
        m.addItem (900, "Save As...");
        m.addItem (901, "Init (reset everything)");
        m.addItem (902, "Open Preset Folder");

        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&presetBtn).withMinimumWidth (presetBtn.getWidth()), [this, users] (int r)
        {
            if (r <= 0) return;
            proc.undo.beginNewTransaction();
            if (r < 900) proc.presets.loadFactory (r - 1);
            else if (r == 900) savePreset();
            else if (r == 901) proc.presets.initDefaults();
            else if (r == 902) proc.presets.userDir().startAsProcess();
            else if (r >= 1000 && r - 1000 < users.size()) proc.presets.loadUser (users[r - 1000]);
        });
    }

    void savePreset()
    {
        auto* w = new AlertWindow ("Save Preset", "Name your preset:", MessageBoxIconType::NoIcon);
        w->addTextEditor ("name", proc.presets.currentName);
        w->addButton ("Save", 1, KeyPress (KeyPress::returnKey));
        w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
        w->enterModalState (true, ModalCallbackFunction::create ([this, w] (int res)
        {
            if (res == 1) proc.presets.saveUser (w->getTextEditorContents ("name"));
        }), true);
    }

    HaloProcessor& proc;
    GlyphButton prev, next, presetBtn, save, a, b, copy, undoB, redoB;
    ChoiceSelector quality;
    Knob inK, outK, mixK;
    NeonToggle bypass;
    String lastName;
    std::unique_ptr<Drawable> logo = makeLogo();
};

//==============================================================================
class MeterView : public Component, public SettableTooltipClient
{
public:
    explicit MeterView (HaloProcessor& p) : proc (p) { setTooltip ("Click the loudness readout to reset Integrated LUFS"); }

    void tick (float dt)
    {
        auto take = [] (std::atomic<float>& a) { return a.exchange (0.0f); };
        const float raw[4] = { take (proc.meters.inL), take (proc.meters.inR), take (proc.meters.outL), take (proc.meters.outR) };
        for (int i = 0; i < 4; ++i)
        {
            const float dbv = gainToDb (raw[i]);
            lvl[i] = dbv > lvl[i] ? dbv : lvl[i] - 28.0f * dt;
            lvl[i] = std::max (lvl[i], -60.0f);
            if (dbv > hold[i]) { hold[i] = dbv; holdT[i] = 1.2f; }
            else if ((holdT[i] -= dt) < 0) hold[i] -= 18.0f * dt;
            hold[i] = std::max (hold[i], -60.0f);
        }
        proc.meters.uiOut = std::max (raw[2], raw[3]);
        const float grs[3] = { take (proc.meters.compGr), take (proc.meters.glueGr), proc.meters.uiLim };
        for (int i = 0; i < 3; ++i) gr[i] = grs[i] > gr[i] ? grs[i] : gr[i] - 14.0f * dt, gr[i] = std::max (0.0f, gr[i]);
        const float pk = gainToDb (take (proc.meters.peak));
        peak = std::max (peak, pk);
        m = proc.meters.lufsM.load(); s = proc.meters.lufsS.load(); in = proc.meters.lufsI.load();
        repaint();
    }

    void mouseDown (const MouseEvent& e) override
    {
        if (e.x > getWidth() / 2) { proc.meters.resetLoudness = true; peak = -100; }
    }

    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        b = b.reduced (16, 10);

        // level bars
        auto bars = b.removeFromLeft (110);
        const char* names[2] = { "IN", "OUT" };
        for (int grp = 0; grp < 2; ++grp)
        {
            auto gArea = bars.removeFromLeft (52);
            g.setColour (col::label);
            g.setFont (fontLabel (9.0f));
            g.drawText (names[grp], gArea.removeFromBottom (14), Justification::centred, false);
            for (int ch = 0; ch < 2; ++ch)
            {
                auto bar = gArea.removeFromLeft (gArea.getWidth() / (2 - ch)).reduced (5, 0);
                const int i = grp * 2 + ch;
                g.setColour (Colour (0xff2a2a2a));
                g.fillRoundedRectangle (bar, 1.5f);
                const float v = std::clamp ((lvl[i] + 60.0f) / 60.0f, 0.0f, 1.0f);
                auto fill = bar.withTop (bar.getBottom() - bar.getHeight() * v);
                g.setColour (col::accent);
                g.fillRoundedRectangle (fill, 1.5f);
                const float hy = bar.getBottom() - bar.getHeight() * std::clamp ((hold[i] + 60.0f) / 60.0f, 0.0f, 1.0f);
                g.setColour (col::text.withAlpha (0.7f));
                g.fillRect (bar.getX(), hy - 0.5f, bar.getWidth(), 1.0f);
            }
        }
        b.removeFromLeft (16);

        // gain reduction
        auto grArea = b.removeFromLeft (150);
        const char* grNames[3] = { "COMP", "GLUE", "LIMIT" };
        const Colour grCols[3] = { moduleColour[0], moduleColour[6], moduleColour[5] };
        g.setColour (col::label);
        g.setFont (fontLabel (9.0f));
        g.drawText ("GAIN REDUCTION", grArea.removeFromTop (16), Justification::centredLeft, false);
        for (int i = 0; i < 3; ++i)
        {
            auto rr = grArea.removeFromTop (grArea.getHeight() / (3 - i));
            auto lbl = rr.removeFromLeft (40);
            g.setColour (col::label);
            g.drawText (grNames[i], lbl, Justification::centredLeft, false);
            auto val = rr.removeFromRight (40);
            g.setColour (col::text);
            g.setFont (font (11.0f, true));
            g.drawText (String (-gr[i], 1), val, Justification::centredRight, false);
            g.setFont (fontLabel (9.0f));
            auto bar = rr.withSizeKeepingCentre (rr.getWidth() - 6, 5);
            g.setColour (Colour (0xff2a2a2a));
            g.fillRoundedRectangle (bar.withSizeKeepingCentre (bar.getWidth(), 3.0f), 1.5f);
            const float v = std::clamp (gr[i] / 18.0f, 0.0f, 1.0f);
            auto fill = bar.withLeft (bar.getRight() - bar.getWidth() * v);
            g.setColour (grCols[i]);
            g.fillRoundedRectangle (fill.withSizeKeepingCentre (fill.getWidth(), 3.0f), 1.5f);
        }
        b.removeFromLeft (18);

        // loudness
        auto lu = b;
        g.setColour (col::label);
        g.drawText ("LOUDNESS (LUFS)", lu.removeFromTop (16), Justification::centredLeft, false);
        auto cell = [&] (const String& name, float v, Rectangle<float> r, bool big)
        {
            g.setColour (col::label);
            g.setFont (fontLabel (9.0f));
            g.drawText (name, r.removeFromTop (13), Justification::centredLeft, false);
            g.setColour (big ? col::accent : col::text);
            g.setFont (big ? fontBold (24.0f) : font (17.0f, true));
            g.drawText (v > -69.0f ? String (v, 1) : String ("--"), r, Justification::centredLeft, false);
        };
        auto top = lu.removeFromTop (lu.getHeight() * 0.55f);
        cell ("INTEGRATED", in, top.removeFromLeft (top.getWidth() * 0.55f), true);
        cell ("PEAK dB", peak, top, false);
        cell ("MOMENTARY", m, lu.removeFromLeft (lu.getWidth() * 0.55f), false);
        cell ("SHORT", s, lu, false);
    }

private:
    HaloProcessor& proc;
    float lvl[4] { -60, -60, -60, -60 }, hold[4] { -60, -60, -60, -60 }, holdT[4] {}, gr[3] {};
    float peak = -100, m = -70, s = -70, in = -70;
};

//==============================================================================
void HaloEditor::Content::paint (Graphics& g)
{
    g.fillAll (col::bg);
    for (auto& d : dividers) divider (g, d.getStartX(), d.getStartY(), d.getEndX(), d.getEndY());
}

HaloEditor::HaloEditor (HaloProcessor& p) : AudioProcessorEditor (p), proc (p), engine (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    top = std::make_unique<TopBar> (p);
    content.addAndMakeVisible (*top);

    panels.push_back (std::make_unique<CompressPanel> (p));
    panels.push_back (std::make_unique<EnhancePanel> (p));
    panels.push_back (std::make_unique<EqPanel> (p, engine));
    panels.push_back (std::make_unique<MultiplyPanel> (p));
    panels.push_back (std::make_unique<SpacePanel> (p));
    panels.push_back (std::make_unique<DrivePanel> (p));
    panels.push_back (std::make_unique<MasterPanel> (p));
    for (auto& pn : panels) content.addAndMakeVisible (*pn);

    spectrum = std::make_unique<SpectrumView> (engine);
    content.addAndMakeVisible (*spectrum);
    meters = std::make_unique<MeterView> (p);
    content.addAndMakeVisible (*meters);
    ear = std::make_unique<EarView> (p, engine);
    content.addAndMakeVisible (*ear);
    splash = std::make_unique<SplashScreen>();
    content.addAndMakeVisible (*splash);

    const double scale = (double) proc.apvts.state.getProperty ("uiScale", 0.85);
    setResizable (true, true);
    setResizeLimits ((int) (BaseW * 0.5), (int) (BaseH * 0.5), (int) (BaseW * 1.6), (int) (BaseH * 1.6));
    getConstrainer()->setFixedAspectRatio ((double) BaseW / BaseH);
    setSize ((int) (BaseW * scale), (int) (BaseH * scale));

    lastTime = Time::getMillisecondCounterHiRes();
    vblank = std::make_unique<VBlankAttachment> (this, [this] { frame(); });
}

HaloEditor::~HaloEditor()
{
    vblank.reset();
    setLookAndFeel (nullptr);
}

void HaloEditor::paint (Graphics& g) { g.fillAll (col::bg); }

void HaloEditor::resized()
{
    const float s = (float) getWidth() / (float) BaseW;
    content.setBounds (0, 0, BaseW, BaseH);
    content.setTransform (AffineTransform::scale (s));
    proc.apvts.state.setProperty ("uiScale", s, nullptr);

    auto r = Rectangle<int> (0, 0, BaseW, BaseH).reduced (14, 0);
    r.removeFromTop (12);
    top->setBounds (r.removeFromTop (78));
    r.removeFromTop (12);
    auto bottom = r.removeFromBottom (168).withTrimmedBottom (12);
    r.removeFromBottom (10);

    const float weights[7] = { 1.0f, 1.0f, 1.18f, 1.0f, 1.78f, 1.0f, 1.1f };
    float total = 0; for (float w : weights) total += w;
    const int gap = 8;
    const float unit = (float) (r.getWidth() - gap * 6) / total;
    int x = r.getX();
    content.dividers.clear();
    for (int i = 0; i < 7; ++i)
    {
        const int w = (int) std::round (weights[i] * unit);
        panels[(size_t) i]->setBounds (x, r.getY(), i == 6 ? r.getRight() - x : w, r.getHeight());
        x += w + gap;
        // vertical open divider between modules: starts below the header, stops before the bottom
        if (i < 6)
        {
            const float dx = (float) x - gap * 0.5f;
            content.dividers.add ({ dx, (float) r.getY() + 46.0f, dx, (float) r.getY() + r.getHeight() * 0.78f });
        }
    }
    // horizontal open dividers under the top bar and above the bottom bar
    const float ty = (float) r.getY() - 6.0f, by = (float) r.getBottom() + 5.0f;
    content.dividers.add ({ 18.0f, ty, BaseW * 0.42f, ty });
    content.dividers.add ({ BaseW * 0.42f + 22.0f, ty, BaseW * 0.58f, ty });
    content.dividers.add ({ BaseW * 0.46f, by, BaseW - 18.0f, by });
    content.dividers.add ({ BaseW * 0.38f, by, BaseW * 0.46f - 22.0f, by });

    meters->setBounds (bottom.removeFromRight (520));
    bottom.removeFromRight (10);
    ear->setBounds (bottom.removeFromRight (300));
    bottom.removeFromRight (10);
    spectrum->setBounds (bottom);
    for (auto* c : std::initializer_list<Component*> { ear.get(), meters.get() })
    {
        const float dx = (float) c->getX() - 5.0f;
        content.dividers.add ({ dx, (float) c->getY() + 22.0f, dx, (float) c->getBottom() - 34.0f });
    }
    splash->setBounds (0, 0, BaseW, BaseH);
}

void HaloEditor::frame()
{
    const double now = Time::getMillisecondCounterHiRes();
    const float dt = (float) std::clamp ((now - lastTime) * 0.001, 0.0, 0.1);
    lastTime = now;
    engine.update();
    proc.meters.uiLim = proc.meters.limGr.exchange (0.0f);
    top->tick();
    for (auto& p : panels) p->tick (dt);
    meters->tick (dt);
    ear->tick (dt);
    spectrum->repaint();
    if (splash->isVisible()) splash->tick (dt);
}
} // namespace halo
