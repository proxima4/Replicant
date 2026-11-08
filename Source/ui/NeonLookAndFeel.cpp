#include "NeonLookAndFeel.h"
#include "BinaryData.h"

namespace rep::ui
{
NeonLookAndFeel::NeonLookAndFeel()
{
    wideFace  = juce::Typeface::createSystemTypefaceFor (BinaryData::MichromaRegular_ttf, BinaryData::MichromaRegular_ttfSize);
    monoFace  = juce::Typeface::createSystemTypefaceFor (BinaryData::ShareTechMonoRegular_ttf, BinaryData::ShareTechMonoRegular_ttfSize);
    serifFace = juce::Typeface::createSystemTypefaceFor (BinaryData::SortsMillGoudyItalic_ttf, BinaryData::SortsMillGoudyItalic_ttfSize);

    setColour (juce::ResizableWindow::backgroundColourId, col::bg);
    setColour (juce::PopupMenu::backgroundColourId, col::crtBg);
    setColour (juce::PopupMenu::textColourId, col::phosphor);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::phosphor.withAlpha (0.18f));
    setColour (juce::PopupMenu::highlightedTextColourId, col::white);
    setColour (juce::ComboBox::textColourId, col::phosphor);
    setColour (juce::ComboBox::backgroundColourId, col::crtBg);
    setColour (juce::ComboBox::outlineColourId, col::phosDim);
    setColour (juce::ComboBox::arrowColourId, col::phosphor);
    setColour (juce::TooltipWindow::backgroundColourId, col::crtBg);
    setColour (juce::TooltipWindow::textColourId, col::phosphor);
    setColour (juce::TextButton::textColourOffId, col::text);
    setColour (juce::TextButton::textColourOnId, col::white);
    setColour (juce::AlertWindow::backgroundColourId, col::crtBg);
    setColour (juce::AlertWindow::textColourId, col::phosphor);
    setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);
    setColour (juce::TextEditor::textColourId, col::phosphor);
    setColour (juce::TextEditor::outlineColourId, col::phosDim);
}

juce::Font NeonLookAndFeel::wide (float h) const  { return juce::Font (juce::FontOptions (wideFace).withHeight (h)); }
juce::Font NeonLookAndFeel::mono (float h) const  { return juce::Font (juce::FontOptions (monoFace).withHeight (h)); }
juce::Font NeonLookAndFeel::serif (float h) const { return juce::Font (juce::FontOptions (serifFace).withHeight (h)); }

juce::Typeface::Ptr NeonLookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName()
        || f.getTypefaceName() == juce::Font::getDefaultMonospacedFontName())
        return monoFace;
    return LookAndFeel_V4::getTypefaceForFont (f);
}

// ---------------------------------------------------------------------------
void NeonLookAndFeel::neonStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float width, float glow)
{
    const juce::PathStrokeType::JointStyle j = juce::PathStrokeType::curved;
    g.setColour (c.withAlpha (0.07f * glow));
    g.strokePath (p, juce::PathStrokeType (width * 7.0f, j, juce::PathStrokeType::rounded));
    g.setColour (c.withAlpha (0.16f * glow));
    g.strokePath (p, juce::PathStrokeType (width * 3.5f, j, juce::PathStrokeType::rounded));
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (width, j, juce::PathStrokeType::rounded));
    g.setColour (c.interpolatedWith (juce::Colours::white, 0.55f).withAlpha (0.8f));
    g.strokePath (p, juce::PathStrokeType (juce::jmax (0.6f, width * 0.35f), j, juce::PathStrokeType::rounded));
}

void NeonLookAndFeel::neonRect (juce::Graphics& g, juce::Rectangle<float> r, float corner, juce::Colour c, float width, float glow)
{
    juce::Path p;
    p.addRoundedRectangle (r, corner);
    neonStroke (g, p, c, width, glow);
}

