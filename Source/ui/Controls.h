#pragma once
#include "Widgets.h"

namespace halo::ui
{
//==============================================================================
// Compact "LABEL  value" chip bound to any parameter.
//  drag up/down = change (shift = fine), wheel = nudge, double-click = type a number,
//  ctrl/cmd-click = reset. Choice params open a list, bool params toggle.
class ValueChip : public Component, public SettableTooltipClient, private TextEditor::Listener
{
public:
    ValueChip (HaloProcessor& p, const String& id, const String& labelText, bool bigValue = false)
        : proc (p), label (labelText), big (bigValue)
    {
        param = p.apvts.getParameter (id);
        jassert (param != nullptr);
        choice = dynamic_cast<AudioParameterChoice*> (param);
        boolean = dynamic_cast<AudioParameterBool*> (param);
        att = std::make_unique<ParameterAttachment> (*param, [this] (float) { repaint(); }, &p.undo);
        att->sendInitialUpdate();
        setTooltip (param->getName (64) + (choice || boolean ? "" : "  (drag, double-click to type, ctrl-click to reset)"));
        setMouseCursor (choice || boolean ? MouseCursor::PointingHandCursor : MouseCursor::UpDownResizeCursor);
    }

    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (0.5f);
        const bool hot = isMouseOver() || dragging;
        const bool on = boolean != nullptr && boolean->get();
        g.setColour (on ? col::accent.withAlpha (0.10f) : Colour (0xff111112));
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (on || hot ? col::accent.withAlpha (on ? 1.0f : 0.55f) : col::border);
        g.drawRoundedRectangle (b, 3.0f, 1.0f);
        if (editor != nullptr) return;

        auto r = b.reduced (8.0f, 0);
        if (big)
        {
            g.setColour (col::label);
            g.setFont (fontLabel (9.0f));
            g.drawText (label.toUpperCase(), r.removeFromTop (r.getHeight() * 0.36f).withTrimmedTop (2), Justification::centredBottom, false);
            g.setColour (col::accent);
            g.setFont (fontBold (std::min (24.0f, r.getHeight() * 0.62f)));
            g.drawText (valueText(), r, Justification::centred, false);
            return;
        }
        if (label.isNotEmpty())
        {
            g.setColour (col::label);
            g.setFont (fontLabel (8.8f));
            g.drawText (label.toUpperCase(), r, Justification::centredLeft, false);
        }
        g.setColour (on ? col::accent : col::text);
        g.setFont (font (10.5f, true));
        g.drawText (valueText(), r, label.isNotEmpty() ? Justification::centredRight : Justification::centred, false);
    }

    void mouseEnter (const MouseEvent&) override { repaint(); }
    void mouseExit (const MouseEvent&) override  { repaint(); }

