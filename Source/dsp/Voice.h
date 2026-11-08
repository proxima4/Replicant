#pragma once
#include "Patch.h"
#include "Vco.h"
#include "CsFilter.h"
#include "Envelopes.h"

namespace rep
{
// ============================================================================
//  One CS-80 voice = two complete, parallel synthesis chains (Channel I and
//  Channel II), each:   VCO/WSC + noise -> HPF -> LPF -> VCF LEVEL --+
//                       WSC sine ----------------------- SINE ------+-> VCA
//  with its own IL/AL filter envelope and ADSR, its own touch response and
//  its own PWM LFO. Every voice has slightly different "component values"
//  (pitch, cutoff, envelope speed, level) scaled by the AGE control, plus
//  slow independent drift - that is where the CS-80's life comes from.
//
//  Audio runs at 2x the host rate; the control section (envelopes, pitch,
//  cutoffs, touch) is computed every kCtl oversampled samples and the VCA
//  gain is ramped across the tick, so nothing zips.
// ============================================================================
class Voice
{
public:
    static constexpr float kVoiceGain = 0.28f;   // headroom: 8 voices x 2 channels

    int   note = 60, midiChannel = 1;
    float velocity = 0.8f;
    bool  gate = false, sustained = false, active = false;
    uint32_t age = 0;
    float basePan = 0.0f;
    float meter = 0.0f;

    // per-note expression (poly aftertouch, MPE)
    float polyPressure = 0.0f, mpePressure = 0.0f, mpeBend = 0.0f;
    float pressure = 0.0f;           // smoothed, as used by the voice

    void prepare (double osRate, int index)
    {
        fsOS = (float) osRate;
        dt = (float) kCtl / fsOS;
        Rng r; r.seed (0x5EEDu + 48271u * (uint32_t) (index + 1));
        for (int c = 0; c < 2; ++c)
        {
            tolCents[c] = r.bipolar();
            tolCut[c]   = r.bipolar();
            tolTime[c]  = r.bipolar();
            tolLevel[c] = r.bipolar();
            tolPw[c]    = r.bipolar();
            vco[c].reset (r.unipolar());
            filt[c].reset();
            noise[c].seed (r.next() | 1u);
            driftVal[c] = driftTarget[c] = 0.0f;
        }
        rng.seed (0xD1F7u + 7919u * (uint32_t) (index + 1));
        driftCounter = index * 97;
        kill();
    }

    void kill() noexcept
    {
        active = gate = sustained = false;
        for (int c = 0; c < 2; ++c) { aenv[c].reset(); fenv[c].reset (0.0f); vca[c] = 0.0f; filt[c].reset(); }
        meter = 0.0f;
    }

    void start (int newNote, float vel, int channel, bool legato, const Patch& p) noexcept
    {
        const bool wasActive = active;
        note = newNote;
        velocity = vel;
        midiChannel = channel;
        gate = true;
        sustained = false;
        active = true;
        mpeBend = 0.0f;
        mpePressure = 0.0f;
        polyPressure = 0.0f;

        if (! hasPlayed || p.glideTime <= 0.0005f) glidePitch = (float) newNote;
        hasPlayed = true;

        bendOffset = -2.0f * p.initBend * vel;
        for (int c = 0; c < 2; ++c)
        {
            if (! legato || ! wasActive)
            {
                fenv[c].trigger (p.ch[c].il, wasActive);
                aenv[c].trigger();
            }
        }
        if (! wasActive) pressure = 0.0f;
    }

    void release() noexcept
    {
        gate = false;
        sustained = false;
        for (int c = 0; c < 2; ++c) { fenv[c].releaseNow(); aenv[c].releaseNow(); }
    }

