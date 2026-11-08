#include "PluginEditor.h"
#include "Presets.h"

namespace rep::ui
{
namespace
{
    constexpr int kRowH = 210;
    constexpr int kRowY[2] = { 112, 330 };
    constexpr int kCtlY = 550, kCtlH = 198;
    constexpr int kRightX = 1178;

    // tone selector colours by instrument family (lit glass, like the
    // CS-80's bright preset buttons)
    juce::uint32 toneColour (int i)
    {
        if (i < 4)  return 0xffcfe6ff;   // strings
        if (i < 7)  return 0xffffb03a;   // brass
        if (i < 9)  return 0xffff4646;   // organ
        if (i < 10) return 0xff4dff9a;   // e.piano
        if (i < 12) return 0xffff7a22;   // guitar
        if (i < 14) return 0xffff2fa6;   // harpsichord
        if (i < 16) return 0xffa070ff;   // clavichord
        if (i < 20) return 0xff1fe4ff;   // funky
        if (i < 21) return 0xff60ffd8;   // flute
        return 0xff4a86ff;               // bass
    }

    // lays out sections left to right; records the plates for the backdrop
    struct RowBuilder
    {
        std::vector<Section>& out;
        int x, y, h;
        int curStart = 0;
        juce::String title;
        juce::Colour colour;
        void begin (const juce::String& t, juce::Colour c) { title = t; colour = c; curStart = x; x += 6; }
        int col (int w) { const int cx = x; x += w; return cx; }
        void end (int gap = 10)
        {
            x += 6;
            out.push_back ({ { curStart, y, x - curStart, h }, title, colour });
            x += gap;
        }
    };
}

// ===========================================================================
Root::Root (ReplicantProcessor& p)
    : proc (p),
      ctx { p.apvts, lnf, {} },
      display (p, lnf),
      keyboard (p.keyboardState, lnf)
{
    setLookAndFeel (&lnf);
    setSize (kW, kH);
    ctx.touch = [this] (juce::RangedAudioParameter* prm) { display.showParam (prm); };

    buildHeader();
    buildChannelRow (1, kRowY[0]);
    buildChannelRow (2, kRowY[1]);
    buildControlPanel (kCtlY);
    buildFx (kRightX, kCtlY, kW - 12 - kRightX, kCtlH);
    buildBottom();

    backdrop = makeBackdrop (sections, lnf, 2.0f);

    updatePatchLabel();
    timerCallback();
    startTimerHz (30);
}

Root::~Root()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

// ---------------------------------------------------------------------------
void Root::buildHeader()
{
    addAndMakeVisible (display);
    display.setBounds (452, 9, 470, 90);
    display.onClick = [this] { showPresetMenu(); };

    int x = 934;
    for (auto* b : { &prevBtn, &nextBtn })
    {
        addAndMakeVisible (*b);
        b->setBounds (x, 12, 30, 28);
        x += 34;
    }
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (1); };
    prevBtn.setTooltip ("Previous sound");
    nextBtn.setTooltip ("Next sound");

    x += 6;
    for (int i = 0; i < ReplicantProcessor::kNumMemories; ++i)
    {
        auto& b = memBtn[i];
        b.setButtonText (juce::String (i + 1));
        b.getProperties().set ("cap", capAmber);
        b.setBounds (x, 12, 30, 28);
        b.onClick = [this, i] { memoryClicked (i); };
        b.setTooltip ("MEMORY " + juce::String (i + 1) + " - recall (STORE then " + juce::String (i + 1) + " to save the panel here)");
        addAndMakeVisible (b);
        x += 33;
    }
    storeBtn.setBounds (x + 2, 12, 60, 28);
    storeBtn.getProperties().set ("cap", capRed);
    storeBtn.setTooltip ("Arm STORE, then press a memory number");
    storeBtn.onClick = [this] { storeArmed = ! storeArmed; storeBtn.setToggleState (storeArmed, juce::dontSendNotification);
                                if (storeArmed) flash ("STORE: PRESS MEMORY 1-4"); else updatePatchLabel(); };
    addAndMakeVisible (storeBtn);
    x += 70;
    for (auto* b : { &loadBtn, &saveBtn, &setupBtn })
    {
        addAndMakeVisible (*b);
        b->setBounds (x, 12, b == &setupBtn ? 60 : 52, 28);
        x += (b == &setupBtn ? 64 : 56);
    }
    loadBtn.onClick = [this] { loadPreset(); };
    saveBtn.onClick = [this] { savePreset(); };
    setupBtn.onClick = [this] { showSetupMenu(); };
    loadBtn.setTooltip ("Load a sound file (.rpl)");
    saveBtn.setTooltip ("Save the panel as a sound file (.rpl)");
    setupBtn.setTooltip ("Ribbon range, pitch wheel, mod wheel, MPE");
    setupBtn.getProperties().set ("cap", capWhite);

