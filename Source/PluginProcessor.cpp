#include "PluginProcessor.h"
#if ! REP_HEADLESS
 #include "PluginEditor.h"
#endif
#include "Presets.h"

using namespace rep;

ReplicantProcessor::ReplicantProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "REPLICANT", createLayout())
{
    cacheParams();
    applyFactoryPreset (0, apvts);
    patchName = factoryPresetName (0);
}

void ReplicantProcessor::cacheParams()
{
    auto get = [this] (const juce::String& pid)
    {
        Raw r; r.p = apvts.getRawParameterValue (pid);
        jassert (r.p != nullptr);
        return r;
    };
    const auto& suf = channelParamSuffixes();
    for (int c = 0; c < 2; ++c)
        for (int k = 0; k < suf.size(); ++k)
            rCh[c][k] = get (id::ch (c + 1, suf[k].toRawUTF8()));

    rSubFunc = get ("sub_func"); rSubSpeed = get ("sub_speed"); rSubVco = get ("sub_vco"); rSubVcf = get ("sub_vcf"); rSubVca = get ("sub_vca");
    rRingMod = get ("ring_mod"); rRingSpeed = get ("ring_speed"); rRingDepth = get ("ring_depth"); rRingA = get ("ring_a"); rRingD = get ("ring_d");
    rInitBend = get ("init_bend"); rBendTime = get ("bend_time"); rAftPitch = get ("aft_pitch"); rAftVib = get ("aft_vib"); rAftSpeed = get ("aft_speed");
    rBrill = get ("brilliance"); rRes = get ("resonance");
    rKbBL = get ("kb_bl"); rKbBH = get ("kb_bh"); rKbLL = get ("kb_ll"); rKbLH = get ("kb_lh"); rKbTrack = get ("kb_track");
    rDetune = get ("detune"); rCoarse = get ("coarse"); rFine = get ("fine"); rMix = get ("mix");
    rGlideMode = get ("glide_mode"); rGlideTime = get ("glide_time");
    rTremMode = get ("trem_mode"); rTremSpeed = get ("trem_speed"); rTremDepth = get ("trem_depth");
    rDlyOn = get ("dly_on"); rDlySync = get ("dly_sync"); rDlyTime = get ("dly_time"); rDlyFb = get ("dly_fb"); rDlyMix = get ("dly_mix"); rDlyTone = get ("dly_tone");
    rHallOn = get ("hall_on"); rHallSize = get ("hall_size"); rHallDecay = get ("hall_decay"); rHallTone = get ("hall_tone"); rHallPre = get ("hall_pre"); rHallMix = get ("hall_mix");
    rVoices = get ("voices"); rAge = get ("age"); rSpread = get ("spread"); rRibbon = get ("ribbon");
    rRibbonRange = get ("ribbon_range"); rBendRange = get ("bend_range"); rWheelDest = get ("wheel_dest"); rMpe = get ("mpe");
    rMaster = get ("master");
}

