#include "Backdrop.h"

namespace rep::ui
{
void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> area, const juce::Font& f,
               juce::Justification j, juce::Colour c, float alpha)
{
    g.setFont (f);
    g.setColour (c.withAlpha (0.14f * alpha));
    for (auto d : { juce::Point<float> (-1.5f, 0), juce::Point<float> (1.5f, 0), juce::Point<float> (0, -1.2f), juce::Point<float> (0, 1.2f) })
        g.drawText (t, area.translated (d.x, d.y), j, false);
    g.setColour (c.withAlpha (alpha));
    g.drawText (t, area, j, false);
}

void drawScanlines (juce::Graphics& g, juce::Rectangle<float> s, float alpha)
{
    g.setColour (juce::Colours::black.withAlpha (alpha));
    for (float y = s.getY(); y < s.getBottom(); y += 3.0f)
        g.fillRect (s.getX(), y, s.getWidth(), 1.0f);
}

juce::Rectangle<float> drawCrt (juce::Graphics& g, juce::Rectangle<float> outer)
{
    // bezel
    juce::ColourGradient bez (juce::Colour (0xff1a2026), outer.getX(), outer.getY(),
                              juce::Colour (0xff050608), outer.getX(), outer.getBottom(), false);
    g.setGradientFill (bez);
    g.fillRoundedRectangle (outer, 7.0f);
    g.setColour (juce::Colour (0xff2b363e));
    g.drawRoundedRectangle (outer.reduced (0.5f), 7.0f, 1.0f);

    auto screen = outer.reduced (6.0f);
    g.setColour (col::crtBg);
    g.fillRoundedRectangle (screen, 9.0f);

    // ESPER corner brackets
    {
        const float L = 9.0f, in = 5.0f;
        auto sc = screen.reduced (in);
        juce::Path br;
        br.startNewSubPath (sc.getX(), sc.getY() + L); br.lineTo (sc.getX(), sc.getY()); br.lineTo (sc.getX() + L, sc.getY());
        br.startNewSubPath (sc.getRight() - L, sc.getY()); br.lineTo (sc.getRight(), sc.getY()); br.lineTo (sc.getRight(), sc.getY() + L);
        br.startNewSubPath (sc.getRight(), sc.getBottom() - L); br.lineTo (sc.getRight(), sc.getBottom()); br.lineTo (sc.getRight() - L, sc.getBottom());
        br.startNewSubPath (sc.getX() + L, sc.getBottom()); br.lineTo (sc.getX(), sc.getBottom()); br.lineTo (sc.getX(), sc.getBottom() - L);
        g.setColour (col::phosphor.withAlpha (0.35f));
        g.strokePath (br, juce::PathStrokeType (1.0f));
    }
    // phosphor bloom in the centre, dark curved corners (vignette)
    juce::ColourGradient glow (col::phosphor.withAlpha (0.07f), screen.getCentreX(), screen.getCentreY(),
                               juce::Colours::transparentBlack, screen.getX(), screen.getY(), true);
    g.setGradientFill (glow);
    g.fillRoundedRectangle (screen, 9.0f);
    return screen;
}

namespace
{
    // neon-sign lettering: the glyph outlines stroked as glass tubes
    void neonText (juce::Graphics& g, const juce::String& text, const juce::Font& f, float x, float baseline,
                   float tracking, juce::Colour c, float tube)
    {
        juce::GlyphArrangement ga;
        float px = x;
        for (int i = 0; i < text.length(); ++i)
        {
            const juce::String ch = text.substring (i, i + 1);
            ga.addLineOfText (f, ch, px, baseline);
            px += juce::GlyphArrangement::getStringWidth (f, ch) + tracking;
        }
        juce::Path p;
        ga.createPath (p);
        g.setColour (c.withAlpha (0.10f));
        g.fillPath (p);
        NeonLookAndFeel::neonStroke (g, p, c, tube, 1.3f);
    }
}