    // instrument knobs
    const char* ids[]  = { "voices", "age", "spread", "master" };
    const char* labs[] = { "VOICES", "AGE", "SPREAD", "VOLUME" };
    for (int i = 0; i < 4; ++i)
        add<SmallKnob> (1196 + i * 57, 44, 54, 56, ctx, ids[i], labs[i], i == 3 ? capAmber : capWhite);
}

// ---------------------------------------------------------------------------
void Root::buildChannelRow (int ch, int y)
{
    const int cw = 38;
    const int top = y + 14, sh = kRowH - 22;
    // channel tag
    sections.push_back ({ { 12, y, 32, kRowH }, {}, col::magenta });

    RowBuilder rb { sections, 52, y, kRowH, 0, {}, {} };
    auto slider = [&] (const char* suffix, const char* label, int cap, bool bi = false)
    {
        add<CsSlider> (rb.col (cw), top, cw, sh, ctx, id::ch (ch, suffix), label, cap, bi);
    };

    rb.begin ("FEET", col::magenta);
    add<StepSelector> (rb.col (46), top + 14, 46, sh - 30, ctx, id::ch (ch, "feet"), feetNames(), capMagenta);
    rb.end();

    rb.begin ("VCO", col::cyan);
    slider ("pwm_speed", "SPEED", capWhite);
    slider ("pwm", "PWM", capWhite);
    slider ("pw", "PW", capCyan);
    {
        const int x = rb.col (cw + 4);
        add<LightSwitch> (x, top + 34, cw + 4, 30, ctx, id::ch (ch, "square"), "SQ", capAmber);
        add<LightSwitch> (x, top + 74, cw + 4, 30, ctx, id::ch (ch, "saw"), "SAW", capAmber);
    }
    slider ("noise", "NOISE", capAmber);
    rb.end();

    rb.begin ("VCF", col::cyan);
    slider ("hpf", "HPF", capCyan);
    slider ("hpf_res", "RES", capRed);
    slider ("lpf", "LPF", capCyan);
    slider ("lpf_res", "RES", capRed);
    rb.end();

    rb.begin ("VCF ENVELOPE", col::magenta);
    slider ("il", "IL", capCyan);
    slider ("al", "AL", capCyan);
    slider ("fa", "A", capMagenta);
    slider ("fd", "D", capMagenta);
    slider ("fr", "R", capMagenta);
    rb.end();

    rb.begin ("VCA", col::amber);
    slider ("vcf_level", "VCF", capAmber);
    slider ("sine", "SINE", capAmber);
    slider ("a", "A", capMagenta);
    slider ("d", "D", capMagenta);
    slider ("s", "S", capAmber);
    slider ("r", "R", capMagenta);
    slider ("level", "LEVEL", capAmber);
    rb.end();

    rb.begin ("TOUCH RESPONSE", col::sodium);
    slider ("tib", "I.BRIL", capCyan);
    slider ("til", "I.LEV", capAmber);
    slider ("tab", "A.BRIL", capCyan);
    slider ("tal", "A.LEV", capAmber);
    rb.end (0);

    crt[ch - 1] = std::make_unique<ChannelCrt> (ctx, ch);
    addAndMakeVisible (*crt[ch - 1]);
    crt[ch - 1]->setBounds (kRightX, y + 2, kW - 12 - kRightX, kRowH - 4);
}

