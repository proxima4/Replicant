#include "Widgets.h"
#include "Backdrop.h"

namespace rep::ui
{
// ===========================================================================
CsSlider::CsSlider (Ctx& c, const juce::String& pid, const juce::String& l, int cap, bool bipolar)
    : ctx (c), label (l)
{
    param = ctx.apvts.getParameter (pid);
    slider.setName (pid);
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.getProperties().set ("cap", cap);
    slider.getProperties().set ("bipolar", bipolar);
    slider.setVelocityBasedMode (false);
    slider.setPopupMenuEnabled (false);
    addAndMakeVisible (slider);
    attachment = std::make_unique<APVTS::SliderAttachment> (ctx.apvts, pid, slider);
    if (param != nullptr)
    {
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        slider.setTooltip (param->getName (64) + "  (double-click to reset)");
    }
    slider.onValueChange = [this] { if (ctx.touch && slider.isMouseOverOrDragging()) ctx.touch (param); repaint(); };
    slider.addMouseListener (this, false);
}

void CsSlider::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (16);
    slider.setBounds (r.reduced (0, 2));
}

void CsSlider::mouseEnter (const juce::MouseEvent&) { if (ctx.touch) ctx.touch (param); repaint(); }

void CsSlider::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().removeFromBottom (15).toFloat();
    if (slider.isMouseOverOrDragging() && param != nullptr)
        glowText (g, param->getCurrentValueAsText(), r.expanded (8.0f, 0.0f), ctx.lnf.mono (11.5f),
                  juce::Justification::centred, col::phosphor);
    else
    {
        g.setColour (col::text.withAlpha (0.88f));
        g.setFont (ctx.lnf.mono (11.0f));
        g.drawText (label, r.expanded (6.0f, 0.0f), juce::Justification::centred, false);
    }
}

// ===========================================================================
SmallKnob::SmallKnob (Ctx& c, const juce::String& pid, const juce::String& l, int cap)
    : ctx (c), label (l)
{
    param = ctx.apvts.getParameter (pid);
    slider.setName (pid);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (200);
    slider.getProperties().set ("cap", cap);
    slider.setPopupMenuEnabled (false);
    addAndMakeVisible (slider);
    attachment = std::make_unique<APVTS::SliderAttachment> (ctx.apvts, pid, slider);
    if (param != nullptr)
    {
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        slider.setTooltip (param->getName (64) + "  (double-click to reset)");
    }
    slider.onValueChange = [this] { if (ctx.touch && slider.isMouseOverOrDragging()) ctx.touch (param); repaint(); };
    slider.addMouseListener (this, false);
}

void SmallKnob::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (14);
    const int s = juce::jmin (r.getWidth(), r.getHeight());
    slider.setBounds (r.withSizeKeepingCentre (s, s));
}

void SmallKnob::mouseEnter (const juce::MouseEvent&) { if (ctx.touch) ctx.touch (param); repaint(); }

void SmallKnob::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().removeFromBottom (14).toFloat();
    if (slider.isMouseOverOrDragging() && param != nullptr)
        glowText (g, param->getCurrentValueAsText(), r.expanded (10.0f, 0.0f), ctx.lnf.mono (11.0f),
                  juce::Justification::centred, col::phosphor);
    else
    {
        g.setColour (col::text.withAlpha (0.85f));
        g.setFont (ctx.lnf.mono (10.5f));
        g.drawText (label, r.expanded (8.0f, 0.0f), juce::Justification::centred, false);
    }
}

// ===========================================================================
LightSwitch::LightSwitch (Ctx& c, const juce::String& pid, const juce::String& text, int cap) : ctx (c)
{
    param = ctx.apvts.getParameter (pid);
    button.setButtonText (text);
    button.getProperties().set ("cap", cap);
    addAndMakeVisible (button);
    attachment = std::make_unique<APVTS::ButtonAttachment> (ctx.apvts, pid, button);
    button.onClick = [this] { if (ctx.touch) ctx.touch (param); };
    if (param != nullptr) button.setTooltip (param->getName (64));
}

// ===========================================================================
StepSelector::StepSelector (Ctx& c, const juce::String& pid, juce::StringArray l, int capCol, bool vert)
    : ctx (c), labels (std::move (l)), cap (capCol), vertical (vert)
{
    param = ctx.apvts.getParameter (pid);
    if (param != nullptr)
    {
        attachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
        {
            index = juce::jlimit (0, labels.size() - 1, juce::roundToInt (v));
            repaint();
        });
        attachment->sendInitialUpdate();
        setTooltip (param->getName (64));
    }
}

