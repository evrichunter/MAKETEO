#include "PluginEditor.h"

using juce::Colour; using juce::Graphics; using juce::Rectangle;

juce::Font ui::font (float h, bool bold) { return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)); }

static juce::String shortLabel (const juce::String& l)
{
    static const std::pair<const char*, const char*> m[] = { { "Frequency", "FREQ" }, { "Threshold", "THRESH" }, { "Sensitivity", "SENS" }, { "Detune Left", "DETUNE L" },
        { "Detune Right", "DETUNE R" }, { "Delay Left", "DELAY L" }, { "Delay Right", "DELAY R" }, { "Harmonic Enhancement", "HARMONICS" }, { "Exciter Drive", "DRIVE" },
        { "Tube Character", "CHARACTER" }, { "Peak Reduction", "PEAK RED." }, { "Analog Color", "COLOR" }, { "Retune Speed", "RETUNE" }, { "Target Level", "TARGET" },
        { "Manual Gain", "MANUAL" }, { "Pre-Delay", "PRE-DLY" }, { "Makeup Gain", "MAKEUP" }, { "Stereo Spread", "SPREAD" }, { "Natural Vibrato", "VIBRATO" },
        { "Upward Gain", "MAX UP" }, { "Downward Gain", "MAX DOWN" }, { "Modulation", "MOD" }, { "Saturation", "SATUR." }, { "Stereo Width", "WIDTH" }, { "EQ Amount", "EQ AMT" } };
    for (auto& p : m) if (l == p.first) return p.second;
    return l.toUpperCase();
}
static bool endsWith (const char* s, const char* suf) { return juce::String (s).endsWith (suf); }
static int textW (const juce::Font& f, const juce::String& t) { return juce::GlyphArrangement::getStringWidthInt (f, t); }

// ================================================================== look and feel
MkLookAndFeel::MkLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, ui::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, ui::gold.withAlpha (0.4f));
    setColour (juce::Label::textColourId, ui::text);
    setColour (juce::TextEditor::backgroundColourId, ui::graphiteHi);
    setColour (juce::TextEditor::textColourId, ui::text);
    setColour (juce::TextEditor::outlineColourId, ui::gold);
    setColour (juce::TextEditor::focusedOutlineColourId, ui::gold);
    setColour (juce::CaretComponent::caretColourId, ui::gold);
    setColour (juce::ComboBox::textColourId, ui::text);
    setColour (juce::PopupMenu::backgroundColourId, ui::graphite);
    setColour (juce::PopupMenu::textColourId, ui::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::gold.withAlpha (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, ui::text);
    setColour (juce::AlertWindow::backgroundColourId, ui::graphite);
    setColour (juce::AlertWindow::textColourId, ui::text);
}

juce::Label* MkLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = juce::LookAndFeel_V4::createSliderTextBox (s);
    l->getProperties().set ("fs", s.getProperties().getWithDefault ("fs", 13.0f));
    l->setJustificationType (juce::Justification::centred);
    return l;
}

void MkLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float d = juce::jmin (area.getWidth(), area.getHeight()) - 4.0f;
    auto b = Rectangle<float> (d, d).withCentre (area.getCentre());
    const float ang = a0 + pos * (a1 - a0), arcW = juce::jmax (2.5f, d * 0.055f);
    const bool hot = s.isMouseOverOrDragging();

    auto ringB = b.reduced (arcW * 0.5f);
    juce::Path track, val;
    track.addCentredArc (ringB.getCentreX(), ringB.getCentreY(), ringB.getWidth() / 2, ringB.getHeight() / 2, 0, a0, a1, true);
    val.addCentredArc (ringB.getCentreX(), ringB.getCentreY(), ringB.getWidth() / 2, ringB.getHeight() / 2, 0, a0, ang, true);
    g.setColour (juce::Colour (0xff2a2d33));
    g.strokePath (track, juce::PathStrokeType (arcW, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    if (hot) { g.setColour (ui::violet.withAlpha (0.22f)); g.strokePath (val, juce::PathStrokeType (arcW + 5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
    g.setColour (hot ? ui::gold.brighter (0.25f) : ui::gold);
    g.strokePath (val, juce::PathStrokeType (arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto body = b.reduced (arcW + d * 0.07f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (body.translated (0, d * 0.03f).expanded (1.0f));
    g.setGradientFill (juce::ColourGradient (ui::chrome, body.getX(), body.getY(), juce::Colour (0xff3a3d44), body.getRight(), body.getBottom(), false));
    g.fillEllipse (body);                                          // chrome bezel
    auto cap = body.reduced (d * 0.07f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b2e35), cap.getX(), cap.getY(), juce::Colour (0xff0e0f12), cap.getRight(), cap.getBottom(), false));
    g.fillEllipse (cap);                                           // graphite cap
    g.setColour (ui::chrome.withAlpha (0.12f));
    g.drawEllipse (cap.reduced (d * 0.05f), 1.0f);

    const auto c = cap.getCentre(); const float rad = cap.getWidth() * 0.5f, sn = std::sin (ang), cs = -std::cos (ang);
    g.setColour (ui::text);
    g.drawLine (c.x + sn * rad * 0.35f, c.y + cs * rad * 0.35f, c.x + sn * rad * 0.9f, c.y + cs * rad * 0.9f, juce::jmax (2.0f, d * 0.04f));
}

void MkLookAndFeel::drawToggleButton (Graphics& g, juce::ToggleButton& b, bool hi, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f); const bool on = b.getToggleState();
    g.setColour (on ? ui::gold : (hi ? ui::graphiteHi : ui::graphite)); g.fillRoundedRectangle (r, 4.0f);
    g.setColour (on ? ui::gold.brighter (0.3f) : (hi ? ui::chrome : ui::chromeDim.withAlpha (0.6f))); g.drawRoundedRectangle (r, 4.0f, 1.0f);
    g.setColour (on ? ui::black : ui::text); g.setFont (ui::font (12.0f, true).withExtraKerningFactor (0.05f));
    g.drawFittedText (b.getButtonText().toUpperCase(), r.toNearestInt().reduced (2, 0), juce::Justification::centred, 1, 0.7f);
}

void MkLookAndFeel::drawButtonBackground (Graphics& g, juce::Button& b, const Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (down ? ui::gold.withAlpha (0.35f) : hi ? ui::graphiteHi : ui::graphite); g.fillRoundedRectangle (r, 4.0f);
    g.setColour (hi ? ui::gold : ui::chromeDim.withAlpha (0.7f)); g.drawRoundedRectangle (r, 4.0f, 1.0f);
}
void MkLookAndFeel::drawButtonText (Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour (ui::text); g.setFont (ui::font (12.0f, true).withExtraKerningFactor (0.06f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (2, 0), juce::Justification::centred, 1, 0.7f);
}
void MkLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    g.setColour (ui::graphiteHi); g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.hasKeyboardFocus (true) ? ui::gold : ui::chromeDim.withAlpha (0.7f)); g.drawRoundedRectangle (r, 4.0f, 1.0f);
    juce::Path p; const float cx = (float) w - 12.0f, cy = (float) h * 0.5f;
    p.addTriangle (cx - 4, cy - 2, cx + 4, cy - 2, cx, cy + 3); g.setColour (ui::gold); g.fillPath (p);
}
void MkLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& l)
{
    l.setBounds (6, 1, box.getWidth() - 24, box.getHeight() - 2); l.setFont (getComboBoxFont (box));
}

// ================================================================== controls
Knob::Knob (juce::AudioProcessorValueTreeState& a, const mk::ParamSpec& s, bool big) : title (shortLabel (s.label)), large (big)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.getProperties().set ("fs", big ? 16.0f : 13.0f);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, big ? 24 : 20);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    slider.setMouseDragSensitivity (200);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, s.id, slider);
    slider.setTextBoxIsEditable (true);
}
void Knob::resized() { slider.setBounds (getLocalBounds().withTrimmedTop (large ? 22 : 17)); }
void Knob::paint (Graphics& g)
{
    g.setColour (large ? ui::text : ui::textDim); g.setFont (ui::font (large ? 14.0f : 11.5f, true).withExtraKerningFactor (0.02f));
    g.drawFittedText (title, getLocalBounds().removeFromTop (large ? 22 : 17), juce::Justification::centred, 1, 0.65f);
}