// ---------------------------------------------------------------------------
void Root::buildControlPanel (int y)
{
    const int cw = 30;
    const int top = y + 14, sh = kCtlH - 22;
    RowBuilder rb { sections, 12, y, kCtlH, 0, {}, {} };
    auto slider = [&] (const char* pid, const char* label, int cap, bool bi = false)
    {
        add<CsSlider> (rb.col (cw), top, cw, sh, ctx, pid, label, cap, bi);
    };

    rb.begin ("SUB OSC", col::cyan);
    add<StepSelector> (rb.col (56), top + 6, 56, sh - 22, ctx, "sub_func", subFuncNames(), capCyan);
    rb.x += 4;
    slider ("sub_speed", "SPEED", capWhite);
    slider ("sub_vco", "VCO", capWhite);
    slider ("sub_vcf", "VCF", capWhite);
    slider ("sub_vca", "VCA", capWhite);
    rb.end (8);

    rb.begin ("RING MOD", col::magenta);
    slider ("ring_mod", "MOD", capAmber);
    slider ("ring_speed", "SPEED", capWhite);
    slider ("ring_depth", "DEPTH", capWhite);
    slider ("ring_a", "A", capMagenta);
    slider ("ring_d", "D", capMagenta);
    rb.end (8);

    rb.begin ("TOUCH", col::sodium);
    slider ("init_bend", "BEND", capWhite);
    slider ("bend_time", "TIME", capMagenta);
    slider ("aft_pitch", "A.PIT", capWhite);
    slider ("aft_vib", "A.VIB", capWhite);
    slider ("aft_speed", "A.SPD", capWhite);
    rb.end (8);

    rb.begin ("OVERALL", col::amber);
    slider ("brilliance", "BRILL", capCyan, true);
    slider ("resonance", "RES", capRed, true);
    rb.end (8);

    rb.begin ("KEYBOARD", col::cyan);
    slider ("kb_bl", "B.LOW", capCyan, true);
    slider ("kb_bh", "B.HI", capCyan, true);
    slider ("kb_ll", "L.LOW", capAmber, true);
    slider ("kb_lh", "L.HI", capAmber, true);
    slider ("kb_track", "TRACK", capCyan);
    rb.end (8);

    rb.begin ("PITCH / MIX", col::amber);
    slider ("detune", "DETUN", capWhite, true);
    slider ("coarse", "COARS", capWhite, true);
    slider ("fine", "FINE", capWhite, true);
    slider ("mix", "I - II", capAmber, true);
    rb.end (8);

    rb.begin ("GLIDE", col::magenta);
    add<StepSelector> (rb.col (46), top + 30, 46, 56, ctx, "glide_mode", juce::StringArray { "PORT", "GLISS" }, capMagenta);
    rb.x += 2;
    slider ("glide_time", "TIME", capMagenta);
    rb.end (8);

    rb.begin ("CHORUS", col::cyan);
    add<StepSelector> (rb.col (52), top + 14, 52, 96, ctx, "trem_mode", juce::StringArray { "OFF", "CHOR 1", "CHOR 2", "TREM" }, capCyan);
    rb.x += 2;
    slider ("trem_speed", "SPEED", capWhite);
    slider ("trem_depth", "DEPTH", capWhite);
    rb.end (0);
}

