#include "Displays.h"
#include "Backdrop.h"
#include "../PluginProcessor.h"
#include <complex>

namespace rep::ui
{
// ===========================================================================
EsperDisplay::EsperDisplay (ReplicantProcessor& p, NeonLookAndFeel& l) : proc (p), lnf (l)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip ("Click for the sound list");
    startTimerHz (30);
}

EsperDisplay::~EsperDisplay() { stopTimer(); }

void EsperDisplay::refresh()
{
    if (param == nullptr) return;
    auto n = param->getName (40).toUpperCase(), v = param->getCurrentValueAsText().toUpperCase();
    pNorm = param->getValue();
    if (n != pName || v != pValue) { pName = n; pValue = v; repaint(); }
}

void EsperDisplay::timerCallback()
{
    // grab the newest ~40 ms of output, trigger on a rising zero crossing
    const int N = ReplicantProcessor::kScopeSize;
    const int w = proc.scopeWrite.load (std::memory_order_relaxed);
    const int span = 512;
    int start = (w - span - 600 + N) & (N - 1);
    for (int k = 0; k < 600; ++k)
    {
        const int i0 = (start + k) & (N - 1), i1 = (i0 + 1) & (N - 1);
        if (proc.scope[i0] <= 0.0f && proc.scope[i1] > 0.0f) { start = i1; break; }
    }
    float pk = 0.02f;
    for (int i = 0; i < span; ++i) pk = juce::jmax (pk, std::abs (proc.scope[(start + i) & (N - 1)]));
    const float gain = 0.9f / juce::jmax (0.08f, pk);   // gentle auto-gain like a scope's VOLTS/DIV
    for (int k = 0; k < 256; ++k)
    {
        const float s = proc.scope[(start + k * 2) & (N - 1)] * gain;
        trace[k] = trace[k] * 0.35f + s * 0.65f;   // phosphor persistence
    }
    ++frame;
    refresh();
    repaint();
}