    void mouseDown (const MouseEvent& e) override
    {
        proc.undo.beginNewTransaction();
        if (e.mods.isCommandDown() || e.mods.isCtrlDown())
        {
            att->setValueAsCompleteGesture (param->convertFrom0to1 (param->getDefaultValue()));
            return;
        }
        if (boolean) { att->setValueAsCompleteGesture (boolean->get() ? 0.0f : 1.0f); return; }
        if (choice)
        {
            PopupMenu m;
            for (int i = 0; i < choice->choices.size(); ++i) m.addItem (i + 1, choice->choices[i], true, i == choice->getIndex());
            m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                             [this] (int r) { if (r > 0) att->setValueAsCompleteGesture ((float) (r - 1)); });
            return;
        }
        startNorm = param->getValue();
        startY = e.position.y;
        dragging = true;
        att->beginGesture();
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (! dragging) return;
        const float range = e.mods.isShiftDown() ? 900.0f : 180.0f;
        const float n = std::clamp (startNorm + (startY - e.position.y) / range, 0.0f, 1.0f);
        att->setValueAsPartOfGesture (param->convertFrom0to1 (n));
    }
    void mouseUp (const MouseEvent&) override
    {
        if (dragging) { att->endGesture(); dragging = false; repaint(); }
    }
    void mouseDoubleClick (const MouseEvent&) override
    {
        if (choice || boolean) return;
        if (dragging) { att->endGesture(); dragging = false; }
        editor = std::make_unique<TextEditor>();
        editor->setFont (big ? fontBold (20.0f) : font (12.0f, true));
        editor->setJustification (Justification::centred);
        editor->setColour (TextEditor::backgroundColourId, Colour (0xff0b0b0d));
        editor->setColour (TextEditor::textColourId, col::accent);
        editor->setColour (TextEditor::outlineColourId, col::accent);
        editor->setColour (TextEditor::focusedOutlineColourId, col::accent);
        editor->setColour (TextEditor::highlightColourId, col::accent.withAlpha (0.3f));
        editor->setText (String (param->convertFrom0to1 (param->getValue()), 2), false);
        editor->setInputRestrictions (12, "-+0123456789.,");
        editor->addListener (this);
        addAndMakeVisible (*editor);
        editor->setBounds (getLocalBounds().reduced (big ? 6 : 2));
        editor->grabKeyboardFocus();
        editor->selectAll();
        repaint();
    }
    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) override
    {
        if (boolean) return;
        if (choice)
        {
            const int n = choice->choices.size();
            att->setValueAsCompleteGesture ((float) std::clamp (choice->getIndex() + (w.deltaY > 0 ? -1 : 1), 0, n - 1));
            return;
        }
        att->setValueAsCompleteGesture (param->convertFrom0to1 (std::clamp (param->getValue() + w.deltaY * 0.03f, 0.0f, 1.0f)));
    }

private:
    String valueText() const
    {
        if (boolean) return boolean->get() ? "ON" : "OFF";
        return param->getText (param->getValue(), 32);
    }
    void commit (bool apply)
    {
        if (editor == nullptr) return;
        if (apply)
        {
            const auto t = editor->getText().replaceCharacter (',', '.').trim();
            if (t.isNotEmpty())
            {
                const auto range = param->getNormalisableRange();
                const float v = std::clamp (t.getFloatValue(), range.start, range.end);
                proc.undo.beginNewTransaction();
                att->setValueAsCompleteGesture (v);
            }
        }
        auto* e = editor.release();
        MessageManager::callAsync ([e] { delete e; });
        repaint();
    }
    void textEditorReturnKeyPressed (TextEditor&) override { commit (true); }
    void textEditorEscapeKeyPressed (TextEditor&) override { commit (false); }
    void textEditorFocusLost (TextEditor&) override        { commit (true); }

    HaloProcessor& proc;
    String label;
    bool big = false;
    RangedAudioParameter* param = nullptr;
    AudioParameterChoice* choice = nullptr;
    AudioParameterBool* boolean = nullptr;
    std::unique_ptr<ParameterAttachment> att;
    std::unique_ptr<TextEditor> editor;
    float startNorm = 0, startY = 0;
    bool dragging = false;
};

//==============================================================================
// Vertical fader with label (value on hover)
class FaderBox : public Component
{
public:
    FaderBox (HaloProcessor& p, const String& id, const String& labelText) : label (labelText)
    {
        slider.setSliderStyle (Slider::LinearVertical);
        slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        slider.setColour (Slider::rotarySliderFillColourId, col::accent);
        slider.onValueChange = [this] { repaint(); };
        slider.onHover = [this] { repaint(); };
        slider.setPopupMenuEnabled (false);
        addAndMakeVisible (slider);
        auto* prm = p.apvts.getParameter (id);
        att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, id, slider);
        slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        const auto range = prm->getNormalisableRange();
        slider.getProperties().set ("bipolar", range.start < 0.0f && range.end > 0.0f);
        auto* undo = &p.undo;
        slider.onDragStart = [undo] { undo->beginNewTransaction(); };
        slider.setTooltip (prm->getName (64));
    }
    void resized() override { slider.setBounds (getLocalBounds().withTrimmedBottom (18)); }
    void paint (Graphics& g) override
    {
        const bool hot = slider.isMouseOverOrDragging();
        auto r = getLocalBounds().removeFromBottom (16).toFloat();
        g.setFont (hot ? font (9.5f, true) : fontLabel (9.0f));
        g.setColour (hot ? col::accent : col::label);
        g.drawText (hot ? slider.getTextFromValue (slider.getValue()) : label.toUpperCase(), r, Justification::centred, false);
    }

