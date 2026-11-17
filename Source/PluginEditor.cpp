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
        : proc (p), undoB ("undo"), redoB ("redo"), prev ("<"), next (">"), presetBtn ("preset"), save ("+"),
          a ("A"), b ("B"), copy ("A > B"), quality (p, "quality", "Quality"), bypass ("Bypass", col::accent)
    {
        for (auto* c : std::initializer_list<Component*> { &undoB, &redoB, &prev, &next, &presetBtn, &save, &a, &b, &copy, &quality, &bypass })
            addAndMakeVisible (c);

        auto arrow = [] (bool left)
        {
            return [left] (Graphics& g, Rectangle<float> r, Colour c)
            {
                Path p; const float cx = r.getCentreX(), cy = r.getCentreY(), s = left ? 1.0f : -1.0f;
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
                const float s0 = redo ? -kPiF * 0.9f : kPiF * 0.9f;
                p.addCentredArc (cx, cy, 6, 6, 0, s0, redo ? kPiF * 0.6f : -kPiF * 0.6f, true);
                const float ex = cx + 6 * std::sin (s0), ey = cy - 6 * std::cos (s0);
                p.startNewSubPath (ex + (redo ? -4.0f : 4.0f), ey - 1); p.lineTo (ex, ey); p.lineTo (ex + (redo ? 1.0f : -1.0f), ey - 5);
                g.setColour (c);
                g.strokePath (p, PathStrokeType (1.3f, PathStrokeType::curved, PathStrokeType::rounded));
            };
        };
        undoB.drawGlyph = curl (false);
        redoB.drawGlyph = curl (true);
        save.drawGlyph = [] (Graphics& g, Rectangle<float> r, Colour c)
        {
            g.setColour (c);
            g.fillRect (r.getCentreX() - 6.0f, r.getCentreY() - 0.75f, 12.0f, 1.5f);
            g.fillRect (r.getCentreX() - 0.75f, r.getCentreY() - 6.0f, 1.5f, 12.0f);
        };
        presetBtn.drawGlyph = [this] (Graphics& g, Rectangle<float> r, Colour)
        {
            g.setColour (col::text);
            g.setFont (font (12.5f, true));
            g.drawText (proc.presets.currentName, r.reduced (14, 0), Justification::centredLeft, true);
            Path p; const float x = r.getRight() - 16, y = r.getCentreY();
            p.startNewSubPath (x - 4, y - 2); p.lineTo (x, y + 2); p.lineTo (x + 4, y - 2);
            g.setColour (col::accent);
            g.strokePath (p, PathStrokeType (1.5f));
        };

        undoB.onClick = [this] { proc.undo.undo(); };
        redoB.onClick = [this] { proc.undo.redo(); };
        prev.onClick = [this] { proc.undo.beginNewTransaction(); proc.presets.step (-1); };
        next.onClick = [this] { proc.undo.beginNewTransaction(); proc.presets.step (1); };
        presetBtn.onClick = [this] { showPresetMenu(); };
        save.onClick = [this] { savePreset(); };
        a.onClick = [this] { proc.selectSlot (0); };
        b.onClick = [this] { proc.selectSlot (1); };
        copy.onClick = [this] { proc.copyToOtherSlot(); };
        undoB.setTooltip ("Undo"); redoB.setTooltip ("Redo");
        save.setTooltip ("Save as user preset");
        a.setTooltip ("Compare A"); b.setTooltip ("Compare B"); copy.setTooltip ("Copy settings into the other slot");
        bypass.attach (p, "bypass");
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
        if (logo != nullptr)
            logo->drawWithin (g, getLocalBounds().toFloat().removeFromLeft (150).reduced (14, 10), RectanglePlacement::xLeft | RectanglePlacement::yMid, 1.0f);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (0, 15);
        r.removeFromLeft (160);
        undoB.setBounds (r.removeFromLeft (30)); r.removeFromLeft (4);
        redoB.setBounds (r.removeFromLeft (30));

        const int pw = 420;
        auto mid = Rectangle<int> ((getWidth() - pw) / 2, r.getY(), pw, r.getHeight());
        prev.setBounds (mid.removeFromLeft (30)); mid.removeFromLeft (6);
        save.setBounds (mid.removeFromRight (30)); mid.removeFromRight (6);
        next.setBounds (mid.removeFromRight (30)); mid.removeFromRight (6);
        presetBtn.setBounds (mid);

        auto right = getLocalBounds().reduced (8, 15);
        bypass.setBounds (right.removeFromRight (100)); right.removeFromRight (16);
        quality.setBounds (right.removeFromRight (150)); right.removeFromRight (16);
        copy.setBounds (right.removeFromRight (56));
        b.setBounds (right.removeFromRight (34));
        a.setBounds (right.removeFromRight (34));
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
        if (users.isEmpty()) um.addItem (-1, "(none yet - use +)", false);
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
        // keep the dialog inside the plugin window (sandboxed AU hosts like Logic block floating windows)
        if (auto* ed = findParentComponentOfClass<AudioProcessorEditor>())
        {
            ed->addAndMakeVisible (w);
            w->setCentrePosition (ed->getLocalBounds().getCentre());
        }
        w->enterModalState (true, ModalCallbackFunction::create ([this, w] (int res)
        {
            if (res == 1) proc.presets.saveUser (w->getTextEditorContents ("name"));
        }), true);
    }

    HaloProcessor& proc;
    GlyphButton undoB, redoB, prev, next, presetBtn, save, a, b, copy;
    ValueChip quality;
    NeonToggle bypass;
    String lastName;
    std::unique_ptr<Drawable> logo = makeLogo();
};