// ---------------------------------------------------------------------------
void Root::buildFx (int x, int y, int w, int h)
{
    sections.push_back ({ { x, y, w, h }, "DELAY  /  HALL", col::cyan });
    const int kw = 58, kh = 54;
    const int x0 = x + 8;
    add<LightSwitch> (x0, y + 14, 56, 22, ctx, "dly_on", "DELAY", capCyan);
    auto& sync = add<juce::ComboBox> (x0 + 62, y + 14, w - 78, 22);
    sync.addItemList (delaySyncNames(), 1);
    sync.setTooltip ("Delay sync to the host tempo (FREE uses the TIME knob)");
    syncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "dly_sync", sync);
    const char* dIds[] = { "dly_time", "dly_fb", "dly_mix", "dly_tone" };
    const char* dLab[] = { "TIME", "FDBK", "MIX", "TONE" };
    for (int i = 0; i < 4; ++i) add<SmallKnob> (x0 + i * kw, y + 40, kw - 2, kh, ctx, dIds[i], dLab[i], i == 2 ? capAmber : capWhite);

    add<LightSwitch> (x0, y + 102, 56, 22, ctx, "hall_on", "HALL", capCyan);
    add<SmallKnob> (x0 + 3 * kw, y + 98, kw - 2, kh - 4, ctx, "hall_pre", "PRE", capWhite);
    const char* hIds[] = { "hall_size", "hall_decay", "hall_tone", "hall_mix" };
    const char* hLab[] = { "SIZE", "DECAY", "TONE", "MIX" };
    for (int i = 0; i < 4; ++i) add<SmallKnob> (x0 + i * kw, y + 140, kw - 2, kh, ctx, hIds[i], hLab[i], i == 3 ? capAmber : capWhite);
}

// ---------------------------------------------------------------------------
void Root::buildBottom()
{
    const int y = 758;
    for (int i = 0; i < 22; ++i)
    {
        auto& b = toneBtn[i];
        b.setButtonText (toneButtonLabel (i));
        b.getProperties().set ("tone", (juce::int64) toneColour (i));
        b.setBounds (12 + i * 52, y, 49, 28);
        b.onClick = [this, i] { proc.loadFactoryPreset (i); };
        b.setTooltip (factoryPresetName (i));
        addAndMakeVisible (b);
    }
    moreBtn.setBounds (12 + 22 * 52 + 8, y, kW - 12 - (12 + 22 * 52 + 8), 28);
    moreBtn.getProperties().set ("cap", capMagenta);
    moreBtn.onClick = [this] { showFilmMenu(); };
    moreBtn.setTooltip ("Sounds in the style of the CS-80's famous film score era");
    addAndMakeVisible (moreBtn);

    ribbon = std::make_unique<Ribbon> (ctx);
    addAndMakeVisible (*ribbon);
    ribbon->setBounds (12, 792, kW - 24, 26);

    keyboard.setAvailableRange (36, 96);
    keyboard.setKeyWidth ((float) (kW - 24) / 36.0f);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setWantsKeyboardFocus (false);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xff1a2128));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff040506));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff2c3a44));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, col::cyan.withAlpha (0.18f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, col::cyan.withAlpha (0.55f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.7f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, col::textDim);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, col::bg);
    addAndMakeVisible (keyboard);
    keyboard.setBounds (12, 824, kW - 24, 68);
}

