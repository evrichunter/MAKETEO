#include "PluginEditor.h"
#include <map>

using juce::Colour; using juce::Graphics; using juce::Rectangle;

namespace ui
{
const Colour bg { 0xff0e1012 }, card { 0xff171a1e }, elevated { 0xff20242a }, border { 0xff2c3138 }, text { 0xfff3f4f6 }, textDim { 0xffa1a8b0 },
             accent { 0xffc8a96e }, success { 0xff39b980 }, warning { 0xffe5af4d }, error { 0xffe35d5d };
juce::Font font (float h, bool bold = false) { return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)); }
}

// ================================================================== helpers
namespace
{
const mk::ParamSpec* specFor (const char* id)
{ for (int i = 0; i < mk::kNumParams; ++i) if (juce::String (mk::paramSpecs()[i].id) == id) return &mk::paramSpecs()[i]; return nullptr; }
float plainOf (MaketeoProcessor& p, const char* id) { return p.apvts.getRawParameterValue (id)->load(); }
void setPlain (MaketeoProcessor& p, const char* id, float v)
{ if (auto* q = p.apvts.getParameter (id)) { q->beginChangeGesture(); q->setValueNotifyingHost (q->convertTo0to1 (v)); q->endChangeGesture(); } }
bool isPower (const mk::ParamSpec& s) { const juce::String id (s.id); return id.endsWith ("_byp") || (id.endsWith ("_on") && ! id.startsWith ("eq") && ! id.startsWith ("at_n")); }
const mk::ParamSpec* powerOf (mk::Group g) { for (int i = 0; i < mk::kNumParams; ++i) if (mk::paramSpecs()[i].group == g && isPower (mk::paramSpecs()[i])) return &mk::paramSpecs()[i]; return nullptr; }
juce::String dbText (float db, int dec = 1) { return db <= -89.0f ? juce::String (juce::CharPointer_UTF8 ("-\xe2\x88\x9e")) : juce::String (db, dec); }
juce::String noteName (int midi) { static const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return juce::String (n[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 1); }

juce::String T (const char* utf8) { return juce::String::fromUTF8 (utf8); }
struct GroupInfo { const char* name; const char* desc; };
const GroupInfo kGroups[] = {
    { "Entrada / Salida", "Ganancia global antes y despu\xc3\xa9s de todo el procesamiento." },
    { "Autotune Pro", "Corrige la afinaci\xc3\xb3n de la voz en tiempo real." },
    { "Vocal Rider", "Sube y baja el volumen solo para que la voz suene pareja." },
    { "Hollywood Preamp", "Preamplificador con saturaci\xc3\xb3n suave: calidez y cuerpo." },
    { "Micr\xc3\xb3""fono U47", "Da el car\xc3\xa1""cter de un micr\xc3\xb3""fono de v\xc3\xa1lvulas cl\xc3\xa1sico." },
    { "Revival", "Agrega brillo sedoso y densidad a la voz." },
    { "Ecualizador de 8 bandas", "Ajusta graves, medios y agudos. Arrastr\xc3\xa1 los nodos del gr\xc3\xa1""fico." },
    { "De-esser agudo", "Reduce las \"eses\" (sibilancias) agudas de la voz." },
    { "De-esser grave", "Reduce sibilancias m\xc3\xa1s graves y agresivas." },
    { "Compresor 1176", "Compresor r\xc3\xa1pido: controla picos y da presencia." },
    { "Compresor LA-2A", "Compresor suave: da densidad y estabilidad." },
    { "Ecualizador SSL", "Ecualizador de consola para definici\xc3\xb3n y presencia." },
    { "Ecualizador Maag", "Ecualizador musical con banda de aire." },
    { "Reverb", "Espacio y profundidad: de sala peque\xc3\xb1""a a gran hall." },
    { "Delay", "Ecos sincronizables con el tempo del proyecto." },
    { "Compresi\xc3\xb3n paralela (New York)", "Mezcla una copia muy comprimida: m\xc3\xa1s cuerpo y densidad sin perder din\xc3\xa1mica." },
    { "Brillo (Brightness)", "Agrega aire y claridad en agudos sin volverse \xc3\xa1spero." },
    { "Doblador", "Dobla la voz en est\xc3\xa9reo para dar amplitud." },
    { "Saturaci\xc3\xb3n de v\xc3\xa1lvulas", "Armoniza y engrosa la voz con calidez de v\xc3\xa1lvula." } };

// short Spanish explanations; exact id first, then label
const char* describe (const mk::ParamSpec& s)
{
    static const std::map<juce::String, const char*> byId = {
        { "dh_sens", "Qu\xc3\xa9 tan f\xc3\xa1""cil detecta sibilancias." }, { "dl_sens", "Qu\xc3\xa9 tan f\xc3\xa1""cil detecta sibilancias." }, { "rd_sens", "M\xc3\xa1s alto = reacciona tambi\xc3\xa9n a pasajes suaves." },
        { "dh_range", "Reducci\xc3\xb3n m\xc3\xa1xima de sibilancias." }, { "dl_range", "Reducci\xc3\xb3n m\xc3\xa1xima de sibilancias." }, { "rd_range", "Cu\xc3\xa1nto puede subir o bajar el volumen." },
        { "dh_freq", "Zona de frecuencias donde se detectan las eses." }, { "dl_freq", "Zona de frecuencias donde se detectan las eses." },
        { "dh_thr", "Nivel desde el cual empieza a actuar." }, { "dl_thr", "Nivel desde el cual empieza a actuar." }, { "ny_thr", "Nivel desde el cual comprime." },
        { "rd_speed", "Qu\xc3\xa9 tan r\xc3\xa1pido sube o baja el volumen." }, { "fet_att", "Qu\xc3\xa9 tan r\xc3\xa1pido reacciona (1 = lento, 7 = ultrarr\xc3\xa1pido)." },
        { "fet_rel", "Qu\xc3\xa9 tan r\xc3\xa1pido se recupera (1 = lento, 7 = r\xc3\xa1pido)." }, { "fet_in", "Cu\xc3\xa1nto empuja la se\xc3\xb1""al al compresor (m\xc3\xa1s = m\xc3\xa1s compresi\xc3\xb3n)." },
        { "fet_out", "Volumen de salida." }, { "la_gain", "Volumen de salida tras comprimir." }, { "ny_ratio", "Cu\xc3\xa1nto se comprime: mayor = m\xc3\xa1s fuerte." },
        { "fet_ratio", "Cu\xc3\xa1nto se comprime: mayor = m\xc3\xa1s fuerte." }, { "at_retune", "Qu\xc3\xa9 tan r\xc3\xa1pido se corrige cada nota (0 ms = instant\xc3\xa1neo)." },
        { "at_input", "Rango de la voz: define el tracking y la latencia." }, { "at_human", "Deja m\xc3\xa1s naturalidad en notas largas." }, { "at_flex", "No corrige desv\xc3\xados peque\xc3\xb1os." },
        { "at_vib", "Cu\xc3\xa1nto del vibrato original se conserva." }, { "at_fcorr", "Evita el efecto \"ardilla\" al corregir." }, { "at_fshift", "Cambia el timbre sin mover la nota." },
        { "at_trans", "Sube o baja la voz en semitonos." }, { "at_key", "Tonalidad de la canci\xc3\xb3n." }, { "at_scale", "Notas permitidas: mayor, menor, crom\xc3\xa1tica o personalizada." },
        { "at_mode", "Natural: suave. Hard Tune: efecto evidente." }, { "rd_target", "Volumen al que intenta llevar la voz." }, { "rd_up", "M\xc3\xa1ximo que puede subir." },
        { "rd_down", "M\xc3\xa1ximo que puede bajar." }, { "rd_man", "Volumen fijo de la voz (fader manual)." }, { "hw_in", "Nivel que entra al preamp." }, { "hw_drive", "Cu\xc3\xa1nta saturaci\xc3\xb3n agrega." },
        { "hw_sat", "Car\xc3\xa1""cter de la saturaci\xc3\xb3n." }, { "u47_prox", "Cercan\xc3\xada al micr\xc3\xb3""fono: graves c\xc3\xa1lidos." }, { "u47_body", "Cuerpo en graves-medios." }, { "u47_pres", "Claridad en medios altos." },
        { "u47_color", "Coloraci\xc3\xb3n arm\xc3\xb3nica de v\xc3\xa1lvula." }, { "u47_amt", "Intensidad total del procesador." }, { "rv_shim", "Brillo sedoso en agudos." }, { "rv_thick", "Grosor y densidad." },
        { "rv_amt", "Intensidad total del procesador." }, { "fet_color", "Car\xc3\xa1""cter anal\xc3\xb3gico (saturaci\xc3\xb3n sutil)." }, { "la_color", "Car\xc3\xa1""cter anal\xc3\xb3gico (saturaci\xc3\xb3n sutil)." },
        { "la_pr", "Cu\xc3\xa1nto comprime (m\xc3\xa1s = m\xc3\xa1s compresi\xc3\xb3n)." }, { "la_mode", "Compress: suave. Limit: m\xc3\xa1s firme." }, { "mg_air", "Aire: brillo suave en agudos." }, { "mg_airf", "Desde d\xc3\xb3nde empieza el aire." },
        { "mg_sub", "Graves profundos." }, { "ssl_hpf_on", "Activa el corte de graves." }, { "ssl_hpf_freq", "Frecuencia del corte de graves." }, { "ssl_lpf_on", "Activa el corte de agudos." },
        { "ssl_lpf_freq", "Frecuencia del corte de agudos." }, { "ssl_hf_bell", "Campana en vez de estante." }, { "ssl_lf_bell", "Campana en vez de estante." },
        { "rvb_algo", "Tipo de espacio: sala, plate, hall o c\xc3\xa1mara." }, { "rvb_cmode", "Car\xc3\xa1""cter: neutro, oscuro, brillante o vintage." }, { "rvb_decay", "Duraci\xc3\xb3n de la cola." },
        { "rvb_pre", "Pausa antes de que empiece el eco." }, { "rvb_size", "Tama\xc3\xb1o del espacio." }, { "rvb_damp", "Absorci\xc3\xb3n de agudos en la cola." }, { "rvb_hc", "Recorta agudos del efecto." },
        { "rvb_lc", "Recorta graves del efecto." }, { "rvb_color", "M\xc3\xa1s oscuro \xe2\x86\x90 \xe2\x86\x92 m\xc3\xa1s brillante." }, { "rvb_mod", "Movimiento suave que evita sonido met\xc3\xa1lico." },
        { "rvb_width", "Amplitud est\xc3\xa9reo." }, { "dly_time", "Tiempo entre ecos." }, { "dly_sync", "Sincroniza con el tempo del proyecto." }, { "dly_div", "Figura r\xc3\xadtmica del eco." },
        { "dly_fb", "Cantidad de repeticiones." }, { "dly_pp", "Alterna ecos izquierda/derecha." }, { "dly_analog", "Sonido c\xc3\xa1lido tipo cinta." }, { "dly_lofi", "Degrada el eco (efecto retro)." },
        { "dly_hp", "Recorta graves de los ecos." }, { "dly_lp", "Recorta agudos de los ecos." }, { "dly_mod", "Movimiento suave de los ecos." }, { "dly_width", "Amplitud est\xc3\xa9reo." },
        { "ny_att", "Qu\xc3\xa9 tan r\xc3\xa1pido reacciona." }, { "ny_rel", "Qu\xc3\xa9 tan r\xc3\xa1pido se recupera." }, { "ny_knee", "Suavidad del inicio de la compresi\xc3\xb3n." }, { "ny_makeup", "Compensa el volumen perdido." },
        { "ny_sat", "Saturaci\xc3\xb3n sutil." }, { "br_drive", "Fuerza del excitador de brillo." }, { "br_harm", "Arm\xc3\xb3nicos agregados para dar claridad." }, { "br_midair", "Aire en medios altos." },
        { "br_hiair", "Aire en agudos extremos." }, { "br_eqamt", "Cu\xc3\xa1nto del EQ principal copia esta cadena." }, { "dbl_width", "Amplitud de las voces dobladas." }, { "dbl_detl", "Desafinaci\xc3\xb3n leve de la doble izquierda." },
        { "dbl_detr", "Desafinaci\xc3\xb3n leve de la doble derecha." }, { "dbl_dlyl", "Retraso de la doble izquierda." }, { "dbl_dlyr", "Retraso de la doble derecha." }, { "dbl_mod", "Movimiento natural de las dobles." },
        { "dbl_spread", "Separaci\xc3\xb3n izquierda-derecha." }, { "tb_drive", "Cu\xc3\xa1nto se empuja la v\xc3\xa1lvula." }, { "tb_char", "Tr\xc3\xa1iodo c\xc3\xa1lido \xe2\x86\x90 \xe2\x86\x92 pentodo agresivo." }, { "tb_harm", "Arm\xc3\xb3nicos generados." },
        { "tb_warm", "Calidez en graves." }, { "tb_tone", "M\xc3\xa1s oscuro \xe2\x86\x90 \xe2\x86\x92 m\xc3\xa1s brillante." }, { "in_trim", "Nivel de entrada global." }, { "out_trim", "Nivel de salida global." },
        { "dh_att", "Qu\xc3\xa9 tan r\xc3\xa1pido reacciona." }, { "dl_att", "Qu\xc3\xa9 tan r\xc3\xa1pido reacciona." }, { "dh_rel", "Qu\xc3\xa9 tan r\xc3\xa1pido se recupera." }, { "dl_rel", "Qu\xc3\xa9 tan r\xc3\xa1pido se recupera." },
        { "dh_listen", "Escucha solo lo detectado para ajustar." }, { "dl_listen", "Escucha solo lo detectado para ajustar." } };
    auto it = byId.find (s.id); if (it != byId.end()) return it->second;
    const juce::String id (s.id), label (s.label);
    if (id.startsWith ("eq") && label.endsWith ("On")) return "Activa esta banda.";
    if (id.startsWith ("eq") && label.endsWith ("Type")) return "Campana, estante o filtro de corte.";
    if (id.startsWith ("eq") && label.endsWith ("Freq")) return "Frecuencia central de la banda.";
    if (id.startsWith ("eq") && label.endsWith ("Gain")) return "Cu\xc3\xa1nto sube o baja.";
    if (id.startsWith ("eq") && label.endsWith ("Q")) return "Ancho: m\xc3\xa1s alto = m\xc3\xa1s estrecho.";
    if (id.startsWith ("at_n")) return "Permite esta nota en la escala personalizada.";
    if (id.startsWith ("ssl_hf")) return "Agudos (brillo).";
    if (id.startsWith ("ssl_hmf")) return "Medios altos (presencia).";
    if (id.startsWith ("ssl_lmf")) return "Medios bajos (cuerpo).";
    if (id.startsWith ("ssl_lf")) return "Graves.";
    if (id.startsWith ("mg_")) return "Banda del ecualizador Maag.";
    if (label == "Mix") return "Cu\xc3\xa1nto efecto se mezcla con la voz.";
    if (label == "Output") return "Volumen de salida.";
    if (label == "Input") return "Nivel de entrada.";
    return "";
}
} // namespace

// ================================================================== look and feel
class MkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MkLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, ui::text); setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack); setColour (juce::Slider::textBoxHighlightColourId, ui::accent.withAlpha (0.4f));
        setColour (juce::Label::textColourId, ui::text); setColour (juce::TextEditor::backgroundColourId, ui::elevated); setColour (juce::TextEditor::textColourId, ui::text);
        setColour (juce::TextEditor::outlineColourId, ui::accent); setColour (juce::TextEditor::focusedOutlineColourId, ui::accent); setColour (juce::CaretComponent::caretColourId, ui::accent);
        setColour (juce::ComboBox::textColourId, ui::text); setColour (juce::PopupMenu::backgroundColourId, ui::elevated); setColour (juce::PopupMenu::textColourId, ui::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::accent.withAlpha (0.3f)); setColour (juce::PopupMenu::highlightedTextColourId, ui::text);
        setColour (juce::AlertWindow::backgroundColourId, ui::card); setColour (juce::AlertWindow::textColourId, ui::text);
        setColour (juce::ScrollBar::thumbColourId, ui::border.brighter (0.4f)); setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    }
    juce::Label* createSliderTextBox (juce::Slider& s) override
    { auto* l = LookAndFeel_V4::createSliderTextBox (s); l->getProperties().set ("fs", s.getProperties().getWithDefault ("fs", 15.0f)); l->setJustificationType (juce::Justification::centredRight); return l; }
    juce::Font getLabelFont (juce::Label& l) override { return ui::font ((float) (double) l.getProperties().getWithDefault ("fs", 15.0f), true); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return ui::font (15.0f, true); }
    juce::Font getPopupMenuFont() override { return ui::font (15.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return ui::font (14.0f, true); }

    void drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s) override
    {
        const float cy = (float) y + (float) h * 0.5f, tx = (float) x + 10.0f, tw = (float) w - 20.0f;
        const bool hot = s.isMouseOverOrDragging(), en = s.isEnabled();
        auto track = Rectangle<float> (tx, cy - 3.0f, tw, 6.0f);
        g.setColour (ui::border); g.fillRoundedRectangle (track, 3.0f);
        const float px = juce::jlimit (tx, tx + tw, pos);
        g.setColour (en ? ui::accent : ui::accent.withAlpha (0.35f)); g.fillRoundedRectangle (track.withRight (px), 3.0f);
        const float r = hot ? 11.0f : 10.0f;
        if (hot) { g.setColour (ui::accent.withAlpha (0.20f)); g.fillEllipse (px - 15.0f, cy - 15.0f, 30.0f, 30.0f); }
        g.setColour (ui::text); g.fillEllipse (px - r, cy - r, r * 2, r * 2);
        g.setColour (ui::accent); g.drawEllipse (px - r, cy - r, r * 2, r * 2, 2.0f);
    }
    void drawButtonBackground (Graphics& g, juce::Button& b, const Colour&, bool hi, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f); const auto kind = b.getProperties().getWithDefault ("kind", "").toString(); const bool on = b.getToggleState();
        if (kind == "tab") { if (on) { g.setColour (ui::accent); g.fillRoundedRectangle (r, r.getHeight() * 0.5f); } else if (hi) { g.setColour (ui::elevated); g.fillRoundedRectangle (r, r.getHeight() * 0.5f); } return; }
        if (kind == "seg")
        {
            g.setColour (on ? ui::accent : (hi ? ui::elevated : ui::card)); g.fillRoundedRectangle (r, 8.0f);
            g.setColour (on ? ui::accent : ui::border); g.drawRoundedRectangle (r, 8.0f, 1.2f); return;
        }
        g.setColour (down ? ui::border : hi ? ui::elevated.brighter (0.08f) : ui::elevated); g.fillRoundedRectangle (r, 8.0f);
        g.setColour (hi ? ui::accent.withAlpha (0.7f) : ui::border); g.drawRoundedRectangle (r, 8.0f, 1.2f);
    }
    void drawButtonText (Graphics& g, juce::TextButton& b, bool, bool) override
    {
        const auto kind = b.getProperties().getWithDefault ("kind", "").toString(); const bool on = b.getToggleState();
        g.setColour ((kind == "tab" || kind == "seg") && on ? ui::bg : ui::text);
        g.setFont (ui::font (14.0f, true).withExtraKerningFactor (kind == "tab" ? 0.05f : 0.03f));
        g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (6, 0), juce::Justification::centred, 1, 0.8f);
    }
    void drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
        g.setColour (ui::elevated); g.fillRoundedRectangle (r, 8.0f); g.setColour (box.isPopupActive() ? ui::accent : ui::border); g.drawRoundedRectangle (r, 8.0f, 1.2f);
        juce::Path p; const float cx = (float) w - 16.0f, cy = (float) h * 0.5f; p.addTriangle (cx - 5, cy - 3, cx + 5, cy - 3, cx, cy + 3); g.setColour (ui::accent); g.fillPath (p);
    }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& l) override { l.setBounds (10, 1, box.getWidth() - 34, box.getHeight() - 2); l.setFont (getComboBoxFont (box)); }
};

