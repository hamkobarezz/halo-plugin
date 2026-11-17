#pragma once
#include "../PluginProcessor.h"
#include "BinaryData.h"

namespace halo::ui
{
using namespace juce;

namespace col
{
    // Flat charcoal UI with the HB orange as the only accent
    inline const Colour bg      { 0xff0b0b0d };
    inline const Colour panel   { 0xff0b0b0d };
    inline const Colour panelHi { 0xff141414 };
    inline const Colour border  { 0xff2a2a2a };
    inline const Colour line    { 0xff232323 };
    inline const Colour cream   { 0xffefead8 };
    inline const Colour text    { 0xffe9e7e2 };
    inline const Colour label   { 0xffa7a49e };
    inline const Colour dim     { 0xff7c7974 };
    inline const Colour faint   { 0xff3e3c39 };
    inline const Colour accent  { 0xffff5a1f };
}

// Every module uses the brand orange
inline const Colour moduleColour[7] = { col::accent, col::accent, col::accent, col::accent, col::accent, col::accent, col::accent };

struct Fonts
{
    Typeface::Ptr regular, medium, semi, bold;
    static Fonts& get() { static Fonts f; return f; }
private:
    Fonts()
    {
        regular = Typeface::createSystemTypefaceFor (BinaryData::MontserratRegular_ttf, BinaryData::MontserratRegular_ttfSize);
        medium  = Typeface::createSystemTypefaceFor (BinaryData::MontserratMedium_ttf, BinaryData::MontserratMedium_ttfSize);
        semi    = Typeface::createSystemTypefaceFor (BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize);
        bold    = Typeface::createSystemTypefaceFor (BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize);
    }
};
// Montserrat: Medium for values / body, SemiBold for labels and headings, Bold for the wordmark
inline Font font (float h, bool semibold = false) { return Font (FontOptions (semibold ? Fonts::get().semi : Fonts::get().medium).withHeight (h)); }
inline Font fontBold (float h) { return Font (FontOptions (Fonts::get().bold).withHeight (h)); }
// Thick, open-ended divider stroke (rounded ends, never closes into a box)
inline void divider (Graphics& g, float x0, float y0, float x1, float y1)
{
    Path p;
    p.startNewSubPath (x0, y0);
    p.lineTo (x1, y1);
    g.setColour (col::line);
    g.strokePath (p, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
}

// The HALO Studios logo, rendered straight from the original SVG (embedded unchanged)
inline std::unique_ptr<Drawable> makeLogo()
{
    return Drawable::createFromImageData (BinaryData::halo_studios_logo_svg, BinaryData::halo_studios_logo_svgSize);
}

inline Font fontLabel (float h = 10.5f) { return font (h, true).withExtraKerningFactor (0.07f); }

// Clean stroke with only a faint soft edge (no neon)
inline void glowStroke (Graphics& g, const Path& p, Colour c, float w, float intensity = 1.0f)
{
    if (intensity <= 0.001f) return;
    const PathStrokeType::JointStyle j = PathStrokeType::curved;
    const PathStrokeType::EndCapStyle e = PathStrokeType::rounded;
    g.setColour (c.withMultipliedAlpha (0.08f * intensity));
    g.strokePath (p, PathStrokeType (w * 3.0f, j, e));
    g.setColour (c.withMultipliedAlpha (intensity));
    g.strokePath (p, PathStrokeType (w, j, e));
}

inline void glowText (Graphics& g, const String& s, Rectangle<float> r, Colour c, Font f, Justification just, float intensity = 1.0f)
{
    g.setFont (f);
    g.setColour (c.withMultipliedAlpha (intensity));
    g.drawText (s, r, just, false);
}

//==============================================================================
class HaloLookAndFeel : public LookAndFeel_V4
{
public:
    HaloLookAndFeel()
    {
        setDefaultSansSerifTypeface (Fonts::get().medium);
        setColour (PopupMenu::backgroundColourId, Colour (0xff0b0b0d));
        setColour (PopupMenu::textColourId, col::text);
        setColour (PopupMenu::highlightedBackgroundColourId, col::accent.withAlpha (0.18f));
        setColour (PopupMenu::highlightedTextColourId, col::cream);
        setColour (PopupMenu::headerTextColourId, col::accent);
        setColour (TooltipWindow::backgroundColourId, Colour (0xff0b0b0d));
        setColour (TooltipWindow::textColourId, col::text);
        setColour (TooltipWindow::outlineColourId, col::accent.withAlpha (0.4f));
        setColour (AlertWindow::backgroundColourId, Colour (0xff0b0b0d));
        setColour (AlertWindow::textColourId, col::text);
        setColour (AlertWindow::outlineColourId, col::accent.withAlpha (0.5f));
        setColour (TextEditor::backgroundColourId, Colour (0xff141210));
        setColour (TextEditor::textColourId, col::text);
        setColour (TextEditor::outlineColourId, col::accent.withAlpha (0.4f));
        setColour (TextEditor::focusedOutlineColourId, col::accent);
        setColour (TextButton::buttonColourId, Colour (0xff141210));
        setColour (TextButton::textColourOffId, col::text);
    }

    Font getPopupMenuFont() override { return font (15.0f); }
    Font getAlertWindowMessageFont() override { return font (15.0f); }
    Font getTextButtonFont (TextButton&, int h) override { return font (std::min (16.0f, (float) h * 0.6f)); }

    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s) override
    {
        const auto c = s.findColour (Slider::rotarySliderFillColourId);
        const auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f - 4.0f;
        const float cx = b.getCentreX(), cy = b.getCentreY();
        const bool bip = (bool) s.getProperties().getWithDefault ("bipolar", false);
        const float ang = a0 + pos * (a1 - a0);
        const float from = bip ? (a0 + a1) * 0.5f : a0;
        const bool hot = s.isMouseOverOrDragging();
        const float stroke = std::max (2.0f, r * 0.075f);

        // body
        const float br = r - stroke - 3.0f;
        g.setColour (hot ? Colour (0xff1a1a1a) : Colour (0xff111112));
        g.fillEllipse (cx - br, cy - br, br * 2, br * 2);
        g.setColour (col::border);
        g.drawEllipse (cx - br, cy - br, br * 2, br * 2, 1.0f);

        // track + value
        Path track;
        track.addCentredArc (cx, cy, r, r, 0.0f, a0, a1, true);
        g.setColour (Colour (0xff2c2c2c));
        g.strokePath (track, PathStrokeType (stroke, PathStrokeType::curved, PathStrokeType::rounded));
        if (std::abs (ang - from) > 0.005f)
        {
            Path val;
            val.addCentredArc (cx, cy, r, r, 0.0f, std::min (from, ang), std::max (from, ang), true);
            g.setColour (c);
            g.strokePath (val, PathStrokeType (stroke, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // pointer
        Path ptr;
        ptr.startNewSubPath (cx + br * 0.30f * std::sin (ang), cy - br * 0.30f * std::cos (ang));
        ptr.lineTo (cx + br * 0.78f * std::sin (ang), cy - br * 0.78f * std::cos (ang));
        g.setColour (c);
        g.strokePath (ptr, PathStrokeType (stroke * 0.9f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // Flat fader: thin track, orange fill (from centre when bipolar), small rounded cap
    void drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider::SliderStyle style, Slider& s) override
    {
        const auto c = s.findColour (Slider::rotarySliderFillColourId);
        const bool vert = style == Slider::LinearVertical;
        const bool bip = (bool) s.getProperties().getWithDefault ("bipolar", false);
        const bool hot = s.isMouseOverOrDragging();
        const auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        if (vert)
        {
            const float cx = b.getCentreX();
            const float top = b.getY() + 6.0f, bot = b.getBottom() - 6.0f;
            g.setColour (Colour (0xff2a2a2a));
            g.fillRoundedRectangle (cx - 1.5f, top, 3.0f, bot - top, 1.5f);
            const float from = bip ? (top + bot) * 0.5f : bot;
            g.setColour (c);
            g.fillRoundedRectangle (cx - 1.5f, std::min (from, pos), 3.0f, std::abs (from - pos), 1.5f);
            if (bip) { g.setColour (Colour (0xff3a3a3a)); g.fillRect (cx - 7.0f, (top + bot) * 0.5f - 0.5f, 14.0f, 1.0f); }
            auto cap = Rectangle<float> (cx - 11.0f, pos - 5.0f, 22.0f, 10.0f);
            g.setColour (hot ? Colour (0xff232323) : Colour (0xff181818));
            g.fillRoundedRectangle (cap, 3.0f);
            g.setColour (c);
            g.drawRoundedRectangle (cap, 3.0f, 1.3f);
            g.fillRect (cap.getCentreX() - 5.0f, cap.getCentreY() - 0.75f, 10.0f, 1.5f);
        }
        else
        {
            const float cy = b.getCentreY();
            const float l = b.getX() + 6.0f, r = b.getRight() - 6.0f;
            g.setColour (Colour (0xff2a2a2a));
            g.fillRoundedRectangle (l, cy - 1.5f, r - l, 3.0f, 1.5f);
            const float from = bip ? (l + r) * 0.5f : l;
            g.setColour (c);
            g.fillRoundedRectangle (std::min (from, pos), cy - 1.5f, std::abs (from - pos), 3.0f, 1.5f);
            g.setColour (hot ? col::cream : c);
            g.fillEllipse (pos - 6.0f, cy - 6.0f, 12.0f, 12.0f);
        }
    }

    void drawPopupMenuBackground (Graphics& g, int w, int h) override
    {
        g.fillAll (col::panel);
        g.setColour (col::border);
        g.drawRect (0, 0, w, h, 1);
    }
};

//==============================================================================
class FineSlider : public Slider
{
public:
    std::function<void()> onHover;
    void mouseDown (const MouseEvent& e) override
    {
        setMouseDragSensitivity (e.mods.isShiftDown() ? 1600 : 280);
        Slider::mouseDown (e);
    }
    void mouseEnter (const MouseEvent& e) override { Slider::mouseEnter (e); if (onHover) onHover(); }
    void mouseExit (const MouseEvent& e) override  { Slider::mouseExit (e); if (onHover) onHover(); }
    void mouseUp (const MouseEvent& e) override    { Slider::mouseUp (e); if (onHover) onHover(); }
};

class Knob : public Component
{
public:
    Knob (const String& labelText, Colour c) : label (labelText), colour (c)
    {
        slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (kPiF * 1.25f, kPiF * 2.75f, true);
        slider.setColour (Slider::rotarySliderFillColourId, c);
        slider.onValueChange = [this] { repaint(); };
        slider.onHover = [this] { repaint(); };
        slider.setPopupMenuEnabled (false);
        addAndMakeVisible (slider);
    }

    void attach (HaloProcessor& p, const String& id)
    {
        att.reset();
        auto* prm = p.apvts.getParameter (id);
        att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, id, slider);
        slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        const auto range = prm->getNormalisableRange();
        slider.getProperties().set ("bipolar", range.start < 0.0f && range.end > 0.0f);
        auto* undo = &p.undo;
        slider.onDragStart = [undo] { undo->beginNewTransaction(); };
        slider.setTooltip (prm->getName (64) + "  (double-click: reset, shift-drag: fine)");
        repaint();
    }
    void setLabel (const String& s) { label = s; repaint(); }
    void setLabelHeight (int h) { labelH = h; resized(); }

    void resized() override
    {
        auto b = getLocalBounds();
        auto area = b.withTrimmedBottom (labelH);
        const int d = std::min (area.getWidth(), area.getHeight());
        slider.setBounds (area.withSizeKeepingCentre (d, d));
    }
    void paint (Graphics& g) override
    {
        const bool hot = slider.isMouseOverOrDragging();
        auto r = getLocalBounds().removeFromBottom (labelH).toFloat();
        const String s = hot ? slider.getTextFromValue (slider.getValue()) : label.toUpperCase();
        g.setFont (hot ? font ((float) labelH * 0.66f, true) : fontLabel ((float) labelH * 0.6f));
        g.setColour (hot ? colour : col::label);
        g.drawText (s, r, Justification::centredTop, false);
    }

    FineSlider slider;

private:
    String label;
    Colour colour;
    int labelH = 17;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> att;
};

//==============================================================================
// "‹ Cathedral ›" selector for choice parameters (click name for the full list)
class ChoiceSelector : public Component, public SettableTooltipClient
{
public:
    ChoiceSelector (HaloProcessor& p, const String& id, Colour c, const String& caption = {})
        : colour (c), cap (caption)
    {
        param = dynamic_cast<AudioParameterChoice*> (p.apvts.getParameter (id));
        jassert (param != nullptr);
        att = std::make_unique<ParameterAttachment> (*param, [this] (float) { repaint(); }, &p.undo);
        att->sendInitialUpdate();
        setTooltip (param->getName (64));
    }

    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        if (cap.isNotEmpty())
        {
            g.setColour (col::label);
            g.setFont (fontLabel (9.5f));
            g.drawText (cap.toUpperCase(), b.removeFromTop (16.0f), Justification::centred, false);
        }
        const bool hot = isMouseOver();
        b = b.reduced (0.5f);
        g.setColour (col::panelHi);
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (hot ? col::accent.withAlpha (0.6f) : col::border);
        g.drawRoundedRectangle (b, 3.0f, 1.0f);

        auto l = b.removeFromLeft (20.0f), r = b.removeFromRight (20.0f);
        Path arrows;
        const float ay = l.getCentreY();
        arrows.startNewSubPath (l.getCentreX() + 2, ay - 3.5f); arrows.lineTo (l.getCentreX() - 1.5f, ay); arrows.lineTo (l.getCentreX() + 2, ay + 3.5f);
        arrows.startNewSubPath (r.getCentreX() - 2, ay - 3.5f); arrows.lineTo (r.getCentreX() + 1.5f, ay); arrows.lineTo (r.getCentreX() - 2, ay + 3.5f);
        g.setColour (hot ? col::accent : col::dim);
        g.strokePath (arrows, PathStrokeType (1.2f, PathStrokeType::curved, PathStrokeType::rounded));

        g.setColour (col::text);
        g.setFont (font (std::min (12.5f, b.getHeight() * 0.48f), true));
        g.drawText (param->getCurrentChoiceName(), b, Justification::centred, true);
    }

    void mouseEnter (const MouseEvent&) override { repaint(); }
    void mouseExit (const MouseEvent&) override  { repaint(); }
    void mouseDown (const MouseEvent& e) override
    {
        const int n = param->choices.size();
        const int idx = param->getIndex();
        if (e.x < 24) att->setValueAsCompleteGesture ((float) ((idx - 1 + n) % n));
        else if (e.x > getWidth() - 24) att->setValueAsCompleteGesture ((float) ((idx + 1) % n));
        else
        {
            PopupMenu m;
            for (int i = 0; i < n; ++i) m.addItem (i + 1, param->choices[i], true, i == idx);
            m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                             [this] (int r) { if (r > 0) att->setValueAsCompleteGesture ((float) (r - 1)); });
        }
    }
    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) override
    {
        const int n = param->choices.size();
        const int d = w.deltaY > 0 ? -1 : 1;
        att->setValueAsCompleteGesture ((float) std::clamp (param->getIndex() + d, 0, n - 1));
    }

private:
    Colour colour;
    String cap;
    AudioParameterChoice* param = nullptr;
    std::unique_ptr<ParameterAttachment> att;
};

//==============================================================================
class NeonToggle : public ToggleButton
{
public:
    NeonToggle (const String& text, Colour c) : ToggleButton (text), colour (c) {}
    void attach (HaloProcessor& p, const String& id)
    {
        att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id, *this);
        setTooltip (p.apvts.getParameter (id)->getName (64));
    }
    void paintButton (Graphics& g, bool over, bool) override
    {
        auto b = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = getToggleState();
        g.setColour (on ? colour.withAlpha (0.08f) : (over ? col::panelHi : col::panel));
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (on ? colour : (over ? Colour (0xff4a4a4a) : Colour (0xff3a3a3a)));
        g.drawRoundedRectangle (b, 3.0f, 1.0f);
        g.setFont (fontLabel (std::min (10.5f, b.getHeight() * 0.42f)));
        g.setColour (on ? colour : col::text);
        g.drawText (getButtonText().toUpperCase(), b, Justification::centred, false);
    }

private:
    Colour colour;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> att;
};

class PowerButton : public ToggleButton
{
public:
    explicit PowerButton (Colour c) : colour (c) {}
    void attach (HaloProcessor& p, const String& id)
    {
        att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id, *this);
        setTooltip ("Module on/off");
    }
    void paintButton (Graphics& g, bool over, bool) override
    {
        auto b = getLocalBounds().toFloat().reduced (5.0f);
        const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
        const float cx = b.getCentreX(), cy = b.getCentreY();
        Path p;
        p.addCentredArc (cx, cy, r * 0.78f, r * 0.78f, 0.0f, 0.7f, kPiF * 2.0f - 0.7f, true);
        p.startNewSubPath (cx, cy - r * 0.95f);
        p.lineTo (cx, cy - r * 0.15f);
        g.setColour (getToggleState() ? colour : col::dim.withAlpha (over ? 1.0f : 0.7f));
        g.strokePath (p, PathStrokeType (1.5f, PathStrokeType::curved, PathStrokeType::rounded));
    }

private:
    Colour colour;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> att;
};