ChoiceBox::ChoiceBox (juce::AudioProcessorValueTreeState& a, const mk::ParamSpec& s, bool st) : title (juce::String (s.label).toUpperCase()), showTitle (st)
{
    box.addItemList (juce::StringArray::fromTokens (s.choices, "|", ""), 1);
    box.setTooltip (s.label);
    addAndMakeVisible (box);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, s.id, box);
}
void ChoiceBox::resized() { box.setBounds (getLocalBounds().withTrimmedTop (showTitle ? 16 : 0)); }
void ChoiceBox::paint (Graphics& g)
{
    if (! showTitle) return;
    g.setColour (ui::textDim); g.setFont (ui::font (11.5f, true).withExtraKerningFactor (0.05f));
    g.drawFittedText (title, getLocalBounds().removeFromTop (15), juce::Justification::centredLeft, 1, 0.7f);
}

ToggleBtn::ToggleBtn (juce::AudioProcessorValueTreeState& a, const mk::ParamSpec& s)
{
    setButtonText (s.label); setClickingTogglesState (true); setTooltip (s.label);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (a, s.id, *this);
}

PowerLed::PowerLed (juce::AudioProcessorValueTreeState& a, const mk::ParamSpec& s) : invert (endsWith (s.id, "_byp")), param (a.getParameter (s.id))
{
    att = std::make_unique<juce::ParameterAttachment> (*dynamic_cast<juce::RangedAudioParameter*> (param),
                                                       [this] (float v) { on = invert ? v < 0.5f : v > 0.5f; repaint(); }, a.undoManager);
    att->sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}
void PowerLed::mouseDown (const juce::MouseEvent&) { att->setValueAsCompleteGesture (on ? (invert ? 1.0f : 0.0f) : (invert ? 0.0f : 1.0f)); }
void PowerLed::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    if (on) { for (int i = 3; i >= 1; --i) { g.setColour (ui::violet.withAlpha (0.10f)); g.fillEllipse (r.expanded ((float) i * 1.5f)); } }
    g.setColour (on ? ui::violet : juce::Colour (0xff25272c)); g.fillEllipse (r);
    g.setColour (on ? ui::violet.brighter (0.5f) : ui::chromeDim.withAlpha (0.6f)); g.drawEllipse (r, 1.2f);
    if (on) { g.setColour (juce::Colours::white.withAlpha (0.55f)); g.fillEllipse (r.reduced (r.getWidth() * 0.32f).translated (-1.0f, -1.0f)); }
}

void Bar::tick()
{
    const float raw = proc.meter (meterId);
    float t = kind == Reduction ? juce::jlimit (0.0f, 1.0f, raw / 18.0f)
                                : juce::jmap (juce::jlimit (-60.0f, 0.0f, juce::Decibels::gainToDecibels (raw, -60.0f)), -60.0f, 0.0f, 0.0f, 1.0f);
    v = t > v ? t : v * 0.88f + t * 0.12f; repaint();
}
void Bar::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0e0f12)); g.fillRoundedRectangle (r, 3.0f);
    g.setColour (ui::chromeDim.withAlpha (0.35f)); g.drawRoundedRectangle (r, 3.0f, 1.0f);
    auto in = r.reduced (2.0f);
    if (kind == Reduction) { auto f = in.removeFromTop (in.getHeight() * v); g.setColour (ui::blue); g.fillRoundedRectangle (f, 2.0f); }
    else
    {
        auto f = in.removeFromBottom (in.getHeight() * v);
        g.setGradientFill (juce::ColourGradient (ui::violet, f.getX(), in.getY(), ui::blue, f.getX(), in.getBottom(), false)); g.fillRoundedRectangle (f, 2.0f);
    }
}

