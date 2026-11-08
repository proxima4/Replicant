#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// ============================================================================
//  REPLICANT - parameter definitions.
//  Channel parameters (c1_..., c2_...) mirror one row of the CS-80's
//  programmable panel; everything else is the control panel above the
//  keyboard, plus the effects and a few modern conveniences.
// ============================================================================
namespace rep
{
namespace id
{
    inline juce::String ch (int c, const char* p) { return "c" + juce::String (c) + "_" + p; }
}

// per-channel parameter suffixes, in panel order
inline const juce::StringArray& channelParamSuffixes()
{
    static const juce::StringArray s { "feet", "pwm_speed", "pwm", "pw", "square", "saw", "noise",
                                       "hpf", "hpf_res", "lpf", "lpf_res",
                                       "il", "al", "fa", "fd", "fr",
                                       "vcf_level", "sine", "a", "d", "s", "r", "level",
                                       "tib", "til", "tab", "tal" };
    return s;
}

juce::StringArray feetNames();
juce::StringArray subFuncNames();
juce::StringArray tremModeNames();
juce::StringArray delaySyncNames();
float delaySyncBeats (int index);      // 0 = free

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
} // namespace rep
