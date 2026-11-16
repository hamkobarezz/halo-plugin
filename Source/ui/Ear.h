#pragma once
#include "Analyzer.h"

namespace halo::ui
{
//==============================================================================
// Stylised side-view ear in a 100 x 140 unit box (opening faces left).
struct EarShape
{
    static Point<float> canal() { return { 50.0f, 82.0f }; }

    static Path helix()
    {
        Path p;
        p.startNewSubPath (40, 32);
        p.cubicTo (37, 10, 62, -1, 79, 9);
        p.cubicTo (97, 21, 99, 52, 87, 70);
        p.cubicTo (78, 83, 74, 90, 74, 103);
        p.cubicTo (74, 121, 62, 137, 48, 133);
        p.cubicTo (37, 130, 35, 118, 42, 110);
        return p;
    }
    static Path inner()
    {
        Path p;
        // inner rim of the helix
        p.startNewSubPath (46, 35);
        p.cubicTo (49, 19, 70, 14, 78, 26);
        // antihelix
        p.startNewSubPath (52, 41);
        p.cubicTo (59, 24, 82, 27, 82, 46);
        p.cubicTo (82, 62, 70, 70, 66, 84);
        p.cubicTo (63, 94, 59, 99, 55, 101);
        // tragus
        p.startNewSubPath (41, 71);
        p.cubicTo (33, 74, 33, 89, 41, 92);
        // antitragus
        p.startNewSubPath (54, 105);
        p.cubicTo (60, 107, 65, 103, 65, 97);
        return p;
    }
    static Path all() { Path p = helix(); p.addPath (inner()); return p; }
};

// Returns the first `frac` (0..1) of a path's length, for draw-on animations
inline Path partialPath (const Path& src, float frac)
{
    Path out;
    if (frac <= 0.0f) return out;
    if (frac >= 1.0f) return src;
    const float target = src.getLength() * frac;
    float acc = 0;
    PathFlatteningIterator it (src);
    while (it.next())
    {
        if (it.subPathIndex == 0) out.startNewSubPath (it.x1, it.y1);
        const float len = Point<float> (it.x1, it.y1).getDistanceFrom ({ it.x2, it.y2 });
        if (acc + len >= target)
        {
            const float u = len > 0 ? (target - acc) / len : 0.0f;
            out.lineTo (it.x1 + (it.x2 - it.x1) * u, it.y1 + (it.y2 - it.y1) * u);
            break;
        }
        out.lineTo (it.x2, it.y2);
        acc += len;
    }
    return out;
}

inline Path spiral (Point<float> c, float r0, float r1, float turns, float phase)
{
    Path p;
    const int n = 90;
    for (int i = 0; i <= n; ++i)
    {
        const float u = (float) i / n;
        const float a = phase + u * turns * kPiF * 2.0f;
        const float r = r1 + (r0 - r1) * u;
        const Point<float> pt (c.x + r * std::sin (a), c.y - r * std::cos (a));
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    return p;
}

//==============================================================================
// Audio-reactive, interactive ear: sound waves converge into the ear canal at the
// rhythm of the music, the cochlea spins with the energy, the ear leans toward the
// cursor, waves come from wherever the mouse is, and clicking fires a ripple burst.
class EarView : public Component, public SettableTooltipClient
{
public:
    EarView (HaloProcessor& p, SpectrumEngine& e) : proc (p), eng (e)
    {
        setMouseCursor (MouseCursor::PointingHandCursor);
        setTooltip ("Halo Ear - move the mouse to steer the sound, click for a pulse");
    }

    void tick (float dt)
    {
        t += dt;
        const float out = gainToDb (proc.meters.uiOut);
        const float lv = std::clamp ((out + 48.0f) / 48.0f, 0.0f, 1.0f);
        level = lv > level ? level + (lv - level) * 0.5f : level + (lv - level) * 0.07f;
        auto band = [this] (float f) { return std::clamp ((eng.levelAt (1, f) + 75.0f) / 60.0f, 0.0f, 1.0f); };
        low += (band (90) - low) * 0.25f;
        mid += (band (1200) - mid) * 0.25f;
        high += (band (7000) - high) * 0.25f;
        playing = level > 0.08f;

        // steer: waves come from the left, or from the cursor while hovering
        const float targetDir = hover ? mouseDir : -kPiF * 0.5f;
        float d = targetDir - dir;
        while (d > kPiF) d -= 2 * kPiF;
        while (d < -kPiF) d += 2 * kPiF;
        dir += d * 0.12f;
        tilt += ((hover ? tiltTarget : 0.0f) - tilt) * 0.1f;
        hoverAmt += ((hover ? 1.0f : 0.0f) - hoverAmt) * 0.12f;
        spin += dt * (0.6f + 7.0f * level + 4.0f * high);

        // spawn waves with the music (faster and stronger when it's loud)
        spawnT -= dt;
        if (spawnT <= 0.0f)
        {
            waves.push_back ({ 0.0f, 0.25f + 0.75f * level, (float) waveCount++ });
            spawnT = playing ? 0.34f - 0.24f * level : 0.9f;
        }
        for (auto& w : waves) w.p += dt * (0.65f + 0.6f * level);
        waves.erase (std::remove_if (waves.begin(), waves.end(), [] (const Wave& w) { return w.p >= 1.0f; }), waves.end());

        for (auto& r : ripples) r += dt * 0.9f;
        ripples.erase (std::remove_if (ripples.begin(), ripples.end(), [] (float r) { return r >= 1.0f; }), ripples.end());
        for (auto& s : sparks) { s.pos += s.vel * dt; s.vel *= 0.95f; s.life -= dt * 1.1f; }
        sparks.erase (std::remove_if (sparks.begin(), sparks.end(), [] (const Spark& s) { return s.life <= 0.0f; }), sparks.end());
        repaint();
    }

    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        g.saveState();
        g.reduceClipRegion (b.reduced (1.0f).toNearestInt());

        const auto T = earTransform();
        const auto c = EarShape::canal().transformedBy (T);
        const float sc = scale();
        const float glow = 0.35f + 0.65f * level;

        // warm halo behind the ear
        {
            const float rr = sc * (70.0f + 25.0f * low);
            ColourGradient halo (col::accent.withAlpha (0.05f + 0.12f * level + 0.05f * hoverAmt), c.x, c.y,
                                 col::accent.withAlpha (0.0f), c.x + rr, c.y, true);
            g.setGradientFill (halo);
            g.fillEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2);
        }

        // incoming sound waves converging on the canal
        const float maxR = b.getWidth() * 0.62f;
        for (auto& w : waves)
        {
            const float r = (1.0f - w.p) * maxR + 6.0f;
            const float a = std::sin (w.p * kPiF) * w.strength;
            const float span = 0.55f + 0.25f * (1.0f - w.p);
            Path arc;
            const int n = 28;
            for (int k = 0; k <= n; ++k)
            {
                const float u = (float) k / n - 0.5f;
                const float ang = dir + u * span * 2.0f;
                const float wob = std::sin (u * 9.0f + t * 9.0f + w.seed) * (2.0f + 6.0f * high) * w.strength * (1.0f - w.p);
                const float rr = r + wob;
                const Point<float> pt (c.x + rr * std::sin (ang), c.y - rr * std::cos (ang));
                if (k == 0) arc.startNewSubPath (pt); else arc.lineTo (pt);
            }
            glowStroke (g, arc, col::accent, 1.2f + 1.4f * w.strength, a);
        }

        // click ripples
        for (float r : ripples)
        {
            Path ring;
            const float rr = 8.0f + r * b.getWidth() * 0.5f;
            ring.addEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2);
            glowStroke (g, ring, col::accent, 2.0f * (1.0f - r), 1.0f - r);
        }