// ================================================================== small widgets
// ON/OFF switch bound to a boolean parameter (bypass parameters are inverted so the switch reads "active")
class Switch : public juce::Component
{
public:
    Switch (MaketeoProcessor& p, const mk::ParamSpec& s, const juce::String& lbl = {}) : label (lbl), invert (juce::String (s.id).endsWith ("_byp"))
    {
        att = std::make_unique<juce::ParameterAttachment> (*p.apvts.getParameter (s.id), [this] (float v) { on = invert ? v < 0.5f : v > 0.5f; repaint(); }, p.apvts.undoManager);
        att->sendInitialUpdate(); setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    bool isOn() const { return on; }
    void mouseDown (const juce::MouseEvent&) override { att->setValueAsCompleteGesture (on ? (invert ? 1.0f : 0.0f) : (invert ? 0.0f : 1.0f)); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat(); auto sw = r.removeFromLeft (46.0f).withSizeKeepingCentre (46.0f, 26.0f);
        g.setColour (on ? ui::accent : ui::border); g.fillRoundedRectangle (sw, 13.0f);
        g.setColour (on ? ui::bg : ui::textDim); g.fillEllipse (Rectangle<float> (20.0f, 20.0f).withCentre ({ on ? sw.getRight() - 13.0f : sw.getX() + 13.0f, sw.getCentreY() }));
        if (label.isNotEmpty()) { g.setColour (on ? ui::text : ui::textDim); g.setFont (ui::font (14.0f, true).withExtraKerningFactor (0.05f)); g.drawText (label, r.withTrimmedLeft (8.0f).toNearestInt(), juce::Justification::centredLeft); }
    }
private:
    juce::String label; bool invert, on = false; std::unique_ptr<juce::ParameterAttachment> att;
};

class ParamSlider : public juce::Component
{
public:
    ParamSlider (MaketeoProcessor& p, const mk::ParamSpec& s, float fs = 15.0f)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal); slider.getProperties().set ("fs", fs); slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 96, 28);
        addAndMakeVisible (slider); att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, s.id, slider); slider.setTextBoxIsEditable (true);
    }
    void resized() override { slider.setBounds (getLocalBounds()); }
    juce::Slider slider;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// slider whose position and value are derived from real parameters (no extra state): used for the simple "artist" controls
