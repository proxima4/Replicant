#pragma once
#include "Widgets.h"

class ReplicantProcessor;

namespace rep::ui
{
// ---------------------------------------------------------------------------
//  The main ESPER screen: patch, the parameter under your hand, and a live
//  phosphor oscilloscope of the output.
// ---------------------------------------------------------------------------
class EsperDisplay : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    EsperDisplay (ReplicantProcessor&, NeonLookAndFeel&);
    ~EsperDisplay() override;
    void setPatch (const juce::String& s) { if (s != patch) { patch = s; repaint(); } }
    void showParam (juce::RangedAudioParameter* p) { param = p; refresh(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    std::function<void()> onClick;

private:
    ReplicantProcessor& proc;
    NeonLookAndFeel& lnf;
    juce::String patch, pName { "READY" }, pValue;
    float pNorm = 0.0f;
    juce::RangedAudioParameter* param = nullptr;
    float trace[256] {};
    int frame = 0;
    void refresh();
    void timerCallback() override;
};

// ---------------------------------------------------------------------------
//  Per-channel CRT monitor: the IL/AL filter envelope (cyan) over the VCA
//  ADSR (amber), and the HPF + LPF response with resonance (phosphor).
// ---------------------------------------------------------------------------
class ChannelCrt : public juce::Component, private juce::Timer
{
public:
    ChannelCrt (Ctx&, int channel);
    ~ChannelCrt() override;
    void paint (juce::Graphics&) override;

private:
    Ctx& ctx;
    int ch;
    std::vector<std::atomic<float>*> raws;
    std::vector<float> last;
    float v (int i) const { return raws[(size_t) i] != nullptr ? raws[(size_t) i]->load() : 0.0f; }
    void timerCallback() override;
};
} // namespace rep::ui