        // the ear
        Path helix = EarShape::helix(), inner = EarShape::inner();
        helix.applyTransform (T);
        inner.applyTransform (T);
        g.setColour (col::accent.withAlpha (0.05f + 0.06f * level));
        g.fillPath (helix);
        glowStroke (g, helix, col::accent, 2.6f, glow + 0.2f * hoverAmt);
        glowStroke (g, inner, col::accent, 1.8f, 0.6f * glow + 0.25f * hoverAmt + 0.15f);

        // cochlea spiral spinning with the music, inside the canal
        const float cr = sc * (6.5f + 2.5f * mid);
        glowStroke (g, spiral (c, 0.8f, cr, 2.3f, spin), col::accent, 1.1f, 0.5f + 0.5f * level);
        {
            const float pr = 2.5f + 4.0f * level;
            g.setColour (col::accent.withAlpha (0.35f * glow));
            g.fillEllipse (c.x - pr * 2.2f, c.y - pr * 2.2f, pr * 4.4f, pr * 4.4f);
            g.setColour (col::cream.withAlpha (0.6f + 0.4f * level));
            g.fillEllipse (c.x - pr * 0.5f, c.y - pr * 0.5f, pr, pr);
        }

        for (auto& s : sparks)
        {
            const float a = std::clamp (s.life, 0.0f, 1.0f);
            g.setColour (col::accent.withAlpha (0.3f * a));
            g.fillEllipse (s.pos.x - 3.5f, s.pos.y - 3.5f, 7, 7);
            g.setColour (col::cream.withAlpha (a));
            g.fillEllipse (s.pos.x - 1.2f, s.pos.y - 1.2f, 2.4f, 2.4f);
        }
        g.restoreState();