class DerivedSlider : public juce::Component, private juce::Timer
{
public:
    DerivedSlider (MaketeoProcessor& p, std::function<float()> getter, std::function<void (float)> setter, std::function<juce::String (double)> text, bool percent = true)
        : proc (p), get (std::move (getter)), set (std::move (setter))
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal); slider.setRange (0.0, 1.0, 0.001); slider.getProperties().set ("fs", 16.0f);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 96, 28); slider.setTextBoxIsEditable (percent);
        slider.textFromValueFunction = text ? text : [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
        slider.valueFromTextFunction = [] (const juce::String& t) { return juce::jlimit (0.0, 1.0, t.retainCharacters ("0123456789.").getDoubleValue() / 100.0); };
        slider.onDragStart = [this] { dragging = true; proc.undo.beginNewTransaction ("Control"); };
        slider.onDragEnd = [this] { dragging = false; };
        slider.onValueChange = [this] { if (! updating) set ((float) slider.getValue()); };
        addAndMakeVisible (slider); startTimerHz (20); timerCallback();
    }
    void resized() override { slider.setBounds (getLocalBounds()); }
    juce::Slider slider;
private:
    void timerCallback() override { if (dragging || slider.isMouseButtonDown()) return; updating = true; slider.setValue (juce::jlimit (0.0f, 1.0f, get()), juce::dontSendNotification); updating = false; slider.updateText(); }
    MaketeoProcessor& proc; std::function<float()> get; std::function<void (float)> set; bool dragging = false, updating = false;
};

class LiveText : public juce::Component, private juce::Timer
{
public:
    LiveText (std::function<juce::String()> f, float size, bool bold, std::function<Colour()> col = {}, juce::Justification j = juce::Justification::centredLeft, int maxLines = 1)
        : fn (std::move (f)), colour (std::move (col)), size (size), bold (bold), just (j), lines (maxLines) { startTimerHz (20); timerCallback(); }
    void paint (Graphics& g) override { g.setColour (colour ? colour() : ui::text); g.setFont (ui::font (size, bold)); g.drawFittedText (txt, getLocalBounds(), just, lines, 0.85f); }
private:
    void timerCallback() override { auto t = fn(); if (t != txt) { txt = t; repaint(); } else if (colour) repaint(); }
    std::function<juce::String()> fn; std::function<Colour()> colour; juce::String txt; float size; bool bold; juce::Justification just; int lines;
};

// horizontal meter bar fed by real DSP values (reduction: grows right; bipolar: grows both ways from the centre)
class DbBar : public juce::Component, private juce::Timer
{
public:
    DbBar (std::function<float()> src, float lo, float hi, bool bipolar, Colour c, float thick = 10.0f) : source (std::move (src)), lo (lo), hi (hi), bip (bipolar), col (c), th (thick) { startTimerHz (30); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().withSizeKeepingCentre ((float) getWidth(), th);
        g.setColour (ui::elevated); g.fillRoundedRectangle (r, th * 0.5f);
        const float t = juce::jlimit (0.0f, 1.0f, (v - lo) / (hi - lo));
        if (bip) { const float c = r.getCentreX(), x = r.getX() + t * r.getWidth(); g.setColour (v >= 0 ? ui::success : ui::warning); g.fillRoundedRectangle (Rectangle<float> (juce::jmin (c, x), r.getY(), std::abs (x - c) + 1.0f, th), th * 0.5f);
                     g.setColour (ui::textDim); g.fillRect (c - 1.0f, r.getY() - 2.0f, 2.0f, th + 4.0f); }
        else { g.setColour (col); g.fillRoundedRectangle (r.withWidth (juce::jmax (0.0f, r.getWidth() * t)), th * 0.5f); }
    }
private:
    void timerCallback() override { const float t = source(); v = (t > v) ? t : v * 0.85f + t * 0.15f; if (bip) v = t * 0.4f + v * 0.6f; repaint(); }
    std::function<float()> source; float lo, hi; bool bip; Colour col; float th, v = 0.0f;
};

// shared peak model for every level display: dBFS, hold, clip latch (single consumer of the processor's atomics)
struct LevelModel
{
    explicit LevelModel (MaketeoProcessor& p) : proc (p) {}
    void tick()
    {
        std::atomic<float>* src[2] = { proc.inPeak, proc.outPeak };
        for (int io = 0; io < 2; ++io) for (int c = 0; c < 2; ++c)
        {
            const float pk = src[io][c].exchange (0.0f); const float db = juce::Decibels::gainToDecibels (pk, -90.0f);
            shown[io][c] = db > shown[io][c] ? db : juce::jmax (db, shown[io][c] - 1.2f);
            if (db >= hold[io][c]) { hold[io][c] = db; timer[io][c] = 45; } else if (timer[io][c] > 0) --timer[io][c]; else hold[io][c] = juce::jmax (-90.0f, hold[io][c] - 0.6f);
            if (pk >= 1.0f) clip[io] = true;
        }
    }
    float peakHold (int io) const { return juce::jmax (hold[io][0], hold[io][1]); }
    MaketeoProcessor& proc; float shown[2][2] { { -90, -90 }, { -90, -90 } }, hold[2][2] { { -90, -90 }, { -90, -90 } }; int timer[2][2] {}; bool clip[2] { false, false };
};

class LevelMeter : public juce::Component
{
public:
    LevelMeter (LevelModel& m, int io, const juce::String& t) : model (m), io (io), title (t) {}
    void mouseDown (const juce::MouseEvent&) override { model.clip[io] = false; }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const float pk = model.peakHold (io); const bool clip = model.clip[io];
        auto head = r.removeFromTop (22.0f);
        g.setColour (ui::textDim); g.setFont (ui::font (13.0f, true).withExtraKerningFactor (0.1f)); g.drawText (title, head.toNearestInt(), juce::Justification::centredLeft);
        auto pill = head.removeFromRight (58.0f).withSizeKeepingCentre (58.0f, 20.0f);
        g.setColour (clip ? ui::error : ui::elevated); g.fillRoundedRectangle (pill, 10.0f); g.setColour (clip ? ui::text : ui::textDim); g.setFont (ui::font (12.5f, true)); g.drawText ("CLIP", pill.toNearestInt(), juce::Justification::centred);
        auto left = r.removeFromLeft (juce::jmin (150.0f, r.getWidth() * 0.32f));
        g.setColour (clip || pk >= -0.5f ? ui::error : pk > -6.0f ? ui::warning : ui::text); g.setFont (ui::font (38.0f, true)); g.drawText (dbText (pk), left.removeFromTop (44.0f).toNearestInt(), juce::Justification::centredLeft);
        g.setColour (ui::textDim); g.setFont (ui::font (13.0f)); g.drawText ("dBFS pico", left.removeFromTop (20.0f).toNearestInt(), juce::Justification::centredLeft);
        r.removeFromLeft (12.0f); r.removeFromRight (16.0f);
        const float bh = juce::jmax (8.0f, juce::jmin (18.0f, (r.getHeight() - 22.0f) / 2 - 3.0f)); float y = r.getY() + 2.0f;
        for (int c = 0; c < 2; ++c, y += bh + 6.0f)
        {
            auto bar = Rectangle<float> (r.getX() + 18.0f, y, r.getWidth() - 18.0f, bh);
            g.setColour (ui::textDim); g.setFont (ui::font (12.0f, true)); g.drawText (c ? "R" : "L", (int) r.getX(), (int) y, 16, (int) bh, juce::Justification::centred);
            g.setColour (ui::elevated); g.fillRoundedRectangle (bar, 4.0f);
            auto pos = [&] (float db) { return juce::jmap (juce::jlimit (-60.0f, 0.0f, db), -60.0f, 0.0f, bar.getX(), bar.getRight()); };
            const float x1 = pos (model.shown[io][c]);
            g.setColour (model.shown[io][c] > -1.0f ? ui::error : model.shown[io][c] > -9.0f ? ui::warning : ui::success); g.fillRoundedRectangle (bar.withRight (x1), 4.0f);
            g.setColour (ui::text); g.fillRect (pos (model.hold[io][c]) - 1.0f, bar.getY(), 2.0f, bh);
        }
        g.setColour (ui::textDim); g.setFont (ui::font (12.0f));
        for (int db : { -48, -36, -24, -12, -6, 0 })
        { const float x = juce::jmap ((float) db, -60.0f, 0.0f, r.getX() + 18.0f, r.getRight()); g.drawText (juce::String (db), (int) x - 16, (int) y, 32, 16, juce::Justification::centred); }
    }