private:
    String label;
    FineSlider slider;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> att;
};

//==============================================================================
// Vertical gain-reduction meter (fills downward), optional dB scale on the left
class ReductionMeter : public Component
{
public:
    ReductionMeter (String t, float maxDb, bool scale) : title (std::move (t)), maxV (maxDb), showScale (scale) {}
    void setValue (float db, float dt)
    {
        v = db > v ? db : std::max (0.0f, v - 16.0f * dt);
        repaint();
    }
    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto lbl = b.removeFromBottom (16);
        g.setColour (col::label);
        g.setFont (fontLabel (9.0f));
        g.drawText (title, lbl, Justification::centred, false);
        auto bar = b.withSizeKeepingCentre (8.0f, b.getHeight() - 4.0f);
        if (showScale)
        {
            g.setFont (font (8.0f));
            for (int i = 0; i <= 4; ++i)
            {
                const float y = bar.getY() + bar.getHeight() * (float) i / 4.0f;
                g.setColour (col::dim);
                g.drawText (String ((int) (-maxV * (float) i / 4.0f)), Rectangle<float> (b.getX() - 4, y - 5, bar.getX() - b.getX() - 2, 10), Justification::centredRight, false);
            }
        }
        // segmented track
        const int segs = 28;
        const float sh = bar.getHeight() / (float) segs;
        const int lit = (int) std::round (std::clamp (v / maxV, 0.0f, 1.0f) * segs);
        for (int i = 0; i < segs; ++i)
        {
            auto s = Rectangle<float> (bar.getX(), bar.getY() + i * sh + 0.75f, bar.getWidth(), sh - 1.5f);
            g.setColour (i < lit ? col::accent : Colour (0xff1e1e1e));
            g.fillRect (s);
        }
    }

private:
    String title;
    float maxV = 24, v = 0;
    bool showScale = false;
};

//==============================================================================
// Footer level meter: two thin segmented bars (L/R) with peak hold
class LevelMeterH : public Component
{
public:
    void setLevels (float l, float r, float dt)
    {
        const float in[2] = { gainToDb (l), gainToDb (r) };
        for (int c = 0; c < 2; ++c)
        {
            lvl[c] = in[c] > lvl[c] ? in[c] : std::max (-60.0f, lvl[c] - 30.0f * dt);
            if (in[c] > hold[c]) { hold[c] = in[c]; holdT[c] = 1.0f; }
            else if ((holdT[c] -= dt) < 0) hold[c] = std::max (-60.0f, hold[c] - 20.0f * dt);
        }
        repaint();
    }
    float peakDb() const { return std::max (hold[0], hold[1]); }
    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const int segs = 64;
        for (int c = 0; c < 2; ++c)
        {
            auto row = b.removeFromTop (b.getHeight() / (2 - c)).reduced (0, 1.5f);
            const float sw = row.getWidth() / segs;
            const int lit = (int) std::round (std::clamp ((lvl[c] + 60.0f) / 60.0f, 0.0f, 1.0f) * segs);
            const int hs = (int) std::round (std::clamp ((hold[c] + 60.0f) / 60.0f, 0.0f, 1.0f) * segs) - 1;
            for (int i = 0; i < segs; ++i)
            {
                const bool hot = i >= (int) (segs * 0.9f);
                Colour cc = i < lit ? (hot ? col::cream : col::accent) : Colour (0xff1c1c1c);
                if (i == hs && hs > 0) cc = col::text;
                g.setColour (cc);
                g.fillRect (row.getX() + i * sw + 0.5f, row.getY(), sw - 1.0f, row.getHeight());
            }
        }
    }

private:
    float lvl[2] { -60, -60 }, hold[2] { -60, -60 }, holdT[2] {};
};
} // namespace halo::ui