// ---------------------------------------------------------------------------
//  Vertical CS-80 style slider: recessed slot with 0..10 ticks, a phosphor
//  fill showing the value, and a colour-coded cap with a lit centre line.
// ---------------------------------------------------------------------------
void NeonLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                        juce::Slider::SliderStyle style, juce::Slider& s)
{
    const auto cap = capColour ((int) s.getProperties().getWithDefault ("cap", 0));
    const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);
    const bool hot = s.isMouseOverOrDragging();

    if (style != juce::Slider::LinearVertical)
    {
        // horizontal: simple neon line + cap
        const float cy = (float) y + (float) h * 0.5f;
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle ((float) x, cy - 3.0f, (float) w, 6.0f, 3.0f);
        g.setColour (cap.withAlpha (0.8f));
        g.fillRect ((float) x, cy - 1.0f, pos - (float) x, 2.0f);
        g.setColour (cap);
        g.fillRoundedRectangle (pos - 5.0f, cy - 8.0f, 10.0f, 16.0f, 2.0f);
        return;
    }

    const float cx = (float) x + (float) w * 0.5f;
    const float top = (float) y, bot = (float) (y + h);

    // ticks 0..10
    for (int i = 0; i <= 10; ++i)
    {
        const float ty = bot - (bot - top) * (float) i / 10.0f;
        const bool major = i % 5 == 0;
        g.setColour (col::textDim.withAlpha (major ? 0.75f : 0.4f));
        g.fillRect (cx - (major ? 12.0f : 10.0f), ty - 0.5f, major ? 5.0f : 3.0f, 1.0f);
        g.fillRect (cx + (major ? 7.0f : 7.0f), ty - 0.5f, major ? 5.0f : 3.0f, 1.0f);
    }

    // slot
    auto slot = juce::Rectangle<float> (cx - 3.0f, top - 4.0f, 6.0f, bot - top + 8.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (slot, 3.0f);
    g.setColour (juce::Colour (0xff1b2a33));
    g.drawRoundedRectangle (slot, 3.0f, 1.0f);

    // value fill (glowing phosphor line in the slot)
    const float from = bipolar ? (top + bot) * 0.5f : bot;
    const float a = juce::jmin (from, pos), b = juce::jmax (from, pos);
    if (b - a > 0.5f)
    {
        g.setColour (cap.withAlpha (0.18f));
        g.fillRoundedRectangle (cx - 4.0f, a, 8.0f, b - a, 3.0f);
        g.setColour (cap.withAlpha (hot ? 1.0f : 0.85f));
        g.fillRect (cx - 1.0f, a, 2.0f, b - a);
    }

    // cap
    const float cw = juce::jmin ((float) w - 6.0f, 24.0f), ch = 14.0f;
    auto c = juce::Rectangle<float> (cx - cw * 0.5f, pos - ch * 0.5f, cw, ch);
    if (hot)
    {
        g.setColour (cap.withAlpha (0.22f));
        g.fillRoundedRectangle (c.expanded (4.0f), 5.0f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (c.translated (0.0f, 2.0f), 2.5f);
    juce::ColourGradient grad (juce::Colour (0xff2a3138), c.getX(), c.getY(), juce::Colour (0xff0b0e11), c.getX(), c.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (c, 2.5f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawHorizontalLine ((int) c.getY() + 1, c.getX() + 2.0f, c.getRight() - 2.0f);
    g.setColour (cap.withAlpha (0.25f));
    g.fillRect (c.getX() + 2.0f, pos - 2.5f, cw - 4.0f, 5.0f);
    g.setColour (cap.interpolatedWith (juce::Colours::white, hot ? 0.45f : 0.2f));
    g.fillRect (c.getX() + 2.0f, pos - 1.0f, cw - 4.0f, 2.0f);
    g.setColour (cap.withAlpha (0.55f));
    g.drawRoundedRectangle (c, 2.5f, 1.0f);
}

void NeonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float a0, float a1, juce::Slider& s)
{
    const auto cap = capColour ((int) s.getProperties().getWithDefault ("cap", 0));
    const bool hot = s.isMouseOverOrDragging();
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    const auto c = r.getCentre();
    const float ang = a0 + pos * (a1 - a0);

    juce::Path track;
    track.addCentredArc (c.x, c.y, rad - 2.0f, rad - 2.0f, 0.0f, a0, a1, true);
    g.setColour (juce::Colour (0xff15222a));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (pos > 0.002f)
    {
        juce::Path val;
        val.addCentredArc (c.x, c.y, rad - 2.0f, rad - 2.0f, 0.0f, a0, ang, true);
        neonStroke (g, val, cap.withMultipliedBrightness (hot ? 1.1f : 0.95f), 2.0f, hot ? 1.3f : 0.9f);
    }

    // knob body
    const float kr = rad * 0.66f;
    juce::ColourGradient grad (juce::Colour (0xff2b333a), c.x, c.y - kr, juce::Colour (0xff07090b), c.x, c.y + kr, false);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (juce::Colour (0xff36424b));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);

    juce::Path ptr;
    ptr.startNewSubPath (c.x + std::sin (ang) * kr * 0.25f, c.y - std::cos (ang) * kr * 0.25f);
    ptr.lineTo (c.x + std::sin (ang) * kr * 0.92f, c.y - std::cos (ang) * kr * 0.92f);
    g.setColour (cap.interpolatedWith (juce::Colours::white, 0.3f));
    g.strokePath (ptr, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Label* NeonLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (mono (13.0f));
    l->setColour (juce::Label::textColourId, col::phosphor);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}

// ---------------------------------------------------------------------------
//  Toggle buttons are illuminated push switches.
// ---------------------------------------------------------------------------
void NeonLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto c = capColour ((int) b.getProperties().getWithDefault ("cap", capAmber));
    auto r = b.getLocalBounds().toFloat().reduced (2.0f);
    const bool on = b.getToggleState();

    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r, 3.0f);
    if (on)
    {
        g.setColour (c.withAlpha (0.28f));
        g.fillRoundedRectangle (r, 3.0f);
        neonRect (g, r.reduced (1.0f), 3.0f, c, 1.2f, 1.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff22323c).withAlpha (highlighted ? 1.0f : 0.8f));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
    }
    g.setColour (on ? c.interpolatedWith (juce::Colours::white, 0.5f) : col::textDim.brighter (highlighted ? 0.4f : 0.0f));
    g.setFont (mono (juce::jmin (13.0f, r.getHeight() * 0.62f)));
    g.drawText (b.getButtonText(), r, juce::Justification::centred, false);
}

void NeonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const auto c = capColour ((int) b.getProperties().getWithDefault ("cap", capCyan));
    const bool on = b.getToggleState();
    if (b.getProperties().contains ("tone"))
    {
        // the CS-80's bright coloured tone selector buttons, as lit glass
        const auto tc = juce::Colour ((juce::uint32) (juce::int64) b.getProperties()["tone"]);
        juce::ColourGradient grad (tc.withAlpha (on ? 0.85f : 0.35f), r.getX(), r.getY(),
                                   tc.darker (0.8f).withAlpha (on ? 0.8f : 0.3f), r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 3.0f);
        if (on) neonRect (g, r, 3.0f, tc.brighter (0.4f), 1.2f, 1.2f);
        else
        {
            g.setColour (tc.withAlpha (highlighted ? 0.9f : 0.55f));
            g.drawRoundedRectangle (r, 3.0f, 1.0f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillRoundedRectangle (r.getX() + 2.0f, r.getY() + 2.0f, r.getWidth() - 4.0f, 3.0f, 1.5f);
        return;
    }
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.fillRoundedRectangle (r, 3.0f);
    if (on || down)
    {
        g.setColour (c.withAlpha (0.22f));
        g.fillRoundedRectangle (r, 3.0f);
        neonRect (g, r, 3.0f, c, 1.2f, 1.0f);
    }
    else
    {
        g.setColour (c.withAlpha (highlighted ? 0.85f : 0.45f));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
    }
}

void NeonLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool highlighted, bool down)
{
    const bool on = b.getToggleState() || down;
    if (b.getProperties().contains ("tone"))
    {
        g.setColour (on ? juce::Colours::black : juce::Colours::white.withAlpha (highlighted ? 1.0f : 0.85f));
        g.setFont (mono (12.0f));
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, false);
        return;
    }
    const auto c = capColour ((int) b.getProperties().getWithDefault ("cap", capCyan));
    g.setColour (on ? c.interpolatedWith (juce::Colours::white, 0.55f) : (highlighted ? col::white : col::text));
    g.setFont (mono (juce::jmin (14.0f, (float) b.getHeight() * 0.55f)));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, false);
}