private:
    LevelModel& model; int io; juce::String title;
};

// real-time tuner readout (detected note, deviation in cents, state)
class TunerDisplay : public juce::Component, private juce::Timer
{
public:
    explicit TunerDisplay (MaketeoProcessor& p) : proc (p) { startTimerHz (30); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const bool on = plainOf (proc, "at_byp") < 0.5f;
        juce::String state = "APAGADO", big = "-", sub = T ("Activ\xc3\xa1 la afinaci\xc3\xb3n");
        Colour sc = ui::textDim; float dev = 0; bool have = false;
        if (on)
        {
            if (voiced) { state = "DETECTANDO"; sc = ui::success; have = true; big = noteName ((int) std::lround (m)); dev = cents;
                          sub = "Objetivo " + noteName ((int) std::lround (tgt)) + "   |   " + juce::String (cents >= 0 ? "+" : "") + juce::String (juce::roundToInt (cents)) + " ct"; }
            else { state = "SIN VOZ"; sub = T ("Esperando se\xc3\xb1""al..."); }
        }
        auto chip = r.removeFromTop (26.0f).withWidth (118.0f);
        g.setColour (sc.withAlpha (0.18f)); g.fillRoundedRectangle (chip, 13.0f); g.setColour (sc); g.setFont (ui::font (13.0f, true).withExtraKerningFactor (0.08f)); g.drawText (state, chip.toNearestInt(), juce::Justification::centred);
        r.removeFromTop (6.0f);
        g.setColour (have ? ui::text : ui::textDim); g.setFont (ui::font (juce::jmin (64.0f, r.getHeight() * 0.42f), true)); g.drawText (big, r.removeFromTop (r.getHeight() * 0.5f).toNearestInt(), juce::Justification::centredLeft);
        g.setColour (ui::textDim); g.setFont (ui::font (14.0f)); g.drawText (sub, r.removeFromTop (22.0f).toNearestInt(), juce::Justification::centredLeft);
        r.removeFromTop (8.0f);
        auto bar = r.removeFromTop (14.0f); g.setColour (ui::elevated); g.fillRoundedRectangle (bar, 7.0f);
        g.setColour (ui::success.withAlpha (0.25f)); g.fillRoundedRectangle (bar.withSizeKeepingCentre (bar.getWidth() * 0.2f, bar.getHeight()), 7.0f);
        g.setColour (ui::textDim); g.fillRect (bar.getCentreX() - 1.0f, bar.getY() - 3.0f, 2.0f, bar.getHeight() + 6.0f);
        if (have) { const float x = bar.getCentreX() + juce::jlimit (-50.0f, 50.0f, dev) / 50.0f * bar.getWidth() * 0.5f; g.setColour (std::abs (dev) < 10 ? ui::success : std::abs (dev) < 30 ? ui::warning : ui::error); g.fillEllipse (x - 8.0f, bar.getCentreY() - 8.0f, 16.0f, 16.0f); }
        r.removeFromTop (4.0f); g.setColour (ui::textDim); g.setFont (ui::font (12.5f)); g.drawText ("-50 ct", r.toNearestInt().removeFromLeft (60), juce::Justification::centredLeft);
        g.drawText ("+50 ct", r.toNearestInt().removeFromRight (60), juce::Justification::centredRight);
        if (have) { g.setColour (ui::textDim); g.setFont (ui::font (13.0f)); g.drawText (T ("Correcci\xc3\xb3n aplicada: ") + juce::String (shiftCt >= 0 ? "+" : "") + juce::String (juce::roundToInt (shiftCt)) + " ct", r.toNearestInt(), juce::Justification::centred); }
    }
private:
    void timerCallback() override
    {
        voiced = proc.meter (mk::MT_AT_VOICED) > 0.5f; const float hz = proc.meter (mk::MT_AT_HZ); tgt = proc.meter (mk::MT_AT_NOTE); shiftCt = proc.meter (mk::MT_AT_SHIFT) * 100.0f;
        if (voiced && hz > 20.0f) { const float mm = 69.0f + 12.0f * std::log2 (hz / 440.0f); m = m * 0.5f + mm * 0.5f; cents = (m - std::round (tgt)) * 100.0f; if (std::fabs (m - mm) > 1.0f) m = mm; }
        repaint();
    }
    MaketeoProcessor& proc; bool voiced = false; float m = 60, tgt = 60, cents = 0, shiftCt = 0;
};

// ================================================================== EQ graph (analyzer, curve, draggable nodes)
class EqGraph : public juce::Component, private juce::Timer
{
public:
    explicit EqGraph (MaketeoProcessor& p) : proc (p) { startTimerHz (30); }
    void paint (Graphics& g) override
    {
        auto p = plot(); const double fs = proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000.0;
        g.setColour (ui::card); g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f); g.setColour (ui::border); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 10.0f, 1.0f);
        g.setFont (ui::font (12.0f));
        for (double f : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
        { const float x = xOf (f); g.setColour (ui::border.withAlpha (0.7f)); g.drawVerticalLine ((int) x, p.getY(), p.getBottom()); g.setColour (ui::textDim);
          g.drawText (f >= 1000 ? juce::String (juce::roundToInt (f / 1000)) + "k" : juce::String ((int) f), (int) x - 18, (int) p.getBottom() + 3, 36, 14, juce::Justification::centred); }
        for (int dB = -18; dB <= 18; dB += 6) { const float y = yOf (dB); g.setColour (ui::border.withAlpha (dB == 0 ? 1.0f : 0.5f)); g.drawHorizontalLine ((int) y, p.getX(), p.getRight()); g.setColour (ui::textDim); g.drawText (juce::String (dB), 0, (int) y - 7, 30, 14, juce::Justification::centredRight); }
        juce::Path sp; sp.startNewSubPath (p.getX(), p.getBottom());
        for (int i = 1; i < 1024; ++i) { const double f = i * fs / 2048.0; if (f < 20.0 || f > 20000.0) continue; sp.lineTo (xOf (f), juce::jmap (juce::jlimit (-90.0f, -6.0f, spec[(size_t) i]), -90.0f, -6.0f, p.getBottom(), p.getY() + 4)); }
        sp.lineTo (p.getRight(), p.getBottom()); sp.closeSubPath();
        g.setColour (ui::accent.withAlpha (0.22f)); g.fillPath (sp);
        juce::Path curve; bool first = true;
        for (int i = 0; i <= 240; ++i)
        {
            const double f = 20.0 * std::pow (1000.0, i / 240.0); double dB = 0.0;
            for (int b = 0; b < 8; ++b) { if (val (b, "on") < 0.5) continue; const int type = (int) val (b, "type"); const double gn = val (b, "gain"); if (type <= 2 && std::fabs (gn) < 0.005) continue;
                dB += mk::filt::magDb (mk::ParamEQ8::coefFor (fs, type, val (b, "freq"), gn, val (b, "q")), fs, f); }
            const float x = xOf (f), y = yOf (juce::jlimit (-24.0, 24.0, dB)); if (first) { curve.startNewSubPath (x, y); first = false; } else curve.lineTo (x, y);
        }
        g.setColour (ui::text); g.strokePath (curve, juce::PathStrokeType (2.2f));
        for (int b = 0; b < 8; ++b)
        {
            const bool on = val (b, "on") > 0.5; const int type = (int) val (b, "type"); const float x = xOf (val (b, "freq")), y = yOf (type <= 2 ? val (b, "gain") : 0.0);
            if (b == selected) { g.setColour (ui::accent.withAlpha (0.3f)); g.fillEllipse (x - 14, y - 14, 28, 28); }
            g.setColour (on ? ui::accent : ui::elevated); g.fillEllipse (x - 10, y - 10, 20, 20); g.setColour (on ? ui::bg : ui::border); g.drawEllipse (x - 10, y - 10, 20, 20, 1.5f);
            g.setColour (on ? ui::bg : ui::textDim); g.setFont (ui::font (12.0f, true)); g.drawText (juce::String (b + 1), (int) x - 10, (int) y - 10, 20, 20, juce::Justification::centred);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    { dragging = hit (e.position); if (dragging >= 0) { selected = dragging; par (dragging, "freq")->beginChangeGesture(); par (dragging, "gain")->beginChangeGesture(); if (val (dragging, "on") < 0.5) setVal (dragging, "on", 1.0); } }
    void mouseDrag (const juce::MouseEvent& e) override
    { if (dragging < 0) return; setVal (dragging, "freq", juce::jlimit (20.0, 20000.0, fOf (e.position.x))); if ((int) val (dragging, "type") <= 2) setVal (dragging, "gain", gOf (e.position.y)); }
    void mouseUp (const juce::MouseEvent&) override { if (dragging >= 0) { par (dragging, "freq")->endChangeGesture(); par (dragging, "gain")->endChangeGesture(); } dragging = -1; }
    void mouseDoubleClick (const juce::MouseEvent& e) override { const int b = hit (e.position); if (b >= 0) { auto* q = par (b, "on"); q->beginChangeGesture(); q->setValueNotifyingHost (q->getValue() > 0.5f ? 0.0f : 1.0f); q->endChangeGesture(); } }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    { int b = hit (e.position); if (b < 0) b = selected; auto* q = par (b, "q"); q->beginChangeGesture(); setVal (b, "q", juce::jlimit (0.1, 18.0, val (b, "q") * std::pow (1.18, (double) w.deltaY * 4.0))); q->endChangeGesture(); }
private:
    void timerCallback() override
    {
        proc.copyAnalyzer (snap.data(), 2048);
        for (int i = 0; i < 2048; ++i) fftData[(size_t) i] = snap[(size_t) i] * (0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / 2047.0f));
        std::fill (fftData.begin() + 2048, fftData.end(), 0.0f); fft.performFrequencyOnlyForwardTransform (fftData.data());
        for (int i = 0; i < 1024; ++i) { const float db = juce::Decibels::gainToDecibels (fftData[(size_t) i] / 512.0f, -100.0f); spec[(size_t) i] = db > spec[(size_t) i] ? db : spec[(size_t) i] * 0.85f + db * 0.15f; }
        repaint();
    }
    Rectangle<float> plot() const { return getLocalBounds().toFloat().withTrimmedLeft (34).withTrimmedBottom (20).reduced (6, 8); }
    float xOf (double f) const { auto p = plot(); return p.getX() + (float) (std::log (f / 20.0) / std::log (1000.0)) * p.getWidth(); }
    double fOf (float x) const { auto p = plot(); return 20.0 * std::pow (1000.0, juce::jlimit (0.0, 1.0, (double) ((x - p.getX()) / p.getWidth()))); }
    float yOf (double g) const { auto p = plot(); return p.getCentreY() - (float) (g / 24.0) * p.getHeight() * 0.5f; }
    double gOf (float y) const { auto p = plot(); return juce::jlimit (-24.0, 24.0, (double) ((p.getCentreY() - y) / (p.getHeight() * 0.5f)) * 24.0); }
    juce::RangedAudioParameter* par (int b, const char* w) const { return dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter ("eq" + juce::String (b + 1) + "_" + w)); }
    double val (int b, const char* w) const { auto* q = par (b, w); return q->convertFrom0to1 (q->getValue()); }
    void setVal (int b, const char* w, double v) { auto* q = par (b, w); q->setValueNotifyingHost (q->convertTo0to1 ((float) v)); }
    int hit (juce::Point<float> pt) const
    { int best = -1; float bd = 14.0f; for (int b = 0; b < 8; ++b) { const int type = (int) val (b, "type"); const float d = pt.getDistanceFrom ({ xOf (val (b, "freq")), yOf (type <= 2 ? val (b, "gain") : 0.0) }); if (d < bd) { bd = d; best = b; } } return best; }
    MaketeoProcessor& proc; int selected = 0, dragging = -1; juce::dsp::FFT fft { 11 };
    std::array<float, 2048> snap {}; std::array<float, 4096> fftData {}; std::array<float, 1024> spec {};
};