void ReplicantProcessor::buildPatch (Patch& p)
{
    auto idx = [] (const Raw& r) { return juce::roundToInt (r()); };
    auto on  = [] (const Raw& r) { return r() >= 0.5f; };

    for (int c = 0; c < 2; ++c)
    {
        auto& C = p.ch[c];
        const Raw* r = rCh[c];
        C.feet = idx (r[0]); C.pwmHz = r[1](); C.pwmDepth = r[2](); C.pw = r[3]();
        C.square = on (r[4]); C.saw = on (r[5]); C.noise = r[6]();
        C.hpfHz = r[7](); C.hpfRes = r[8](); C.lpfHz = r[9](); C.lpfRes = r[10]();
        C.il = r[11](); C.al = r[12](); C.fa = r[13](); C.fd = r[14](); C.fr = r[15]();
        C.vcfLevel = r[16](); C.sine = r[17]();
        C.a = r[18](); C.d = r[19](); C.s = r[20](); C.r = r[21](); C.level = r[22]();
        C.tib = r[23](); C.til = r[24](); C.tab = r[25](); C.tal = r[26]();
    }
    p.subFunc = idx (rSubFunc); p.subHz = rSubSpeed(); p.subVco = rSubVco(); p.subVcf = rSubVcf(); p.subVca = rSubVca();
    p.ringMod = rRingMod(); p.ringHz = rRingSpeed(); p.ringDepth = rRingDepth(); p.ringA = rRingA(); p.ringD = rRingD();
    p.initBend = rInitBend(); p.bendTime = rBendTime(); p.aftPitch = rAftPitch(); p.aftVib = rAftVib(); p.aftSpeed = rAftSpeed();
    p.brilliance = rBrill(); p.resonance = rRes();
    p.kbBL = rKbBL(); p.kbBH = rKbBH(); p.kbLL = rKbLL(); p.kbLH = rKbLH(); p.kbTrack = rKbTrack();
    p.detuneCents = rDetune(); p.coarse = (float) idx (rCoarse); p.fineCents = rFine(); p.mix = rMix();
    p.glideMode = idx (rGlideMode); p.glideTime = rGlideTime();
    p.numVoices = juce::jlimit (1, kMaxVoices, idx (rVoices));
    p.age = rAge(); p.spread = rSpread();
    p.ribbonRange = rRibbonRange(); p.bendRange = rBendRange();
    p.wheelDest = idx (rWheelDest); p.mpe = on (rMpe);
}

void ReplicantProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (64, juce::jmin (samplesPerBlock, 2048));
    oversampling.reset();
    oversampling.initProcessing ((size_t) maxBlock);
    synth.prepare (sampleRate);
    buildPatch (patch);
    synth.setPatch (patch);
    chorus.prepare (sampleRate);
    delay.prepare (sampleRate);
    hall.prepare (sampleRate);
    dcCoef = (float) std::exp (-2.0 * juce::MathConstants<double>::pi * 10.0 / sampleRate);
    dcL = dcR = dcInL = dcInR = 0.0f;
    masterSm.reset (sampleRate, 0.03);
    masterSm.setCurrentAndTargetValue (rMaster());
    keyboardState.reset();
    hostStop.prepare (sampleRate);
    setLatencySamples (juce::roundToInt (oversampling.getLatencyInSamples()));
}