        g.setFont (fontLabel (9.5f));
        g.setColour (playing ? col::accent : col::label);
        g.drawText (playing ? "LISTENING" : "IDLE", b.reduced (16, 10), Justification::topLeft, false);
    }

    void mouseMove (const MouseEvent& e) override { hover = true; aim (e.position); }
    void mouseDrag (const MouseEvent& e) override { aim (e.position); }
    void mouseEnter (const MouseEvent& e) override { hover = true; aim (e.position); }
    void mouseExit (const MouseEvent&) override { hover = false; }
    void mouseDown (const MouseEvent& e) override
    {
        aim (e.position);
        ripples.push_back (0.0f);
        const auto c = EarShape::canal().transformedBy (earTransform());
        Random rng;
        for (int i = 0; i < 26; ++i)
        {
            const float a = rng.nextFloat() * kPiF * 2.0f, v = 40.0f + rng.nextFloat() * 140.0f;
            sparks.push_back ({ c, { std::sin (a) * v, -std::cos (a) * v }, 0.7f + rng.nextFloat() * 0.5f });
        }
        for (int i = 0; i < 3; ++i) waves.push_back ({ 0.15f * (float) i, 1.0f, (float) waveCount++ });
    }

private:
    struct Wave { float p, strength, seed; };
    struct Spark { Point<float> pos, vel; float life; };

    float scale() const { return (getHeight() - 26.0f) / 140.0f; }
    AffineTransform earTransform() const
    {
        const float sc = scale() * (1.0f + 0.03f * low);
        const float x = getWidth() * 0.66f - 50.0f * sc, y = 16.0f;
        const auto c = EarShape::canal();
        return AffineTransform::scale (sc).translated (x, y)
            .rotated (tilt, x + c.x * sc, y + c.y * sc);
    }
    void aim (Point<float> m)
    {
        const auto c = EarShape::canal().transformedBy (earTransform());
        mouseDir = std::atan2 (m.x - c.x, -(m.y - c.y));
        tiltTarget = std::clamp ((m.x - c.x) / (float) getWidth(), -0.5f, 0.5f) * -0.18f
                   + std::clamp ((m.y - c.y) / (float) getHeight(), -0.5f, 0.5f) * 0.12f;
    }

    HaloProcessor& proc;
    SpectrumEngine& eng;
    float t = 0, level = 0, low = 0, mid = 0, high = 0, spin = 0;
    float dir = -kPiF * 0.5f, mouseDir = -kPiF * 0.5f, tilt = 0, tiltTarget = 0, hoverAmt = 0, spawnT = 0;
    bool hover = false, playing = false;
    int waveCount = 0;
    std::vector<Wave> waves;
    std::vector<float> ripples;
    std::vector<Spark> sparks;
};

//==============================================================================
// Opening screen: the ear draws in, the HALO Studios logo glows up, and a HUD
// progress bar loads through the engine stages before everything fades away.
class SplashScreen : public Component
{
public:
    static constexpr float Hold = 2.5f, Fade = 0.45f;