// ================================================================== parameter rows / group panel (ADVANCED page and FX "EDIT")
class ParamRow : public juce::Component
{
public:
    ParamRow (MaketeoProcessor& p, const mk::ParamSpec& s) : name (s.label), desc (T (describe (s)))
    {
        if (s.kind == mk::K_FLOAT) { slider = std::make_unique<ParamSlider> (p, s); addAndMakeVisible (*slider); }
        else if (s.kind == mk::K_BOOL) { sw = std::make_unique<Switch> (p, s); addAndMakeVisible (*sw); }
        else { combo = std::make_unique<juce::ComboBox>(); combo->addItemList (juce::StringArray::fromTokens (s.choices, "|", ""), 1); addAndMakeVisible (*combo);
               cAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, s.id, *combo); }
    }
    void resized() override
    {
        auto r = getLocalBounds(); auto nameCol = r.removeFromLeft (juce::jmax (200, (int) (r.getWidth() * 0.40f))); (void) nameCol; r.removeFromLeft (8);
        if (slider) slider->setBounds (r.withTrimmedRight (6).withSizeKeepingCentre (juce::jmin (r.getWidth() - 6, 520), 34));
        if (sw) sw->setBounds (r.withSizeKeepingCentre (r.getWidth(), 30).withWidth (60));
        if (combo) combo->setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 6, 240), 34).withX (r.getX()));
    }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat(); g.setColour (ui::border); g.fillRect (r.removeFromBottom (1.0f));
        auto nameCol = getLocalBounds().removeFromLeft (juce::jmax (200, (int) (getWidth() * 0.40f)));
        g.setColour (ui::text); g.setFont (ui::font (15.0f, true)); g.drawFittedText (name, nameCol.removeFromTop (getHeight() / 2 + (desc.isEmpty() ? 8 : 4)).withTrimmedTop (desc.isEmpty() ? 0 : 6), juce::Justification::bottomLeft, 1, 0.8f);
        if (desc.isNotEmpty()) { g.setColour (ui::textDim); g.setFont (ui::font (13.0f)); g.drawFittedText (desc, nameCol.withTrimmedRight (6), juce::Justification::topLeft, 2, 0.85f); }
    }
private:
    juce::String name, desc; std::unique_ptr<ParamSlider> slider; std::unique_ptr<Switch> sw; std::unique_ptr<juce::ComboBox> combo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
};

class GroupPanel : public juce::Component
{
public:
    explicit GroupPanel (MaketeoProcessor& p) : proc (p) { viewport.setViewedComponent (&holder, false); viewport.setScrollBarsShown (true, false); addAndMakeVisible (viewport); }
    void setGroup (mk::Group g)
    {
        group = g; power.reset(); rows.clear(); eq.reset();
        if (auto* ps = powerOf (g)) { power = std::make_unique<Switch> (proc, *ps, T ("ACTIVO")); addAndMakeVisible (*power); }
        if (g == mk::G_EQ) { eq = std::make_unique<EqGraph> (proc); holder.addAndMakeVisible (*eq); }
        for (int i = 0; i < mk::kNumParams; ++i) { const auto& s = mk::paramSpecs()[i]; if (s.group != g || isPower (s)) continue; auto* r = new ParamRow (proc, s); rows.add (r); holder.addAndMakeVisible (r); }
        getTopLevelComponent()->sendLookAndFeelChange(); resized(); viewport.setViewPosition (0, 0);
    }
    void resized() override
    {
        auto r = getLocalBounds(); auto head = r.removeFromTop (70);
        if (power) power->setBounds (head.removeFromRight (150).withSizeKeepingCentre (150, 30));
        viewport.setBounds (r);
        const int w = r.getWidth() - (viewport.isVerticalScrollBarShown() ? 12 : 0); int y = 6;
        if (eq) { eq->setBounds (4, y, w - 8, 250); y += 262; }
        for (auto* row : rows) { row->setBounds (4, y, w - 8, 60); y += 60; }
        holder.setSize (w, y + 8);
    }
    void paint (Graphics& g) override
    {
        auto head = getLocalBounds().removeFromTop (70).withTrimmedRight (power ? 160 : 0);
        g.setColour (ui::text); g.setFont (ui::font (24.0f, true)); g.drawText (T (kGroups[group].name), head.removeFromTop (38), juce::Justification::centredLeft);
        g.setColour (ui::textDim); g.setFont (ui::font (14.5f)); g.drawFittedText (T (kGroups[group].desc), head, juce::Justification::topLeft, 2, 0.9f);
    }
private:
    MaketeoProcessor& proc; mk::Group group = mk::G_AT; std::unique_ptr<Switch> power; juce::OwnedArray<ParamRow> rows; std::unique_ptr<EqGraph> eq;
    juce::Viewport viewport; juce::Component holder;
};

class EditOverlay : public juce::Component
{
public:
    EditOverlay (MaketeoProcessor& p, mk::Group g, std::function<void()> onClose) : panel (p), cb (std::move (onClose))
    { addAndMakeVisible (panel); panel.setGroup (g); addAndMakeVisible (closeBtn); closeBtn.onClick = [this] { if (cb) cb(); }; }
    void resized() override
    {
        box = getLocalBounds().withSizeKeepingCentre (juce::jmin (860, getWidth() - 60), juce::jmin (640, getHeight() - 60));
        panel.setBounds (box.reduced (24, 20)); closeBtn.setBounds (box.getRight() - 120, box.getY() + 22, 96, 34);
    }
    void paint (Graphics& g) override { g.fillAll (juce::Colours::black.withAlpha (0.66f)); g.setColour (ui::card); g.fillRoundedRectangle (box.toFloat(), 14.0f); g.setColour (ui::border); g.drawRoundedRectangle (box.toFloat().reduced (0.5f), 14.0f, 1.2f); }
    void mouseDown (const juce::MouseEvent& e) override { if (! box.contains (e.getPosition()) && cb) cb(); }
private:
    GroupPanel panel; juce::TextButton closeBtn { "CERRAR" }; std::function<void()> cb; Rectangle<int> box;
};

// ================================================================== cards and pages
static void drawCard (Graphics& g, Rectangle<int> r, const juce::String& title)
{
    g.setColour (ui::card); g.fillRoundedRectangle (r.toFloat(), 14.0f); g.setColour (ui::border); g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 14.0f, 1.0f);
    g.setColour (ui::textDim); g.setFont (ui::font (13.0f, true).withExtraKerningFactor (0.14f)); g.drawText (title, r.getX() + 20, r.getY() + 14, r.getWidth() - 40, 20, juce::Justification::centredLeft);
}

class VocalPage : public juce::Component, private juce::Timer
{
public:
    VocalPage (MaketeoProcessor& p, LevelModel& lm) : proc (p), tuner (p), inLevel (lm, 0, T ("ENTRADA")), outLevel (lm, 1, T ("SALIDA"))
    {
        swAt = std::make_unique<Switch> (p, *specFor ("at_byp"), T ("ACTIVA"));
        auto combo = [this] (juce::ComboBox& b, const char* id, std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& a)
        { b.addItemList (juce::StringArray::fromTokens (specFor (id)->choices, "|", ""), 1); addAndMakeVisible (b); a = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, b); };
        combo (keyBox, "at_key", keyAtt); combo (scaleBox, "at_scale", scaleAtt); combo (voiceBox, "at_input", voiceAtt);
        speed = std::make_unique<DerivedSlider> (p, [this] { return 1.0f - std::sqrt (juce::jlimit (0.0f, 1.0f, plainOf (proc, "at_retune") / 400.0f)); },
                                                 [this] (float v) { setPlain (proc, "at_retune", 400.0f * (1.0f - v) * (1.0f - v)); },
                                                 [] (double v) { return v >= 0.995 ? T ("Instant\xc3\xa1nea") : juce::String (juce::roundToInt (400.0 * (1.0 - v) * (1.0 - v))) + " ms"; }, false);
        for (auto* b : { &natBtn, &hardBtn }) { b->getProperties().set ("kind", "seg"); b->setClickingTogglesState (false); addAndMakeVisible (*b); }
        natBtn.onClick = [this] { setPlain (proc, "at_mode", 0.0f); }; hardBtn.onClick = [this] { setPlain (proc, "at_mode", 1.0f); };
        latency = std::make_unique<LiveText> ([this] { const double ms = proc.getSampleRate() > 0 ? 1000.0 * proc.getLatencySamples() / proc.getSampleRate() : 0.0;
            return plainOf (proc, "at_byp") < 0.5f ? T ("Latencia: +") + juce::String (ms, 1) + T (" ms (el DAW la compensa)") : T ("Sin latencia mientras la afinaci\xc3\xb3n est\xc3\xa1 apagada"); }, 13.0f, false, [] { return ui::textDim; }, juce::Justification::centredLeft, 2);
        for (auto* c : std::initializer_list<juce::Component*> { &tuner, swAt.get(), speed.get(), latency.get(), &inLevel, &outLevel }) addAndMakeVisible (*c);