void EsperDisplay::paint (juce::Graphics& g)
{
    auto screen = drawCrt (g, getLocalBounds().toFloat());
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (screen.toNearestInt());

    auto inner = screen.reduced (10.0f, 6.0f);
    auto scopeArea = inner.removeFromRight (inner.getWidth() * 0.36f);
    inner.removeFromRight (8.0f);

    // ---- text side ----
    auto t1 = inner.removeFromTop (inner.getHeight() * 0.44f);
    glowText (g, patch, t1, lnf.mono (24.0f), juce::Justification::centredLeft, col::phosphor);
    auto t2 = inner.removeFromTop (inner.getHeight() * 0.58f);
    glowText (g, pName, t2, lnf.mono (15.0f), juce::Justification::centredLeft, col::phosphor, 0.72f);
    glowText (g, pValue, t2, lnf.mono (15.0f), juce::Justification::centredRight, col::amber);
    // ESPER status line - coordinates follow the control you touch
    const int a = 100 + juce::roundToInt (pNorm * 899.0f);
    const juce::String status = "ENHANCE 224-176   TRK " + juce::String (a).paddedLeft ('0', 3)
                              + "   GRID " + juce::String (juce::roundToInt (pNorm * 45.0f)).paddedLeft ('0', 2) + "R";
    glowText (g, status, inner, lnf.mono (11.0f), juce::Justification::centredLeft, col::phosphor, 0.45f);

    // ---- scope side: grid, crosshair, trace ----
    g.setColour (col::phosphor.withAlpha (0.10f));
    for (int k = 1; k < 6; ++k)
        g.drawVerticalLine ((int) (scopeArea.getX() + scopeArea.getWidth() * (float) k / 6.0f), scopeArea.getY(), scopeArea.getBottom());
    for (int k = 1; k < 4; ++k)
        g.drawHorizontalLine ((int) (scopeArea.getY() + scopeArea.getHeight() * (float) k / 4.0f), scopeArea.getX(), scopeArea.getRight());
    g.setColour (col::phosphor.withAlpha (0.22f));
    g.drawRect (scopeArea, 1.0f);

    juce::Path p;
    for (int k = 0; k < 256; ++k)
    {
        const float x = scopeArea.getX() + scopeArea.getWidth() * (float) k / 255.0f;
        const float y = scopeArea.getCentreY() - juce::jlimit (-1.0f, 1.0f, trace[k]) * scopeArea.getHeight() * 0.45f;
        if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (col::phosphor.withAlpha (0.15f));
    g.strokePath (p, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (col::phosphor);
    g.strokePath (p, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // rolling scan bar + scanlines
    const float bar = screen.getY() + std::fmod ((float) frame * 2.5f, screen.getHeight() + 30.0f) - 15.0f;
    juce::ColourGradient roll (col::phosphor.withAlpha (0.0f), 0.0f, bar - 15.0f, col::phosphor.withAlpha (0.0f), 0.0f, bar + 15.0f, false);
    roll.addColour (0.5, col::phosphor.withAlpha (0.05f));
    g.setGradientFill (roll);
    g.fillRect (screen.getX(), bar - 15.0f, screen.getWidth(), 30.0f);
    drawScanlines (g, screen);
}

// ===========================================================================
ChannelCrt::ChannelCrt (Ctx& c, int channel) : ctx (c), ch (channel)
{
    for (auto* s : { "il", "al", "fa", "fd", "fr", "a", "d", "s", "r", "hpf", "hpf_res", "lpf", "lpf_res" })
    {
        raws.push_back (ctx.apvts.getRawParameterValue (id::ch (ch, s)));
        last.push_back (-1.0f);
    }
    for (auto* s : { "brilliance", "resonance" })
    {
        raws.push_back (ctx.apvts.getRawParameterValue (s));
        last.push_back (-1.0f);
    }
    setInterceptsMouseClicks (false, false);
    startTimerHz (15);
}

ChannelCrt::~ChannelCrt() { stopTimer(); }

void ChannelCrt::timerCallback()
{
    bool changed = false;
    for (size_t i = 0; i < raws.size(); ++i)
    {
        const float x = raws[i] != nullptr ? raws[i]->load() : 0.0f;
        if (x != last[i]) { last[i] = x; changed = true; }
    }
    if (changed) repaint();
}

void ChannelCrt::paint (juce::Graphics& g)
{
    auto screen = drawCrt (g, getLocalBounds().toFloat());
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (screen.toNearestInt());
    auto r = screen.reduced (8.0f, 5.0f);

    const float il = v (0), al = v (1), fa = v (2), fd = v (3), fr = v (4);
    const float A = v (5), D = v (6), S = v (7), R = v (8);
    const float brill = v (13), res = v (14);
    const float hpf = v (9) * std::exp2 (brill * 3.0f), lpf = v (11) * std::exp2 (brill * 3.0f);
    const float hres = juce::jlimit (0.0f, 1.0f, v (10) + res * 0.5f), lres = juce::jlimit (0.0f, 1.0f, v (12) + res * 0.5f);

    auto labelRow = r.removeFromTop (12.0f);
    auto envArea = r.removeFromTop (r.getHeight() * 0.56f);
    r.removeFromTop (4.0f);
    auto fltArea = r;

    // ---- envelopes on a shared, compressed time axis ----
    auto w = [] (float t) { return std::sqrt (t + 0.0004f); };
    const float hold = 0.45f;
    const float totF = w (fa) + w (fd) + hold + w (fr);
    const float totA = w (A) + w (D) + hold + w (R);
    const float tot = juce::jmax (totF, totA);
    const float sx = envArea.getWidth() / tot;

    g.setColour (col::phosphor.withAlpha (0.08f));
    for (int k = 1; k < 8; ++k)
        g.drawVerticalLine ((int) (envArea.getX() + envArea.getWidth() * (float) k / 8.0f), envArea.getY(), envArea.getBottom());

    {
        // filter EG: from -IL up to +AL, back to 0 (the slider), release to -IL
        const float mid = envArea.getY() + envArea.getHeight() * 0.5f;
        auto Y = [&] (float lv) { return mid - lv * envArea.getHeight() * 0.44f; };
        g.setColour (col::cyan.withAlpha (0.25f));
        g.drawHorizontalLine ((int) mid, envArea.getX(), envArea.getRight());
        juce::Path p;
        float x = envArea.getX();
        p.startNewSubPath (x, Y (-il));
        const float wa = w (fa) * sx;
        for (int i = 1; i <= 14; ++i) { const float t = (float) i / 14.0f; p.lineTo (x + wa * t, Y (-il + (al + il) * (1.35f * (1.0f - std::exp (-1.5f * t))) / 1.0f)); }
        x += wa;
        const float wd = w (fd) * sx;
        for (int i = 1; i <= 16; ++i) { const float t = (float) i / 16.0f; p.lineTo (x + wd * t, Y (al * std::exp (-4.6f * t))); }
        x += wd + hold * sx;
        p.lineTo (x, Y (0.0f));
        const float wr = w (fr) * sx;
        for (int i = 1; i <= 16; ++i) { const float t = (float) i / 16.0f; p.lineTo (x + wr * t, Y (-il * (1.0f - std::exp (-4.6f * t)))); }
        NeonLookAndFeel::neonStroke (g, p, col::cyan, 1.3f, 0.7f);
    }
    {
        auto Y = [&] (float lv) { return envArea.getBottom() - 2.0f - lv * (envArea.getHeight() - 6.0f); };
        juce::Path p;
        float x = envArea.getX();
        p.startNewSubPath (x, Y (0.0f));
        const float wa = w (A) * sx;
        for (int i = 1; i <= 14; ++i) { const float t = (float) i / 14.0f; p.lineTo (x + wa * t, Y (juce::jmin (1.0f, 1.3f * (1.0f - std::exp (-1.47f * t))))); }
        x += wa;
        const float wd = w (D) * sx;
        for (int i = 1; i <= 16; ++i) { const float t = (float) i / 16.0f; p.lineTo (x + wd * t, Y (S + (1.0f - S) * std::exp (-4.6f * t))); }
        x += wd + hold * sx;
        p.lineTo (x, Y (S));
        const float wr = w (R) * sx;
        for (int i = 1; i <= 16; ++i) { const float t = (float) i / 16.0f; p.lineTo (x + wr * t, Y (S * std::exp (-4.6f * t))); }
        NeonLookAndFeel::neonStroke (g, p, col::amber, 1.2f, 0.6f);
    }
    g.setFont (ctx.lnf.mono (10.0f));
    g.setColour (col::cyan.withAlpha (0.8f));
    g.drawText ("VCF EG  IL/AL", labelRow, juce::Justification::centredLeft, false);
    g.setColour (col::amber.withAlpha (0.8f));
    g.drawText ("VCA EG  ADSR", labelRow, juce::Justification::centredRight, false);
    g.setColour (col::phosphor.withAlpha (0.4f));
    g.drawText ("CH " + juce::String (ch == 1 ? "I" : "II"), labelRow, juce::Justification::centred, false);

    // ---- HPF -> LPF response ----
    g.setColour (col::phosphor.withAlpha (0.10f));
    for (float f : { 100.0f, 1000.0f, 10000.0f })
    {
        const float x = fltArea.getX() + fltArea.getWidth() * std::log10 (f / 20.0f) / 3.0f;
        g.drawVerticalLine ((int) x, fltArea.getY(), fltArea.getBottom());
    }
    auto dbToY = [&] (float db) { return juce::jmap (db, 20.0f, -40.0f, fltArea.getY(), fltArea.getBottom()); };
    g.drawHorizontalLine ((int) dbToY (0.0f), fltArea.getX(), fltArea.getRight());

    auto resToK = [] (float rr) { return 1.0f / (0.6f * std::exp2 (4.2f * rr)); };
    const float kh = resToK (hres), kl = resToK (lres);
    juce::Path p;
    const int Np = (int) fltArea.getWidth();
    for (int i = 0; i <= Np; ++i)
    {
        const float f = 20.0f * std::pow (1000.0f, (float) i / (float) Np);
        const std::complex<float> sh (0.0f, f / juce::jmax (1.0f, hpf)), sl (0.0f, f / juce::jmax (1.0f, lpf));
        const auto Hh = (sh * sh) / (sh * sh + kh * sh + 1.0f);
        const auto Hl = 1.0f / (sl * sl + kl * sl + 1.0f);
        const float db = juce::jlimit (-60.0f, 30.0f, 20.0f * std::log10 (std::abs (Hh * Hl) + 1.0e-6f));
        const float x = fltArea.getX() + (float) i, y = dbToY (db);
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    NeonLookAndFeel::neonStroke (g, p, col::phosphor, 1.2f, 0.6f);
    g.setColour (col::phosphor.withAlpha (0.55f));
    g.setFont (ctx.lnf.mono (10.0f));
    g.drawText ("HPF > LPF", fltArea.reduced (2.0f), juce::Justification::topLeft, false);

    drawScanlines (g, screen, 0.18f);
}
} // namespace rep::ui