    // Render one control tick (n <= kCtl oversampled samples)
    void renderTick (float* L, float* R, int n, const Patch& p, const Globals& g) noexcept
    {
        control (p, g);

        float peak = 0.0f;
        const float inv = 1.0f / (float) n;
        for (int c = 0; c < 2; ++c)
        {
            const float v0 = vca[c], v1 = vcaTarget[c];
            if (v0 < 1.0e-6f && v1 < 1.0e-6f) { vca[c] = v1; continue; }
            const float step = (v1 - v0) * inv;
            float v = v0;
            const auto& cp = p.ch[c];
            const float gl = panL[c], gr = panR[c];
            for (int i = 0; i < n; ++i)
            {
                v += step;
                const auto o = vco[c].process (inc[c], pw[c], gSaw[c], gSq[c]);
                float x = o.shaped;
                if (noiseLvl[c] > 0.0f) x += noiseLvl[c] * noise[c].bipolar();
                const float y = filt[c].process (x * 0.6f, gH[c], kH[c], gL[c], kL[c]);
                const float out = (y * cp.vcfLevel * 1.35f + o.sine * cp.sine * 0.7f) * v * kVoiceGain;
                L[i] += out * gl;
                R[i] += out * gr;
                peak = std::max (peak, std::abs (out));
            }
            vca[c] = v1;
        }
        meter = std::max (peak, meter * 0.995f);

        if (! gate && aenv[0].isIdle() && aenv[1].isIdle())
        {
            active = false;
            meter = 0.0f;
        }
    }

private:
    float fsOS = 96000.0f, dt = 4.0f / 96000.0f;
    Vco vco[2];
    CsFilter filt[2];
    FilterEnv fenv[2];
    Adsr aenv[2];
    Rng noise[2], rng;

    // analog tolerances (-1..1, scaled by AGE) and drift
    float tolCents[2] {}, tolCut[2] {}, tolTime[2] {}, tolLevel[2] {}, tolPw[2] {};
    float driftVal[2] {}, driftTarget[2] {};
    int driftCounter = 0;

    float glidePitch = 60.0f, bendOffset = 0.0f;
    bool hasPlayed = false;

    // control-tick outputs used by the audio loop
    float inc[2] {}, pw[2] { 0.5f, 0.5f }, gSaw[2] {}, gSq[2] {}, noiseLvl[2] {};
    float gH[2] {}, kH[2] { 1.0f, 1.0f }, gL[2] {}, kL[2] { 1.0f, 1.0f };
    float vca[2] {}, vcaTarget[2] {};
    float panL[2] { 0.707f, 0.707f }, panR[2] { 0.707f, 0.707f };

    float tanG (float hz) const noexcept
    {
        hz = clampf (hz, 8.0f, 0.44f * fsOS);
        return std::tan (3.14159265f * hz / fsOS);
    }