        auto mk1 = [this] (std::function<float()> g, std::function<void (float)> s) { auto* d = new DerivedSlider (proc, std::move (g), std::move (s), {}); voz.add (d); addAndMakeVisible (d); };
        auto P = [this] (const char* id) { return plainOf (proc, id); };
        mk1 ([=] { return P ("rd_byp") > 0.5f ? 0.0f : (P ("rd_range") - 3.0f) / 12.0f; },
             [this] (float v) { setPlain (proc, "rd_byp", v < 0.02f ? 1.0f : 0.0f); if (v >= 0.02f) { setPlain (proc, "rd_range", 3.0f + 12.0f * v); setPlain (proc, "rd_up", juce::jmin (12.0f, 3.0f + 9.0f * v)); setPlain (proc, "rd_down", juce::jmin (12.0f, 3.0f + 9.0f * v)); } });
        mk1 ([=] { return P ("dh_byp") > 0.5f ? 0.0f : P ("dh_range") / 9.0f; },
             [this] (float v) { setPlain (proc, "dh_byp", v < 0.02f ? 1.0f : 0.0f); setPlain (proc, "dh_range", 9.0f * v); setPlain (proc, "dh_sens", 0.3f + 0.6f * v);
                                setPlain (proc, "dl_byp", v < 0.6f ? 1.0f : 0.0f); setPlain (proc, "dl_range", 6.0f * juce::jmax (0.0f, v - 0.6f) / 0.4f); });
        mk1 ([=] { return P ("u47_byp") > 0.5f ? 0.0f : P ("u47_prox"); },
             [this] (float v) { setPlain (proc, "u47_byp", v < 0.02f ? 1.0f : 0.0f); setPlain (proc, "u47_prox", v); setPlain (proc, "u47_body", 0.8f * v); setPlain (proc, "eq2_on", 1.0f); setPlain (proc, "eq2_gain", 3.0f * v); });
        mk1 ([=] { return juce::jlimit (0.0f, 1.0f, P ("eq5_gain") / 5.0f); }, [this] (float v) { setPlain (proc, "eq5_on", 1.0f); setPlain (proc, "eq5_gain", 5.0f * v); });
        mk1 ([=] { return P ("hw_byp") > 0.5f ? 0.0f : P ("hw_sat"); },
             [this] (float v) { setPlain (proc, "hw_byp", v < 0.02f ? 1.0f : 0.0f); setPlain (proc, "hw_drive", 0.1f + 0.6f * v); setPlain (proc, "hw_sat", v); setPlain (proc, "fet_color", 0.6f * v); });
        comp = std::make_unique<DerivedSlider> (p, [=] { return (P ("fet_byp") > 0.5f && P ("la_byp") > 0.5f) ? 0.0f : (P ("fet_in") - 2.0f) / 16.0f; },
                                                [this] (float v) { const float off = v < 0.02f ? 1.0f : 0.0f; setPlain (proc, "fet_byp", off); setPlain (proc, "la_byp", off); setPlain (proc, "fet_in", 2.0f + 16.0f * v); setPlain (proc, "la_pr", 0.15f + 0.6f * v); }, std::function<juce::String (double)>());
        addAndMakeVisible (*comp);
        riderBar = std::make_unique<DbBar> ([this] { return proc.meter (mk::MT_RIDER); }, -12.0f, 12.0f, true, ui::success, 8.0f);
        deHigh = std::make_unique<DbBar> ([this] { return proc.meter (mk::MT_DH); }, 0.0f, 12.0f, false, ui::accent, 8.0f);
        deLow = std::make_unique<DbBar> ([this] { return proc.meter (mk::MT_DL); }, 0.0f, 12.0f, false, ui::accent, 8.0f);
        grBar = std::make_unique<DbBar> ([this] { return proc.meter (mk::MT_FET) + proc.meter (mk::MT_LA); }, 0.0f, 18.0f, false, ui::accent, 18.0f);
        riderTxt = std::make_unique<LiveText> ([this] { const float v = proc.meter (mk::MT_RIDER); return (plainOf (proc, "rd_byp") > 0.5f ? T ("apagado") : juce::String (v >= 0 ? "+" : "") + juce::String (v, 1) + " dB"); }, 13.0f, true, [] { return ui::textDim; }, juce::Justification::centredRight);
        deTxt = std::make_unique<LiveText> ([this] { return juce::String ("-") + juce::String (proc.meter (mk::MT_DH), 1) + juce::String (" | -") + juce::String (proc.meter (mk::MT_DL), 1) + " dB"; }, 13.0f, true, [] { return ui::textDim; }, juce::Justification::centredRight);
        grTxt = std::make_unique<LiveText> ([this] { const float g = proc.meter (mk::MT_FET) + proc.meter (mk::MT_LA); return juce::String (g > 0.05f ? "-" : "") + juce::String (g, 1) + " dB"; }, 40.0f, true, [] { return ui::text; });
        grDetail = std::make_unique<LiveText> ([this] { return juce::String ("1176 -") + juce::String (proc.meter (mk::MT_FET), 1) + juce::String (" dB   |   LA-2A -") + juce::String (proc.meter (mk::MT_LA), 1) + " dB"; }, 13.5f, false, [] { return ui::textDim; });
        for (auto* c : std::initializer_list<juce::Component*> { riderBar.get(), deHigh.get(), deLow.get(), grBar.get(), riderTxt.get(), deTxt.get(), grTxt.get(), grDetail.get() }) addAndMakeVisible (*c);
        startTimerHz (20);
    }
    void resized() override
    {
        auto area = getLocalBounds().reduced (20, 14); const int gap = 16; const int colW = (area.getWidth() - gap) / 2; const int row1 = (int) ((area.getHeight() - gap) * 0.585f);
        cA = { area.getX(), area.getY(), colW, row1 }; cB = { area.getX() + colW + gap, area.getY(), area.getWidth() - colW - gap, row1 };
        cC = { area.getX(), area.getY() + row1 + gap, colW, area.getHeight() - row1 - gap }; cD = { cB.getX(), cC.getY(), cB.getWidth(), cC.getHeight() };
        // AFINACION
        { auto in = cA.reduced (20, 0).withTrimmedTop (44).withTrimmedBottom (14); swAt->setBounds (cA.getRight() - 150, cA.getY() + 10, 134, 28);
          auto left = in.removeFromLeft ((int) (in.getWidth() * 0.38f)); in.removeFromLeft (18);
          latency->setBounds (left.removeFromBottom (36)); tuner.setBounds (left);
          const int hA = 54, hB = 54, hC = 40, hD = 54; const int gap = juce::jmax (6, (in.getHeight() - (hA + hB + hC + hD)) / 3);
          auto rowA = in.removeFromTop (hA); in.removeFromTop (gap); auto rowB = in.removeFromTop (hB); in.removeFromTop (gap); auto rowC = in.removeFromTop (hC); in.removeFromTop (gap); auto rowD = in.removeFromTop (hD);
          const int hw = (rowA.getWidth() - 10) / 2; keyBox.setBounds (rowA.removeFromLeft (hw).withTrimmedTop (hA - 34)); rowA.removeFromLeft (10); scaleBox.setBounds (rowA.withTrimmedTop (hA - 34));
          speed->setBounds (rowB.withTrimmedTop (hB - 34));
          const int sw = (rowC.getWidth() - 10) / 2; natBtn.setBounds (rowC.removeFromLeft (sw)); hardBtn.setBounds (rowC.withTrimmedLeft (10));
          voiceBox.setBounds (rowD.withTrimmedTop (hD - 34).withWidth (juce::jmin (rowD.getWidth(), 220))); }
        // VOZ
        { auto in = cB.reduced (20, 0).withTrimmedTop (44).withTrimmedBottom (12); const int rh = in.getHeight() / 5; rowsB.clear();
          for (int i = 0; i < 5; ++i) { auto r = in.removeFromTop (rh); rowsB.add (r); auto sl = r.withTrimmedLeft (juce::jlimit (180, 250, (int) (r.getWidth() * 0.40f)) + 6); voz[i]->setBounds (sl.removeFromTop (juce::jmin (34, rh - 8)));
              if (i == 0) { auto m = sl.withTrimmedLeft (10).withTrimmedRight (104); riderBar->setBounds (m.withHeight (10).withY (voz[i]->getBottom() + 3)); riderTxt->setBounds (Rectangle<int> (voz[i]->getRight() - 104, voz[i]->getBottom() - 4, 104, 16)); }
              else if (i == 1) { auto m = sl.withTrimmedLeft (10).withTrimmedRight (104); const int hh = (m.getWidth() - 10) / 2; deHigh->setBounds (m.removeFromLeft (hh).withHeight (10).withY (voz[i]->getBottom() + 3)); deLow->setBounds (m.withTrimmedLeft (10).withHeight (10).withY (voz[i]->getBottom() + 3)); deTxt->setBounds (Rectangle<int> (voz[i]->getRight() - 104, voz[i]->getBottom() - 4, 104, 16)); } }
          for (auto* b : { riderBar.get(), deHigh.get(), deLow.get() }) b->setVisible (rh >= 46); for (auto* t : { riderTxt.get(), deTxt.get() }) t->setVisible (rh >= 46); }
        // DINAMICA
        { auto in = cC.reduced (20, 0).withTrimmedTop (44).withTrimmedBottom (14); compLabel = in.removeFromTop (22); comp->setBounds (in.removeFromTop (36)); in.removeFromTop (8);
          auto r = in.removeFromTop (juce::jmin (50, in.getHeight() - 24)); grTxt->setBounds (r.removeFromLeft (150)); grBar->setBounds (r.withTrimmedLeft (10).withSizeKeepingCentre (r.getWidth() - 10, 18)); grDetail->setBounds (in.removeFromTop (22)); }
        // NIVELES
        { auto in = cD.reduced (20, 0).withTrimmedTop (36).withTrimmedBottom (8); const int h = in.getHeight() / 2; inLevel.setBounds (in.removeFromTop (h)); outLevel.setBounds (in); }
    }
    void paint (Graphics& g) override
    {
        drawCard (g, cA, T ("AFINACI\xc3\x93N")); drawCard (g, cB, "VOZ"); drawCard (g, cC, T ("DIN\xc3\x81MICA")); drawCard (g, cD, "NIVELES");
        auto label = [&] (Rectangle<int> r, const juce::String& t) { g.setColour (ui::textDim); g.setFont (ui::font (12.5f, true).withExtraKerningFactor (0.1f)); g.drawText (t, r, juce::Justification::centredLeft); };
        label (keyBox.getBounds().withY (keyBox.getY() - 20).withHeight (18), "TONALIDAD"); label (scaleBox.getBounds().withY (scaleBox.getY() - 20).withHeight (18), "ESCALA");
        label (speed->getBounds().withY (speed->getY() - 20).withHeight (18), T ("VELOCIDAD DE CORRECCI\xc3\x93N")); label (voiceBox.getBounds().withY (voiceBox.getY() - 20).withHeight (18), "TIPO DE VOZ");
        const char* names[] = { "Nivel autom\xc3\xa1tico", "Limpieza de sibilancias", "Cuerpo", "Presencia", "Color" };
        const char* descs[] = { "Mantiene la voz pareja sin tocar el fader.", "Suaviza las eses. Barras: agudas y graves.", "M\xc3\xa1s calidez y peso en la voz.", "M\xc3\xa1s claridad para que se destaque.", "Car\xc3\xa1""cter y saturaci\xc3\xb3n suave." };
        for (int i = 0; i < rowsB.size(); ++i)
        { const int lines = rowsB[i].getHeight() >= 56 ? 2 : 1; auto r = rowsB[i].withWidth (juce::jlimit (180, 250, (int) (rowsB[i].getWidth() * 0.40f))); g.setColour (ui::text); g.setFont (ui::font (16.0f, true)); g.drawFittedText (T (names[i]), r.removeFromTop (r.getHeight() / 2 + 2), juce::Justification::bottomLeft, 1, 0.8f);
          g.setColour (ui::textDim); g.setFont (ui::font (13.0f)); g.drawFittedText (T (descs[i]), r.withTrimmedRight (6), juce::Justification::topLeft, lines, 0.8f); }
        g.setColour (ui::text); g.setFont (ui::font (16.0f, true)); g.drawText (T ("Intensidad de compresi\xc3\xb3n"), compLabel.withWidth (260), juce::Justification::centredLeft);
        g.setColour (ui::textDim); g.setFont (ui::font (13.0f)); g.drawText (T ("m\xc3\xa1s = voz m\xc3\xa1s firme y constante"), compLabel.withTrimmedLeft (250), juce::Justification::centredLeft);
        g.setColour (ui::textDim); g.setFont (ui::font (12.5f, true).withExtraKerningFactor (0.1f)); g.drawText (T ("REDUCCI\xc3\x93N DE GANANCIA"), grBar->getBounds().withY (grBar->getY() - 22).withHeight (18), juce::Justification::centredLeft);
    }
    void repaintLevels() { inLevel.repaint(); outLevel.repaint(); }