    SplashScreen()
    {
        setInterceptsMouseClicks (true, false);
        logo = makeLogo();
        if (logo != nullptr)
        {
            // soft orange bloom built once from the logo's own silhouette
            const int gw = 150, gh = 88, pad = 14;
            Image sil (Image::ARGB, gw + pad * 2, gh + pad * 2, true);
            {
                Graphics lg (sil);
                logo->drawWithin (lg, Rectangle<float> ((float) pad, (float) pad, (float) gw, (float) gh), RectanglePlacement::centred, 1.0f);
            }
            for (int y = 0; y < sil.getHeight(); ++y)
                for (int x = 0; x < sil.getWidth(); ++x)
                    sil.setPixelAt (x, y, col::accent.withAlpha (sil.getPixelAt (x, y).getFloatAlpha()));
            glow = Image (Image::ARGB, sil.getWidth(), sil.getHeight(), true);
            ImageConvolutionKernel k (15);
            k.createGaussianBlur (5.0f);
            k.applyToImage (glow, sil, sil.getBounds());
        }
    }

    bool finished() const { return t >= Hold + Fade; }
    void tick (float dt)
    {
        t += dt;
        if (finished()) { setVisible (false); return; }
        repaint();
    }
    void mouseDown (const MouseEvent&) override { t = std::max (t, Hold); }

