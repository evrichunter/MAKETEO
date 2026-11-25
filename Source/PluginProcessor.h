#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/Engine.h"

class MaketeoProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    MaketeoProcessor();
    ~MaketeoProcessor() override { cancelPendingUpdate(); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "MAKETEO By @EVRICHUNTER"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- presets
    juce::StringArray factoryPresetNames() const;
    void loadFactoryPreset (int index);
    juce::Array<juce::File> userPresetFiles() const;
    bool saveUserPreset (const juce::String& name);
    void loadUserPreset (const juce::File&);
    void resetAllParameters();
    juce::String currentPresetName() const { return apvts.state.getProperty ("presetName", "DEFAULT").toString(); }

    // ---- GUI helpers (all lock-free)
    void copyAnalyzer (float* dst, int n) const;
    float meter (int id) const { return engine.meters[id].load (std::memory_order_relaxed); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> inPeak[2] { 0.f, 0.f }, outPeak[2] { 0.f, 0.f }, cpuLoad { 0.f };

private:
    void handleAsyncUpdate() override { setLatencySamples (engine.latencySamples()); }
    void finishPresetLoad();
    static juce::File presetDir();
    mk::Engine engine;
    std::atomic<float>* raw[mk::kNumParams] {};
    int reportedLatency = 0;
    double sr = 48000.0;
    static void pushPeak (std::atomic<float>& a, float v)
    { float c = a.load (std::memory_order_relaxed); while (v > c && ! a.compare_exchange_weak (c, v, std::memory_order_relaxed)) {} }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaketeoProcessor)
};
