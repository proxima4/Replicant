#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace rep
{
inline float clampf (float x, float a, float b) noexcept { return x < a ? a : (x > b ? b : x); }
inline float clamp01 (float x) noexcept { return clampf (x, 0.0f, 1.0f); }

// Small, fast white-noise / random source (xorshift32).
struct Rng
{
    uint32_t s = 0x9E3779B9u;
    void seed (uint32_t v) { s = v ? v : 0x1234567u; }
    uint32_t next() noexcept { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float bipolar() noexcept { return (float) (int32_t) next() * (1.0f / 2147483648.0f); }
    float unipolar() noexcept { return (float) (next() >> 8) * (1.0f / 16777216.0f); }
};

// tanh-like rational soft clipper (exact at 0, saturates to +-1 at |x| = 3)
inline float softClip (float x) noexcept
{
    if (x >  3.0f) return  1.0f;
    if (x < -3.0f) return -1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// sin(2*pi*p) for p in [0, 1) - parabolic approximation with one refinement
// step (max error about 0.1%), plenty for an oscillator or an LFO.
inline float sin2pi (float p) noexcept
{
    float y = 2.0f * p - 1.0f;                // -1..1
    float s = 4.0f * y * (1.0f - std::abs (y));
    s = 0.225f * (s * std::abs (s) - s) + s;
    return -s;
}

// 2-point polyBLEP residual. t = phase (0..1), dt = phase increment.
inline float polyBlep (float t, float dt) noexcept
{
    if (t < dt)        { t /= dt;               return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt;   return t * t + t + t + 1.0f; }
    return 0.0f;
}

inline float semisToRatio (float s) noexcept { return std::exp2 (s * (1.0f / 12.0f)); }
inline float dbToGain (float db) noexcept    { return std::pow (10.0f, db * 0.05f); }

// one-pole coefficient for a time constant, given the update interval
inline float onePoleCoef (float seconds, float dt) noexcept
{
    return seconds <= 0.0f ? 1.0f : 1.0f - std::exp (-dt / seconds);
}
} // namespace rep