void MeterH::tick()
{
    for (int c = 0; c < 2; ++c)
    {
        const float pk = src[c].exchange (0.0f);
        const float v = juce::jmap (juce::jlimit (-60.0f, 0.0f, juce::Decibels::gainToDecibels (pk, -60.0f)), -60.0f, 0.0f, 0.0f, 1.0f);
        level[c] = v > level[c] ? v : level[c] * 0.86f + v * 0.14f; hold[c] = v > hold[c] ? v : juce::jmax (0.0f, hold[c] - 0.004f);
        if (pk >= 1.0f) clip[c] = true;
    }
    repaint();
}
void MeterH::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (ui::textDim); g.setFont (ui::font (12.0f, true).withExtraKerningFactor (0.08f));
    g.drawText (title, r.removeFromLeft (34.0f).toNearestInt(), juce::Justification::centredLeft);
    const float gap = 5.0f; float y = r.getCentreY() - bh - gap * 0.5f;
    for (int c = 0; c < 2; ++c, y += bh + gap)
    {
        auto bar = Rectangle<float> (r.getX(), y, r.getWidth() - 14.0f, bh);
        g.setColour (juce::Colour (0xff0e0f12)); g.fillRoundedRectangle (bar, 2.5f);
        auto fill = bar.withWidth (bar.getWidth() * level[c]);
        g.setGradientFill (juce::ColourGradient (ui::blue, bar.getX(), 0, ui::violet, bar.getRight(), 0, false));
        if (level[c] > 0.97f) g.setColour (ui::clip);
        g.fillRoundedRectangle (fill, 2.5f);
        if (hold[c] > 0.02f) { g.setColour (ui::text); g.fillRect (bar.getX() + bar.getWidth() * hold[c] - 1.0f, bar.getY(), 2.0f, bh); }
        g.setColour (clip[c] ? ui::clip : juce::Colour (0xff2a2d33)); g.fillEllipse (bar.getRight() + 4.0f, bar.getCentreY() - 4.0f, 8.0f, 8.0f);
    }
}

// ================================================================== module card
ModuleCard::ModuleCard (MaketeoEditor& e, mk::Group g, const juce::String& t, int meterId, Bar::Kind kind) : ed (e), title (t), group (g)
{
    int nHeader = 0; std::vector<const mk::ParamSpec*> fl, ch, bo;
    for (int i = 0; i < mk::kNumParams; ++i)
    {
        const auto& s = mk::paramSpecs()[i];
        if (s.group != g || ! s.primary) continue;
        if (! led && (endsWith (s.id, "_byp") || (endsWith (s.id, "_on") && ! juce::String (s.id).startsWith ("eq")))) { led = std::make_unique<PowerLed> (e.proc.apvts, s); continue; }
        (s.kind == mk::K_FLOAT ? fl : s.kind == mk::K_CHOICE ? ch : bo).push_back (&s);
    }
    nHeader = (int) (ch.size() + bo.size()); headerWidgets = nHeader <= 2;
    for (auto* s : fl) { knobs.add (new Knob (e.proc.apvts, *s)); addAndMakeVisible (knobs.getLast()); }
    for (auto* s : ch) { choices.add (new ChoiceBox (e.proc.apvts, *s, false)); addAndMakeVisible (choices.getLast()); }
    for (auto* s : bo) { toggles.add (new ToggleBtn (e.proc.apvts, *s)); addAndMakeVisible (toggles.getLast()); }
    if (led) addAndMakeVisible (*led);
    addAndMakeVisible (adv); adv.onClick = [this] { ed.openAdv (group, title); };
    if (meterId >= 0) { bar = std::make_unique<Bar> (e.proc, meterId, kind); addAndMakeVisible (*bar); }
}

int ModuleCard::preferredWidth() const
{
    const int col = headerWidgets ? 0 : 92;
    int w = 16 + col + knobs.size() * kCell + (bar ? 16 : 0);
    int hdr = 34 + textW (ui::font (14.0f, true).withExtraKerningFactor (0.06f), title) + 48;
    if (headerWidgets) hdr += choices.size() * 88 + toggles.size() * 80;
    return juce::jmax (w, hdr + 8);
}

void ModuleCard::resized()
{
    auto r = getLocalBounds();
    if (led) led->setBounds (8, 7, 16, 16);
    adv.setBounds (r.getWidth() - 46, 6, 38, 18);
    int hx = r.getWidth() - 52;
    if (headerWidgets)
    {
        for (auto* t : toggles) { hx -= 78; t->setBounds (hx, 5, 76, 20); hx -= 4; }
        for (auto* c : choices) { hx -= 86; c->setBounds (hx, 4, 84, 22); hx -= 4; }
    }
    int col = 0;
    if (! headerWidgets)
    {
        col = 92; int y = 34;
        for (auto* c : choices) { c->setBounds (8, y, 84, 28); y += 32; }
        for (auto* t : toggles) { t->setBounds (8, y, 84, 26); y += 30; }
    }
    int x = 8 + col; const int h = r.getHeight() - 34;
    for (auto* k : knobs) { k->setBounds (x, 32, kCell, h); x += kCell; }
    if (bar) bar->setBounds (r.getWidth() - 14, 34, 7, r.getHeight() - 44);
}