juce::Image makeBackdrop (const std::vector<Section>& sections, NeonLookAndFeel& lnf, float sc)
{
    juce::Image img (juce::Image::RGB, (int) (kW * sc), (int) (kH * sc), true);
    juce::Graphics g (img);
    g.addTransform (juce::AffineTransform::scale (sc));
    juce::Random rnd (2019);

    // ---- wet black glass ----
    juce::ColourGradient base (juce::Colour (0xff070b11), 0.0f, 0.0f, juce::Colour (0xff020305), 0.0f, (float) kH, false);
    base.addColour (0.45, juce::Colour (0xff05080c));
    g.setGradientFill (base);
    g.fillAll();

    // ---- out-of-focus city lights behind the glass ----
    const juce::Colour lights[] = { col::cyan, col::magenta, col::amber, col::sodium, col::cyan, col::red };
    for (int i = 0; i < 70; ++i)
    {
        const float x = rnd.nextFloat() * kW;
        const float y = rnd.nextFloat() < 0.5f ? rnd.nextFloat() * 130.0f : rnd.nextFloat() * kH;
        const float r = 14.0f + rnd.nextFloat() * 60.0f;
        const auto c = lights[rnd.nextInt (6)];
        juce::ColourGradient bokeh (c.withAlpha (0.05f + 0.07f * rnd.nextFloat()), x, y, c.withAlpha (0.0f), x + r, y, true);
        g.setGradientFill (bokeh);
        g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);
    }
    // a magenta and a cyan neon wash at the top edge, sodium haze at the bottom
    {
        juce::ColourGradient wash (col::magenta.withAlpha (0.10f), kW * 0.15f, -40.0f, col::magenta.withAlpha (0.0f), kW * 0.15f, 220.0f, true);
        g.setGradientFill (wash); g.fillRect (0, 0, kW, 260);
        juce::ColourGradient wash2 (col::cyan.withAlpha (0.08f), kW * 0.85f, -40.0f, col::cyan.withAlpha (0.0f), kW * 0.85f, 240.0f, true);
        g.setGradientFill (wash2); g.fillRect (0, 0, kW, 260);
        juce::ColourGradient haze (col::sodium.withAlpha (0.07f), kW * 0.5f, (float) kH + 60.0f, col::sodium.withAlpha (0.0f), kW * 0.5f, kH - 260.0f, true);
        g.setGradientFill (haze); g.fillRect (0, kH - 300, kW, 300);
    }

    // ---- rain running down the glass ----
    for (int i = 0; i < 900; ++i)
    {
        const float x = rnd.nextFloat() * kW;
        const float y = rnd.nextFloat() * kH;
        const float len = 6.0f + rnd.nextFloat() * rnd.nextFloat() * 70.0f;
        const float slant = (rnd.nextFloat() - 0.5f) * 3.0f;
        const auto c = rnd.nextFloat() < 0.75f ? juce::Colour (0xffa8c8d8) : lights[rnd.nextInt (6)];
        g.setColour (c.withAlpha (0.025f + 0.05f * rnd.nextFloat()));
        g.drawLine (x, y, x + slant, y + len, 0.6f + rnd.nextFloat() * 0.8f);
    }

    // ---- smoked glass plates with neon tube outlines ----
    for (auto& s : sections)
    {
        auto r = s.r.toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f).expanded (2.0f), 8.0f);
        juce::ColourGradient plate (juce::Colour (0xe00a1219), r.getX(), r.getY(), juce::Colour (0xf0040709), r.getX(), r.getBottom(), false);
        g.setGradientFill (plate);
        g.fillRoundedRectangle (r, 6.0f);
        // reflection band across the top of the glass
        g.setColour (juce::Colours::white.withAlpha (0.025f));
        g.fillRoundedRectangle (r.getX() + 3.0f, r.getY() + 3.0f, r.getWidth() - 6.0f, juce::jmin (26.0f, r.getHeight() * 0.25f), 4.0f);

        NeonLookAndFeel::neonRect (g, r.reduced (1.5f), 6.0f, s.colour.withAlpha (0.85f), 1.1f, 0.7f);

        if (s.title.isNotEmpty())
        {
            const auto f = lnf.wide (11.5f);
            const float tw = juce::GlyphArrangement::getStringWidth (f, s.title) + 14.0f;
            auto tr = juce::Rectangle<float> (r.getX() + 10.0f, r.getY() - 8.0f, tw, 16.0f);
            g.setColour (col::bg);
            g.fillRect (tr);
            glowText (g, s.title, tr, f, juce::Justification::centred, s.colour.interpolatedWith (juce::Colours::white, 0.25f));
        }
    }

    // ---- a few droplets clinging to the glass ----
    for (int i = 0; i < 70; ++i)
    {
        const float x = rnd.nextFloat() * kW, y = rnd.nextFloat() * kH;
        const float r = 1.0f + rnd.nextFloat() * 2.4f;
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillEllipse (x - r, y - r + 0.8f, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colour (0xffcfe8f2).withAlpha (0.10f));
        g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.fillEllipse (x - r * 0.45f, y - r * 0.6f, r * 0.5f, r * 0.45f);
    }

    // ---- header: maker's badge, neon wordmark, red credits ----
    {
        // APT INDUSTRIAL badge: a stepped pyramid mark
        juce::Path pyr;
        const float bx = 24.0f, by = 22.0f;
        for (int k = 0; k < 4; ++k)
        {
            const float w = 34.0f - k * 8.0f, y = by + 30.0f - k * 7.5f;
            pyr.addRectangle (bx + (34.0f - w) * 0.5f, y, w, 6.0f);
        }
        g.setColour (col::amber.withAlpha (0.18f));
        g.fillPath (pyr);
        NeonLookAndFeel::neonStroke (g, pyr, col::amber, 0.9f, 0.8f);
        g.setFont (lnf.mono (10.0f));
        g.setColour (col::amber.withAlpha (0.85f));
        g.drawText ("APT", (int) bx - 6, 62, 46, 12, juce::Justification::centred);
        g.drawText ("INDUSTRIAL", (int) bx - 14, 73, 62, 12, juce::Justification::centred);

        neonText (g, "REPLICANT", lnf.wide (40.0f), 82.0f, 58.0f, 6.0f, col::cyan, 1.4f);
        g.setFont (lnf.serif (19.0f));
        g.setColour (col::red.withAlpha (0.25f));
        g.drawText ("Nexus-80 Polyphonic Synthesizer", 86, 63, 360, 24, juce::Justification::centredLeft);
        g.setColour (col::red);
        g.drawText ("Nexus-80 Polyphonic Synthesizer", 85, 62, 360, 24, juce::Justification::centredLeft);
        g.setFont (lnf.mono (10.5f));
        g.setColour (col::textDim);
        g.drawText ("8 VOICES  //  2 CHANNELS  //  LOS ANGELES", 88, 84, 340, 12, juce::Justification::centredLeft);
    }

    // ---- the whole panel seen through a faint CRT raster ----
    g.setColour (juce::Colours::black.withAlpha (0.10f));
    for (int y = 0; y < kH; y += 3) g.fillRect (0.0f, (float) y, (float) kW, 1.0f);

    return img;
}
} // namespace rep::ui
