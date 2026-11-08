#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "ui/NeonLookAndFeel.h"
#include "ui/Widgets.h"
#include "ui/Displays.h"
#include "ui/Backdrop.h"

namespace rep::ui
{
class Root : public juce::Component, private juce::Timer
{
public:
    explicit Root (ReplicantProcessor&);
    ~Root() override;
    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    ReplicantProcessor& proc;
    NeonLookAndFeel lnf;
    Ctx ctx;
    std::vector<Section> sections;
    juce::Image backdrop;

    EsperDisplay display;
    std::unique_ptr<ChannelCrt> crt[2];
    std::vector<std::unique_ptr<juce::Component>> controls;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, storeBtn { "STORE" }, loadBtn { "LOAD" }, saveBtn { "SAVE" },
                     setupBtn { "SETUP" }, moreBtn { "FILM SCORE SOUNDS  >>" };
    juce::TextButton memBtn[ReplicantProcessor::kNumMemories];
    juce::TextButton toneBtn[22];
    std::unique_ptr<Ribbon> ribbon;
    NeonKeyboard keyboard;
    juce::TooltipWindow tooltips { this, 900 };
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncAttach;

    bool storeArmed = false;
    int lastPatchVersion = -1, blink = 0;
    float lamps[kMaxVoices] {}, outLevel = 0.0f, subLamp = 0.0f, ringLamp = 0.0f;
    juce::String flashMsg;
    int flashTimer = 0;

    template <typename T, typename... Args> T& add (int x, int y, int w, int h, Args&&... args)
    {
        auto c = std::make_unique<T> (std::forward<Args> (args)...);
        auto& ref = *c;
        addAndMakeVisible (ref);
        ref.setBounds (x, y, w, h);
        controls.push_back (std::move (c));
        return ref;
    }

    void buildChannelRow (int ch, int y);
    void buildControlPanel (int y);
    void buildHeader();
    void buildFx (int x, int y, int w, int h);
    void buildBottom();

    void showPresetMenu();
    void showFilmMenu();
    void showSetupMenu();
    void stepPreset (int delta);
    void memoryClicked (int slot);
    void savePreset();
    void loadPreset();
    void flash (const juce::String& msg) { flashMsg = msg; flashTimer = 60; display.setPatch (msg); }
    juce::File presetFolder() const;
    void timerCallback() override;
    void updatePatchLabel();
};
} // namespace rep::ui

class ReplicantEditor : public juce::AudioProcessorEditor
{
public:
    explicit ReplicantEditor (ReplicantProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }

private:
    rep::ui::Root root;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReplicantEditor)
};