    void control (const Patch& p, const Globals& g) noexcept
    {
        const float ageAmt = p.age;

        // ---- expression ---------------------------------------------------
        float press = std::max (polyPressure, p.mpe && midiChannel != 1 ? mpePressure : g.chanPressure);
        if (p.wheelDest == WD_PRESSURE) press = std::max (press, g.wheel);
        pressure += (press - pressure) * onePoleCoef (0.012f, dt);
        const float pr = pressure;
        const float vel = velocity;

        // ---- glide (polyphonic portamento / glissando) ---------------------
        if (p.glideTime <= 0.0005f) glidePitch = (float) note;
        else glidePitch += ((float) note - glidePitch) * onePoleCoef (p.glideTime * 0.42f, dt);
        const float keyPitch = p.glideMode == GM_GLISSANDO ? std::round (glidePitch) : glidePitch;

        // velocity-driven initial pitch bend (slides up into the note)
        bendOffset *= 1.0f - onePoleCoef (p.bendTime * 0.35f, dt);

        // ---- drift: slow independent wander per channel -------------------
        if (--driftCounter <= 0)
        {
            driftCounter = (int) (0.11f / dt);
            for (auto& t : driftTarget) t = rng.bipolar();
        }
        const float dc = onePoleCoef (1.6f, dt);
        for (int c = 0; c < 2; ++c) driftVal[c] += (driftTarget[c] - driftVal[c]) * dc;

        // ---- vibrato depth (sub oscillator -> VCO), poly aftertouch adds ---
        float vibD = p.subVco + p.aftVib * pr + (p.wheelDest == WD_VIBRATO ? g.wheel * 0.55f : 0.0f);
        vibD = clamp01 (vibD);
        const float vib = g.sub * vibD * vibD * 3.0f;

        const float common = keyPitch + p.coarse + p.fineCents * 0.01f + bendOffset + g.ribbonSemis
                           + (p.mpe && midiChannel != 1 ? mpeBend * 48.0f : 0.0f)
                           + p.aftPitch * pr * 2.0f + vib;

        // keyboard control (scaling left / right of the keyboard centre)
        const float k = clampf (((float) note - 66.0f) / 30.0f, -1.2f, 1.2f);
        const float kbBrill = k < 0.0f ? -k * p.kbBL * 3.0f : k * p.kbBH * 3.0f;
        const float kbDb    = k < 0.0f ? -k * p.kbLL * 15.0f : k * p.kbLH * 15.0f;
        const float kbGain  = dbToGain (kbDb);

        const float trem = 1.0f - p.subVca * (0.5f - 0.5f * g.sub);
        const float wheelBrill = p.wheelDest == WD_BRILLIANCE ? g.wheel * 3.0f : 0.0f;
        const float mixW[2] = { std::min (1.0f, 2.0f * (1.0f - p.mix)), std::min (1.0f, 2.0f * p.mix) };

        for (int c = 0; c < 2; ++c)
        {
            const auto& cp = p.ch[c];
            const float tScale = 1.0f + 0.07f * ageAmt * tolTime[c];

            // ---- pitch ----
            const float semis = common + feetSemis (cp.feet) + (c == 1 ? p.detuneCents * 0.01f : 0.0f)
                              + ageAmt * (tolCents[c] * 0.04f + driftVal[c] * 0.06f);
            const float hz = 440.0f * std::exp2 ((semis - 69.0f) * (1.0f / 12.0f));
            inc[c] = clampf (hz / fsOS, 0.0f, 0.45f);

            // ---- WSC ----
            pw[c] = clampf (0.5f + 0.4f * cp.pw + cp.pwmDepth * 0.42f * g.pwm[c] + ageAmt * tolPw[c] * 0.012f, 0.04f, 0.96f);
            const int nw = (int) cp.saw + (int) cp.square;
            const float wg = nw == 2 ? 0.72f : 1.0f;
            gSaw[c] = cp.saw ? wg : 0.0f;
            gSq[c]  = cp.square ? wg : 0.0f;
            noiseLvl[c] = cp.noise * cp.noise * 1.3f;

            // ---- filter envelope + brilliance (moves HPF and LPF together) ----
            const float fe = fenv[c].process (cp.il, cp.al, cp.fa * tScale, cp.fd * tScale, cp.fr * tScale, dt);
            const float oct = p.brilliance * 3.0f + fe * 5.0f
                            + cp.tib * vel * 3.5f + cp.tab * pr * 3.5f
                            + kbBrill + p.kbTrack * ((float) note - 60.0f) * (1.0f / 12.0f)
                            + p.subVcf * g.sub * 2.0f + wheelBrill
                            + ageAmt * (tolCut[c] * 0.12f + driftVal[c] * 0.04f);
            const float ratio = std::exp2 (oct);
            gH[c] = tanG (cp.hpfHz * ratio);
            gL[c] = tanG (cp.lpfHz * ratio);
            kH[c] = CsFilter::resToK (cp.hpfRes + p.resonance * 0.5f);
            kL[c] = CsFilter::resToK (cp.lpfRes + p.resonance * 0.5f);

            // ---- VCA ----
            const float env = aenv[c].process (cp.a * tScale, cp.d * tScale, cp.s, cp.r * tScale, dt);
            const float gVel = 1.0f - cp.til * (1.0f - vel) * 0.92f;
            const float gAft = (1.0f - 0.55f * cp.tal) + 1.1f * cp.tal * pr;
            const float lvl = cp.level * (1.0f + 0.06f * ageAmt * tolLevel[c]);
            vcaTarget[c] = env * lvl * gVel * gAft * kbGain * trem * mixW[c];

            // ---- stereo placement: channel I left of the voice, II right ----
            const float pan = clampf (basePan + (c == 0 ? -0.55f : 0.55f) * p.spread, -1.0f, 1.0f);
            panL[c] = std::sqrt (0.5f * (1.0f - pan));
            panR[c] = std::sqrt (0.5f * (1.0f + pan));
        }
    }
};
} // namespace rep
