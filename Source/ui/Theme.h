#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// ============================================================================
//  REPLICANT look: ESPER-machine CRT phosphor displays on a rain-streaked
//  black glass panel lit by neon tubes - cyan, magenta, sodium amber - with
//  red serif credits.
// ============================================================================
namespace rep::ui
{
constexpr int kW = 1440;
constexpr int kH = 900;

namespace col
{
    const juce::Colour bg        { 0xff04060a };
    const juce::Colour glass     { 0xff081018 };
    const juce::Colour plate     { 0xd8060b11 };
    const juce::Colour cyan      { 0xff1fe4ff };
    const juce::Colour magenta   { 0xffff2fa6 };
    const juce::Colour amber     { 0xffffb03a };
    const juce::Colour sodium    { 0xffff7a22 };
    const juce::Colour red       { 0xffe0262e };
    const juce::Colour phosphor  { 0xff9dffd6 };   // ESPER CRT trace
    const juce::Colour phosDim   { 0xff2f6f5c };
    const juce::Colour crtBg     { 0xff031410 };
    const juce::Colour text      { 0xffb9d3de };
    const juce::Colour textDim   { 0xff5f7c8a };
    const juce::Colour white     { 0xffe8f4f8 };
}

// slider cap colour codes (like the CS-80's coloured slider caps)
enum CapColour { capCyan = 0, capAmber, capMagenta, capWhite, capRed };

inline juce::Colour capColour (int c)
{
    switch (c)
    {
        case capAmber:   return col::amber;
        case capMagenta: return col::magenta;
        case capWhite:   return col::white;
        case capRed:     return col::red;
        default:         return col::cyan;
    }
}
} // namespace rep::ui
