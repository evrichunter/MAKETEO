#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

// MAKETEO - modern minimal interface (Evric Hunter identity). Three pages: VOCAL (default), FX and ADVANCED (every original parameter).
class MaketeoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MaketeoEditor (MaketeoProcessor&);
    ~MaketeoEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void openEdit (mk::Group);                    // FX "EDIT" overlay
    MaketeoProcessor& proc;

    struct Impl;                                   // all widgets live in the .cpp
private:
    void timerCallback() override;
    std::unique_ptr<Impl> impl;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaketeoEditor)
};
