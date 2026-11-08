#pragma once
#include "Common.h"

namespace rep
{
// ============================================================================
//  VCO + wave shape converter, as on the CS-80.
//
//  The CS-80's main oscillators are sawtooth cores; the "WSC" (wave shape
//  converter) derives a variable-width pulse and a sine from the same ramp,
//  so all three are phase-locked. Square and sawtooth are on/off tabs, the
//  sine goes round the filter straight into the VCA (it is mixed there).
//  Steps are band-limited with polyBLEP; the whole voice runs at 2x.
// ============================================================================
class Vco
{
public:
    float phase = 0.0f;

    void reset (float startPhase) noexcept { phase = startPhase; }

    struct Out { float shaped; float sine; };

    // inc : phase increment per sample (0 .. 0.45)
    // pw  : pulse width (0.05 .. 0.95) - 0.5 is a square
    // gSaw / gSq : gains for the sawtooth and pulse (0 when the tab is off)
    Out process (float inc, float pw, float gSaw, float gSq) noexcept
    {
        const float p = phase;
        float shaped = 0.0f;
        if (gSaw != 0.0f)
            shaped += gSaw * ((2.0f * p - 1.0f) - polyBlep (p, inc));
        if (gSq != 0.0f)
        {
            float sq = p < pw ? 1.0f : -1.0f;
            sq += polyBlep (p, inc);
            float p2 = p - pw; if (p2 < 0.0f) p2 += 1.0f;
            sq -= polyBlep (p2, inc);
            // a pulse narrower than a square carries DC - remove it like the
            // AC-coupled original would
            shaped += gSq * (sq - (2.0f * pw - 1.0f));
        }

        // WSC sine: very slightly bent, the analog shaper is never perfect
        float s = sin2pi (p);
        s += 0.018f * (s * s - 0.5f);

        phase = p + inc;
        if (phase >= 1.0f) phase -= 1.0f;
        return { shaped, s };
    }
};
} // namespace rep
