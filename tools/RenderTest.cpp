// Offline test harness for the Replicant engine (not part of the plugin).
// Renders every factory preset plus a set of expression and torture tests
// to WAV files and reports peak level, NaN/Inf and CPU load.
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"
#include "Presets.h"
#include <chrono>

using namespace rep;

static constexpr double SR = 48000.0;
static constexpr int BS = 512;

struct Note { double t; double len; int note; float vel; };
struct Ev   { double t; juce::MidiMessage m; };

static void setP (ReplicantProcessor& p, const juce::String& id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    if (prm == nullptr) { std::cerr << "missing param " << id << "\n"; std::exit (2); }
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

struct Result { float peak = 0; bool bad = false; double seconds = 0; float rmsTail = 0; };

static Result render (ReplicantProcessor& proc, const std::vector<Note>& notes, double totalSec,
                      const juce::File& wav, std::function<void (double)> automation = {},
                      const std::vector<Ev>& extra = {})
{
    proc.prepareToPlay (SR, BS);
    const int total = (int) (totalSec * SR);
    juce::AudioBuffer<float> out (2, total);
    juce::AudioBuffer<float> buf (2, BS);
    Result res;

    auto t0 = std::chrono::steady_clock::now();
    for (int pos = 0; pos < total; pos += BS)
    {
        const int n = std::min (BS, total - pos);
        juce::AudioBuffer<float> b (buf.getArrayOfWritePointers(), 2, n);
        juce::MidiBuffer midi;
        for (auto& nt : notes)
        {
            const int on = (int) (nt.t * SR), off = (int) ((nt.t + nt.len) * SR);
            if (on >= pos && on < pos + n)  midi.addEvent (juce::MidiMessage::noteOn (1, nt.note, nt.vel), on - pos);
            if (off >= pos && off < pos + n) midi.addEvent (juce::MidiMessage::noteOff (1, nt.note), off - pos);
        }
        for (auto& e : extra)
        {
            const int at = (int) (e.t * SR);
            if (at >= pos && at < pos + n) midi.addEvent (e.m, at - pos);
        }
        if (automation) automation ((double) pos / SR);
        proc.processBlock (b, midi);
        for (int c = 0; c < 2; ++c)
        {
            out.copyFrom (c, pos, b, c, 0, n);
            for (int i = 0; i < n; ++i)
            {
                const float v = b.getSample (c, i);
                if (! std::isfinite (v)) res.bad = true;
                res.peak = std::max (res.peak, std::abs (v));
            }
        }
    }
    res.seconds = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    res.rmsTail = out.getRMSLevel (0, total - (int) (0.05 * SR), (int) (0.05 * SR));

    wav.deleteFile();
    juce::WavAudioFormat fmt;
    if (auto os = std::unique_ptr<juce::OutputStream> (wav.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (fmt.createWriterFor (os.get(), SR, 2, 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (out, 0, total);
        }
    return res;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? argv[1] : "render");
    outDir.createDirectory();
    bool allOk = true;

    auto report = [&] (const juce::String& name, const Result& r, double audioSec, bool expectSound = true)
    {
        const bool ok = ! r.bad && r.peak < 1.0f && (! expectSound || r.peak > 0.005f);
        allOk = allOk && ok;
        std::cout << (ok ? "  ok   " : "  FAIL ") << name.paddedRight (' ', 30)
                  << " peak " << juce::String (juce::Decibels::gainToDecibels (r.peak), 1).paddedLeft (' ', 6) << " dB"
                  << "   cpu x" << juce::String (audioSec / r.seconds, 0).paddedLeft (' ', 4) << " realtime"
                  << (r.bad ? "   NaN/Inf!" : "") << "\n";
    };

    // ---- 1. every factory preset: chord + melody + bass ----
    std::cout << "Factory presets\n";
    const std::vector<Note> phrase = {
        { 0.0, 1.8, 48, 0.8f }, { 0.0, 1.8, 55, 0.7f }, { 0.0, 1.8, 60, 0.75f }, { 0.0, 1.8, 64, 0.7f },
        { 2.2, 0.3, 60, 0.9f }, { 2.5, 0.3, 63, 0.6f }, { 2.8, 0.3, 67, 0.8f }, { 3.1, 0.7, 72, 1.0f },
        { 4.0, 0.25, 36, 1.0f }, { 4.25, 0.25, 48, 0.5f }, { 4.5, 0.8, 43, 0.9f } };
    for (int i = 0; i < numFactoryPresets(); ++i)
    {
        ReplicantProcessor proc;
        applyFactoryPreset (i, proc.apvts);
        const auto name = factoryPresetName (i);
        auto r = render (proc, phrase, 8.0, outDir.getChildFile ("preset_" + juce::String (i).paddedLeft ('0', 2) + "_"
                                                       + name.replaceCharacter (' ', '_').replaceCharacter ('/', '-') + ".wav"));
        report (name, r, 8.0);
    }

    // ---- 2. expression: poly aftertouch on one note of a chord + ribbon glide ----
    std::cout << "Expression\n";
    {
        ReplicantProcessor proc;
        applyFactoryPreset (22, proc.apvts);
        std::vector<Ev> ev;
        for (int k = 0; k <= 40; ++k)   // press the top note only, then let go
        {
            const double t = 0.8 + k * 0.05;
            const int v = k <= 20 ? k * 6 : (40 - k) * 6;
            ev.push_back ({ t, juce::MidiMessage::aftertouchChange (1, 67, juce::jlimit (0, 127, v)) });
        }
        auto r = render (proc, { { 0.0, 3.5, 55, 0.8f }, { 0.0, 3.5, 60, 0.8f }, { 0.0, 3.5, 67, 0.8f } }, 5.0,
                         outDir.getChildFile ("expr_poly_aftertouch.wav"),
                         [&] (double t) { setP (proc, "ribbon", t > 2.6 && t < 3.3 ? (float) juce::jmin (1.0, (t - 2.6) * 2.0) : 0.0f); }, ev);
        report ("poly AT + ribbon", r, 5.0);
    }
    {
        ReplicantProcessor proc;
        applyFactoryPreset (22, proc.apvts);
        setP (proc, "mpe", 1.0f);
        std::vector<Ev> ev;
        ev.push_back ({ 0.0, juce::MidiMessage::noteOn (2, 60, 0.8f) });
        ev.push_back ({ 0.0, juce::MidiMessage::noteOn (3, 64, 0.8f) });
        for (int k = 0; k < 30; ++k)
            ev.push_back ({ 0.5 + k * 0.03, juce::MidiMessage::pitchWheel (3, 8192 + k * 30) });   // 64 bends alone
        ev.push_back ({ 2.5, juce::MidiMessage::noteOff (2, 60) });
        ev.push_back ({ 2.5, juce::MidiMessage::noteOff (3, 64) });
        auto r = render (proc, {}, 4.0, outDir.getChildFile ("expr_mpe.wav"), {}, ev);
        report ("MPE per-note bend", r, 4.0);
    }
    {
        ReplicantProcessor proc;
        applyFactoryPreset (4, proc.apvts);
        setP (proc, "glide_time", 0.4f);
        setP (proc, "glide_mode", 1.0f);
        auto r = render (proc, { { 0.0, 0.6, 48, 0.8f }, { 0.6, 0.6, 60, 0.8f }, { 1.2, 0.6, 55, 0.8f }, { 1.8, 1.0, 67, 0.8f } }, 3.5,
                         outDir.getChildFile ("expr_glissando.wav"));
        report ("glissando line", r, 3.5);
    }

    // ---- 3. torture tests ----
    std::cout << "Torture tests\n";
    {
        ReplicantProcessor proc;
        for (int c = 1; c <= 2; ++c)
        {
            setP (proc, id::ch (c, "lpf_res"), 1.0f); setP (proc, id::ch (c, "hpf_res"), 1.0f);
            setP (proc, id::ch (c, "square"), 1.0f); setP (proc, id::ch (c, "noise"), 1.0f);
            setP (proc, id::ch (c, "al"), 1.0f); setP (proc, id::ch (c, "il"), 1.0f);
            setP (proc, id::ch (c, "fa"), 0.4f); setP (proc, id::ch (c, "fd"), 0.4f);
            setP (proc, id::ch (c, "feet"), 5.0f); setP (proc, id::ch (c, "sine"), 1.0f);
            setP (proc, id::ch (c, "level"), 1.0f); setP (proc, id::ch (c, "tib"), 1.0f);
        }
        setP (proc, "resonance", 1.0f); setP (proc, "brilliance", 1.0f);
        setP (proc, "ring_mod", 1.0f); setP (proc, "ring_speed", 1500.0f); setP (proc, "ring_depth", 1.0f);
        setP (proc, "sub_func", 5.0f); setP (proc, "sub_vco", 1.0f); setP (proc, "sub_vcf", 1.0f); setP (proc, "sub_vca", 1.0f);
        setP (proc, "voices", 16.0f);
        std::vector<Note> cluster;
        for (int k = 0; k < 16; ++k) cluster.push_back ({ 0.0, 2.5, 24 + k * 6, 1.0f });
        auto r = render (proc, cluster, 3.0, outDir.getChildFile ("torture_everything.wav"));
        report ("everything at max, 16 voices", r, 3.0);
    }
    {
        ReplicantProcessor proc;
        applyFactoryPreset (41, proc.apvts);
        setP (proc, "dly_on", 1.0f); setP (proc, "dly_fb", 0.95f); setP (proc, "dly_mix", 1.0f);
        setP (proc, "hall_mix", 1.0f); setP (proc, "hall_decay", 20.0f); setP (proc, "trem_mode", 2.0f); setP (proc, "trem_depth", 1.0f);
        auto r = render (proc, { { 0.0, 0.5, 60, 1.0f }, { 0.0, 0.5, 72, 1.0f } }, 12.0, outDir.getChildFile ("torture_fx_max.wav"),
                         [&] (double t) { setP (proc, "dly_time", (float) (0.05 + 0.9 * std::fmod (t, 1.0))); });
        report ("FX feedback max + time sweep", r, 12.0);
    }
    {
        ReplicantProcessor proc;
        for (int c = 1; c <= 2; ++c) { setP (proc, id::ch (c, "lpf_res"), 1.0f); setP (proc, id::ch (c, "a"), 0.001f); }
        auto r = render (proc, { { 0.0, 3.0, 40, 1.0f } }, 3.2, outDir.getChildFile ("filter_sweep_res.wav"),
                         [&] (double t) { setP (proc, "c1_lpf", (float) (30.0 * std::pow (600.0, juce::jmin (1.0, t / 3.0))));
                                          setP (proc, "c2_lpf", (float) (30.0 * std::pow (600.0, juce::jmin (1.0, t / 3.0)))); });
        report ("LPF sweep, max resonance", r, 3.2);
    }

    // ---- 4. CPU ----
    std::cout << "CPU load\n";
    for (int nv : { 8, 16 })
    {
        ReplicantProcessor proc;
        applyFactoryPreset (0, proc.apvts);
        setP (proc, "voices", (float) nv);
        setP (proc, "dly_on", 1.0f);
        std::vector<Note> chord;
        for (int k = 0; k < nv; ++k) chord.push_back ({ 0.0, 9.5, 36 + k * 3, 0.8f });
        auto r = render (proc, chord, 10.0, outDir.getChildFile ("cpu_" + juce::String (nv) + "voices.wav"));
        report (juce::String (nv) + " voices + all FX", r, 10.0);
    }

    // ---- 5. silence after release (no stuck voices, tails die) ----
    {
        ReplicantProcessor proc;
        applyFactoryPreset (4, proc.apvts);
        setP (proc, "hall_on", 0.0f);
        auto r = render (proc, { { 0.0, 0.5, 60, 1.0f } }, 3.0, outDir.getChildFile ("release_silence.wav"));
        const bool silent = r.rmsTail < 1.0e-5f;
        allOk = allOk && silent;
        std::cout << (silent ? "  ok   " : "  FAIL ") << "voice ends after release      tail rms "
                  << juce::String (r.rmsTail, 8) << "\n";
    }

    // ---- 6. project state round trip incl. the 4 memories ----
    {
        ReplicantProcessor a;
        applyFactoryPreset (4, a.apvts);
        setP (a, "c1_lpf", 1234.0f);
        a.storeMemory (2);
        applyFactoryPreset (10, a.apvts);
        juce::MemoryBlock mb;
        a.getStateInformation (mb);

        ReplicantProcessor b;
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        const float lpfNow = b.apvts.getRawParameterValue ("c1_lpf")->load();
        const bool memOk = b.memoryUsed (2) && ! b.memoryUsed (0) && b.recallMemory (2);
        const float lpfMem = b.apvts.getRawParameterValue ("c1_lpf")->load();
        const bool ok = memOk && std::abs (lpfNow - 850.0f) < 1.0f && std::abs (lpfMem - 1234.0f) < 1.0f;
        allOk = allOk && ok;
        std::cout << (ok ? "  ok   " : "  FAIL ") << "state + memories round trip   lpf now " << lpfNow << ", memory 3 " << lpfMem << "\n";
    }

    std::cout << (allOk ? "\nALL TESTS PASSED\n" : "\nSOME TESTS FAILED\n");
    return allOk ? 0 : 1;
}
