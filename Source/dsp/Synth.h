#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Voice.h"

namespace rep
{
// ============================================================================
//  The voice manager and the CS-80's global modulators:
//   * SUB OSCILLATOR - the one shared LFO (sine, saw, inverted saw, square,
//     sample & hold, noise) routed to VCO / VCF / VCA. Aftertouch can speed
//     it up.
//   * PWM LFOs       - one per channel, shared by that channel's 8 voices.
//   * RING MODULATOR - one for the whole instrument, with its own attack/
//     decay envelope (retriggered by every key) that sweeps the ring speed.
//   * The ribbon and the pitch wheel bend every sounding voice.
//
//  Voice assignment rotates (round robin) so consecutive notes fall on
//  different voices with different tolerances. With portamento on, the
//  most recently used voice is reused first so a played line glides from
//  the previous note (the CS-80's portamento is per-voice).
//  Polyphonic aftertouch, channel aftertouch and MPE are all understood.
// ============================================================================
class Synth
{
public:
    void prepare (double hostRate)
    {
        fsOS = (float) (hostRate * 2.0);
        dt = (float) kCtl / fsOS;
        for (int i = 0; i < kMaxVoices; ++i) voices[i].prepare (fsOS, i);
        subPhase = 0.0f; subHold = 0.0f; subSq = 0.0f;
        pwmPhase[0] = 0.13f; pwmPhase[1] = 0.61f;
        ringPhase = 0.0f;
        sustainPedal = false;
        rng.seed (0xBADA55u);
        tickPos = kCtl;
        smRibbon = smWheel = smPress = smBend = 0.0f;
    }

    void setPatch (const Patch& p)
    {
        for (int i = p.numVoices; i < kMaxVoices; ++i)
            if (voices[i].active && voices[i].gate) voices[i].release();
        patch = p;
        static const float panPattern[kMaxVoices] = { -0.7f, 0.7f, -0.35f, 0.35f, -0.9f, 0.9f, -0.15f, 0.15f,
                                                      -0.55f, 0.55f, -0.8f, 0.8f, -0.25f, 0.25f, -0.45f, 0.45f };
        for (int i = 0; i < kMaxVoices; ++i) voices[i].basePan = panPattern[i] * p.spread * 0.55f;
    }

    void setRibbon (float r) noexcept { perf.ribbon = r; }

    void handleMidi (const juce::MidiMessage& m)
    {
        const int ch = m.getChannel();
        const bool mpeMember = patch.mpe && ch != 1;

        if (m.isNoteOn())              noteOn (m.getNoteNumber(), m.getFloatVelocity(), ch);
        else if (m.isNoteOff())        noteOff (m.getNoteNumber(), ch);
        else if (m.isPitchWheel())
        {
            const float b = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
            if (mpeMember) { for (auto& v : voices) if (v.active && v.midiChannel == ch) v.mpeBend = b; }
            else perf.bend = b;
        }
        else if (m.isChannelPressure())
        {
            const float v = (float) m.getChannelPressureValue() / 127.0f;
            if (mpeMember) { for (auto& vc : voices) if (vc.active && vc.midiChannel == ch) vc.mpePressure = v; }
            else perf.pressure = v;
        }
        else if (m.isAftertouch())     // polyphonic key pressure
        {
            const float v = (float) m.getAfterTouchValue() / 127.0f;
            for (auto& vc : voices)
                if (vc.active && vc.gate && vc.note == m.getNoteNumber()) vc.polyPressure = v;
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) allNotesOff (m.isAllSoundOff());
        else if (m.isController())
        {
            const float v = (float) m.getControllerValue() / 127.0f;
            switch (m.getControllerNumber())
            {
                case 1:   if (! mpeMember) perf.wheel = v; break;
                case 64:  setSustain (v >= 0.5f); break;
                case 121: perf.bend = 0.0f; perf.wheel = 0.0f; perf.pressure = 0.0f; break;
                default:  break;
            }
        }
    }

    // Renders numOS oversampled samples (adds into L/R)
    void render (float* L, float* R, int numOS) noexcept
    {
        int pos = 0;
        while (pos < numOS)
        {
            if (tickPos >= kCtl) { tickPos = 0; controlTick(); }
            const int n = std::min (kCtl - tickPos, numOS - pos);
            for (int i = 0; i < patch.numVoices; ++i)
                if (voices[i].active) voices[i].renderTick (L + pos, R + pos, n, patch, glob);
            // released voices beyond the current polyphony still finish their tails
            for (int i = patch.numVoices; i < kMaxVoices; ++i)
                if (voices[i].active) voices[i].renderTick (L + pos, R + pos, n, patch, glob);

            // ring modulator (audio rate, at 2x so its sidebands stay clean)
            if (patch.ringMod > 0.0005f)
            {
                const float m = patch.ringMod;
                for (int i = 0; i < n; ++i)
                {
                    const float s = sin2pi (ringPhase);
                    ringPhase += ringInc;
                    if (ringPhase >= 1.0f) ringPhase -= 1.0f;
                    const float gmod = 1.0f - m + m * s;
                    L[pos + i] *= gmod;
                    R[pos + i] *= gmod;
                }
            }
            pos += n;
            tickPos += n;
        }
    }

    void allNotesOff (bool hard)
    {
        for (auto& v : voices)
        {
            if (hard) v.kill();
            else if (v.active) v.release();
        }
    }