void ModuleCard::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1d2025), 0, 0, juce::Colour (0xff121418), 0, r.getHeight(), false));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (ui::chrome.withAlpha (0.16f)); g.drawRoundedRectangle (r, 7.0f, 1.0f);
    g.setColour (ui::chrome.withAlpha (0.10f)); g.drawLine (8, 1.5f, r.getWidth() - 8, 1.5f);
    g.setColour (ui::chromeDim.withAlpha (0.5f));
    for (auto p : { juce::Point<float> (6, r.getHeight() - 6), juce::Point<float> (r.getWidth() - 6, r.getHeight() - 6) }) // screws
    { g.fillEllipse (p.x - 2.5f, p.y - 2.5f, 5.0f, 5.0f); g.setColour (ui::black); g.drawLine (p.x - 1.7f, p.y + 1.0f, p.x + 1.7f, p.y - 1.0f, 1.0f); g.setColour (ui::chromeDim.withAlpha (0.5f)); }
    const bool on = led ? led->isOn() : true;
    g.setColour (on ? ui::chrome : ui::textDim); g.setFont (ui::font (14.0f, true).withExtraKerningFactor (0.06f));
    g.drawText (title, Rectangle<int> (30, 5, headerWidgets ? juce::jmax (40, getWidth() - 30 - 52 - choices.size() * 90 - toggles.size() * 82) : getWidth() - 80, 20), juce::Justification::centredLeft);
}

// ================================================================== EQ graph (analyzer + curve + draggable nodes)
EqGraph::EqGraph (MaketeoProcessor& p) : proc (p) { setWantsKeyboardFocus (false); }

static juce::Rectangle<float> plotOf (const juce::Component& c) { return c.getLocalBounds().toFloat().withTrimmedLeft (32).withTrimmedBottom (16).reduced (4, 4); }
float EqGraph::xOf (double f) const { auto p = plotOf (*this); return p.getX() + (float) (std::log (f / 20.0) / std::log (1000.0)) * p.getWidth(); }
double EqGraph::fOf (float x) const { auto p = plotOf (*this); return 20.0 * std::pow (1000.0, juce::jlimit (0.0, 1.0, (double) ((x - p.getX()) / p.getWidth()))); }
float EqGraph::yOf (double g) const { auto p = plotOf (*this); return p.getCentreY() - (float) (g / 24.0) * p.getHeight() * 0.5f; }
double EqGraph::gOf (float y) const { auto p = plotOf (*this); return juce::jlimit (-24.0, 24.0, (double) ((p.getCentreY() - y) / (p.getHeight() * 0.5f)) * 24.0); }

juce::RangedAudioParameter* EqGraph::par (int b, const char* what) const
{ return dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter ("eq" + juce::String (b + 1) + "_" + what)); }
double EqGraph::val (int b, const char* what) const { auto* p = par (b, what); return p->convertFrom0to1 (p->getValue()); }
void EqGraph::setVal (int b, const char* what, double v) { auto* p = par (b, what); p->setValueNotifyingHost (p->convertTo0to1 ((float) v)); }

int EqGraph::hit (juce::Point<float> pt) const
{
    int best = -1; float bd = 12.0f;
    for (int b = 0; b < 8; ++b)
    {
        const int type = (int) val (b, "type");
        const float x = xOf (val (b, "freq")), y = yOf (type <= 2 ? val (b, "gain") : 0.0);
        const float d = pt.getDistanceFrom ({ x, y }); if (d < bd) { bd = d; best = b; }
    }
    return best;
}

void EqGraph::tick()
{
    proc.copyAnalyzer (snap.data(), 2048);
    for (int i = 0; i < 2048; ++i) fftData[(size_t) i] = snap[(size_t) i] * (0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / 2047.0f));
    std::fill (fftData.begin() + 2048, fftData.end(), 0.0f);
    fft.performFrequencyOnlyForwardTransform (fftData.data());
    for (int i = 0; i < 1024; ++i)
    {
        const float db = juce::Decibels::gainToDecibels (fftData[(size_t) i] / 512.0f, -100.0f);
        spec[(size_t) i] = db > spec[(size_t) i] ? db : spec[(size_t) i] * 0.85f + db * 0.15f;
    }
    repaint();
}

