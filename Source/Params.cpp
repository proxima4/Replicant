#include "Params.h"

namespace rep
{
using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using APB = juce::AudioParameterBool;
using API = juce::AudioParameterInt;
using Range = juce::NormalisableRange<float>;

juce::StringArray feetNames()      { return { "16'", "8'", "5 1/3'", "4'", "2 2/3'", "2'" }; }
juce::StringArray subFuncNames()   { return { "SINE", "SAW", "INV SAW", "SQUARE", "S/H", "NOISE" }; }
juce::StringArray tremModeNames()  { return { "OFF", "CHORUS 1", "CHORUS 2", "TREMOLO" }; }
juce::StringArray delaySyncNames() { return { "FREE", "1/16", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/1" }; }
float delaySyncBeats (int i)
{
    static const float b[] = { 0.0f, 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 2.0f / 3.0f, 1.0f, 1.5f, 2.0f, 4.0f };
    return b[juce::jlimit (0, 9, i)];
}

namespace
{
    Range skewed (float lo, float hi, float centre)
    {
        Range r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::String fmtTime (float s)
    {
        if (s < 1.0f) return juce::String (juce::roundToInt (s * 1000.0f)) + " ms";
        return juce::String (s, s < 10.0f ? 2 : 1) + " s";
    }
    juce::String fmtHz (float hz)
    {
        if (hz >= 1000.0f) return juce::String (hz / 1000.0f, hz >= 10000.0f ? 1 : 2) + " kHz";
        if (hz >= 100.0f)  return juce::String (juce::roundToInt (hz)) + " Hz";
        return juce::String (hz, hz >= 10.0f ? 1 : 2) + " Hz";
    }
    juce::String fmt10 (float v)   { return juce::String (v * 10.0f, 1); }        // CS-80 panels read 0..10
    juce::String fmtBi (float v)   { return (v > 0.0f ? "+" : "") + juce::String (v * 10.0f, 1); }
    juce::String fmtPct (float v)  { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }

    struct Adder
    {
        juce::AudioProcessorValueTreeState::ParameterLayout& l;

        void f (const juce::String& pid, const juce::String& name, Range r, float def,
                std::function<juce::String (float)> fmt)
        {
            l.add (std::make_unique<APF> (juce::ParameterID { pid, 1 }, name, r, def,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                    [fmt] (float v, int) { return fmt (v); })));
        }
        void unit (const juce::String& pid, const juce::String& name, float def)
        { f (pid, name, Range (0.0f, 1.0f), def, fmt10); }
        void bi (const juce::String& pid, const juce::String& name, float def = 0.0f)
        { f (pid, name, Range (-1.0f, 1.0f), def, fmtBi); }
        void time (const juce::String& pid, const juce::String& name, float lo, float hi, float centre, float def)
        { f (pid, name, skewed (lo, hi, centre), def, fmtTime); }
        void hz (const juce::String& pid, const juce::String& name, float lo, float hi, float centre, float def)
        { f (pid, name, skewed (lo, hi, centre), def, fmtHz); }
        void choice (const juce::String& pid, const juce::String& name, juce::StringArray items, int def)
        { l.add (std::make_unique<APC> (juce::ParameterID { pid, 1 }, name, items, def)); }
        void toggle (const juce::String& pid, const juce::String& name, bool def)
        { l.add (std::make_unique<APB> (juce::ParameterID { pid, 1 }, name, def)); }
    };
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    Adder a { layout };

    // ---- the two programmable channels --------------------------------------
    for (int c = 1; c <= 2; ++c)
    {
        const juce::String n = c == 1 ? "I " : "II ";
        a.choice (id::ch (c, "feet"), n + "Feet", feetNames(), 1);
        a.hz     (id::ch (c, "pwm_speed"), n + "PWM Speed", 0.1f, 30.0f, 4.0f, c == 1 ? 4.5f : 5.3f);
        a.unit   (id::ch (c, "pwm"), n + "PWM Depth", 0.0f);
        a.f      (id::ch (c, "pw"), n + "Pulse Width", Range (0.0f, 1.0f), 0.0f,
                  [] (float v) { return juce::String (juce::roundToInt (50.0f + 40.0f * v)) + "%"; });
        a.toggle (id::ch (c, "square"), n + "Square", false);
        a.toggle (id::ch (c, "saw"), n + "Sawtooth", true);
        a.unit   (id::ch (c, "noise"), n + "Noise", 0.0f);
        a.hz     (id::ch (c, "hpf"), n + "HPF Cutoff", 16.0f, 10000.0f, 400.0f, 16.0f);
        a.unit   (id::ch (c, "hpf_res"), n + "HPF Resonance", 0.0f);
        a.hz     (id::ch (c, "lpf"), n + "LPF Cutoff", 30.0f, 18000.0f, 1500.0f, 2500.0f);
        a.unit   (id::ch (c, "lpf_res"), n + "LPF Resonance", 0.15f);
        a.unit   (id::ch (c, "il"), n + "VCF Initial Level", 0.0f);
        a.unit   (id::ch (c, "al"), n + "VCF Attack Level", 0.3f);
        a.time   (id::ch (c, "fa"), n + "VCF Attack", 0.001f, 4.0f, 0.25f, 0.05f);
        a.time   (id::ch (c, "fd"), n + "VCF Decay", 0.001f, 12.0f, 0.6f, 0.5f);
        a.time   (id::ch (c, "fr"), n + "VCF Release", 0.001f, 12.0f, 0.6f, 0.4f);
        a.unit   (id::ch (c, "vcf_level"), n + "VCF Level", 0.8f);
        a.unit   (id::ch (c, "sine"), n + "Sine Level", 0.0f);
        a.time   (id::ch (c, "a"), n + "VCA Attack", 0.001f, 6.0f, 0.3f, 0.008f);
        a.time   (id::ch (c, "d"), n + "VCA Decay", 0.001f, 12.0f, 0.8f, 0.6f);
        a.unit   (id::ch (c, "s"), n + "VCA Sustain", 0.85f);
        a.time   (id::ch (c, "r"), n + "VCA Release", 0.001f, 15.0f, 0.8f, 0.45f);
        a.unit   (id::ch (c, "level"), n + "Level", 0.8f);
        a.unit   (id::ch (c, "tib"), n + "Initial Brilliance", 0.0f);
        a.unit   (id::ch (c, "til"), n + "Initial Level", 0.3f);
        a.unit   (id::ch (c, "tab"), n + "After Brilliance", 0.0f);
        a.unit   (id::ch (c, "tal"), n + "After Level", 0.0f);
    }

    // ---- sub oscillator --------------------------------------------------
    a.choice ("sub_func", "Sub Osc Function", subFuncNames(), 0);
    a.hz     ("sub_speed", "Sub Osc Speed", 0.1f, 80.0f, 5.0f, 5.5f);
    a.unit   ("sub_vco", "Sub Osc VCO", 0.0f);
    a.unit   ("sub_vcf", "Sub Osc VCF", 0.0f);
    a.unit   ("sub_vca", "Sub Osc VCA", 0.0f);

    // ---- ring modulator ----------------------------------------------------
    a.unit   ("ring_mod", "Ring Mod Modulation", 0.0f);
    a.hz     ("ring_speed", "Ring Mod Speed", 1.0f, 1500.0f, 60.0f, 30.0f);
    a.unit   ("ring_depth", "Ring Mod Env Depth", 0.0f);
    a.time   ("ring_a", "Ring Mod Attack", 0.001f, 4.0f, 0.3f, 0.01f);
    a.time   ("ring_d", "Ring Mod Decay", 0.001f, 8.0f, 0.6f, 0.6f);

    // ---- performance touch ------------------------------------------------
    a.unit   ("init_bend", "Initial Pitch Bend", 0.0f);
    a.time   ("bend_time", "Pitch Bend Time", 0.01f, 1.5f, 0.2f, 0.15f);
    a.unit   ("aft_pitch", "After Pitch", 0.0f);
    a.unit   ("aft_vib", "After Vibrato", 0.0f);
    a.unit   ("aft_speed", "After Sub Speed", 0.0f);

    // ---- overall brilliance / resonance, keyboard control ------------------
    a.bi     ("brilliance", "Brilliance");
    a.bi     ("resonance", "Resonance");
    a.bi     ("kb_bl", "Keyboard Brilliance Low");
    a.bi     ("kb_bh", "Keyboard Brilliance High");
    a.bi     ("kb_ll", "Keyboard Level Low");
    a.bi     ("kb_lh", "Keyboard Level High");
    a.f      ("kb_track", "Keyboard Filter Track", Range (0.0f, 1.0f), 0.5f, fmtPct);

    // ---- pitch, mix, glide ---------------------------------------------------
    a.f      ("detune", "Channel II Detune", Range (-50.0f, 50.0f), 0.0f,
              [] (float v) { return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " ct"; });
    a.f      ("coarse", "Pitch Coarse", Range (-12.0f, 12.0f, 1.0f), 0.0f,
              [] (float v) { return (v > 0.0f ? "+" : "") + juce::String (juce::roundToInt (v)) + " st"; });
    a.f      ("fine", "Pitch Fine", Range (-50.0f, 50.0f), 0.0f,
              [] (float v) { return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " ct"; });
    a.f      ("mix", "Mix I / II", Range (0.0f, 1.0f), 0.5f,
              [] (float v) { return v < 0.49f ? "I " + juce::String (juce::roundToInt ((1.0f - v) * 100.0f)) + "%"
                                  : (v > 0.51f ? "II " + juce::String (juce::roundToInt (v * 100.0f)) + "%" : "I = II"); });
    a.choice ("glide_mode", "Glide Mode", { "PORTAMENTO", "GLISSANDO" }, 0);
    a.f      ("glide_time", "Glide Time", skewed (0.0f, 3.0f, 0.3f), 0.0f,
              [] (float v) { return v < 0.0006f ? juce::String ("OFF") : fmtTime (v); });

    // ---- chorus / tremolo ----------------------------------------------------
    a.choice ("trem_mode", "Chorus / Tremolo", tremModeNames(), 0);
    a.hz     ("trem_speed", "Chorus / Trem Speed", 0.05f, 12.0f, 1.0f, 0.6f);
    a.unit   ("trem_depth", "Chorus / Trem Depth", 0.6f);

    // ---- delay -------------------------------------------------------------
    a.toggle ("dly_on", "Delay On", false);
    a.choice ("dly_sync", "Delay Sync", delaySyncNames(), 0);
    a.time   ("dly_time", "Delay Time", 0.02f, 2.0f, 0.35f, 0.38f);
    a.f      ("dly_fb", "Delay Feedback", Range (0.0f, 0.95f), 0.35f, fmtPct);
    a.f      ("dly_mix", "Delay Mix", Range (0.0f, 1.0f), 0.25f, fmtPct);
    a.f      ("dly_tone", "Delay Tone", Range (0.0f, 1.0f), 0.55f, fmtPct);

    // ---- hall ----------------------------------------------------------------
    a.toggle ("hall_on", "Hall On", true);
    a.f      ("hall_size", "Hall Size", Range (0.0f, 1.0f), 0.7f, fmtPct);
    a.time   ("hall_decay", "Hall Decay", 0.4f, 20.0f, 3.5f, 4.5f);
    a.f      ("hall_tone", "Hall Tone", Range (0.0f, 1.0f), 0.5f, fmtPct);
    a.f      ("hall_pre", "Hall Pre-delay", Range (0.0f, 150.0f), 28.0f,
              [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; });
    a.f      ("hall_mix", "Hall Mix", Range (0.0f, 1.0f), 0.28f, fmtPct);

    // ---- instrument --------------------------------------------------------
    layout.add (std::make_unique<API> (juce::ParameterID { "voices", 1 }, "Voices", 1, 16, 8));
    a.f      ("age", "Analog Age", Range (0.0f, 1.0f), 0.5f, fmtPct);
    a.f      ("spread", "Stereo Spread", Range (0.0f, 1.0f), 0.4f, fmtPct);
    a.f      ("ribbon", "Ribbon", Range (-1.0f, 1.0f), 0.0f,
              [] (float v) { return (v > 0.0f ? "+" : "") + juce::String (v, 2); });
    a.f      ("ribbon_range", "Ribbon Range", Range (1.0f, 24.0f, 1.0f), 12.0f,
              [] (float v) { return juce::String (juce::roundToInt (v)) + " st"; });
    a.f      ("bend_range", "Pitch Wheel Range", Range (0.0f, 24.0f, 1.0f), 2.0f,
              [] (float v) { return juce::String (juce::roundToInt (v)) + " st"; });
    a.choice ("wheel_dest", "Mod Wheel", { "VIBRATO", "PRESSURE", "BRILLIANCE" }, 0);
    a.toggle ("mpe", "MPE", false);
    a.f      ("master", "Volume", Range (0.0f, 1.0f), 0.7f,
              [] (float v) { return v <= 0.0001f ? juce::String ("-INF") : juce::String (juce::Decibels::gainToDecibels (v * 1.6f), 1) + " dB"; });

    return layout;
}
} // namespace rep
