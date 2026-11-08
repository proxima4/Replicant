#pragma once
#include "Common.h"

namespace rep
{
// ============================================================================
//  CS-80 filter envelope: IL - AL - A - D - R.
//
//  The output is in "filter octaves" relative to the cutoff sliders:
//    key down : starts at -IL, rises to +AL in time A,
//               falls back to 0 (the slider position) in time D and stays;
//    key up   : moves from wherever it is to -IL in time R.
//  So IL alone gives an attack/release sweep (the classic brass "blat"
//  when combined with AL), AL alone gives an attack/decay pluck, and the
//  sustain level is simply where the HPF/LPF sliders and BRILLIANCE sit.
//  Segments are RC curves, retriggering starts from the current level.
// ============================================================================
class FilterEnv
{
public:
    enum Stage { idle, attack, decay, sustain, release };

    void reset (float il) noexcept { value = -il; stage = idle; }

    void trigger (float il, bool fromCurrent) noexcept
    {
        if (! fromCurrent) value = -il;
        stage = attack;
    }
    void releaseNow() noexcept { if (stage != idle) stage = release; }

    // dt = time step in seconds. Times in seconds.
    float process (float il, float al, float a, float d, float r, float dt) noexcept
    {
        switch (stage)
        {
            case attack:
            {
                // aim beyond AL (RC charging towards an over-target) so the
                // segment ends in finite time, like the analog comparator
                const float span = juce_max (al + il, 0.02f);
                const float aim = al + 0.35f * span;
                value += (aim - value) * onePoleCoef (a * 0.82f, dt);
                if (value >= al) { value = al; stage = decay; }
                break;
            }
            case decay:
                value += (0.0f - value) * onePoleCoef (d * 0.22f, dt);
                if (std::abs (value) < 1.0e-4f) { value = 0.0f; stage = sustain; }
                break;
            case sustain: value = 0.0f; break;
            case release:
                value += (-il - value) * onePoleCoef (r * 0.22f, dt);
                if (std::abs (value + il) < 1.0e-4f) { value = -il; stage = idle; }
                break;
            case idle:
            default: break;
        }
        return value;
    }

    float value = 0.0f;
    Stage stage = idle;

private:
    static float juce_max (float a, float b) noexcept { return a > b ? a : b; }
};

// ============================================================================
//  Amplitude ADSR with RC segments (attack aims at 1.3 like most analog
//  envelope chips, decay/release are exponential). Times are "to ~-40 dB".
// ============================================================================
class Adsr
{
public:
    enum Stage { idle, attack, decay, sustain, release };

    void reset() noexcept { value = 0.0f; stage = idle; }
    void trigger() noexcept { stage = attack; }
    void releaseNow() noexcept { if (stage != idle) stage = release; }
    bool isIdle() const noexcept { return stage == idle; }

    float process (float a, float d, float s, float r, float dt) noexcept
    {
        switch (stage)
        {
            case attack:
                value += (1.3f - value) * onePoleCoef (a * 0.83f, dt);
                if (value >= 1.0f) { value = 1.0f; stage = decay; }
                break;
            case decay:
                value += (s - value) * onePoleCoef (d * 0.22f, dt);
                if (std::abs (value - s) < 1.0e-4f) { value = s; stage = sustain; }
                break;
            case sustain:
                value += (s - value) * onePoleCoef (0.01f, dt);   // follows the slider
                break;
            case release:
                value += (0.0f - value) * onePoleCoef (r * 0.22f, dt);
                if (value < 2.0e-5f) { value = 0.0f; stage = idle; }
                break;
            case idle:
            default: value = 0.0f; break;
        }
        return value;
    }

    float value = 0.0f;
    Stage stage = idle;
};

// ============================================================================
//  The ring modulator's attack/decay envelope (it sweeps the ring speed).
//  Retriggered by every new key, like the single ring mod of the original.
// ============================================================================
class RingEnv
{
public:
    void trigger() noexcept { stage = 1; }
    float process (float a, float d, float dt) noexcept
    {
        if (stage == 1)
        {
            value += dt / juce_max (a, 0.001f);
            if (value >= 1.0f) { value = 1.0f; stage = 2; }
        }
        else if (stage == 2)
        {
            value += (0.0f - value) * onePoleCoef (d * 0.25f, dt);
            if (value < 1.0e-4f) { value = 0.0f; stage = 0; }
        }
        return value;
    }
    float value = 0.0f;
    int stage = 0;
private:
    static float juce_max (float a, float b) noexcept { return a > b ? a : b; }
};
} // namespace rep