void EqGraph::paint (Graphics& g)
{
    auto p = plotOf (*this);
    g.setColour (juce::Colour (0xff0b0c0f)); g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
    g.setColour (ui::chromeDim.withAlpha (0.35f)); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
    g.setFont (ui::font (10.5f, true));
    for (double f : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
    {
        const float x = xOf (f); g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawVerticalLine ((int) x, p.getY(), p.getBottom());
        g.setColour (ui::textDim); g.drawText (f >= 1000 ? juce::String (f / 1000.0, f >= 10000 ? 0 : 0) + "k" : juce::String ((int) f), (int) x - 16, (int) p.getBottom() + 2, 32, 12, juce::Justification::centred);
    }
    for (int dB = -18; dB <= 18; dB += 6)
    {
        const float y = yOf (dB); g.setColour (juce::Colours::white.withAlpha (dB == 0 ? 0.16f : 0.06f)); g.drawHorizontalLine ((int) y, p.getX(), p.getRight());
        g.setColour (ui::textDim); g.drawText (juce::String (dB), 0, (int) y - 6, 28, 12, juce::Justification::centredRight);
    }
    // spectrum
    const double fs = proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000.0;
    juce::Path sp; sp.startNewSubPath (p.getX(), p.getBottom());
    for (int i = 1; i < 1024; ++i)
    {
        const double f = i * fs / 2048.0; if (f < 20.0 || f > 20000.0) continue;
        sp.lineTo (xOf (f), juce::jmap (juce::jlimit (-90.0f, -6.0f, spec[(size_t) i]), -90.0f, -6.0f, p.getBottom(), p.getY() + 4));
    }
    sp.lineTo (p.getRight(), p.getBottom()); sp.closeSubPath();
    g.setGradientFill (juce::ColourGradient (ui::violet.withAlpha (0.42f), 0, p.getY(), ui::blue.withAlpha (0.05f), 0, p.getBottom(), false)); g.fillPath (sp);

    // curve
    juce::Path curve; bool first = true; float lastY = p.getCentreY();
    for (int i = 0; i <= 240; ++i)
    {
        const double f = 20.0 * std::pow (1000.0, i / 240.0); double dB = 0.0;
        for (int b = 0; b < 8; ++b)
        {
            if (val (b, "on") < 0.5) continue;
            const int type = (int) val (b, "type"); const double gn = val (b, "gain");
            if (type <= 2 && std::fabs (gn) < 0.005) continue;
            dB += mk::filt::magDb (mk::ParamEQ8::coefFor (fs, type, val (b, "freq"), gn, val (b, "q")), fs, f);
        }
        const float x = xOf (f), y = yOf (juce::jlimit (-24.0, 24.0, dB)); lastY = y;
        if (first) { curve.startNewSubPath (x, y); first = false; } else curve.lineTo (x, y);
    }
    (void) lastY;
    g.setColour (ui::chrome); g.strokePath (curve, juce::PathStrokeType (2.0f));

    for (int b = 0; b < 8; ++b)
    {
        const bool on = val (b, "on") > 0.5; const int type = (int) val (b, "type");
        const float x = xOf (val (b, "freq")), y = yOf (type <= 2 ? val (b, "gain") : 0.0);
        if (b == selected) { g.setColour (ui::violet.withAlpha (0.35f)); g.fillEllipse (x - 12, y - 12, 24, 24); }
        g.setColour (on ? ui::gold : ui::graphite); g.fillEllipse (x - 8, y - 8, 16, 16);
        g.setColour (on ? ui::black : ui::chromeDim); g.drawEllipse (x - 8, y - 8, 16, 16, 1.5f);
        g.setColour (on ? ui::black : ui::textDim); g.setFont (ui::font (11.0f, true)); g.drawText (juce::String (b + 1), (int) x - 8, (int) y - 8, 16, 16, juce::Justification::centred);
    }
}

void EqGraph::mouseDown (const juce::MouseEvent& e)
{
    dragging = hit (e.position);
    if (dragging >= 0)
    {
        selected = dragging; if (onSelect) onSelect (selected);
        par (dragging, "freq")->beginChangeGesture(); par (dragging, "gain")->beginChangeGesture();
        if (val (dragging, "on") < 0.5) setVal (dragging, "on", 1.0);
    }
}
void EqGraph::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0) return;
    setVal (dragging, "freq", juce::jlimit (20.0, 20000.0, fOf (e.position.x)));
    if ((int) val (dragging, "type") <= 2) setVal (dragging, "gain", gOf (e.position.y));
}
void EqGraph::mouseUp (const juce::MouseEvent&)
{
    if (dragging >= 0) { par (dragging, "freq")->endChangeGesture(); par (dragging, "gain")->endChangeGesture(); }
    dragging = -1;
}
void EqGraph::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int b = hit (e.position);
    if (b >= 0) { auto* p = par (b, "on"); p->beginChangeGesture(); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); p->endChangeGesture(); }
}
void EqGraph::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    int b = hit (e.position); if (b < 0) b = selected;
    auto* p = par (b, "q"); p->beginChangeGesture();
    setVal (b, "q", juce::jlimit (0.1, 18.0, val (b, "q") * std::pow (1.18, (double) w.deltaY * 4.0)));
    p->endChangeGesture();
}

// ================================================================== EQ card
EqCard::EqCard (MaketeoEditor& e) : ed (e), graph (e.proc)
{
    const mk::ParamSpec* byp = nullptr;
    for (int i = 0; i < mk::kNumParams; ++i) if (juce::String (mk::paramSpecs()[i].id) == "eq_byp") byp = &mk::paramSpecs()[i];
    led = std::make_unique<PowerLed> (e.proc.apvts, *byp);
    addAndMakeVisible (*led); addAndMakeVisible (graph);
    graph.onSelect = [this] (int) { rebuildSelected(); resized(); };
    rebuildSelected();
}
void EqCard::rebuildSelected()
{
    const int n = graph.selected + 1; auto spec = [&] (const char* w) -> const mk::ParamSpec& {
        const juce::String id = "eq" + juce::String (n) + "_" + w;
        for (int i = 0; i < mk::kNumParams; ++i) if (id == mk::paramSpecs()[i].id) return mk::paramSpecs()[i];
        return mk::paramSpecs()[0]; };
    type.reset(); bandOn.reset(); kf.reset(); kg.reset(); kq.reset();
    type = std::make_unique<ChoiceBox> (ed.proc.apvts, spec ("type")); bandOn = std::make_unique<ToggleBtn> (ed.proc.apvts, spec ("on"));
    bandOn->setButtonText ("BAND " + juce::String (n) + " ON");
    kf = std::make_unique<Knob> (ed.proc.apvts, spec ("freq")); kg = std::make_unique<Knob> (ed.proc.apvts, spec ("gain")); kq = std::make_unique<Knob> (ed.proc.apvts, spec ("q"));
    for (auto* c : std::initializer_list<juce::Component*> { type.get(), bandOn.get(), kf.get(), kg.get(), kq.get() }) addAndMakeVisible (*c);
    ed.sendLookAndFeelChange();
}
void EqCard::resized()
{
    auto r = getLocalBounds();
    led->setBounds (8, 7, 16, 16); graph.setBounds (8, 32, r.getWidth() - 16, 148);
    const int y = 186;
    type->setBounds (12, y, 120, 40); bandOn->setBounds (12, y + 46, 120, 26);
    int x = 146; for (auto* k : { kf.get(), kg.get(), kq.get() }) { k->setBounds (x, y - 4, 84, 96); x += 92; }
}
void EqCard::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1d2025), 0, 0, juce::Colour (0xff121418), 0, r.getHeight(), false)); g.fillRoundedRectangle (r, 7.0f);
    g.setColour (ui::chrome.withAlpha (0.16f)); g.drawRoundedRectangle (r, 7.0f, 1.0f);
    g.setColour (led->isOn() ? ui::chrome : ui::textDim); g.setFont (ui::font (14.0f, true).withExtraKerningFactor (0.06f));
    g.drawText ("PARAMETRIC EQ  8 BAND", 30, 5, 220, 20, juce::Justification::centredLeft);
    g.setColour (ui::textDim); g.setFont (ui::font (11.0f)); g.drawText ("drag nodes | wheel = Q | double-click = on/off", getWidth() - 270, 6, 258, 18, juce::Justification::centredRight);
}

