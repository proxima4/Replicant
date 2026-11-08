#pragma once
#include "NeonLookAndFeel.h"

namespace rep::ui
{
struct Section
{
    juce::Rectangle<int> r;
    juce::String title;
    juce::Colour colour;
};

// Renders the static panel: wet black glass with rain streaks and out-of-
// focus city lights, the neon-sign wordmark, and a smoked-glass plate with
// a neon tube outline for every control section. Built once (at 2x for
// sharp scaling) and simply blitted afterwards.
juce::Image makeBackdrop (const std::vector<Section>& sections, NeonLookAndFeel& lnf, float scale);

// CRT bezel + phosphor screen with scanlines and vignette (used live by the
// displays). Returns the inner screen area.
juce::Rectangle<float> drawCrt (juce::Graphics&, juce::Rectangle<float> outer);
void drawScanlines (juce::Graphics&, juce::Rectangle<float> screen, float alpha = 0.22f);

// Phosphor text with bloom
void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, const juce::Font&,
               juce::Justification, juce::Colour, float alpha = 1.0f);
} // namespace rep::ui
