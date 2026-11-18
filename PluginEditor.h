#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

// MAKETEO visual identity: carbon black, graphite, chrome, matte gold, with violet / blue neon used only for state and metering.
namespace ui
{
    const juce::Colour black { 0xff08090b }, graphite { 0xff17191d }, graphiteHi { 0xff22252b }, chrome { 0xffc7cbd1 }, chromeDim { 0xff7b8088 },
                       gold { 0xffc8a96e }, violet { 0xffa855f7 }, blue { 0xff36c5f0 }, text { 0xffe8eaed }, textDim { 0xff9aa0a8 }, clip { 0xffe5484d };
    juce::Font font (float h, bool bold = false);
    constexpr int kW = 1600, kH = 850;
}

class MkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MkLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool hi, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool hi, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return ui::font (13.0f, true); }
    juce::Font getPopupMenuFont() override { return ui::font (14.0f); }
    juce::Font getLabelFont (juce::Label& l) override { return ui::font ((float) (double) l.getProperties().getWithDefault ("fs", 13.0f), true); }
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};

class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState&, const mk::ParamSpec&, bool large = false);
    void resized() override;
    void paint (juce::Graphics&) override;
private:
    juce::Slider slider; juce::String title; bool large;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class ChoiceBox : public juce::Component
{
public:
    ChoiceBox (juce::AudioProcessorValueTreeState&, const mk::ParamSpec&, bool showTitle = true);
    void resized() override;
    void paint (juce::Graphics&) override;
private:
    juce::ComboBox box; juce::String title; bool showTitle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
};

class ToggleBtn : public juce::ToggleButton
{
public:
    ToggleBtn (juce::AudioProcessorValueTreeState&, const mk::ParamSpec&);
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

// ON LED bound to a "bypass" (inverted) or "on" parameter
class PowerLed : public juce::Component
{
public:
    PowerLed (juce::AudioProcessorValueTreeState&, const mk::ParamSpec&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool isOn() const { return on; }
private:
    bool invert = false, on = false;
    juce::AudioProcessorParameter* param = nullptr;
    std::unique_ptr<juce::ParameterAttachment> att;
};

// thin vertical meter: gain reduction (top-down) or activity level
class Bar : public juce::Component, private juce::Timer
{
public:
    enum Kind { Reduction, Activity };
    Bar (MaketeoProcessor& p, int id, Kind k) : proc (p), meterId (id), kind (k) { startTimerHz (30); }
    ~Bar() override { stopTimer(); }
    void tick();
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { tick(); }
    MaketeoProcessor& proc; int meterId; Kind kind; float v = 0.f;
};

class MeterH : public juce::Component
{
public:
    MeterH (std::atomic<float>* peaks, const juce::String& name, float barH) : src (peaks), title (name), bh (barH) {}
    void tick();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { clip[0] = clip[1] = false; repaint(); }
private:
    std::atomic<float>* src; juce::String title; float bh; float level[2] {}, hold[2] {}; bool clip[2] {};
};

class MaketeoEditor;

class ModuleCard : public juce::Component
{
public:
    ModuleCard (MaketeoEditor&, mk::Group, const juce::String& title, int meterId = -1, Bar::Kind = Bar::Reduction);
    int preferredWidth() const;
    void resized() override;
    void paint (juce::Graphics&) override;
    static constexpr int kHeight = 136, kCell = 60;
private:
    MaketeoEditor& ed; juce::String title; mk::Group group;
    std::unique_ptr<PowerLed> led; juce::TextButton adv { "ADV" };
    juce::OwnedArray<Knob> knobs; juce::OwnedArray<ChoiceBox> choices; juce::OwnedArray<ToggleBtn> toggles;
    bool headerWidgets = false; std::unique_ptr<Bar> bar;
};

class EqGraph : public juce::Component
{
public:
    explicit EqGraph (MaketeoProcessor&);
    int selected = 0; std::function<void (int)> onSelect;
    void tick();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    float xOf (double f) const; float yOf (double g) const; double fOf (float x) const; double gOf (float y) const;
    int hit (juce::Point<float>) const;
    juce::RangedAudioParameter* par (int band, const char* what) const;
    double val (int band, const char* what) const;
    void setVal (int band, const char* what, double plain);
    MaketeoProcessor& proc; int dragging = -1;
    juce::dsp::FFT fft { 11 };
    std::array<float, 2048> snap {}; std::array<float, 4096> fftData {}; std::array<float, 1024> spec {};
};

class EqCard : public juce::Component
{
public:
    explicit EqCard (MaketeoEditor&);
    void tick() { graph.tick(); }
    void resized() override;
    void paint (juce::Graphics&) override;
private:
    void rebuildSelected();
    MaketeoEditor& ed; EqGraph graph; std::unique_ptr<PowerLed> led;
    std::unique_ptr<ChoiceBox> type; std::unique_ptr<ToggleBtn> bandOn; std::unique_ptr<Knob> kf, kg, kq;
};

class AdvPanel : public juce::Component
{
public:
    AdvPanel (MaketeoEditor&, mk::Group, const juce::String& title, std::function<void()> onClose);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override { if (! panel.contains (e.getPosition())) close(); }
private:
    void close() { if (onClose) onClose(); }
    juce::String title; std::function<void()> onClose; juce::Rectangle<int> panel;
    juce::OwnedArray<Knob> knobs; juce::OwnedArray<ChoiceBox> choices; juce::OwnedArray<ToggleBtn> toggles; juce::TextButton closeBtn { "CLOSE" };
};

class MaketeoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MaketeoEditor (MaketeoProcessor&);
    ~MaketeoEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    MaketeoProcessor& proc;
    void openAdv (mk::Group, const juce::String& title);
private:
    void timerCallback() override;
    void refreshPresets();
    void showSettings();
public:
    juce::Rectangle<int> masterBounds;
private:
    void promptSave();
    struct Content : public juce::Component { void paint (juce::Graphics&) override; MaketeoEditor* owner = nullptr; } content;
    MkLookAndFeel laf;
    juce::TooltipWindow tooltip { this, 500 };
    MeterH inMeter, outMeter;
    juce::ComboBox presetBox; juce::TextButton prev { "<" }, next { ">" }, save { "SAVE" }, undoBtn { "UNDO" }, redoBtn { "REDO" }, settings { "SETTINGS" };
    juce::Label cpuLabel;
    juce::OwnedArray<ModuleCard> cards; std::unique_ptr<EqCard> eqCard;
    std::unique_ptr<Knob> inTrim, outTrim; std::unique_ptr<MeterH> bigIn, bigOut;
    std::unique_ptr<AdvPanel> adv; std::unique_ptr<juce::AlertWindow> saveWindow;
    juce::Array<juce::File> userFiles; int numFactory = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaketeoEditor)
};