// ================================================================== ADV panel
AdvPanel::AdvPanel (MaketeoEditor& e, mk::Group g, const juce::String& t, std::function<void()> oc) : title (t), onClose (std::move (oc))
{
    for (int i = 0; i < mk::kNumParams; ++i)
    {
        const auto& s = mk::paramSpecs()[i];
        if (s.group != g) continue;
        if (endsWith (s.id, "_byp") || (endsWith (s.id, "_on") && ! juce::String (s.id).startsWith ("at_n"))) continue;
        if (s.kind == mk::K_FLOAT) knobs.add (new Knob (e.proc.apvts, s));
        else if (s.kind == mk::K_CHOICE) choices.add (new ChoiceBox (e.proc.apvts, s));
        else toggles.add (new ToggleBtn (e.proc.apvts, s));
    }
    for (auto* k : knobs) addAndMakeVisible (*k);
    for (auto* c : choices) addAndMakeVisible (*c);
    for (auto* b : toggles) addAndMakeVisible (*b);
    addAndMakeVisible (closeBtn); closeBtn.onClick = [this] { close(); };
}
void AdvPanel::resized()
{
    const int maxW = 1000, pad = 24;
    int x = pad, y = 64, rowH = 0, usedW = 0;
    auto place = [&] (juce::Component& c, int w, int h) { if (x + w > maxW + pad) { x = pad; y += rowH + 8; rowH = 0; } c.setBounds (x, y, w, h); x += w + 8; rowH = juce::jmax (rowH, h); usedW = juce::jmax (usedW, x); };
    for (auto* c : choices) place (*c, 130, 52);
    for (auto* b : toggles) place (*b, 110, 28);
    if (! choices.isEmpty() || ! toggles.isEmpty()) { x = pad; y += rowH + 14; rowH = 0; }
    for (auto* k : knobs) place (*k, 84, 112);
    const int pw = juce::jmax (420, usedW + pad), ph = y + rowH + 28;
    panel = Rectangle<int> (0, 0, pw, ph).withCentre (getLocalBounds().getCentre());
    for (auto* c : getChildren()) if (c != &closeBtn) c->setTopLeftPosition (c->getPosition() + panel.getTopLeft());
    closeBtn.setBounds (panel.getRight() - 90, panel.getY() + 14, 76, 26);
}
void AdvPanel::paint (Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.62f));
    g.setColour (ui::graphite); g.fillRoundedRectangle (panel.toFloat(), 10.0f);
    g.setColour (ui::gold.withAlpha (0.6f)); g.drawRoundedRectangle (panel.toFloat().reduced (0.5f), 10.0f, 1.2f);
    g.setColour (ui::chrome); g.setFont (ui::font (18.0f, true).withExtraKerningFactor (0.08f));
    g.drawText (title + "  -  ADVANCED", panel.getX() + 24, panel.getY() + 14, 500, 26, juce::Justification::centredLeft);
}

// ================================================================== editor
void MaketeoEditor::Content::paint (Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0d0e11), 0, 0, ui::black, 0, (float) ui::kH, false)); g.fillAll();
    // brushed-metal header
    g.setColour (ui::graphite); g.fillRect (0, 0, ui::kW, 64);
    g.setColour (juce::Colours::white.withAlpha (0.025f)); for (int y = 0; y < 64; y += 2) g.drawHorizontalLine (y, 0.0f, (float) ui::kW);
    g.setColour (ui::gold.withAlpha (0.55f)); g.fillRect (0, 64, ui::kW, 1);
    g.setGradientFill (juce::ColourGradient (ui::text, 0, 8, ui::chromeDim, 0, 44, false));
    g.setFont (juce::Font (juce::FontOptions ("Impact", 38.0f, juce::Font::plain)).withExtraKerningFactor (0.06f));
    g.drawText ("MAKETEO", 22, 4, 260, 44, juce::Justification::centredLeft);
    g.setColour (ui::gold); g.setFont (ui::font (12.0f, true).withExtraKerningFactor (0.3f));
    g.drawText ("BY @EVRICHUNTER", 24, 44, 260, 16, juce::Justification::centredLeft);
    g.setFont (ui::font (12.0f, true).withExtraKerningFactor (0.3f));
    g.drawText ("MAIN VOCAL ENGINE", 18, 70, 300, 16, juce::Justification::centredLeft);
    g.drawText ("PARALLEL PROCESSING ENGINE", 18, 526, 400, 16, juce::Justification::centredLeft);
    g.setColour (ui::gold.withAlpha (0.25f)); g.fillRect (240, 78, ui::kW - 258, 1); g.fillRect (330, 534, ui::kW - 348, 1);
    if (owner) // master panel
    {
        auto r = owner->masterBounds.toFloat();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1d2025), 0, r.getY(), juce::Colour (0xff121418), 0, r.getBottom(), false)); g.fillRoundedRectangle (r, 7.0f);
        g.setColour (ui::chrome.withAlpha (0.16f)); g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);
        g.setColour (ui::chrome); g.setFont (ui::font (14.0f, true).withExtraKerningFactor (0.06f)); g.drawText ("MASTER", (int) r.getX() + 16, (int) r.getY() + 5, 200, 20, juce::Justification::centredLeft);
    }
}