    void paint (Graphics& g) override
    {
        const float alpha = t < Hold ? 1.0f : 1.0f - (t - Hold) / Fade;
        auto ease = [] (float v) { v = std::clamp (v, 0.0f, 1.0f); return v * v * (3.0f - 2.0f * v); };
        auto b = getLocalBounds().toFloat();
        g.fillAll (col::bg.withAlpha (alpha));
        const float cx = b.getCentreX();

        // ---- ear (draws itself in) ----
        const float sc = 1.75f;
        const float ex = cx - 50.0f * sc, ey = b.getCentreY() - 330.0f;
        const auto T = AffineTransform::scale (sc).translated (ex, ey);
        const auto c = EarShape::canal().transformedBy (T);
        for (int i = 0; i < 5; ++i)
        {
            const float p = std::fmod (t * 0.85f + (float) i / 5.0f, 1.0f);
            const float r = (1.0f - p) * 330.0f + 10.0f;
            Path arc;
            arc.addCentredArc (c.x, c.y, r, r, 0, -kPiF * 0.5f - 0.55f, -kPiF * 0.5f + 0.55f, true);
            glowStroke (g, arc, col::accent, 1.6f, std::sin (p * kPiF) * 0.55f * alpha * ease (t / 0.6f));
        }
        Path helix = partialPath (EarShape::helix(), ease (t / 1.0f));
        Path inner = partialPath (EarShape::inner(), ease ((t - 0.3f) / 0.9f));
        helix.applyTransform (T);
        inner.applyTransform (T);
        glowStroke (g, helix, col::accent, 3.2f, alpha);
        glowStroke (g, inner, col::accent, 2.2f, alpha * 0.85f);
        if (t > 0.8f)
            glowStroke (g, spiral (c, 1.0f, 12.0f, 2.3f, t * 3.0f), col::accent, 1.4f, alpha * ease ((t - 0.8f) / 0.4f));

        // ---- logo glows up ----
        const float la = ease ((t - 0.55f) / 0.6f);
        const auto logoBox = Rectangle<float> (cx - 110.0f, ey + 140.0f * sc + 34.0f, 220.0f, 129.0f).translated (0, (1.0f - la) * 8.0f);
        if (glow.isValid())
        {
            const float pulse = 0.5f + 0.5f * std::sin (t * 2.6f);
            const float gAmt = ease ((t - 0.9f) / 0.9f) * (0.55f + 0.25f * pulse) * alpha;
            const float padScale = logoBox.getWidth() / 150.0f;
            g.setOpacity (gAmt);
            g.drawImage (glow, logoBox.expanded (14.0f * padScale), RectanglePlacement::stretchToFit);
            g.setOpacity (1.0f);
        }
        if (logo != nullptr) logo->drawWithin (g, logoBox, RectanglePlacement::centred, la * alpha);

        // ---- HUD progress ----
        const float prog = ease (t / (Hold - 0.15f));
        const float hudA = alpha * ease ((t - 0.2f) / 0.4f);
        const float bw = 420.0f, bh = 10.0f;
        auto bar = Rectangle<float> (cx - bw * 0.5f, logoBox.getBottom() + 52.0f, bw, bh);

        // corner brackets framing the HUD
        auto frame = bar.expanded (26.0f, 30.0f).withTrimmedBottom (-8.0f);
        Path br;
        const float L = 14.0f;
        br.startNewSubPath (frame.getX(), frame.getY() + L); br.lineTo (frame.getX(), frame.getY()); br.lineTo (frame.getX() + L, frame.getY());
        br.startNewSubPath (frame.getRight() - L, frame.getY()); br.lineTo (frame.getRight(), frame.getY()); br.lineTo (frame.getRight(), frame.getY() + L);
        br.startNewSubPath (frame.getX(), frame.getBottom() - L); br.lineTo (frame.getX(), frame.getBottom()); br.lineTo (frame.getX() + L, frame.getBottom());
        br.startNewSubPath (frame.getRight() - L, frame.getBottom()); br.lineTo (frame.getRight(), frame.getBottom()); br.lineTo (frame.getRight(), frame.getBottom() - L);
        g.setColour (col::accent.withAlpha (0.8f * hudA));
        g.strokePath (br, PathStrokeType (1.5f));

        // stage label + percentage
        static const char* stages[] = { "INITIALISING", "LOADING DSP ENGINE", "OVERSAMPLING FILTERS", "LOADING PRESETS", "CALIBRATING METERS", "READY" };
        const int stage = std::min (5, (int) (prog * 5.999f));
        g.setFont (fontLabel (9.5f).withExtraKerningFactor (0.18f));
        g.setColour (col::label.withAlpha (hudA));
        g.drawText (stages[stage], bar.withY (bar.getY() - 20.0f).withHeight (14.0f), Justification::centredLeft, false);
        g.setColour (col::accent.withAlpha (hudA));
        g.setFont (font (11.0f, true).withExtraKerningFactor (0.1f));
        g.drawText (String ((int) std::round (prog * 100.0f)).paddedLeft ('0', 3) + "%", bar.withY (bar.getY() - 20.0f).withHeight (14.0f), Justification::centredRight, false);

        // segmented bar
        const int segs = 42;
        const float sw = bw / segs;
        const int lit = (int) std::round (prog * segs);
        for (int i = 0; i < segs; ++i)
        {
            auto s = Rectangle<float> (bar.getX() + i * sw + 1.0f, bar.getY(), sw - 2.0f, bh);
            const bool on = i < lit;
            const bool head = i == lit - 1 && prog < 0.999f;
            g.setColour ((on ? (head ? col::cream : col::accent) : Colour (0xff1e1e1e)).withAlpha (hudA));
            g.fillRect (s);
        }

        // ruler ticks + scanning marker
        const float ry = bar.getBottom() + 8.0f;
        for (int i = 0; i <= 40; ++i)
        {
            const float x = bar.getX() + bw * (float) i / 40.0f;
            const float h = (i % 10 == 0) ? 7.0f : (i % 5 == 0 ? 4.5f : 2.5f);
            g.setColour ((i <= (int) (prog * 40) ? col::accent.withAlpha (0.7f) : col::faint).withAlpha (hudA));
            g.fillRect (x - 0.5f, ry, 1.0f, h);
        }
        const float scan = bar.getX() + bw * std::fmod (t * 0.7f, 1.0f);
        g.setColour (col::accent.withAlpha (0.35f * hudA));
        g.fillRect (scan - 0.5f, bar.getY() - 6.0f, 1.0f, bh + 12.0f);

        // footer readouts
        g.setFont (fontLabel (8.5f).withExtraKerningFactor (0.2f));
        g.setColour (col::dim.withAlpha (hudA));
        g.drawText ("7 MODULES  /  70 PRESETS", Rectangle<float> (bar.getX(), ry + 14.0f, bw, 12.0f), Justification::centredLeft, false);
        g.drawText ("v1.0", Rectangle<float> (bar.getX(), ry + 14.0f, bw, 12.0f), Justification::centredRight, false);
    }

private:
    float t = 0;
    std::unique_ptr<Drawable> logo;
    Image glow;
};
} // namespace halo::ui
