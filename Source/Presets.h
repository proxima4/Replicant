#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace rep
{
constexpr int kNumToneSelector = 22;       // presets 0..21 = the CS-80's tone selector buttons

int numFactoryPresets();
juce::String factoryPresetName (int index);
juce::String toneButtonLabel (int index);  // short label for the coloured tone buttons
void applyFactoryPreset (int index, juce::AudioProcessorValueTreeState& apvts);
} // namespace rep