MaketeoEditor::MaketeoEditor (MaketeoProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), inMeter (p.inPeak, "IN", 7.0f), outMeter (p.outPeak, "OUT", 7.0f)
{
    content.owner = this;
    addAndMakeVisible (content); content.setLookAndFeel (&laf);
    auto addC = [this] (juce::Component& c) { content.addAndMakeVisible (c); };

    auto card = [&] (mk::Group g, const char* title, int meter = -1, Bar::Kind k = Bar::Reduction) { auto* c = new ModuleCard (*this, g, title, meter, k); cards.add (c); addC (*c); return c; };
    auto* at = card (mk::G_AT, "AUTOTUNE PRO"); auto* rd = card (mk::G_RD, "VOCAL RIDER"); auto* hw = card (mk::G_HW, "HOLLYWOOD PREAMP");
    auto* u47 = card (mk::G_U47, "U47 MIC"); auto* rv = card (mk::G_RV, "REVIVAL");
    auto* dh = card (mk::G_DH, "DE-ESS HIGH", mk::MT_DH); auto* dl = card (mk::G_DL, "DE-ESS LOW", mk::MT_DL); auto* la = card (mk::G_LA, "LA-2A", mk::MT_LA);
    auto* fet = card (mk::G_FET, "1176 REV A", mk::MT_FET); auto* ssl = card (mk::G_SSL, "SSL 4000 J EQ"); auto* mg = card (mk::G_MG, "MAAG EQ4");
    auto* rvb = card (mk::G_RVB, "REVERB", mk::MT_PAR0 + 0, Bar::Activity); auto* dly = card (mk::G_DLY, "DELAY", mk::MT_PAR0 + 1, Bar::Activity);
    auto* ny = card (mk::G_NY, "NEW YORK", mk::MT_PAR0 + 2, Bar::Activity); auto* br = card (mk::G_BR, "BRIGHTNESS", mk::MT_PAR0 + 3, Bar::Activity);
    auto* dbl = card (mk::G_DBL, "DOUBLER", mk::MT_PAR0 + 4, Bar::Activity); auto* tb = card (mk::G_TB, "TUBE", mk::MT_PAR0 + 5, Bar::Activity);
    eqCard = std::make_unique<EqCard> (*this); addC (*eqCard);

    const int H = ModuleCard::kHeight, gap = 8;
    auto row = [&] (std::initializer_list<ModuleCard*> list, int x, int y) { for (auto* c : list) { c->setBounds (x, y, c->preferredWidth(), H); x += c->getWidth() + gap; } return x; };
    row ({ at, rd, hw, u47, rv }, 16, 92);
    eqCard->setBounds (16, 92 + H + gap, 430, H * 2 + gap);
    row ({ dh, dl, la, fet }, 454, 92 + H + gap);
    row ({ ssl, mg }, 454, 92 + 2 * (H + gap));
    row ({ rvb, dly, ny, br }, 16, 548);
    const int x5 = row ({ dbl, tb }, 16, 548 + H + gap);
    masterBounds = Rectangle<int> (x5, 548 + H + gap, ui::kW - 16 - x5, H);

    for (int i = 0; i < mk::kNumParams; ++i)
    {
        if (juce::String (mk::paramSpecs()[i].id) == "in_trim") inTrim = std::make_unique<Knob> (proc.apvts, mk::paramSpecs()[i]);
        if (juce::String (mk::paramSpecs()[i].id) == "out_trim") outTrim = std::make_unique<Knob> (proc.apvts, mk::paramSpecs()[i]);
    }
    bigIn = std::make_unique<MeterH> (proc.inPeak, "IN", 12.0f); bigOut = std::make_unique<MeterH> (proc.outPeak, "OUT", 12.0f);
    for (auto* c : std::initializer_list<juce::Component*> { inTrim.get(), outTrim.get(), bigIn.get(), bigOut.get() }) addC (*c);
    inTrim->setBounds (masterBounds.getX() + 16, masterBounds.getY() + 28, 84, 104); outTrim->setBounds (masterBounds.getX() + 106, masterBounds.getY() + 28, 84, 104);
    bigIn->setBounds (masterBounds.getX() + 220, masterBounds.getY() + 40, masterBounds.getWidth() - 240, 34);
    bigOut->setBounds (masterBounds.getX() + 220, masterBounds.getY() + 84, masterBounds.getWidth() - 240, 34);

    // header
    for (auto* c : std::initializer_list<juce::Component*> { &inMeter, &outMeter, &presetBox, &prev, &next, &save, &undoBtn, &redoBtn, &settings, &cpuLabel }) addC (*c);
    prev.setBounds (370, 18, 26, 28); presetBox.setBounds (400, 18, 220, 28); next.setBounds (624, 18, 26, 28); save.setBounds (658, 18, 60, 28);
    undoBtn.setBounds (734, 18, 54, 28); redoBtn.setBounds (792, 18, 54, 28);
    inMeter.setBounds (872, 12, 170, 42); outMeter.setBounds (1056, 12, 170, 42);
    cpuLabel.setBounds (1250, 14, 210, 38); cpuLabel.getProperties().set ("fs", 12.0f); cpuLabel.setJustificationType (juce::Justification::centredLeft); cpuLabel.setColour (juce::Label::textColourId, ui::textDim);
    settings.setBounds (1476, 18, 108, 28);

    refreshPresets();
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 1 && id <= numFactory) proc.loadFactoryPreset (id - 1);
        else if (id >= 1000) { const int k = id - 1000; if (k < userFiles.size()) proc.loadUserPreset (userFiles[k]); }
    };
    auto step = [this] (int d) { const int n = presetBox.getNumItems(); if (n == 0) return; presetBox.setSelectedItemIndex ((presetBox.getSelectedItemIndex() + d + n) % n, juce::sendNotification); };
    prev.onClick = [step] { step (-1); }; next.onClick = [step] { step (1); };
    save.onClick = [this] { promptSave(); };
    undoBtn.onClick = [this] { proc.undo.undo(); }; redoBtn.onClick = [this] { proc.undo.redo(); };
    settings.onClick = [this] { showSettings(); };

    content.sendLookAndFeelChange();
    juce::Component::SafePointer<MaketeoEditor> sp (this);
    setResizable (true, true); setResizeLimits (800, 425, 2400, 1275);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) ui::kW / ui::kH);
    double scale = 1.0;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()) scale = juce::jmin (1.0, (d->userArea.getHeight() - 90) / (double) ui::kH, (d->userArea.getWidth() - 40) / (double) ui::kW);
    setSize ((int) std::round (ui::kW * scale), (int) std::round (ui::kH * scale));
    startTimerHz (30);
}
MaketeoEditor::~MaketeoEditor() { stopTimer(); content.setLookAndFeel (nullptr); }

