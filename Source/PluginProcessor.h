#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "dsp/Synth.h"
#include "dsp/Effects.h"
#include "HostStop.h"

class ReplicantProcessor : public juce::AudioProcessor
{
public:
    ReplicantProcessor();
    ~ReplicantProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override { hostStop.requestStop(); }
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Replicant"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // factory presets (re-applies even if already selected)
    void loadFactoryPreset (int index)
    {
        currentProgram = -1;
        setCurrentProgram (index);
        updateHostDisplay (ChangeDetails().withProgramChanged (true));
    }

    // the CS-80's four memories: store the panel / recall it
    static constexpr int kNumMemories = 4;
    void storeMemory (int slot);
    bool recallMemory (int slot);
    bool memoryUsed (int slot) const;
    juce::String memoryName (int slot) const;
    int lastMemory() const { return activeMemory; }

    bool savePresetFile (const juce::File& f, const juce::String& name);
    bool loadPresetFile (const juce::File& f);
    juce::String getPatchName() const { return patchName; }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    // ---- read by the UI ----
    std::atomic<float> voiceLevels[rep::kMaxVoices] {};
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> ringEnv { 0.0f }, subOsc { 0.0f };
    std::atomic<int>   patchVersion { 0 };
    static constexpr int kScopeSize = 2048;
    float scope[kScopeSize] {};
    std::atomic<int> scopeWrite { 0 };

private:
    HostStop hostStop;   // FL Studio Stop = silence (see HostStop.h)
    void panic();
    void buildPatch (rep::Patch& p);
    void cacheParams();
    void applyState (const juce::ValueTree& vt);

    rep::Synth synth;
    rep::Patch patch;
    rep::ChorusTrem chorus;
    rep::StereoDelay delay;
    rep::Hall hall;
    juce::dsp::Oversampling<float> oversampling { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };
    int maxBlock = 512;
    double sr = 48000.0;
    float dcL = 0, dcR = 0, dcInL = 0, dcInR = 0, dcCoef = 0.9995f;
    juce::SmoothedValue<float> masterSm;
    int scopeDecim = 0;

    int currentProgram = 0;
    int activeMemory = -1;
    juce::String patchName { "INIT" };
    juce::ValueTree memories[kNumMemories];

    struct Raw { std::atomic<float>* p = nullptr; float operator()() const { return p->load (std::memory_order_relaxed); } };
    Raw rCh[2][27];
    Raw rSubFunc, rSubSpeed, rSubVco, rSubVcf, rSubVca;
    Raw rRingMod, rRingSpeed, rRingDepth, rRingA, rRingD;
    Raw rInitBend, rBendTime, rAftPitch, rAftVib, rAftSpeed;
    Raw rBrill, rRes, rKbBL, rKbBH, rKbLL, rKbLH, rKbTrack;
    Raw rDetune, rCoarse, rFine, rMix, rGlideMode, rGlideTime;
    Raw rTremMode, rTremSpeed, rTremDepth;
    Raw rDlyOn, rDlySync, rDlyTime, rDlyFb, rDlyMix, rDlyTone;
    Raw rHallOn, rHallSize, rHallDecay, rHallTone, rHallPre, rHallMix;
    Raw rVoices, rAge, rSpread, rRibbon, rRibbonRange, rBendRange, rWheelDest, rMpe, rMaster;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReplicantProcessor)
};
