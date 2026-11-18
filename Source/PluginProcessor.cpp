#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"
#include <chrono>

namespace
{
const char* kGroupNames[] = { "I/O", "Autotune", "Rider", "Hollywood", "U47", "Revival", "EQ", "DeEss High", "DeEss Low", "1176", "LA-2A", "SSL EQ", "Maag EQ",
                              "Reverb", "Delay", "NY Comp", "Brightness", "Doubler", "Tube" };

juce::String formatValue (const mk::ParamSpec& s, float v)
{
    const juce::String u (s.unit);
    if (u == "%") return juce::String (juce::roundToInt (v * 100.0f)) + " %";
    if (u == "dB") return juce::String (v, 1) + " dB";
    if (u == "Hz") return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
    if (u == "ms") return (v < 10.0f ? juce::String (v, 2) : juce::String (juce::roundToInt (v))) + " ms";
    if (u == "s") return juce::String (v, 2) + " s";
    if (u == "st") return juce::String (v, 2) + " st";
    if (u == "ct") return juce::String (v, 1) + " ct";
    if (u == "x") return juce::String (v, 2) + " x";
    if (juce::String (s.id) == "fet_att" || juce::String (s.id) == "fet_rel") return juce::String (1.0f + 6.0f * v, 1);
    return juce::String (v, 2);
}
float parseValue (const mk::ParamSpec& s, const juce::String& t)
{
    const juce::String u (s.unit);
    float v = t.retainCharacters ("0123456789.-").getFloatValue();
    if (u == "%") return v / 100.0f;
    if (u == "Hz" && t.containsIgnoreCase ("k")) return v * 1000.0f;
    if (juce::String (s.id) == "fet_att" || juce::String (s.id) == "fet_rel") return (v - 1.0f) / 6.0f;
    return v;
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout MaketeoProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < mk::kNumParams; ++i)
    {
        const auto& s = mk::paramSpecs()[i];
        const String name = String (kGroupNames[s.group]) + " " + s.label;
        const ParameterID pid { s.id, 1 };
        if (s.kind == mk::K_BOOL) layout.add (std::make_unique<AudioParameterBool> (pid, name, s.def > 0.5f));
        else if (s.kind == mk::K_CHOICE) layout.add (std::make_unique<AudioParameterChoice> (pid, name, StringArray::fromTokens (s.choices, "|", ""), (int) s.def));
        else
        {
            NormalisableRange<float> r (s.lo, s.hi);
            if (s.map == mk::M_LOG)
                r = NormalisableRange<float> (s.lo, s.hi, [] (float a, float b, float t) { return a * std::pow (b / a, t); },
                                              [] (float a, float b, float v) { return std::log (v / a) / std::log (b / a); });
            const mk::ParamSpec* sp = &s;
            auto attr = AudioParameterFloatAttributes()
                            .withStringFromValueFunction ([sp] (float v, int) { return formatValue (*sp, v); })
                            .withValueFromStringFunction ([sp] (const String& t) { return parseValue (*sp, t); });
            layout.add (std::make_unique<AudioParameterFloat> (pid, name, r, s.def, attr));
        }
    }
    return layout;
}

MaketeoProcessor::MaketeoProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "MAKETEO", createLayout())
{
    for (int i = 0; i < mk::kNumParams; ++i) raw[i] = apvts.getRawParameterValue (mk::paramSpecs()[i].id);
}

bool MaketeoProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet();
    return (o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo()) && o == l.getMainInputChannelSet();
}

void MaketeoProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    engine.prepare (sampleRate);
    float v[mk::kNumParams];
    for (int i = 0; i < mk::kNumParams; ++i) v[i] = raw[i]->load();
    engine.setParams (v, true);
    reportedLatency = engine.latencySamples();
    setLatencySamples (reportedLatency);
}

void MaketeoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto t0 = std::chrono::steady_clock::now();
    const int nCh = juce::jmin (buffer.getNumChannels(), 2), n = buffer.getNumSamples();

    float v[mk::kNumParams];
    for (int i = 0; i < mk::kNumParams; ++i) v[i] = raw[i]->load (std::memory_order_relaxed);
    engine.setParams (v);
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm()) engine.setTempo (*bpm);

    const int lat = engine.latencySamples();
    if (lat != reportedLatency) { reportedLatency = lat; triggerAsyncUpdate(); } // host is told on the message thread

    for (int c = 0; c < nCh; ++c) pushPeak (inPeak[c], buffer.getMagnitude (c, 0, n));
    engine.process (buffer.getArrayOfWritePointers(), nCh, n);
    for (int c = 0; c < nCh; ++c) pushPeak (outPeak[c], buffer.getMagnitude (c, 0, n));
    if (nCh == 1) { pushPeak (inPeak[1], inPeak[0].load()); pushPeak (outPeak[1], outPeak[0].load()); }

    const double dt = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    const float load = (float) (dt / ((double) n / sr));
    cpuLoad.store (cpuLoad.load (std::memory_order_relaxed) * 0.9f + load * 0.1f, std::memory_order_relaxed);
}

void MaketeoProcessor::copyAnalyzer (float* dst, int n) const
{
    const uint32_t w = engine.anaWrite.load (std::memory_order_acquire);
    for (int i = 0; i < n; ++i) dst[i] = engine.ana[(w - (uint32_t) n + (uint32_t) i) & (mk::Engine::kAnaSize - 1)];
}

// ---------------------------------------------------------------- state and presets
void MaketeoProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void MaketeoProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) { apvts.replaceState (juce::ValueTree::fromXml (*xml)); finishPresetLoad(); }
}
void MaketeoProcessor::finishPresetLoad()
{
    for (auto* id : { "dh_listen", "dl_listen" }) // LISTEN is a monitoring aid: never restored as "on"
        if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (0.0f);
}

juce::StringArray MaketeoProcessor::factoryPresetNames() const
{
    int n; const auto* p = mk::factoryPresets (n); juce::StringArray a; for (int i = 0; i < n; ++i) a.add (p[i].name); return a;
}
void MaketeoProcessor::loadFactoryPreset (int index)
{
    int n; const auto* pr = mk::factoryPresets (n);
    if (index < 0 || index >= n) return;
    float values[mk::kNumParams];
    for (int i = 0; i < mk::kNumParams; ++i) values[i] = mk::paramSpecs()[i].def;
    for (int k = 0; k < pr[index].count; ++k)
        for (int i = 0; i < mk::kNumParams; ++i)
            if (juce::String (mk::paramSpecs()[i].id) == pr[index].values[k].id) values[i] = pr[index].values[k].value;
    undo.beginNewTransaction ("Preset");
    for (int i = 0; i < mk::kNumParams; ++i)
        if (auto* p = apvts.getParameter (mk::paramSpecs()[i].id))
        { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (values[i])); p->endChangeGesture(); }
    apvts.state.setProperty ("presetName", pr[index].name, nullptr);
}
juce::File MaketeoProcessor::presetDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Evric Hunter").getChildFile ("MAKETEO").getChildFile ("Presets");
}
juce::Array<juce::File> MaketeoProcessor::userPresetFiles() const
{
    juce::Array<juce::File> f; presetDir().findChildFiles (f, juce::File::findFiles, false, "*.xml"); f.sort(); return f;
}
bool MaketeoProcessor::saveUserPreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    apvts.state.setProperty ("presetName", clean, nullptr);
    auto dir = presetDir(); if (! dir.createDirectory()) return false;
    if (auto xml = apvts.copyState().createXml()) return xml->writeTo (dir.getChildFile (clean + ".xml"));
    return false;
}
void MaketeoProcessor::loadUserPreset (const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse (f))
        if (xml->hasTagName (apvts.state.getType()))
        { undo.beginNewTransaction ("Preset"); apvts.replaceState (juce::ValueTree::fromXml (*xml)); apvts.state.setProperty ("presetName", f.getFileNameWithoutExtension(), nullptr); finishPresetLoad(); }
}
void MaketeoProcessor::resetAllParameters() { loadFactoryPreset (0); }

juce::AudioProcessorEditor* MaketeoProcessor::createEditor() { return new MaketeoEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MaketeoProcessor(); }