//==============================================================================
// Footer: input meter + gain, dry/wet, output gain + meter + peak
class FooterBar : public Component
{
public:
    explicit FooterBar (HaloProcessor& p) : proc (p), inChip (p, "in_gain", "In"), outChip (p, "out_gain", "Out"), mixChip (p, "mix", "Dry/Wet")
    {
        for (auto* c : std::initializer_list<Component*> { &inChip, &outChip, &mixChip, &inMeter, &outMeter }) addAndMakeVisible (c);
    }
    void tick (float dt)
    {
        auto take = [] (std::atomic<float>& a) { return a.exchange (0.0f); };
        const float il = take (proc.meters.inL), ir = take (proc.meters.inR), ol = take (proc.meters.outL), orr = take (proc.meters.outR);
        inMeter.setLevels (il, ir, dt);
        outMeter.setLevels (ol, orr, dt);
        proc.meters.uiOut = std::max (ol, orr);
        repaint (valueArea);
    }
    void paint (Graphics& g) override
    {
        g.setFont (fontLabel (9.0f));
        g.setColour (col::label);
        g.drawText ("INPUT", inLabel, Justification::centredLeft, false);
        g.drawText ("OUTPUT", outLabel, Justification::centredRight, false);
        const float pk = outMeter.peakDb();
        g.setColour (pk > -0.3f ? col::cream : col::text);
        g.setFont (font (12.0f, true));
        g.drawText (pk > -59.0f ? String (pk, 1) + " dB" : String ("-inf"), valueArea, Justification::centredRight, false);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (14, 8);
        inLabel = r.removeFromLeft (50);
        inChip.setBounds (r.removeFromLeft (110)); r.removeFromLeft (12);
        inMeter.setBounds (r.removeFromLeft (360).withSizeKeepingCentre (360, 12));
        valueArea = r.removeFromRight (74);
        r.removeFromRight (10);
        outMeter.setBounds (r.removeFromRight (360).withSizeKeepingCentre (360, 12)); r.removeFromRight (12);
        outChip.setBounds (r.removeFromRight (110)); r.removeFromRight (8);
        outLabel = r.removeFromRight (56);
        mixChip.setBounds (r.withSizeKeepingCentre (150, r.getHeight()));
    }

private:
    HaloProcessor& proc;
    ValueChip inChip, outChip, mixChip;
    LevelMeterH inMeter, outMeter;
    Rectangle<int> inLabel, outLabel, valueArea;
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
    footer = std::make_unique<FooterBar> (p);
    content.addAndMakeVisible (*footer);

    for (int d = 0; d < dept::Count; ++d)
    {
        cards.push_back (std::make_unique<DeptCard> (p, d));
        content.addAndMakeVisible (*cards.back());
    }
    auto pv = [&p] (const char* id) { return p.apvts.getRawParameterValue (id)->load(); };

