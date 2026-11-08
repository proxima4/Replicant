#pragma once
#include "Common.h"

namespace rep
{
constexpr int kMaxVoices = 16;
constexpr int kCtl = 4;          // control-rate decimation (in oversampled samples)

enum SubFunc   { SF_SINE = 0, SF_SAW, SF_INVSAW, SF_SQUARE, SF_SH, SF_NOISE, NUM_SUBFUNC };
enum GlideMode { GM_PORTAMENTO = 0, GM_GLISSANDO };
enum WheelDest { WD_VIBRATO = 0, WD_PRESSURE, WD_BRILLIANCE };
enum TremMode  { TM_OFF = 0, TM_CHORUS1, TM_CHORUS2, TM_TREMOLO };

// One channel ("layer") of the programmable panel
struct ChanPatch
{
    int   feet = 1;                      // 16', 8', 5 1/3', 4', 2 2/3', 2'
    float pwmHz = 4.0f, pwmDepth = 0.0f, pw = 0.0f;
    bool  square = false, saw = true;
    float noise = 0.0f;
    float hpfHz = 20.0f, hpfRes = 0.0f, lpfHz = 8000.0f, lpfRes = 0.0f;
    float il = 0.0f, al = 0.0f, fa = 0.1f, fd = 0.5f, fr = 0.5f;
    float vcfLevel = 0.8f, sine = 0.0f;
    float a = 0.01f, d = 0.5f, s = 1.0f, r = 0.4f;
    float level = 0.8f;
    float tib = 0.0f, til = 0.0f, tab = 0.0f, tal = 0.0f;   // touch: initial/after brilliance/level
};

struct Patch
{
    ChanPatch ch[2];

    int   subFunc = SF_SINE;
    float subHz = 5.0f, subVco = 0.0f, subVcf = 0.0f, subVca = 0.0f;

    float ringMod = 0.0f, ringHz = 30.0f, ringDepth = 0.0f, ringA = 0.01f, ringD = 0.5f;

    float initBend = 0.0f, bendTime = 0.15f, aftPitch = 0.0f, aftVib = 0.0f, aftSpeed = 0.0f;

    float brilliance = 0.0f, resonance = 0.0f;                         // bipolar
    float kbBL = 0.0f, kbBH = 0.0f, kbLL = 0.0f, kbLH = 0.0f, kbTrack = 0.5f;

    float detuneCents = 0.0f, coarse = 0.0f, fineCents = 0.0f;
    float mix = 0.5f;                                                  // 0 = ch I .. 1 = ch II

    int   glideMode = GM_PORTAMENTO;
    float glideTime = 0.0f;

    int   numVoices = 8;
    float age = 0.5f, spread = 0.35f;
    float ribbonRange = 12.0f, bendRange = 2.0f;
    int   wheelDest = WD_VIBRATO;
    bool  mpe = false;
};

// Performance state shared by every voice (set from MIDI / the UI)
struct Perf
{
    float bend = 0.0f;        // -1..1 (MIDI pitch wheel, channel 1 / non-MPE)
    float wheel = 0.0f;       // 0..1
    float pressure = 0.0f;    // channel aftertouch 0..1
    float ribbon = 0.0f;      // -1..1
};

// Values computed once per control tick by the synth and read by every voice
struct Globals
{
    float sub = 0.0f;          // sub oscillator -1..1
    float pwm[2] {};           // PWM LFO of each channel -1..1
    float ribbonSemis = 0.0f;  // ribbon + global pitch wheel, in semitones
    float wheel = 0.0f;        // smoothed mod wheel
    float chanPressure = 0.0f; // smoothed channel pressure
};

inline float feetSemis (int feet) noexcept
{
    static const float t[] = { -12.0f, 0.0f, 7.0f, 12.0f, 19.0f, 24.0f };
    return t[feet < 0 ? 0 : (feet > 5 ? 5 : feet)];
}
} // namespace rep