bool ReplicantProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void ReplicantProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    buffer.clear();

    hostStop.beginBlock (getPlayHead(), midi);
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    buildPatch (patch);
    synth.setPatch (patch);
    synth.setRibbon (rRibbon());
    masterSm.setTargetValue (rMaster());

    // effect settings for this block
    const int tremMode = juce::roundToInt (rTremMode());
    float dlyMs = rDlyTime() * 1000.0f;
    const int sync = juce::roundToInt (rDlySync());
    if (sync > 0)
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm()) bpm = *b;
        dlyMs = (float) (60000.0 / juce::jlimit (30.0, 300.0, bpm) * delaySyncBeats (sync));
        dlyMs = juce::jlimit (20.0f, 2000.0f, dlyMs);
    }
    const float dlyMix = rDlyOn() >= 0.5f ? rDlyMix() : 0.0f;
    const float hallMix = rHallOn() >= 0.5f ? rHallMix() : 0.0f;

    float work[2][2048];
    float peak = 0.0f;

    for (int chunkStart = 0; chunkStart < numSamples; chunkStart += maxBlock)
    {
        const int len = juce::jmin (maxBlock, numSamples - chunkStart);
        float* chans[2] = { work[0], work[1] };
        juce::dsp::AudioBlock<float> block (chans, 2, (size_t) len);
        block.clear();

        auto osBlock = oversampling.processSamplesUp (block);
        osBlock.clear();
        float* L = osBlock.getChannelPointer (0);
        float* R = osBlock.getChannelPointer (1);

        int pos = 0;
        for (auto it = midi.findNextSamplePosition (chunkStart); it != midi.cend(); ++it)
        {
            const auto meta = *it;
            if (meta.samplePosition >= chunkStart + len) break;
            const int rel = juce::jlimit (0, len, meta.samplePosition - chunkStart);
            if (rel > pos) { synth.render (L + 2 * pos, R + 2 * pos, 2 * (rel - pos)); pos = rel; }
            synth.handleMidi (meta.getMessage());
        }
        if (pos < len) synth.render (L + 2 * pos, R + 2 * pos, 2 * (len - pos));

        oversampling.processSamplesDown (block);

        // DC blocker before the effects
        for (int i = 0; i < len; ++i)
        {
            float l = work[0][i], r = work[1][i];
            dcL = l - dcInL + dcCoef * dcL; dcInL = l; work[0][i] = dcL;
            dcR = r - dcInR + dcCoef * dcR; dcInR = r; work[1][i] = dcR;
        }

        chorus.process (work[0], work[1], len, tremMode, rTremSpeed(), rTremDepth());
        delay.process (work[0], work[1], len, dlyMs, rDlyFb(), dlyMix, rDlyTone());
        hall.process (work[0], work[1], len, rHallSize(), rHallDecay(), rHallTone(), rHallPre(), hallMix);

        // master volume, gentle safety limiter above -2 dBFS
        int sw = scopeWrite.load (std::memory_order_relaxed);
        for (int i = 0; i < len; ++i)
        {
            const float g = masterSm.getNextValue() * 1.6f;
            float l = work[0][i] * g, r = work[1][i] * g;
            auto knee = [] (float x)
            {
                const float a = std::abs (x);
                if (a <= 0.8f) return x;
                return std::copysign (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f), x);
            };
            l = knee (l); r = knee (r);
            peak = juce::jmax (peak, std::abs (l), std::abs (r));
            if (numCh >= 2)
            {
                buffer.setSample (0, chunkStart + i, l);
                buffer.setSample (1, chunkStart + i, r);
            }
            else if (numCh == 1) buffer.setSample (0, chunkStart + i, 0.5f * (l + r));

            if (++scopeDecim >= 2)
            {
                scopeDecim = 0;
                scope[sw] = 0.5f * (l + r);
                sw = (sw + 1) & (kScopeSize - 1);
            }
        }
        scopeWrite.store (sw, std::memory_order_relaxed);
    }

    for (int i = 0; i < kMaxVoices; ++i) voiceLevels[i].store (synth.voiceMeter (i), std::memory_order_relaxed);
    outputPeak.store (juce::jmax (peak, outputPeak.load() * 0.85f), std::memory_order_relaxed);
    ringEnv.store (synth.ringEnvValue(), std::memory_order_relaxed);
    subOsc.store (synth.subValue(), std::memory_order_relaxed);

    hostStop.endBlock (buffer, [this] { panic(); });
}

// Runs on the audio thread once the stop fade has reached silence.
void ReplicantProcessor::panic()
{
    synth.setSustain (false);
    synth.allNotesOff (true);
    oversampling.reset();
    chorus.reset();
    delay.reset();
    hall.reset();
    dcL = dcR = dcInL = dcInR = 0.0f;
}

// ---------------------------------------------------------------------------
// Programs (factory presets)
// ---------------------------------------------------------------------------
int ReplicantProcessor::getNumPrograms() { return numFactoryPresets(); }

const juce::String ReplicantProcessor::getProgramName (int index) { return factoryPresetName (index); }

void ReplicantProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numFactoryPresets() || index == currentProgram) return;
    currentProgram = index;
    activeMemory = -1;
    applyFactoryPreset (index, apvts);
    patchName = factoryPresetName (index);
    ++patchVersion;
}

// ---------------------------------------------------------------------------
// Memories (stored with the project, like the CS-80's memory panel)
// ---------------------------------------------------------------------------
void ReplicantProcessor::storeMemory (int slot)
{
    if (slot < 0 || slot >= kNumMemories) return;
    auto s = apvts.copyState();
    s.setProperty ("patchName", patchName, nullptr);
    memories[slot] = s;
    activeMemory = slot;
    ++patchVersion;
}