// ---------------------------------------------------------------------------
void Root::paint (juce::Graphics& g)
{
    g.drawImage (backdrop, getLocalBounds().toFloat());

    // channel tags
    for (int c = 0; c < 2; ++c)
    {
        auto r = juce::Rectangle<float> (12.0f, (float) kRowY[c], 32.0f, (float) kRowH);
        glowText (g, c == 0 ? "I" : "II", r.withTrimmedBottom (r.getHeight() * 0.55f), lnf.wide (22.0f),
                  juce::Justification::centred, col::magenta);
        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, r.getCentreX(), r.getCentreY() + 40.0f));
        glowText (g, "CHANNEL", juce::Rectangle<float> (r.getCentreX() - 50.0f, r.getCentreY() + 32.0f, 100.0f, 16.0f),
                  lnf.mono (11.0f), juce::Justification::centred, col::magenta, 0.7f);
        g.restoreState();
    }

    // voice lamps
    g.setFont (lnf.mono (10.0f));
    g.setColour (col::textDim);
    g.drawText ("VOICES", 934, 46, 80, 12, juce::Justification::centredLeft);
    const int nv = juce::roundToInt (proc.apvts.getRawParameterValue ("voices")->load());
    for (int i = 0; i < kMaxVoices; ++i)
    {
        const float cx = 940.0f + (float) (i % 8) * 15.0f, cy = 66.0f + (float) (i / 8) * 14.0f;
        const float lv = juce::jlimit (0.0f, 1.0f, lamps[i] * 6.0f);
        g.setColour (juce::Colours::black);
        g.fillEllipse (cx - 5.5f, cy - 5.5f, 11.0f, 11.0f);
        if (lv > 0.02f)
        {
            g.setColour (col::cyan.withAlpha (0.3f * lv));
            g.fillEllipse (cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
        }
        g.setColour (i < nv ? juce::Colour (0xff0b2a33).interpolatedWith (col::cyan, lv) : juce::Colour (0xff15191c));
        g.fillEllipse (cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);
    }
    // SUB and RING activity lamps
    auto lamp = [&] (float x, float y, const juce::String& t, float v, juce::Colour c)
    {
        g.setColour (juce::Colours::black);
        g.fillEllipse (x - 5.5f, y - 5.5f, 11.0f, 11.0f);
        g.setColour (c.withAlpha (0.3f * v));
        g.fillEllipse (x - 8.0f, y - 8.0f, 16.0f, 16.0f);
        g.setColour (juce::Colour (0xff1a1a1a).interpolatedWith (c, v));
        g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
        g.setColour (col::textDim);
        g.drawText (t, (int) x + 9, (int) y - 6, 40, 12, juce::Justification::centredLeft);
    };
    lamp (1072.0f, 66.0f, "SUB", subLamp, col::cyan);
    lamp (1072.0f, 80.0f, "RING", ringLamp, col::magenta);
    lamp (1130.0f, 66.0f, "STORE", storeArmed && (blink / 8) % 2 == 0 ? 1.0f : 0.0f, col::red);

    // output meter
    g.setColour (col::textDim);
    g.drawText ("OUT", 934, 86, 30, 12, juce::Justification::centredLeft);
    auto bar = juce::Rectangle<float> (960.0f, 88.0f, 224.0f, 8.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (bar, 2.0f);
    const float lvl = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (outLevel, -60.0f) + 60.0f) / 60.0f);
    for (int s = 0; s < 28; ++s)
    {
        const bool on = (float) s / 28.0f < lvl;
        const auto c = s < 19 ? col::cyan : (s < 25 ? col::amber : col::red);
        g.setColour (on ? c : c.withAlpha (0.1f));
        g.fillRect (bar.getX() + 2.0f + (float) s * 7.9f, bar.getY() + 2.0f, 5.6f, bar.getHeight() - 4.0f);
    }
}

// ---------------------------------------------------------------------------
void Root::timerCallback()
{
    bool dirty = false;
    for (int i = 0; i < kMaxVoices; ++i)
    {
        const float v = proc.voiceLevels[i].load();
        const float nv = juce::jmax (v, lamps[i] * 0.82f);
        if (std::abs (nv - lamps[i]) > 0.002f) dirty = true;
        lamps[i] = nv;
    }
    const float pk = proc.outputPeak.load();
    const float no = juce::jmax (pk, outLevel * 0.85f);
    if (std::abs (no - outLevel) > 0.001f) { outLevel = no; dirty = true; }

    const float sv = proc.apvts.getRawParameterValue ("sub_vco")->load() + proc.apvts.getRawParameterValue ("sub_vcf")->load()
                   + proc.apvts.getRawParameterValue ("sub_vca")->load();
    const float ns = sv > 0.001f ? 0.5f + 0.5f * proc.subOsc.load() : 0.0f;
    const float nr = proc.apvts.getRawParameterValue ("ring_mod")->load() > 0.001f ? proc.ringEnv.load() * 0.8f + 0.2f : 0.0f;
    if (std::abs (ns - subLamp) > 0.01f || std::abs (nr - ringLamp) > 0.01f) { subLamp = ns; ringLamp = nr; dirty = true; }

    ++blink;
    if (storeArmed && blink % 8 == 0) dirty = true;
    if (dirty) repaint (926, 44, 266, 56);

    if (flashTimer > 0 && --flashTimer == 0) updatePatchLabel();

    const int pv = proc.patchVersion.load();
    if (pv != lastPatchVersion)
    {
        lastPatchVersion = pv;
        updatePatchLabel();
        const int prg = proc.getCurrentProgram();
        for (int i = 0; i < 22; ++i) toneBtn[i].setToggleState (i == prg, juce::dontSendNotification);
        moreBtn.setToggleState (prg >= 22, juce::dontSendNotification);
        for (int i = 0; i < ReplicantProcessor::kNumMemories; ++i)
            memBtn[i].setToggleState (proc.lastMemory() == i, juce::dontSendNotification);
    }
    for (int i = 0; i < ReplicantProcessor::kNumMemories; ++i)
    {
        const auto cap = proc.memoryUsed (i) ? capAmber : capWhite;
        if ((int) memBtn[i].getProperties()["cap"] != cap) { memBtn[i].getProperties().set ("cap", cap); memBtn[i].repaint(); }
    }
}