// Plain action button (prev/next, undo, A/B …)
class GlyphButton : public Button
{
public:
    GlyphButton (const String& t, Colour c = col::accent) : Button (t), colour (c) {}
    bool selected = false;
    void setPaintFrame (bool f) { frame = f; }
    void paintButton (Graphics& g, bool over, bool down) override
    {
        auto b = getLocalBounds().toFloat().reduced (0.5f);
        if (frame)
        {
            g.setColour (selected ? colour.withAlpha (0.10f) : (down ? Colour (0xff1a1a1a) : over ? col::panelHi : col::panel));
            g.fillRoundedRectangle (b, 3.0f);
            g.setColour (selected ? colour : (over ? Colour (0xff444444) : col::border));
            g.drawRoundedRectangle (b, 3.0f, 1.0f);
        }
        const Colour fg = selected ? colour : (over ? (frame ? col::text : colour) : (frame ? col::text.withAlpha (0.85f) : col::dim));
        if (drawGlyph) { drawGlyph (g, b, fg); return; }
        g.setFont (fontLabel (std::min (11.0f, b.getHeight() * 0.42f)));
        g.setColour (fg);
        g.drawText (getButtonText(), b, Justification::centred, false);
    }
    std::function<void (Graphics&, Rectangle<float>, Colour)> drawGlyph;

private:
    bool frame = true;
    Colour colour;
};