private:
    void timerCallback() override { natBtn.setToggleState (plainOf (proc, "at_mode") < 0.5f, juce::dontSendNotification); hardBtn.setToggleState (plainOf (proc, "at_mode") > 0.5f, juce::dontSendNotification); natBtn.setButtonText ("NATURAL"); hardBtn.setButtonText ("HARD TUNE"); }
    MaketeoProcessor& proc; TunerDisplay tuner; LevelMeter inLevel, outLevel; std::unique_ptr<Switch> swAt; juce::ComboBox keyBox, scaleBox, voiceBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAtt, scaleAtt, voiceAtt; std::unique_ptr<DerivedSlider> speed, comp; juce::TextButton natBtn, hardBtn;
    std::unique_ptr<LiveText> latency, riderTxt, deTxt, grTxt, grDetail; std::unique_ptr<DbBar> riderBar, deHigh, deLow, grBar; juce::OwnedArray<DerivedSlider> voz;
    Rectangle<int> cA, cB, cC, cD, compLabel; juce::Array<Rectangle<int>> rowsB;
};

class FxPage : public juce::Component
{
public:
    FxPage (MaketeoEditor& e) : ed (e)
    {
        struct Def { mk::Group g; const char* title; const char* desc; const char* mix; int meter; } defs[] = {
            { mk::G_RVB, "REVERB", "Espacio y profundidad para la voz.", "rvb_mix", mk::MT_PAR0 + 0 }, { mk::G_DLY, "DELAY", "Ecos sincronizados con el tempo.", "dly_mix", mk::MT_PAR0 + 1 },
            { mk::G_DBL, "DOBLE VOZ", "Dobla la voz para dar amplitud.", "dbl_mix", mk::MT_PAR0 + 4 }, { mk::G_BR, "BRILLO", "Aire y claridad en los agudos.", "br_mix", mk::MT_PAR0 + 3 },
            { mk::G_TB, "SATURACI\xc3\x93N", "Calidez de v\xc3\xa1lvula y cuerpo.", "tb_mix", mk::MT_PAR0 + 5 }, { mk::G_NY, "PARALELO", "Compresi\xc3\xb3n paralela New York: m\xc3\xa1s cuerpo.", "ny_mix", mk::MT_PAR0 + 2 } };
        for (auto& d : defs)
        {
            auto* c = new Card(); c->group = d.g; c->title = T (d.title); c->desc = T (d.desc);
            c->sw = std::make_unique<Switch> (e.proc, *powerOf (d.g), "ACTIVO"); c->mix = std::make_unique<ParamSlider> (e.proc, *specFor (d.mix), 16.0f);
            const int mid = d.meter; c->bar = std::make_unique<DbBar> ([this, mid] { return juce::Decibels::gainToDecibels (ed.proc.meter (mid), -60.0f); }, -60.0f, 0.0f, false, ui::accent, 8.0f);
            c->edit.setButtonText ("EDITAR"); auto g = d.g; c->edit.onClick = [this, g] { ed.openEdit (g); };
            c->addAndMakeVisible (*c->sw); c->addAndMakeVisible (*c->mix); c->addAndMakeVisible (*c->bar); c->addAndMakeVisible (c->edit); cards.add (c); addAndMakeVisible (c);
        }
    }
    void resized() override
    {
        auto area = getLocalBounds().reduced (20, 14); const int gap = 16, cw = (area.getWidth() - 2 * gap) / 3, ch = (area.getHeight() - gap) / 2;
        for (int i = 0; i < cards.size(); ++i) cards[i]->setBounds (area.getX() + (i % 3) * (cw + gap), area.getY() + (i / 3) * (ch + gap), cw, ch);
    }
private:
    struct Card : public juce::Component
    {
        mk::Group group = mk::G_RVB; juce::String title, desc; std::unique_ptr<Switch> sw; std::unique_ptr<ParamSlider> mix; std::unique_ptr<DbBar> bar; juce::TextButton edit;
        void resized() override
        {
            auto r = getLocalBounds().reduced (20, 0); sw->setBounds (r.removeFromTop (46).removeFromRight (130).withSizeKeepingCentre (130, 28).translated (0, 8));
            auto bottom = r.removeFromBottom (48); edit.setBounds (bottom.removeFromRight (120).withTrimmedTop (6).withTrimmedBottom (6)); bar->setBounds (bottom.withTrimmedRight (14).withSizeKeepingCentre (bottom.getWidth() - 14, 10));
            r.removeFromTop (62); mix->setBounds (r.removeFromTop (46));
        }
        void paint (Graphics& g) override
        {
            drawCard (g, getLocalBounds(), title); g.setColour (ui::textDim); g.setFont (ui::font (14.5f)); g.drawFittedText (desc, Rectangle<int> (20, 42, getWidth() - 40, 44), juce::Justification::topLeft, 2, 0.9f);
            g.setColour (ui::text); g.setFont (ui::font (15.0f, true)); g.drawText ("Cantidad", 20, 92, 140, 22, juce::Justification::centredLeft);
            g.setColour (ui::textDim); g.setFont (ui::font (12.5f, true).withExtraKerningFactor (0.1f)); g.drawText ("ACTIVIDAD", 20, getHeight() - 42, 100, 18, juce::Justification::centredLeft);
        }
    };
    MaketeoEditor& ed; juce::OwnedArray<Card> cards;
};

class AdvancedPage : public juce::Component, private juce::ListBoxModel
{
public:
    explicit AdvancedPage (MaketeoProcessor& p) : panel (p) { list.setModel (this); list.setRowHeight (44); list.setColour (juce::ListBox::backgroundColourId, ui::card); addAndMakeVisible (list); addAndMakeVisible (panel); list.selectRow (1); }
    void resized() override { auto r = getLocalBounds().reduced (20, 14); list.setBounds (r.removeFromLeft (250)); r.removeFromLeft (20); panel.setBounds (r.reduced (4, 0)); }
    void paint (Graphics& g) override { g.setColour (ui::card); g.fillRoundedRectangle (list.getBounds().toFloat(), 14.0f); g.setColour (ui::border); g.drawRoundedRectangle (list.getBounds().toFloat().reduced (0.5f), 14.0f, 1.0f); }
private:
    int getNumRows() override { return mk::G_COUNT; }
    void paintListBoxItem (int row, Graphics& g, int w, int h, bool sel) override
    {
        if (row < 0 || row >= mk::G_COUNT) return; auto r = Rectangle<float> (8, 3, (float) w - 16, (float) h - 6);
        if (sel) { g.setColour (ui::accent.withAlpha (0.18f)); g.fillRoundedRectangle (r, 8.0f); g.setColour (ui::accent); g.fillRoundedRectangle (r.withWidth (4.0f), 2.0f); }
        g.setColour (sel ? ui::text : ui::textDim); g.setFont (ui::font (15.0f, sel)); g.drawText (T (kGroups[row].name), r.reduced (16, 0).toNearestInt(), juce::Justification::centredLeft);
    }
    void selectedRowsChanged (int row) override { if (row >= 0 && row < mk::G_COUNT) panel.setGroup ((mk::Group) row); }
    juce::ListBox list; GroupPanel panel;
};