int StepSelector::itemAt (juce::Point<float> p) const
{
    const int n = labels.size();
    const float t = vertical ? p.y / (float) getHeight() : p.x / (float) getWidth();
    return juce::jlimit (0, n - 1, (int) (t * (float) n));
}

void StepSelector::choose (int i)
{
    if (attachment == nullptr) return;
    i = juce::jlimit (0, labels.size() - 1, i);
    attachment->setValueAsCompleteGesture ((float) i);
    if (ctx.touch) ctx.touch (param);
}

void StepSelector::mouseDown (const juce::MouseEvent& e) { choose (itemAt (e.position)); }
void StepSelector::mouseEnter (const juce::MouseEvent&) { if (ctx.touch) ctx.touch (param); }

void StepSelector::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const float d = w.deltaY != 0.0f ? w.deltaY : w.deltaX;
    if (d != 0.0f) choose (index + (d > 0.0f ? (vertical ? -1 : 1) : (vertical ? 1 : -1)));
}

void StepSelector::paint (juce::Graphics& g)
{
    const auto c = capColour (cap);
    const int n = labels.size();
    auto all = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillRoundedRectangle (all, 3.0f);
    g.setColour (juce::Colour (0xff1c2a33));
    g.drawRoundedRectangle (all.reduced (0.5f), 3.0f, 1.0f);

    // vertical lists read top = first option, like a lever's detents
    for (int i = 0; i < n; ++i)
    {
        juce::Rectangle<float> r = vertical
            ? juce::Rectangle<float> (all.getX(), all.getY() + all.getHeight() * (float) i / (float) n, all.getWidth(), all.getHeight() / (float) n)
            : juce::Rectangle<float> (all.getX() + all.getWidth() * (float) i / (float) n, all.getY(), all.getWidth() / (float) n, all.getHeight());
        const bool on = i == index;
        if (on)
        {
            g.setColour (c.withAlpha (0.2f));
            g.fillRoundedRectangle (r.reduced (2.0f, 1.5f), 2.0f);
            NeonLookAndFeel::neonRect (g, r.reduced (2.0f, 1.5f), 2.0f, c, 1.0f, 0.8f);
        }
        const float fh = juce::jmin (11.5f, r.getHeight() * 0.72f);
        g.setFont (ctx.lnf.mono (fh));
        g.setColour (on ? c.interpolatedWith (juce::Colours::white, 0.45f) : col::textDim);
        g.drawText (labels[i], r, juce::Justification::centred, false);
    }
}

// ===========================================================================
Ribbon::Ribbon (Ctx& c) : ctx (c)
{
    param = ctx.apvts.getParameter ("ribbon");
    range = ctx.apvts.getRawParameterValue ("ribbon_range");
    if (param != nullptr)
    {
        attachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v) { value = v; repaint(); });
        attachment->sendInitialUpdate();
    }
    setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    setTooltip ("RIBBON - touch and slide to bend every sounding note; springs back when released");
}

Ribbon::~Ribbon() { stopTimer(); }

void Ribbon::mouseDown (const juce::MouseEvent& e)
{
    touchX = curX = e.position.x;
    if (attachment) attachment->beginGesture();
    if (ctx.touch) ctx.touch (param);
    repaint();
}

void Ribbon::mouseDrag (const juce::MouseEvent& e)
{
    curX = e.position.x;
    const float v = juce::jlimit (-1.0f, 1.0f, (curX - touchX) / ((float) getWidth() * 0.42f));
    if (attachment) attachment->setValueAsPartOfGesture (v);
    if (ctx.touch) ctx.touch (param);
}

void Ribbon::mouseUp (const juce::MouseEvent&)
{
    if (attachment)
    {
        attachment->setValueAsPartOfGesture (0.0f);
        attachment->endGesture();
    }
    touchX = curX = -1.0f;
    repaint();
}

