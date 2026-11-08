#pragma once
#include <vector>
#include "Common.h"

namespace rep
{
// Fractional delay line with 4-point Hermite interpolation.
class DelayLine
{
public:
    void prepare (int maxSamples)
    {
        size = 1;
        while (size < maxSamples + 4) size <<= 1;
        buf.assign ((size_t) size, 0.0f);
        mask = size - 1;
        w = 0;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
    inline void push (float x) noexcept { buf[(size_t) w] = x; w = (w + 1) & mask; }
    // d = delay in samples (>= 1)
    inline float read (float d) const noexcept
    {
        const float rp = (float) w - d;
        const int i = (int) std::floor (rp);
        const float f = rp - (float) i;
        const float y0 = buf[(size_t) ((i - 1) & mask)], y1 = buf[(size_t) (i & mask)];
        const float y2 = buf[(size_t) ((i + 1) & mask)], y3 = buf[(size_t) ((i + 2) & mask)];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }
    inline float readInt (int d) const noexcept { return buf[(size_t) ((w - d) & mask)]; }

private:
    std::vector<float> buf;
    int size = 1, mask = 0, w = 0;
};

// ============================================================================
//  CS-80 CHORUS / TREMOLO.
//  CHORUS 1: two BBD-style modulated delays in anti-phase (wide, gentle).
//  CHORUS 2: three-phase ensemble - a slow and a fast LFO on three taps,
//            the lush string-machine shimmer.
//  TREMOLO : stereo amplitude modulation, left and right in opposite phase.
//  The BBD path is band-limited (and very slightly saturated) like the
//  bucket-brigade original.
// ============================================================================
class ChorusTrem
{
public:
    void prepare (double sr);
    void reset();
    void process (float* L, float* R, int n, int mode, float speedHz, float depth) noexcept;

private:
    float fs = 48000.0f;
    DelayLine dl[2];
    float ph1 = 0.0f, ph2 = 0.25f, lpL = 0.0f, lpR = 0.0f, lpA = 0.3f;
    float mixSm = 0.0f, depthSm = 0.0f;
    int lastMode = -1;
};

// ============================================================================
//  Stereo delay with cross-feedback (ping-pong), band-limited repeats and
//  smoothed delay time (no zipper when you move it), optional tempo sync.
// ============================================================================
class StereoDelay
{
public:
    void prepare (double sr);
    void reset();
    void process (float* L, float* R, int n, float timeMs, float feedback, float mix, float tone) noexcept;

private:
    float fs = 48000.0f;
    DelayLine dl[2];
    float tSm = 300.0f, lp[2] {}, hp[2] {}, mixSm = 0.0f;
    bool first = true;
};

// ============================================================================
//  HALL - an algorithmic plate/hall in the spirit of the big digital
//  reverbs of the early 80s: pre-delay, four input diffusers, then an
//  8-line feedback delay network (Householder mixing) with slowly
//  modulated line lengths for a chorused, never-metallic tail, per-line
//  high damping and a decay time that is exact for every line length.
// ============================================================================
class Hall
{
public:
    void prepare (double sr);
    void reset();
    // size 0..1, decay seconds, tone 0 (dark) .. 1 (bright), predelay ms
    void process (float* L, float* R, int n, float size, float decaySec, float tone,
                  float predelayMs, float mix) noexcept;

private:
    static constexpr int N = 8;
    float fs = 48000.0f;
    DelayLine pre, diff[4], line[N];
    float diffLen[4] {}, baseLen[N] {};
    float damp[N] {}, lfoPh[N] {};
    float mixSm = 0.0f, sizeSm = 0.5f;
    bool first = true;
    float hpL = 0.0f, hpR = 0.0f;
};
} // namespace rep
