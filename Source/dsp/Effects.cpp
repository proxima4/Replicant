#include "Effects.h"

namespace rep
{
static constexpr float kTwoPi = 6.28318530718f;

// ============================================================================
void ChorusTrem::prepare (double sr)
{
    fs = (float) sr;
    for (auto& d : dl) d.prepare ((int) (0.04f * fs));
    lpA = 1.0f - std::exp (-kTwoPi * 7500.0f / fs);   // BBD reconstruction filter
    reset();
}

void ChorusTrem::reset()
{
    for (auto& d : dl) d.clear();
    lpL = lpR = 0.0f;
    mixSm = 0.0f;
}

void ChorusTrem::process (float* L, float* R, int n, int mode, float speedHz, float depth) noexcept
{
    const bool chorus = mode == 1 || mode == 2;
    const bool trem = mode == 3;
    const float mixTarget = chorus ? 1.0f : 0.0f;
    if (! chorus && ! trem && mixSm < 1.0e-4f) { lastMode = mode; return; }

    const float inc1 = speedHz / fs;
    const float inc2 = speedHz * 6.3f / fs;          // ensemble's fast vibrato
    const float sm = 1.0f - std::exp (-1.0f / (0.03f * fs));

    for (int i = 0; i < n; ++i)
    {
        mixSm += (mixTarget - mixSm) * sm;
        depthSm += (depth - depthSm) * sm;
        const float x0 = L[i], x1 = R[i];

        ph1 += inc1; if (ph1 >= 1.0f) ph1 -= 1.0f;
        ph2 += inc2; if (ph2 >= 1.0f) ph2 -= 1.0f;

        if (trem)
        {
            const float s = sin2pi (ph1);
            const float d = depthSm;
            L[i] = x0 * (1.0f - d * (0.5f + 0.5f * s));
            R[i] = x1 * (1.0f - d * (0.5f - 0.5f * s));
            continue;
        }

        // feed the BBD lines (mono sum per side, softly limited like a BBD)
        dl[0].push (softClip (x0 * 0.9f));
        dl[1].push (softClip (x1 * 0.9f));
        if (mixSm < 1.0e-4f) continue;

        float wl, wr;
        const float msec = fs * 0.001f;
        if (mode == 2)
        {
            // three taps, 120 degrees apart, slow + fast modulation
            float t[3];
            for (int k = 0; k < 3; ++k)
            {
                float a = ph1 + (float) k / 3.0f; if (a >= 1.0f) a -= 1.0f;
                float b = ph2 + (float) k / 3.0f; if (b >= 1.0f) b -= 1.0f;
                t[k] = (9.0f + depthSm * (3.2f * sin2pi (a) + 0.45f * sin2pi (b))) * msec;
            }
            const float a0 = dl[0].read (t[0]), a1 = dl[0].read (t[1]), a2 = dl[1].read (t[2]);
            const float b0 = dl[1].read (t[0]), b1 = dl[1].read (t[1]), b2 = dl[0].read (t[2]);
            wl = (a0 + 0.8f * a1 + 0.6f * b2) * 0.45f;
            wr = (b0 + 0.8f * b1 + 0.6f * a2) * 0.45f;
        }
        else
        {
            const float s = sin2pi (ph1);
            const float dL = (7.0f + depthSm * 3.5f * s) * msec;
            const float dR = (7.0f - depthSm * 3.5f * s) * msec;
            wl = dl[0].read (dL);
            wr = dl[1].read (dR);
        }
        lpL += (wl - lpL) * lpA;
        lpR += (wr - lpR) * lpA;
        const float m = 0.62f * mixSm;
        L[i] = x0 * (1.0f - 0.35f * mixSm) + lpL * m;
        R[i] = x1 * (1.0f - 0.35f * mixSm) + lpR * m;
    }
    lastMode = mode;
}

// ============================================================================
void StereoDelay::prepare (double sr)
{
    fs = (float) sr;
    for (auto& d : dl) d.prepare ((int) (2.1f * fs));
    reset();
}

void StereoDelay::reset()
{
    for (auto& d : dl) d.clear();
    lp[0] = lp[1] = hp[0] = hp[1] = 0.0f;
    first = true;
    mixSm = 0.0f;
}

void StereoDelay::process (float* L, float* R, int n, float timeMs, float feedback, float mix, float tone) noexcept
{
    if (mix <= 0.0f && mixSm < 1.0e-4f) { first = true; return; }
    if (first) { tSm = timeMs; first = false; }
    const float sm = 1.0f - std::exp (-1.0f / (0.08f * fs));
    const float msm = 1.0f - std::exp (-1.0f / (0.02f * fs));
    const float lpA = 1.0f - std::exp (-kTwoPi * (1500.0f + 9000.0f * tone * tone) / fs);
    const float hpA = 1.0f - std::exp (-kTwoPi * 110.0f / fs);
    const float fb = clampf (feedback, 0.0f, 0.95f);

    for (int i = 0; i < n; ++i)
    {
        tSm += (timeMs - tSm) * sm;
        mixSm += (mix - mixSm) * msm;
        const float d = clampf (tSm * 0.001f * fs, 2.0f, 2.05f * fs);
        const float yl = dl[0].read (d), yr = dl[1].read (d);

        // band-limit the repeats
        lp[0] += (yl - lp[0]) * lpA; lp[1] += (yr - lp[1]) * lpA;
        hp[0] += (lp[0] - hp[0]) * hpA; hp[1] += (lp[1] - hp[1]) * hpA;
        const float fl = lp[0] - hp[0], fr = lp[1] - hp[1];

        // ping-pong: the mono input enters the left line, repeats cross over
        const float in = 0.5f * (L[i] + R[i]);
        dl[0].push (softClip (in + fb * fr));
        dl[1].push (softClip (fb * fl + 0.0f * in));
        L[i] += yl * mixSm;
        R[i] += yr * mixSm;
    }
}

// ============================================================================
void Hall::prepare (double sr)
{
    fs = (float) sr;
    const float scale = fs / 48000.0f;
    pre.prepare ((int) (0.2f * fs));
    static const float dms[4] = { 4.77f, 3.59f, 12.73f, 9.31f };
    for (int k = 0; k < 4; ++k) { diffLen[k] = dms[k] * 0.001f * fs; diff[k].prepare ((int) diffLen[k] + 8); }
    // mutually prime-ish lengths (ms at size 1.0)
    static const float lms[N] = { 37.1f, 41.9f, 47.3f, 53.9f, 61.7f, 67.1f, 73.3f, 79.9f };
    for (int k = 0; k < N; ++k)
    {
        baseLen[k] = lms[k] * 0.001f * fs;
        line[k].prepare ((int) (baseLen[k] * 1.6f + 64.0f * scale));
        lfoPh[k] = (float) k * 0.137f;
    }
    reset();
}

void Hall::reset()
{
    pre.clear();
    for (auto& d : diff) d.clear();
    for (auto& d : line) d.clear();
    for (auto& d : damp) d = 0.0f;
    hpL = hpR = 0.0f;
    first = true;
    mixSm = 0.0f;
}

void Hall::process (float* L, float* R, int n, float size, float decaySec, float tone,
                    float predelayMs, float mix) noexcept
{
    if (mix <= 0.0f && mixSm < 1.0e-4f) return;
    if (first) { sizeSm = size; first = false; }

    const float sm = 1.0f - std::exp (-1.0f / (0.25f * fs));
    const float msm = 1.0f - std::exp (-1.0f / (0.02f * fs));
    const float dampA = 1.0f - std::exp (-kTwoPi * (1800.0f + 12000.0f * tone * tone) / fs);
    const float hpA = 1.0f - std::exp (-kTwoPi * 70.0f / fs);
    const float preD = clampf (predelayMs * 0.001f * fs, 1.0f, 0.19f * fs);
    const float modDepth = 0.0009f * fs;                 // +-0.9 ms of line modulation
    const float lfoInc[N] = { 0.11f / fs, 0.17f / fs, 0.23f / fs, 0.29f / fs, 0.13f / fs, 0.19f / fs, 0.31f / fs, 0.37f / fs };

    // per-line gains for the requested T60 (once per block - size moves slowly)
    float gk[N];
    {
        const float sc = 0.45f + 1.05f * sizeSm;
        const float dec = std::max (0.2f, decaySec);
        for (int k = 0; k < N; ++k) gk[k] = std::pow (10.0f, -3.0f * baseLen[k] * sc / (fs * dec));
    }

    for (int i = 0; i < n; ++i)
    {
        sizeSm += (size - sizeSm) * sm;
        mixSm += (mix - mixSm) * msm;
        const float sizeScale = 0.45f + 1.05f * sizeSm;

        // ---- input: mono-ish sum, low cut, pre-delay, diffusion ----
        float x = 0.5f * (L[i] + R[i]);
        pre.push (x);
        x = pre.read (preD);
        for (int k = 0; k < 4; ++k)
        {
            const float g = 0.62f;
            const float dly = diff[k].readInt ((int) diffLen[k]);
            const float v = x + g * dly;
            diff[k].push (v);
            x = dly - g * v;
        }

        // ---- FDN ----
        float y[N];
        float sum = 0.0f;
        for (int k = 0; k < N; ++k)
        {
            lfoPh[k] += lfoInc[k]; if (lfoPh[k] >= 1.0f) lfoPh[k] -= 1.0f;
            const float len = baseLen[k] * sizeScale + (k < 6 ? modDepth * sin2pi (lfoPh[k]) : 0.0f);
            float v = line[k].read (len);
            damp[k] += (v - damp[k]) * dampA;
            v = damp[k] * gk[k];
            y[k] = v;
            sum += v;
        }
        const float hh = sum * (2.0f / (float) N);           // Householder reflection
        for (int k = 0; k < N; ++k)
        {
            const float inj = (k & 1) ? -x : x;
            line[k].push (y[k] - hh + inj * 0.42f);
        }

        float wl = (y[0] - y[2] + y[4] - y[6]) * 0.62f + (y[1] * 0.3f);
        float wr = (y[1] - y[3] + y[5] - y[7]) * 0.62f + (y[0] * 0.3f);
        hpL += (wl - hpL) * hpA; hpR += (wr - hpR) * hpA;
        wl -= hpL; wr -= hpR;

        // equal-ish power blend: dry stays full until about half way
        const float dry = 1.0f - 0.45f * mixSm * mixSm;
        L[i] = L[i] * dry + wl * mixSm;
        R[i] = R[i] * dry + wr * mixSm;
    }
}
} // namespace rep
