// Renders the Replicant editor to PNG (development tool). Plays a chord
// through the engine first so the scope, lamps and meter show signal.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

static void snap (juce::Component& c, const juce::File& f, float scale)
{
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, scale);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::cout << "wrote " << f.getFullPathName() << "\n";
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? argv[1] : "snap");
    const int preset = argc > 2 ? juce::String (argv[2]).getIntValue() : 22;
    outDir.createDirectory();

    ReplicantProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    proc.loadFactoryPreset (preset);

    auto t0 = juce::Time::getMillisecondCounterHiRes();
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    std::cout << "editor created in " << (juce::Time::getMillisecondCounterHiRes() - t0) << " ms\n";
    ed->setSize (1440, 900);

    // play a chord for half a second so the displays have something to show
    juce::AudioBuffer<float> buf (2, 512);
    for (int b = 0; b < 48; ++b)
    {
        juce::MidiBuffer m;
        if (b == 0) for (int n : { 48, 55, 60, 64, 67 }) m.addEvent (juce::MidiMessage::noteOn (1, n, 0.8f), 0);
        proc.processBlock (buf, m);
    }
    // let the editor timers pick the state up
    for (int i = 0; i < 20; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (20);

    snap (*ed, outDir.getChildFile ("replicant_full.png"), 1.0f);
    ed->setSize (1008, 630);
    for (int i = 0; i < 5; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
    snap (*ed, outDir.getChildFile ("replicant_small.png"), 1.0f);
    ed.reset();
    return 0;
}