    float voiceMeter (int i) const { return voices[i].active ? voices[i].meter : 0.0f; }
    float ringEnvValue() const { return ringEnv.value; }
    float subValue() const { return glob.sub; }

private:
    Voice voices[kMaxVoices];
    Patch patch;
    Perf perf;
    Globals glob;
    Rng rng;
    float fsOS = 96000.0f, dt = 4.0f / 96000.0f;
    int tickPos = kCtl;

    float subPhase = 0.0f, subHold = 0.0f, subSq = 0.0f;
    float pwmPhase[2] {};
    float ringPhase = 0.0f, ringInc = 0.0f;
    RingEnv ringEnv;
    float smRibbon = 0.0f, smWheel = 0.0f, smPress = 0.0f, smBend = 0.0f;

    uint32_t ageCounter = 0;
    int rr = 0, lastVoice = -1;
    bool sustainPedal = false;

    int numVoices() const { return juce::jlimit (1, kMaxVoices, patch.numVoices); }

    void controlTick() noexcept
    {
        const float sc = onePoleCoef (0.006f, dt);
        smRibbon += (perf.ribbon - smRibbon) * sc;
        smBend   += (perf.bend - smBend) * sc;
        smWheel  += (perf.wheel - smWheel) * sc;
        smPress  += (perf.pressure - smPress) * onePoleCoef (0.012f, dt);
        glob.ribbonSemis  = smRibbon * patch.ribbonRange + smBend * patch.bendRange;
        glob.wheel        = smWheel;
        glob.chanPressure = smPress;

        // highest pressure on any held key drives the (global) sub osc speed
        float maxPress = smPress;
        for (auto& v : voices) if (v.active) maxPress = std::max (maxPress, v.pressure);

        // ---- sub oscillator ----
        const float hz = patch.subHz * std::exp2 (patch.aftSpeed * maxPress * 2.5f);
        subPhase += hz * dt;
        const bool wrapped = subPhase >= 1.0f;
        if (wrapped) { subPhase -= std::floor (subPhase); subHold = rng.bipolar(); }
        const float p = subPhase;
        float s;
        switch (patch.subFunc)
        {
            case SF_SAW:    s = 1.0f - 2.0f * p; break;            // falls: the CS "sawtooth"
            case SF_INVSAW: s = 2.0f * p - 1.0f; break;
            case SF_SQUARE:
            {
                const float target = p < 0.5f ? 1.0f : -1.0f;      // slightly slewed, no clicks
                subSq += (target - subSq) * onePoleCoef (0.0015f, dt);
                s = subSq;
                break;
            }
            case SF_SH:     s = subHold; break;
            case SF_NOISE:  s = rng.bipolar(); break;
            case SF_SINE:
            default:        s = sin2pi (p); break;
        }
        glob.sub = s;

        // ---- PWM LFOs (triangle) ----
        for (int c = 0; c < 2; ++c)
        {
            pwmPhase[c] += patch.ch[c].pwmHz * dt;
            if (pwmPhase[c] >= 1.0f) pwmPhase[c] -= std::floor (pwmPhase[c]);
            const float q = pwmPhase[c];
            glob.pwm[c] = q < 0.5f ? 4.0f * q - 1.0f : 3.0f - 4.0f * q;
        }

        // ---- ring modulator speed, swept by its AD envelope ----
        const float re = ringEnv.process (patch.ringA, patch.ringD, dt);
        const float rhz = patch.ringHz * std::exp2 (patch.ringDepth * re * 4.0f);
        ringInc = clampf (rhz / fsOS, 0.0f, 0.4f);
    }

    int allocate (int note)
    {
        const int n = numVoices();
        for (int i = 0; i < n; ++i)                                  // same key again
            if (voices[i].active && voices[i].note == note) return i;
        if (patch.glideTime > 0.0005f && lastVoice >= 0 && lastVoice < n && ! voices[lastVoice].gate)
            return lastVoice;                                        // portamento: glide from the last note
        for (int k = 0; k < n; ++k)                                  // next free voice (rotating)
        {
            const int i = (rr + k) % n;
            if (! voices[i].active) return i;
        }
        int v = -1; uint32_t best = UINT32_MAX;
        for (int i = 0; i < n; ++i)                                  // oldest released
            if (! voices[i].gate && voices[i].age < best) { best = voices[i].age; v = i; }
        if (v >= 0) return v;
        for (int i = 0; i < n; ++i)                                  // oldest
            if (voices[i].age < best) { best = voices[i].age; v = i; }
        return v < 0 ? 0 : v;
    }

    void noteOn (int note, float vel, int ch)
    {
        if (vel <= 0.0f) { noteOff (note, ch); return; }
        const int v = allocate (note);
        rr = (v + 1) % numVoices();
        voices[v].age = ++ageCounter;
        voices[v].start (note, vel, ch, false, patch);
        lastVoice = v;
        ringEnv.trigger();
    }

    void noteOff (int note, int ch)
    {
        for (auto& v : voices)
            if (v.active && v.gate && v.note == note && (! patch.mpe || v.midiChannel == ch || ch == 1))
            {
                if (sustainPedal) v.sustained = true;
                else v.release();
            }
    }

    void setSustain (bool on)
    {
        sustainPedal = on;
        if (! on)
            for (auto& v : voices)
                if (v.active && v.sustained) v.release();
    }
};
} // namespace rep