    cards[0]->drawIcon = [] (Graphics& g, Rectangle<float> r, float t, float act, float a) { icons::dynamics (g, r, t, act, a); };
    cards[0]->activity = [&p] { return clamp01 (p.meters.uiComp / 12.0f); };
    cards[1]->drawIcon = [this] (Graphics& g, Rectangle<float> r, float t, float, float a) { icons::tone (g, r, t, engine, a, toneSmooth); };
    cards[2]->drawIcon = [pv] (Graphics& g, Rectangle<float> r, float t, float act, float a)
        { icons::space (g, r, t, act, pv ("rev_decay") * 0.01f, pv ("dly_mix") * 0.01f, a); };
    cards[2]->activity = [&p, pv] { return clamp01 ((gainToDb (p.meters.uiOut) + 48.0f) / 48.0f) * std::max (pv ("rev_mix"), pv ("dly_mix")) * 0.012f; };
    cards[3]->drawIcon = [pv] (Graphics& g, Rectangle<float> r, float t, float act, float a)
        { icons::fx (g, r, t, act, pv ("sat_drive") * 0.01f, pv ("mul_amount") * 0.01f, a); };
    cards[3]->activity = [&p] { return clamp01 (p.meters.satAct.exchange (0.0f) * 3.0f); };
    ear = std::make_unique<EarView> (p, engine);
    cards[4]->setCustomIcon (ear.get());

    details.push_back (std::make_unique<DynamicsDetail> (p));
    details.push_back (std::make_unique<ToneDetail> (p, engine));
    details.push_back (std::make_unique<SpaceDetail> (p));
    details.push_back (std::make_unique<FxDetail> (p));
    details.push_back (std::make_unique<MasterDetail> (p));
    for (auto& d : details) content.addAndMakeVisible (*d);

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
    r.removeFromTop (8);
    top->setBounds (r.removeFromTop (64));
    r.removeFromTop (14);
    footer->setBounds (r.removeFromBottom (48).translated (0, -4));
    r.removeFromBottom (12);

    const float weights[dept::Count] = { 1.0f, 1.2f, 1.1f, 0.9f, 1.0f };
    float total = 0; for (float w : weights) total += w;
    const int gap = 14;
    const float unit = (float) (r.getWidth() - gap * (dept::Count - 1)) / total;
    const int cardH = 214;
    content.dividers.clear();
    int x = r.getX();
    for (int i = 0; i < dept::Count; ++i)
    {
        const int w = i == dept::Count - 1 ? r.getRight() - x : (int) std::round (weights[i] * unit);
        cards[(size_t) i]->setBounds (x, r.getY(), w, cardH);
        details[(size_t) i]->setBounds (x, r.getY() + cardH + 16, w, r.getHeight() - cardH - 16);
        // broken stroke between card and detail
        const float y = (float) (r.getY() + cardH + 8);
        content.dividers.add ({ (float) x + 12.0f, y, (float) x + w * 0.42f, y });
        content.dividers.add ({ (float) x + w * 0.42f + 14.0f, y, (float) x + w * 0.62f, y });
        x += w + gap;
        if (i < dept::Count - 1)
        {
            const float dx = (float) x - gap * 0.5f;
            content.dividers.add ({ dx, (float) r.getY() + 36.0f, dx, (float) r.getY() + cardH - 30.0f });
            content.dividers.add ({ dx, (float) r.getY() + cardH + 40.0f, dx, (float) r.getBottom() - 70.0f });
        }
    }
    const float ty = (float) top->getBottom() + 6.0f, fy = (float) footer->getY() - 6.0f;
    content.dividers.add ({ 18.0f, ty, BaseW * 0.40f, ty });
    content.dividers.add ({ BaseW * 0.40f + 20.0f, ty, BaseW * 0.56f, ty });
    content.dividers.add ({ BaseW * 0.44f, fy, BaseW - 18.0f, fy });
    content.dividers.add ({ BaseW * 0.36f, fy, BaseW * 0.44f - 20.0f, fy });

    splash->setBounds (0, 0, BaseW, BaseH);
}

void HaloEditor::frame()
{
    const double now = Time::getMillisecondCounterHiRes();
    const float dt = (float) std::clamp ((now - lastTime) * 0.001, 0.0, 0.1);
    lastTime = now;
    engine.update();
    proc.meters.uiLim = proc.meters.limGr.exchange (0.0f);
    proc.meters.uiComp = proc.meters.compGr.exchange (0.0f);
    footer->tick (dt);
    top->tick();
    for (int i = 0; i < dept::Count; ++i)
    {
        cards[(size_t) i]->tick (dt);
        details[(size_t) i]->tick (dt);
        const float target = cards[(size_t) i]->isOn() ? 1.0f : 0.4f;
        if (std::abs (detailAlpha[i] - target) > 0.01f)
        {
            detailAlpha[i] += (target - detailAlpha[i]) * 0.25f;
            details[(size_t) i]->setAlpha (detailAlpha[i]);
        }
    }
    ear->tick (dt);
    if (splash->isVisible()) splash->tick (dt);
}
} // namespace halo