// ================================================================== editor
struct MaketeoEditor::Impl
{
    explicit Impl (MaketeoEditor& e) : ed (e), proc (e.proc), levels (e.proc) {}
    MaketeoEditor& ed; MaketeoProcessor& proc; MkLookAndFeel laf; LevelModel levels;
    juce::TextButton tabs[3]; juce::ComboBox presetBox; juce::TextButton prev { "<" }, next { ">" }, save { "GUARDAR" }, undoB { "UNDO" }, redoB { "REDO" }, menu { "..." };
    std::unique_ptr<VocalPage> vocal; std::unique_ptr<FxPage> fx; std::unique_ptr<AdvancedPage> adv;
    std::unique_ptr<LiveText> footLeft, footRight; std::unique_ptr<EditOverlay> overlay; std::unique_ptr<juce::AlertWindow> saveWindow;
    juce::Array<juce::File> userFiles; int numFactory = 0, page = 0;

    void showPage (int i) { page = i; for (int k = 0; k < 3; ++k) tabs[k].setToggleState (k == i, juce::dontSendNotification); vocal->setVisible (i == 0); fx->setVisible (i == 1); adv->setVisible (i == 2); }
    void refreshPresets()
    {
        presetBox.clear (juce::dontSendNotification); auto names = proc.factoryPresetNames(); numFactory = names.size();
        for (int i = 0; i < names.size(); ++i) presetBox.addItem (names[i], i + 1);
        userFiles = proc.userPresetFiles(); if (! userFiles.isEmpty()) { presetBox.addSeparator(); for (int i = 0; i < userFiles.size(); ++i) presetBox.addItem ("USER  " + userFiles[i].getFileNameWithoutExtension(), 1000 + i); }
        const auto cur = proc.currentPresetName();
        for (int i = 0; i < names.size(); ++i) if (names[i] == cur) presetBox.setSelectedId (i + 1, juce::dontSendNotification);
        for (int i = 0; i < userFiles.size(); ++i) if (userFiles[i].getFileNameWithoutExtension() == cur) presetBox.setSelectedId (1000 + i, juce::dontSendNotification);
    }
    void promptSave()
    {
        saveWindow = std::make_unique<juce::AlertWindow> ("Guardar preset", "Nombre del preset:", juce::MessageBoxIconType::NoIcon);
        saveWindow->addTextEditor ("n", proc.currentPresetName() == "DEFAULT" ? juce::String() : proc.currentPresetName());
        saveWindow->addButton ("Guardar", 1, juce::KeyPress (juce::KeyPress::returnKey)); saveWindow->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        juce::Component::SafePointer<MaketeoEditor> sp (&ed);
        saveWindow->enterModalState (true, juce::ModalCallbackFunction::create ([sp] (int r)
        {
            if (sp == nullptr) return;
            if (r == 1) { sp->impl->proc.saveUserPreset (sp->impl->saveWindow->getTextEditorContents ("n")); sp->impl->refreshPresets(); }
            juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->impl->saveWindow.reset(); });
        }), false);
    }
    void showMenu()
    {
        juce::PopupMenu m; m.setLookAndFeel (&laf); m.addItem (1, "Restablecer todo (DEFAULT)"); m.addItem (2, "Abrir carpeta de presets"); juce::Component::SafePointer<MaketeoEditor> sp (&ed);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menu), [sp] (int r)
        {
            if (sp == nullptr) return;
            if (r == 1) sp->impl->proc.resetAllParameters();
            else if (r == 2) { auto d = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Evric Hunter").getChildFile ("MAKETEO").getChildFile ("Presets"); d.createDirectory(); d.revealToUser(); }
        });
    }
};

MaketeoEditor::MaketeoEditor (MaketeoProcessor& p) : juce::AudioProcessorEditor (&p), proc (p), impl (std::make_unique<Impl> (*this))
{
    auto& I = *impl; setLookAndFeel (&I.laf);
    I.vocal = std::make_unique<VocalPage> (p, I.levels); I.fx = std::make_unique<FxPage> (*this); I.adv = std::make_unique<AdvancedPage> (p);
    const char* tabNames[] = { "VOCAL", "FX", "ADVANCED" };
    for (int i = 0; i < 3; ++i) { auto& t = I.tabs[i]; t.setButtonText (tabNames[i]); t.getProperties().set ("kind", "tab"); t.setClickingTogglesState (false); t.onClick = [this, i] { impl->showPage (i); }; addAndMakeVisible (t); }
    for (auto* c : std::initializer_list<juce::Component*> { I.vocal.get(), I.fx.get(), I.adv.get(), &I.presetBox, &I.prev, &I.next, &I.save, &I.undoB, &I.redoB, &I.menu }) addAndMakeVisible (*c);
    I.footLeft = std::make_unique<LiveText> ([this] { const double ms = proc.getSampleRate() > 0 ? 1000.0 * proc.getLatencySamples() / proc.getSampleRate() : 0.0;
        return "LATENCIA " + juce::String (ms, 1) + " ms     CPU " + juce::String (juce::roundToInt (proc.cpuLoad.load() * 100.0f)) + " %"; }, 13.0f, true, [] { return ui::textDim; });
    I.footRight = std::make_unique<LiveText> ([this] { return "ENTRADA " + dbText (impl->levels.peakHold (0)) + "     SALIDA " + dbText (impl->levels.peakHold (1)) + " dBFS"; }, 13.0f, true,
                                              [this] { return (impl->levels.clip[0] || impl->levels.clip[1]) ? ui::error : ui::textDim; }, juce::Justification::centredRight);
    addAndMakeVisible (*I.footLeft); addAndMakeVisible (*I.footRight);
    I.refreshPresets();
    I.presetBox.onChange = [this] { const int id = impl->presetBox.getSelectedId(); if (id >= 1 && id <= impl->numFactory) proc.loadFactoryPreset (id - 1); else if (id >= 1000 && id - 1000 < impl->userFiles.size()) proc.loadUserPreset (impl->userFiles[id - 1000]); };
    auto step = [this] (int d) { auto& pb = impl->presetBox; const int n = pb.getNumItems(); if (n > 0) pb.setSelectedItemIndex ((pb.getSelectedItemIndex() + d + n) % n, juce::sendNotification); };
    I.prev.onClick = [step] { step (-1); }; I.next.onClick = [step] { step (1); };
    I.save.onClick = [this] { impl->promptSave(); }; I.undoB.onClick = [this] { proc.undo.undo(); }; I.redoB.onClick = [this] { proc.undo.redo(); }; I.menu.onClick = [this] { impl->showMenu(); };
    I.showPage (0);
    setResizable (true, true); setResizeLimits (1000, 680, 2200, 1400); setSize (1200, 760);
    sendLookAndFeelChange(); startTimerHz (30);
}
MaketeoEditor::~MaketeoEditor() { stopTimer(); setLookAndFeel (nullptr); }

void MaketeoEditor::paint (Graphics& g)
{
    g.fillAll (ui::bg);
    g.setColour (ui::card); g.fillRect (0, 0, getWidth(), 72); g.setColour (ui::border); g.fillRect (0, 72, getWidth(), 1);
    g.setColour (ui::card); g.fillRect (0, getHeight() - 36, getWidth(), 36); g.setColour (ui::border); g.fillRect (0, getHeight() - 36, getWidth(), 1);
    g.setColour (ui::text); g.setFont (ui::font (24.0f, true).withExtraKerningFactor (0.14f)); g.drawText ("MAKETEO", 20, 10, 190, 34, juce::Justification::centredLeft);
    g.setColour (ui::accent); g.setFont (ui::font (12.5f, true).withExtraKerningFactor (0.22f)); g.drawText ("BY EVRIC HUNTER", 21, 42, 190, 18, juce::Justification::centredLeft);
}
void MaketeoEditor::resized()
{
    auto& I = *impl; const int W = getWidth(), H = getHeight();
    int x = 206; for (auto& t : I.tabs) { t.setBounds (x, 20, 94, 34); x += 98; }
    int rx = W - 16; auto put = [&] (juce::Component& c, int w) { c.setBounds (rx - w, 20, w, 34); rx -= w + 6; };
    put (I.menu, 34); put (I.redoB, 50); put (I.undoB, 50); put (I.save, 84); rx -= 6;
    const int avail = rx - (x + 14), comboW = juce::jlimit (150, 290, avail - 64);
    const int px = x + 14 + juce::jmax (0, (avail - comboW - 64) / 2);
    I.prev.setBounds (px, 20, 28, 34); I.presetBox.setBounds (px + 32, 20, comboW, 34); I.next.setBounds (px + 36 + comboW, 20, 28, 34);
    auto pages = Rectangle<int> (0, 73, W, H - 73 - 36); I.vocal->setBounds (pages); I.fx->setBounds (pages); I.adv->setBounds (pages);
    I.footLeft->setBounds (20, H - 34, W / 2 - 20, 32); I.footRight->setBounds (W / 2, H - 34, W / 2 - 20, 32);
    if (I.overlay) I.overlay->setBounds (getLocalBounds());
    I.undoB.setEnabled (true);
}
void MaketeoEditor::openEdit (mk::Group g)
{
    juce::Component::SafePointer<MaketeoEditor> sp (this);
    impl->overlay = std::make_unique<EditOverlay> (proc, g, [sp] { juce::MessageManager::callAsync ([sp] { if (sp != nullptr) sp->impl->overlay.reset(); }); });
    addAndMakeVisible (*impl->overlay); impl->overlay->setBounds (getLocalBounds()); sendLookAndFeelChange(); impl->overlay->toFront (true);
}
void MaketeoEditor::timerCallback()
{
    impl->levels.tick(); impl->vocal->repaintLevels();
    impl->undoB.setEnabled (proc.undo.canUndo()); impl->redoB.setEnabled (proc.undo.canRedo());
}