void MaketeoEditor::paint (Graphics& g) { g.fillAll (ui::black); }
void MaketeoEditor::resized()
{
    content.setBounds (0, 0, ui::kW, ui::kH);
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) ui::kW));
    if (adv) adv->setBounds (content.getLocalBounds());
}

void MaketeoEditor::refreshPresets()
{
    presetBox.clear (juce::dontSendNotification);
    auto names = proc.factoryPresetNames(); numFactory = names.size();
    for (int i = 0; i < names.size(); ++i) presetBox.addItem (names[i], i + 1);
    userFiles = proc.userPresetFiles();
    if (! userFiles.isEmpty()) { presetBox.addSeparator(); for (int i = 0; i < userFiles.size(); ++i) presetBox.addItem ("USER  " + userFiles[i].getFileNameWithoutExtension(), 1000 + i); }
    const auto cur = proc.currentPresetName();
    for (int i = 0; i < names.size(); ++i) if (names[i] == cur) presetBox.setSelectedId (i + 1, juce::dontSendNotification);
    for (int i = 0; i < userFiles.size(); ++i) if (userFiles[i].getFileNameWithoutExtension() == cur) presetBox.setSelectedId (1000 + i, juce::dontSendNotification);
}
void MaketeoEditor::promptSave()
{
    saveWindow = std::make_unique<juce::AlertWindow> ("Save preset", "Preset name:", juce::MessageBoxIconType::NoIcon);
    saveWindow->addTextEditor ("n", proc.currentPresetName() == "DEFAULT" ? juce::String() : proc.currentPresetName());
    saveWindow->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey)); saveWindow->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<MaketeoEditor> sp (this);
    saveWindow->enterModalState (true, juce::ModalCallbackFunction::create ([sp] (int r)
    {
        if (sp == nullptr) return;
        if (r == 1) { sp->proc.saveUserPreset (sp->saveWindow->getTextEditorContents ("n")); sp->refreshPresets(); }
        juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->saveWindow.reset(); });
    }), false);
}
void MaketeoEditor::showSettings()
{
    juce::PopupMenu m; m.setLookAndFeel (&laf);
    m.addSectionHeader ("Window size"); m.addItem (1, "75 %"); m.addItem (2, "100 %"); m.addItem (3, "125 %"); m.addSeparator();
    m.addItem (4, "Reset all parameters (DEFAULT)"); m.addItem (5, "Open preset folder");
    juce::Component::SafePointer<MaketeoEditor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settings), [sp] (int r)
    {
        if (sp == nullptr) return;
        const double sc[] = { 0.75, 1.0, 1.25 };
        if (r >= 1 && r <= 3) sp->setSize ((int) (ui::kW * sc[r - 1]), (int) (ui::kH * sc[r - 1]));
        else if (r == 4) sp->proc.resetAllParameters();
        else if (r == 5) { auto d = sp->proc.userPresetFiles(); juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Evric Hunter").getChildFile ("MAKETEO").getChildFile ("Presets").createDirectory(); juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Evric Hunter").getChildFile ("MAKETEO").getChildFile ("Presets").revealToUser(); }
    });
}
void MaketeoEditor::openAdv (mk::Group g, const juce::String& title)
{
    juce::Component::SafePointer<MaketeoEditor> self (this); // created outside the lambdas: MSVC resolves 'this' wrongly in a nested init-capture
    adv = std::make_unique<AdvPanel> (*this, g, title, [self] { juce::MessageManager::callAsync ([self] { if (self != nullptr) self->adv.reset(); }); });
    content.addAndMakeVisible (*adv); adv->setBounds (content.getLocalBounds()); content.sendLookAndFeelChange(); adv->toFront (false);
}
void MaketeoEditor::timerCallback()
{
    inMeter.tick(); outMeter.tick(); bigIn->tick(); bigOut->tick(); eqCard->tick();
    const double lat = proc.getSampleRate() > 0 ? 1000.0 * proc.getLatencySamples() / proc.getSampleRate() : 0.0;
    cpuLabel.setText ("CPU " + juce::String (juce::roundToInt (proc.cpuLoad.load() * 100.0f)) + " %\nLATENCY " + juce::String (lat, 1) + " ms", juce::dontSendNotification);
}