void Ribbon::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r, 4.0f);
    juce::ColourGradient sheen (juce::Colour (0xff141c22), r.getX(), r.getY(), juce::Colour (0xff050709), r.getX(), r.getBottom(), false);
    g.setGradientFill (sheen);
    g.fillRoundedRectangle (r.reduced (2.0f), 3.0f);

    // semitone ticks across the strip
    const float semis = range != nullptr ? range->load() : 12.0f;
    const float cx = r.getCentreX();
    const float half = r.getWidth() * 0.42f;
    const int nTicks = juce::jlimit (1, 24, juce::roundToInt (semis));
    for (int k = -nTicks; k <= nTicks; ++k)
    {
        const float x = cx + half * (float) k / (float) nTicks;
        const bool oct = (k % 12) == 0;
        g.setColour (col::textDim.withAlpha (oct ? 0.7f : 0.3f));
        g.fillRect (x - 0.5f, r.getBottom() - (oct ? 9.0f : 6.0f), 1.0f, oct ? 6.0f : 3.0f);
    }

    const bool touching = touchX >= 0.0f;
    const float px = touching ? curX : cx + value * half;
    const float from = touching ? touchX : cx;
    const float y = r.getCentreY() - 2.0f;

    // the lit section between the touch point and the finger
    if (std::abs (px - from) > 1.0f)
    {
        juce::Path p;
        p.startNewSubPath (from, y);
        p.lineTo (px, y);
        NeonLookAndFeel::neonStroke (g, p, col::cyan, 2.0f, 1.1f);
    }
    else
    {
        g.setColour (col::cyan.withAlpha (0.12f));
        g.fillRect (r.getX() + 8.0f, y - 2.0f, r.getWidth() - 16.0f, 4.0f);
        g.setColour (col::cyan.withAlpha (0.55f));
        g.fillRect (r.getX() + 8.0f, y - 0.5f, r.getWidth() - 16.0f, 1.0f);
    }
    if (touching || std::abs (value) > 0.002f)
    {
        juce::Path dot;
        dot.addEllipse (px - 4.0f, y - 4.0f, 8.0f, 8.0f);
        g.setColour (col::white);
        g.fillPath (dot);
        NeonLookAndFeel::neonStroke (g, dot, col::cyan, 1.2f, 1.4f);
    }

    g.setColour (col::cyan.withAlpha (0.35f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    g.setFont (ctx.lnf.mono (10.5f));
    g.setColour (col::textDim);
    g.drawText ("RIBBON  +/-" + juce::String (juce::roundToInt (semis)) + " ST", r.reduced (10.0f, 0.0f).withTrimmedBottom (8.0f),
                juce::Justification::centredLeft, false);
}
// ===========================================================================
void NeonKeyboard::drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> a, bool isDown, bool isOver,
                                  juce::Colour, juce::Colour)
{
    auto r = a.reduced (0.6f, 0.0f);
    juce::ColourGradient body (juce::Colour (0xff222b33), r.getX(), r.getY(), juce::Colour (0xff0e1318), r.getX(), r.getBottom(), false);
    g.setGradientFill (body);
    g.fillRect (r);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRect (r.getX(), r.getBottom() - 5.0f, r.getWidth(), 5.0f);
    if (isDown)
    {
        juce::ColourGradient glow (col::cyan.withAlpha (0.25f), r.getX(), r.getY(), col::cyan.withAlpha (0.75f), r.getX(), r.getBottom(), false);
        g.setGradientFill (glow);
        g.fillRect (r);
        g.setColour (col::cyan);
        g.fillRect (r.getX(), r.getBottom() - 3.0f, r.getWidth(), 3.0f);
    }
    else if (isOver)
    {
        g.setColour (col::cyan.withAlpha (0.10f));
        g.fillRect (r);
    }
    g.setColour (juce::Colour (0xff050709));
    g.fillRect (a.getRight() - 0.6f, a.getY(), 1.2f, a.getHeight());
    if (note % 12 == 0)
    {
        g.setFont (lnf.mono (10.0f));
        g.setColour (isDown ? juce::Colours::black : col::textDim);
        g.drawText ("C" + juce::String (note / 12 - 1), r.withTrimmedTop (r.getHeight() - 16.0f), juce::Justification::centred, false);
    }
}

void NeonKeyboard::drawBlackNote (int, juce::Graphics& g, juce::Rectangle<float> a, bool isDown, bool isOver, juce::Colour)
{
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRect (a.translated (1.5f, 0.0f).withTrimmedBottom (-2.0f));
    juce::ColourGradient body (juce::Colour (0xff0b0f13), a.getX(), a.getY(), juce::Colour (0xff020304), a.getX(), a.getBottom(), false);
    g.setGradientFill (body);
    g.fillRect (a);
    g.setColour (juce::Colour (0xff1e2830));
    g.drawRect (a, 1.0f);
    auto top = a.reduced (a.getWidth() * 0.18f, 0.0f).withTrimmedBottom (a.getHeight() * 0.18f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRect (top);
    if (isDown)
    {
        g.setColour (col::magenta.withAlpha (0.6f));
        g.fillRect (a.reduced (1.0f));
        g.setColour (col::magenta.interpolatedWith (juce::Colours::white, 0.4f));
        g.fillRect (a.getX() + 1.0f, a.getBottom() - 3.0f, a.getWidth() - 2.0f, 2.0f);
    }
    else if (isOver)
    {
        g.setColour (col::magenta.withAlpha (0.15f));
        g.fillRect (a);
    }
}
} // namespace rep::ui