void Root::updatePatchLabel()
{
    if (flashTimer > 0) return;
    const int prg = proc.getCurrentProgram();
    const int mem = proc.lastMemory();
    const auto name = proc.getPatchName();
    juce::String tag = "USR ";
    if (mem >= 0) tag = "M" + juce::String (mem + 1) + "  ";
    else if (prg >= 0 && factoryPresetName (prg) == name) tag = juce::String (prg + 1).paddedLeft ('0', 2) + "  ";
    display.setPatch (tag + name);
}

void Root::stepPreset (int delta)
{
    const int n = numFactoryPresets();
    int cur = proc.getCurrentProgram();
    if (cur < 0) cur = 0;
    proc.loadFactoryPreset ((cur + delta + n) % n);
}

void Root::memoryClicked (int slot)
{
    if (storeArmed)
    {
        proc.storeMemory (slot);
        storeArmed = false;
        storeBtn.setToggleState (false, juce::dontSendNotification);
        flash ("STORED IN MEMORY " + juce::String (slot + 1));
        return;
    }
    if (! proc.recallMemory (slot))
        flash ("MEMORY " + juce::String (slot + 1) + " EMPTY - STORE FIRST");
}

void Root::showPresetMenu()
{
    juce::PopupMenu tones, film, m;
    const int cur = proc.getCurrentProgram();
    for (int i = 0; i < numFactoryPresets(); ++i)
    {
        auto label = juce::String (i + 1).paddedLeft ('0', 2) + "  " + factoryPresetName (i);
        (i < kNumToneSelector ? tones : film).addItem (i + 1, label, true, i == cur);
    }
    m.addSectionHeader ("TONE SELECTOR");
    m.addSubMenu ("CS-80 PRESET VOICES", tones);
    m.addSubMenu ("FILM SCORE SOUNDS", film);
    m.addSeparator();
    for (int i = 0; i < ReplicantProcessor::kNumMemories; ++i)
        m.addItem (2000 + i, "MEMORY " + juce::String (i + 1) + (proc.memoryUsed (i) ? "  " + proc.memoryName (i) : "  (empty)"),
                   proc.memoryUsed (i), proc.lastMemory() == i);
    m.addSeparator();
    m.addItem (1000, "LOAD SOUND FILE...");
    m.addItem (1001, "SAVE SOUND FILE...");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (display).withMinimumWidth (display.getWidth()),
                     [this] (int r)
                     {
                         if (r >= 1 && r <= numFactoryPresets()) proc.loadFactoryPreset (r - 1);
                         else if (r >= 2000 && r < 2000 + ReplicantProcessor::kNumMemories) proc.recallMemory (r - 2000);
                         else if (r == 1000) loadPreset();
                         else if (r == 1001) savePreset();
                     });
}

