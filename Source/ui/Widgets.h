#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "NeonLookAndFeel.h"

namespace rep::ui
{
using APVTS = juce::AudioProcessorValueTreeState;

// Shared context handed to every widget
struct Ctx
{
    APVTS& apvts;
    NeonLookAndFeel& lnf;
    std::function<void (juce::RangedAudioParameter*)> touch;   // shows the parameter on the ESPER display
};

// ---------------------------------------------------------------------------
//  CS-80 vertical slider with a label underneath (value while touched)
// ---------------------------------------------------------------------------
class CsSlider : public juce::Component
{
public:
    CsSlider (Ctx&, const juce::String& pid, const juce::String& label, int cap, bool bipolar = false);
    void resized() override;
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    juce::Slider slider;

private:
    Ctx& ctx;
    juce::String label;
    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

// ---------------------------------------------------------------------------
//  Small rotary (effects and instrument settings)
// ---------------------------------------------------------------------------
class SmallKnob : public juce::Component
{
public:
    SmallKnob (Ctx&, const juce::String& pid, const juce::String& label, int cap);
    void resized() override;
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    juce::Slider slider;

private:
    Ctx& ctx;
    juce::String label;
    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

// ---------------------------------------------------------------------------
//  Illuminated push switch bound to a bool parameter
// ---------------------------------------------------------------------------
class LightSwitch : public juce::Component
{
public:
    LightSwitch (Ctx&, const juce::String& pid, const juce::String& text, int cap);
    void resized() override { button.setBounds (getLocalBounds()); }
    juce::ToggleButton button;

private:
    Ctx& ctx;
    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};

// ---------------------------------------------------------------------------
//  Lever-style selector for a choice parameter: the options stacked as lit
//  positions (FEET, SUB OSC FUNCTION, CHORUS/TREMOLO, GLIDE MODE ...)
// ---------------------------------------------------------------------------
class StepSelector : public juce::Component, public juce::SettableTooltipClient
{
public:
    StepSelector (Ctx&, const juce::String& pid, juce::StringArray labels, int cap, bool vertical = true);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseEnter (const juce::MouseEvent&) override;

private:
    Ctx& ctx;
    juce::StringArray labels;
    int cap;
    bool vertical;
    int index = 0;
    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    void choose (int i);
    int itemAt (juce::Point<float>) const;
};

// ---------------------------------------------------------------------------
//  The ribbon controller. Touch anywhere: that point is "no bend"; slide left
//  or right to bend every sounding note; let go and it springs back - as on
//  the CS-80. Drives the automatable "ribbon" parameter.
// ---------------------------------------------------------------------------
class Ribbon : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit Ribbon (Ctx&);
    ~Ribbon() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    Ctx& ctx;
    juce::RangedAudioParameter* param = nullptr;
    std::atomic<float>* range = nullptr;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    float value = 0.0f, touchX = -1.0f, curX = -1.0f;
    void timerCallback() override { repaint(); }
};
// ---------------------------------------------------------------------------
//  Black-glass keyboard: smoked keys, cyan glow when played
// ---------------------------------------------------------------------------
class NeonKeyboard : public juce::MidiKeyboardComponent
{
public:
    NeonKeyboard (juce::MidiKeyboardState& s, NeonLookAndFeel& l)
        : juce::MidiKeyboardComponent (s, juce::MidiKeyboardComponent::horizontalKeyboard), lnf (l) {}
    void drawWhiteNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour lineColour, juce::Colour textColour) override;
    void drawBlackNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour fillColour) override;
private:
    NeonLookAndFeel& lnf;
};
} // namespace rep::ui