// Horizontal gain-reduction / activity bar
class MiniBar : public Component
{
public:
    MiniBar (Colour c, String t, float maxV) : colour (c), text (std::move (t)), maxVal (maxV) {}
    void setValue (float v)
    {
        v = std::clamp (v / maxVal, 0.0f, 1.0f);
        if (std::abs (v - value) > 0.002f) { value = v; repaint(); }
    }
    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto lbl = b.removeFromLeft (text.isEmpty() ? 0.0f : 34.0f);
        if (text.isNotEmpty())
        {
            g.setColour (col::label);
            g.setFont (fontLabel (9.5f));
            g.drawText (text, lbl, Justification::centredLeft, false);
        }
        auto bar = b.withSizeKeepingCentre (b.getWidth(), std::min (3.0f, b.getHeight()));
        g.setColour (Colour (0xff2e2e2e));
        g.fillRoundedRectangle (bar, 1.5f);
        if (value > 0.001f)
        {
            g.setColour (colour);
            g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * value), 1.5f);
        }
    }

private:
    Colour colour;
    String text;
    float maxVal = 1, value = 0;
};

// Component that delegates painting (used for animated icons)
class Canvas : public Component
{
public:
    Canvas() { setInterceptsMouseClicks (false, false); setOpaque (false); }
    std::function<void (Graphics&, Rectangle<float>)> draw;
    void paint (Graphics& g) override { if (draw) draw (g, getLocalBounds().toFloat()); }
};
} // namespace halo::ui