void Root::showFilmMenu()
{
    juce::PopupMenu m;
    const int cur = proc.getCurrentProgram();
    for (int i = kNumToneSelector; i < numFactoryPresets(); ++i)
        m.addItem (i + 1, factoryPresetName (i), true, i == cur);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (moreBtn).withMinimumWidth (moreBtn.getWidth()),
                     [this] (int r) { if (r >= 1) proc.loadFactoryPreset (r - 1); });
}

void Root::showSetupMenu()
{
    auto setParam = [this] (const char* pid, float v)
    {
        if (auto* p = proc.apvts.getParameter (pid)) p->setValueNotifyingHost (p->convertTo0to1 (v));
    };
    auto val = [this] (const char* pid) { return proc.apvts.getRawParameterValue (pid)->load(); };

    juce::PopupMenu rib, bend, wheel, m;
    for (int s : { 2, 5, 7, 12, 24 })
        rib.addItem (100 + s, juce::String (s) + " semitones", true, juce::roundToInt (val ("ribbon_range")) == s);
    for (int s : { 0, 1, 2, 7, 12, 24 })
        bend.addItem (200 + s, juce::String (s) + " semitones", true, juce::roundToInt (val ("bend_range")) == s);
    const char* wn[] = { "VIBRATO (sub osc to VCO)", "PRESSURE (acts as aftertouch)", "BRILLIANCE" };
    for (int i = 0; i < 3; ++i) wheel.addItem (300 + i, wn[i], true, juce::roundToInt (val ("wheel_dest")) == i);
    m.addSectionHeader ("PERFORMANCE SETUP");
    m.addSubMenu ("RIBBON RANGE", rib);
    m.addSubMenu ("PITCH WHEEL RANGE", bend);
    m.addSubMenu ("MOD WHEEL", wheel);
    m.addItem (400, "MPE (per-note bend & pressure)", true, val ("mpe") >= 0.5f);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (setupBtn).withMinimumWidth (220),
                     [setParam, val] (int r)
                     {
                         if (r >= 100 && r < 200) setParam ("ribbon_range", (float) (r - 100));
                         else if (r >= 200 && r < 300) setParam ("bend_range", (float) (r - 200));
                         else if (r >= 300 && r < 303) setParam ("wheel_dest", (float) (r - 300));
                         else if (r == 400) setParam ("mpe", val ("mpe") >= 0.5f ? 0.0f : 1.0f);
                     });
}

juce::File Root::presetFolder() const
{
    auto f = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Replicant Sounds");
    f.createDirectory();
    return f;
}

void Root::savePreset()
{
    chooser = std::make_unique<juce::FileChooser> ("Save Replicant sound", presetFolder(), "*.rpl");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File()) return;
                              f = f.withFileExtension ("rpl");
                              proc.savePresetFile (f, f.getFileNameWithoutExtension().toUpperCase());
                          });
}

void Root::loadPreset()
{
    chooser = std::make_unique<juce::FileChooser> ("Load Replicant sound", presetFolder(), "*.rpl");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f.existsAsFile()) proc.loadPresetFile (f);
                          });
}
} // namespace rep::ui

// ---------------------------------------------------------------------------
ReplicantEditor::ReplicantEditor (ReplicantProcessor& p) : AudioProcessorEditor (p), root (p)
{
    addAndMakeVisible (root);
    setResizable (true, true);
    setResizeLimits (rep::ui::kW * 5 / 10, rep::ui::kH * 5 / 10, rep::ui::kW * 16 / 10, rep::ui::kH * 16 / 10);
    getConstrainer()->setFixedAspectRatio ((double) rep::ui::kW / (double) rep::ui::kH);
    setSize (rep::ui::kW, rep::ui::kH);
}

void ReplicantEditor::resized()
{
    const float s = (float) getWidth() / (float) rep::ui::kW;
    root.setTransform (juce::AffineTransform::scale (s));
    root.setBounds (0, 0, rep::ui::kW, rep::ui::kH);
}
