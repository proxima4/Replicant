#pragma once
#include "Common.h"

namespace rep
{
// ============================================================================
//  One CS-80 VCF: a 12 dB/oct high-pass followed by a 12 dB/oct low-pass,
//  each with its own resonance. Both are modelled as zero-delay-feedback
//  state-variable filters (Simper/TPT) with a soft-saturating input stage
//  and bandpass state, which gives the OTA-style thickening when the
//  resonance is pushed. As on the original, the resonance peaks hard but
//  stops short of self-oscillation.
// ============================================================================
struct Svf2
{
    float s1 = 0.0f, s2 = 0.0f;
    void reset() noexcept { s1 = s2 = 0.0f; }

    // g = tan(pi fc / fs), k = 1/Q.  Writes the low- and high-pass outputs.
    inline void process (float x, float g, float k, float& lp, float& hp) noexcept
    {
        const float a1 = 1.0f / (1.0f + g * (g + k));
        hp = (x - (g + k) * s1 - s2) * a1;
        const float v1 = g * hp;
        const float bp = v1 + s1;
        s1 = 2.2f * softClip ((bp + v1) * (1.0f / 2.2f));   // OTA limiting
        const float v2 = g * bp;
        lp = v2 + s2;
        s2 = lp + v2;
    }
};

class CsFilter
{
public:
    void reset() noexcept { hpf.reset(); lpf.reset(); }

    // gH/gL = tan() coefficients, kH/kL = damping (1/Q)
    inline float process (float x, float gH, float kH, float gL, float kL) noexcept
    {
        float lp, hp;
        // input stage: soft drive into the high-pass
        x = 1.6f * softClip (x * (1.0f / 1.6f));
        hpf.process (x, gH, kH, lp, hp);
        float lp2, hp2;
        lpf.process (hp, gL, kL, lp2, hp2);
        return lp2;
    }

    // resonance 0..1 -> damping. Q runs from 0.6 to about 11.
    static float resToK (float res) noexcept
    {
        const float q = 0.6f * std::exp2 (4.2f * clamp01 (res));
        return 1.0f / q;
    }

private:
    Svf2 hpf, lpf;
};
} // namespace rep