bool ReplicantProcessor::memoryUsed (int slot) const
{
    return slot >= 0 && slot < kNumMemories && memories[slot].isValid();
}

juce::String ReplicantProcessor::memoryName (int slot) const
{
    return memoryUsed (slot) ? memories[slot].getProperty ("patchName", "MEMORY").toString() : juce::String();
}

bool ReplicantProcessor::recallMemory (int slot)
{
    if (! memoryUsed (slot)) return false;
    applyState (memories[slot]);
    patchName = memoryName (slot);
    activeMemory = slot;
    currentProgram = -1;
    ++patchVersion;
    return true;
}

// sets every parameter through the host-visible path
void ReplicantProcessor::applyState (const juce::ValueTree& vt)
{
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
        {
            if (rp->getParameterID() == "ribbon") continue;
            auto child = vt.getChildWithProperty ("id", rp->getParameterID());
            if (child.isValid())
                rp->setValueNotifyingHost (rp->convertTo0to1 ((float) child.getProperty ("value")));
            else
                rp->setValueNotifyingHost (rp->getDefaultValue());
        }
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
void ReplicantProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("patchName", patchName, nullptr);
    juce::ValueTree mem ("MEMORIES");
    for (int i = 0; i < kNumMemories; ++i)
        if (memories[i].isValid())
        {
            auto m = memories[i].createCopy();
            m.setProperty ("slot", i, nullptr);
            mem.appendChild (juce::ValueTree ("MEM").setProperty ("slot", i, nullptr)
                                 .setProperty ("xml", m.toXmlString(), nullptr), nullptr);
        }
    if (auto xml = state.createXml())
    {
        if (auto mx = mem.createXml()) xml->addChildElement (mx.release());
        copyXmlToBinary (*xml, destData);
    }
}

void ReplicantProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    hostStop.requestStop();   // a project/state load starts from silence
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            for (auto& m : memories) m = juce::ValueTree();
            if (auto* mx = xml->getChildByName ("MEMORIES"))
            {
                for (auto* e : mx->getChildIterator())
                {
                    const int slot = e->getIntAttribute ("slot", -1);
                    if (slot >= 0 && slot < kNumMemories)
                        if (auto inner = juce::parseXML (e->getStringAttribute ("xml")))
                            memories[slot] = juce::ValueTree::fromXml (*inner);
                }
                xml->removeChildElement (mx, true);
            }
            auto vt = juce::ValueTree::fromXml (*xml);
            currentProgram = vt.getProperty ("program", 0);
            patchName = vt.getProperty ("patchName", "INIT").toString();
            apvts.replaceState (vt);
            for (auto* prm : getParameters())
                if (auto* b = dynamic_cast<juce::AudioParameterBool*> (prm))
                    if (auto* raw = apvts.getRawParameterValue (b->getParameterID()))
                        b->setValueNotifyingHost (raw->load() >= 0.5f ? 1.0f : 0.0f);
            activeMemory = -1;
            ++patchVersion;
        }
}

bool ReplicantProcessor::savePresetFile (const juce::File& f, const juce::String& name)
{
    auto state = apvts.copyState();
    state.setProperty ("patchName", name, nullptr);
    if (auto xml = state.createXml())
        if (xml->writeTo (f))
        {
            patchName = name;
            ++patchVersion;
            return true;
        }
    return false;
}

bool ReplicantProcessor::loadPresetFile (const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse (f))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            applyState (vt);
            patchName = vt.getProperty ("patchName", f.getFileNameWithoutExtension()).toString();
            currentProgram = -1;
            activeMemory = -1;
            ++patchVersion;
            return true;
        }
    return false;
}

juce::AudioProcessorEditor* ReplicantProcessor::createEditor()
{
#if REP_HEADLESS
    return new juce::GenericAudioProcessorEditor (*this);
#else
    return new ReplicantEditor (*this);
#endif
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ReplicantProcessor(); }