// ---------------------------------------------------------------------------
void NeonLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& cb)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    g.setColour (col::crtBg);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (cb.isMouseOver() ? col::phosphor.withAlpha (0.8f) : col::phosDim);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    juce::Path arrow;
    const float ax = r.getRight() - 10.0f, ay = r.getCentreY();
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (col::phosphor);
    g.fillPath (arrow);
}

juce::Font NeonLookAndFeel::getComboBoxFont (juce::ComboBox& cb) { return mono (juce::jmin (14.0f, (float) cb.getHeight() * 0.65f)); }

void NeonLookAndFeel::positionComboBoxText (juce::ComboBox& cb, juce::Label& l)
{
    l.setBounds (2, 1, cb.getWidth() - 16, cb.getHeight() - 2);
    l.setFont (getComboBoxFont (cb));
    l.setJustificationType (juce::Justification::centred);
}

void NeonLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::crtBg);
    for (int y = 0; y < h; y += 3)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawHorizontalLine (y, 0.0f, (float) w);
    }
    g.setColour (col::phosDim);
    g.drawRect (0, 0, w, h, 1);
}

void NeonLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                                         bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                                         const juce::String&, const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour (col::phosDim.withAlpha (0.7f));
        g.fillRect (area.getX() + 6, area.getCentreY(), area.getWidth() - 12, 1);
        return;
    }
    auto r = area.reduced (2, 0);
    if (isHighlighted && isActive)
    {
        g.setColour (col::phosphor.withAlpha (0.16f));
        g.fillRect (r);
    }
    g.setColour (! isActive ? col::phosDim : (isTicked ? col::amber : (isHighlighted ? col::white : col::phosphor)));
    g.setFont (getPopupMenuFont());
    auto tr = r.reduced (10, 0);
    if (isTicked) g.drawText (">", tr.removeFromLeft (14), juce::Justification::centredLeft, false);
    else tr.removeFromLeft (14);
    g.drawText (text, tr, juce::Justification::centredLeft, true);
    if (hasSubMenu) g.drawText (">>", r.reduced (8, 0), juce::Justification::centredRight, false);
}

juce::Font NeonLookAndFeel::getPopupMenuFont() { return mono (16.0f); }

void NeonLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    g.fillAll (col::crtBg);
    g.setColour (col::phosDim);
    g.drawRect (0, 0, w, h, 1);
    g.setColour (col::phosphor);
    g.setFont (mono (14.0f));
    g.drawFittedText (text, 6, 2, w - 12, h - 4, juce::Justification::centredLeft, 3);
}

juce::Rectangle<int> NeonLookAndFeel::getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea)
{
    const int w = juce::jmin (420, (int) juce::GlyphArrangement::getStringWidth (mono (14.0f), tipText) + 18);
    const int h = 24;
    return juce::Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 18,
                                 screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 8) : screenPos.y + 14, w, h)
        .constrainedWithin (parentArea);
}
} // namespace rep::ui
