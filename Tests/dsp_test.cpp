// MAKETEO headless validation (no JUCE). Build:  g++ -O2 -std=c++17 -I../Source dsp_test.cpp -o dsp_test && ./dsp_test
// Extra: ./dsp_test --docs > ../docs/PARAMETERS.md   |   ./dsp_test --cpu   (CPU benchmark only)
#include "DSP/Engine.h"
#include "Presets.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>

using namespace mk;
static int fails = 0;
static std::vector<std::string> failedChecks;
#define CHECK(cond, ...) do { \
    const bool ok_ = (cond); char buf_[512]; std::snprintf (buf_, sizeof buf_, __VA_ARGS__); \
    std::printf ("  %s  %s\n", ok_ ? "PASS" : "FAIL", buf_); \
    if (! ok_) { ++fails; failedChecks.push_back ("line " + std::to_string (__LINE__) + ": " + buf_); \
                 std::cerr << "[CHECK FAILED] dsp_test.cpp:" << __LINE__ << ": " << buf_ << std::endl; } \
} while (0)

static std::map<std::string, int>& idMap() { static std::map<std::string, int> m; if (m.empty()) for (int i = 0; i < kNumParams; ++i) m[paramSpecs()[i].id] = i; return m; }
struct Rig
{
    Engine e; float v[kNumParams]; double fs;
    explicit Rig (double sr = 48000, bool allOff = true) : fs (sr)
    {
        e.prepare (fs); for (int i = 0; i < kNumParams; ++i) v[i] = paramSpecs()[i].def;
        if (allOff) quiet();
        e.setParams (v, true);
    }
    void quiet() { for (const char* b : { "at_byp", "rd_byp", "hw_byp", "u47_byp", "rv_byp", "eq_byp", "dh_byp", "dl_byp", "fet_byp", "la_byp", "ssl_byp", "mg_byp" }) set (b, 1);
                   for (const char* b : { "rvb_on", "dly_on", "ny_on", "br_on", "dbl_on", "tb_on" }) set (b, 0); }
    Rig& set (const char* id, double x) { v[idMap().at (id)] = (float) x; return *this; }
    Rig& on (const char* byp) { return set (byp, 0); }
    void apply (bool snap = true) { e.setParams (v, snap); }
    void run (std::vector<float>& l, std::vector<float>* r = nullptr, int block = 256)
    {
        for (size_t p = 0; p < l.size(); p += block)
        {
            const int m = (int) std::min<size_t> (block, l.size() - p);
            float* ptr[2] = { l.data() + p, r ? r->data() + p : nullptr };
            e.setParams (v); e.process (ptr, r ? 2 : 1, m);
        }
    }
};
static std::vector<float> sine (double fs, double f, double a, int n) { std::vector<float> x ((size_t) n); for (int i = 0; i < n; ++i) x[(size_t) i] = (float) (a * std::sin (2 * kPi * f * i / fs)); return x; }
static std::vector<float> noise (int n, float a, unsigned seed = 1)
{
    std::mt19937 g (seed); std::vector<float> x ((size_t) n);
    for (auto& s : x) s = (float) (a * ((double) g() * (1.0 / 4294967296.0) * 2.0 - 1.0));
    return x;
}
static double peak (const std::vector<float>& a, size_t from = 0) { double m = 0; for (size_t i = from; i < a.size(); ++i) m = std::max (m, (double) std::fabs (a[i])); return m; }
static double rms (const std::vector<float>& a, size_t from, size_t to) { double s = 0; for (size_t i = from; i < to && i < a.size(); ++i) s += a[i] * (double) a[i]; return std::sqrt (s / std::max<size_t> (1, to - from)); }
static double maxDiff (const std::vector<float>& a, const std::vector<float>& b, size_t from = 0) { double m = 0; for (size_t i = from; i < a.size(); ++i) m = std::max (m, (double) std::fabs (a[i] - b[i])); return m; }
static bool finite (const std::vector<float>& a) { for (float x : a) if (! std::isfinite (x)) return false; return true; }
static double goertzel (const std::vector<float>& x, double fs, double f, size_t skip)
{ double w = 2 * kPi * f / fs, c = 2 * std::cos (w), s1 = 0, s2 = 0; size_t n = 0; for (size_t i = skip; i < x.size(); ++i, ++n) { double s = x[i] + c * s1 - s2; s2 = s1; s1 = s; }
  double re = s1 - s2 * std::cos (w), im = s2 * std::sin (w); return std::sqrt (re * re + im * im) * 2.0 / (double) n; }
// gain in dB of the configured chain at frequency f (sine, steady state)
static double gainAt (Rig& g, double f, double a = 0.05)
{ g.e.reset(); auto x = sine (g.fs, f, a, (int) g.fs); auto y = x; g.run (y); return 20 * std::log10 (goertzel (y, g.fs, f, (size_t) g.fs / 2) / goertzel (x, g.fs, f, (size_t) g.fs / 2)); }
// fundamental estimate by autocorrelation with parabolic refinement
static double estimateHz (const std::vector<float>& x, double fs, size_t from, size_t len, double fmin = 80, double fmax = 500)
{
    const int t0 = (int) (fs / fmax), t1 = (int) (fs / fmin); std::vector<double> ac ((size_t) t1 + 2); double best = -1e30;
    for (int t = t0 - 1; t <= t1 + 1; ++t) { double s = 0; for (size_t i = from; i < from + len; ++i) s += (double) x[i] * x[i + (size_t) t]; ac[(size_t) t] = s; if (t >= t0 && t <= t1) best = std::max (best, s); }
    int bt = t0; // smallest lag that is a local maximum within 8 % of the best one (avoids sub-octave picks)
    for (int t = t0; t <= t1; ++t) if (ac[(size_t) t] >= 0.92 * best && ac[(size_t) t] >= ac[(size_t) t - 1] && ac[(size_t) t] >= ac[(size_t) t + 1]) { bt = t; break; }
    const double a = ac[(size_t) bt - 1], b = ac[(size_t) bt], c = ac[(size_t) bt + 1], d = a - 2 * b + c;
    return fs / (bt + (std::fabs (d) > 1e-12 ? 0.5 * (a - c) / d : 0.0));
}
static double cents (double f, double ref) { return 1200.0 * std::log2 (f / ref); }
// synthetic "voice": harmonic-rich glottal-ish source with a couple of formant-like emphasis, optional sibilant burst
static std::vector<float> voice (double fs, double f0, int n, double amp = 0.25)
{
    std::vector<float> v ((size_t) n);
    for (int i = 0; i < n; ++i) { double t = (double) i / fs, s = 0; for (int h = 1; h <= 20; ++h) s += std::sin (2 * kPi * f0 * h * t) / h * (1.0 + 1.5 * std::exp (-std::pow ((f0 * h - 700) / 300, 2)) + 1.0 * std::exp (-std::pow ((f0 * h - 1800) / 400, 2))); v[(size_t) i] = (float) (amp * s * 0.6); }
    return v;
}

int main (int argc, char** argv)
{
    std::setvbuf (stdout, nullptr, _IONBF, 0);
    if (argc > 1 && ! std::strcmp (argv[1], "--docs"))
    {
        static const char* gn[] = { "I/O", "Autotune Pro", "Vocal Rider", "Hollywood Preamp", "U47 Microphone", "Revival", "Parametric EQ (8 band)", "De-Esser High", "De-Esser Low", "1176 Rev A", "LA-2A", "SSL 4000 J EQ", "Maag EQ4", "Reverb", "Delay", "New York Compression", "Brightness Engine", "Doubler", "Tube Saturation" };
        std::printf ("# MAKETEO parameter reference\n\nAll %d parameters are host-automatable. Percent parameters are stored 0..1 and shown 0..100 %%.\n", (int) kNumParams);
        int gcur = -1;
        for (int i = 0; i < kNumParams; ++i)
        {
            const auto& s = paramSpecs()[i];
            if ((int) s.group != gcur) { gcur = (int) s.group; std::printf ("\n## %s\n\n| ID | Name | Range | Default | Unit |\n|---|---|---|---|---|\n", gn[gcur]); }
            char range[160], def[64];
            if (s.kind == K_BOOL) { std::snprintf (range, sizeof range, "off / on"); std::snprintf (def, sizeof def, "%s", s.def > 0.5f ? "on" : "off"); }
            else if (s.kind == K_CHOICE) { std::snprintf (range, sizeof range, "%s", s.choices); std::string c = s.choices; int k = (int) s.def; size_t p = 0; while (k-- > 0) p = c.find ('|', p) + 1; std::snprintf (def, sizeof def, "%s", c.substr (p, c.find ('|', p) - p).c_str()); }
            else { std::snprintf (range, sizeof range, "%g .. %g%s", s.lo, s.hi, s.map == M_LOG ? " (log)" : ""); std::snprintf (def, sizeof def, "%g", s.def); }
            std::printf ("| `%s` | %s | %s | %s | %s |\n", s.id, s.label, range, def, s.unit);
        }
        return 0;
    }
    const bool cpuOnly = argc > 1 && ! std::strcmp (argv[1], "--cpu");
    const double fs = 48000; const int N = 48000;

    if (! cpuOnly)
    {
    std::printf ("1. Parameter table and presets\n");
    {
        bool ok = true; std::map<std::string, int> seen;
        for (int i = 0; i < kNumParams; ++i)
        {
            const auto& s = paramSpecs()[i]; if (seen[s.id]++) ok = false;
            const double hi = s.kind == K_CHOICE ? choiceCount (s.choices) - 1 : s.hi;
            if (s.def < s.lo - 1e-6 || s.def > hi + 1e-6) ok = false;
            if (s.map == M_LOG && s.lo <= 0) ok = false;
        }
        CHECK (ok, "%d parameters: unique IDs, defaults in range", (int) kNumParams);
        int n; const auto* pr = factoryPresets (n); bool pok = n == 11; int bad = 0;
        for (int i = 0; i < n; ++i) for (int k = 0; k < pr[i].count; ++k)
        {
            auto it = idMap().find (pr[i].values[k].id); if (it == idMap().end()) { ++bad; continue; }
            const auto& s = paramSpecs()[it->second]; const double hi = s.kind == K_CHOICE ? choiceCount (s.choices) - 1 : s.hi, lo = s.lo;
            if (pr[i].values[k].value < lo - 1e-6 || pr[i].values[k].value > hi + 1e-6) ++bad;
        }
        CHECK (pok && bad == 0, "%d factory presets (incl. DEFAULT): all IDs exist and every value is inside its range (%d problems)", n, bad);
    }

    std::printf ("2. Every module processes audio; bypass is transparent\n");
    {
        auto x = voice (fs, 200, N); { Rig g; auto y = x; g.run (y); CHECK (maxDiff (x, y) < 1e-6, "all modules bypassed -> bit-transparent (max diff %.2g)", maxDiff (x, y)); }
        const char* mods[] = { "rd_byp", "hw_byp", "u47_byp", "rv_byp", "eq_byp", "dh_byp", "dl_byp", "fet_byp", "la_byp", "ssl_byp", "mg_byp" };
        for (const char* m : mods)
        {
            Rig g; g.on (m);
            if (!std::strcmp (m, "hw_byp")) g.set ("hw_drive", 0.8);
            if (!std::strcmp (m, "u47_byp")) g.set ("u47_prox", 1).set ("u47_pres", 1);
            if (!std::strcmp (m, "rv_byp")) g.set ("rv_shim", 1).set ("rv_thick", 1);
            if (!std::strcmp (m, "eq_byp")) g.set ("eq3_gain", 6);
            if (!std::strcmp (m, "dh_byp") || !std::strcmp (m, "dl_byp")) { g.set ("dh_thr", -50).set ("dl_thr", -50).set ("dh_range", 12).set ("dl_range", 12).set ("dh_sens", 1).set ("dl_sens", 1); }
            if (!std::strcmp (m, "ssl_byp")) g.set ("ssl_hmf_gain", 8);
            if (!std::strcmp (m, "mg_byp")) g.set ("mg_160", 8);
            g.apply (); auto y = x; g.run (y);
            const double d = maxDiff (x, y, 4800);
            CHECK (finite (y) && d > 1e-3, "%-8s active -> output differs from input (max diff %.3f)", m, d);
        }
        { Rig g; g.on ("at_byp"); g.set ("at_byp", 0); g.set ("at_mode", 1).set ("at_retune", 0).set ("at_scale", 0); g.apply (); auto y = x; g.run (y); CHECK (finite (y) && peak (y, 24000) > 0.05, "autotune active -> produces audio"); }
        const char* par[] = { "rvb_on", "dly_on", "ny_on", "br_on", "dbl_on", "tb_on" }; const char* mix[] = { "rvb_mix", "dly_mix", "ny_mix", "br_mix", "dbl_mix", "tb_mix" };
        for (int i = 0; i < 6; ++i)
        {
            Rig g; g.set (par[i], 1).set (mix[i], 0.8); g.set ("dly_sync", 0).set ("dly_time", 100); g.apply ();
            auto y = x; g.run (y); const double d = maxDiff (x, y, 4800);
            CHECK (finite (y) && d > 1e-3, "%-7s active -> adds signal to the main output (max diff %.3f)", par[i], d);
        }
    }

    std::printf ("3. Parallel routing: main signal untouched, MIX 0 = silent return, no level change from toggling\n");
    {
        auto x = voice (fs, 200, N);
        Rig base; base.on ("fet_byp").on ("la_byp"); base.apply (); auto m0 = x; base.run (m0);
        bool allSame = true, allAdd = true;
        const char* par[] = { "rvb_on", "dly_on", "ny_on", "br_on", "dbl_on", "tb_on" }; const char* mix[] = { "rvb_mix", "dly_mix", "ny_mix", "br_mix", "dbl_mix", "tb_mix" };
        for (int i = 0; i < 6; ++i)
        {
            Rig g; g.on ("fet_byp").on ("la_byp"); g.set (par[i], 1).set (mix[i], 0); g.apply (); auto y = x; g.run (y);
            if (maxDiff (m0, y) > 1e-9) { allSame = false; std::printf ("  note  %s differs by %.3g\n", par[i], maxDiff (m0, y)); }
        }
        CHECK (allSame, "each chain ON with MIX 0%% -> output bit-identical to main only");
        // main contributes unchanged: (main+parallel) - main must equal the parallel return, and be uncorrelated with a level loss in main
        Rig g; g.on ("fet_byp").on ("la_byp"); g.set ("tb_on", 1).set ("tb_mix", 1); g.apply (); auto y = x; g.run (y);
        auto diff = y; for (size_t i = 0; i < diff.size(); ++i) diff[i] -= m0[i];
        CHECK (rms (diff, 4800, N) > 0.01 && rms (m0, 4800, N) > 0.05, "tube return adds on top of main (return rms %.3f, main rms %.3f)", rms (diff, 4800, N), rms (m0, 4800, N));
        (void) allAdd;
    }

    std::printf ("4. Latency (measured by cross-correlation, not assumed)\n");
    {
        auto nz = noise (N, 0.3f, 7);
        auto lagOf = [&] (Rig& g)
        {
            auto y = nz; g.run (y); int best = 0; double bv = -1e30;
            for (int lag = -20; lag <= 4400; ++lag) { double s = 0; for (int i = 6000; i < 14000; ++i) s += (double) nz[(size_t) i] * y[(size_t) (i + lag)]; if (s > bv) { bv = s; best = lag; } }
            return best;
        };
        const char* mods[] = { "rd_byp", "hw_byp", "u47_byp", "rv_byp", "eq_byp", "dh_byp", "fet_byp", "la_byp", "ssl_byp", "mg_byp" };
        for (const char* m : mods)
        { Rig g; g.on (m); g.apply (); std::vector<float> imp (512, 0.f); imp[0] = 0.1f; g.run (imp); size_t first = 0; while (first < imp.size() && std::fabs (imp[first]) < 1e-5) ++first;
          CHECK (first == 0 && g.e.latencySamples() == 0, "%-7s impulse response starts at sample %zu (no latency)", m, first); }
        for (const char* m : { "dh_byp", "dl_byp" }) { Rig g; g.on (m); g.apply (); std::vector<float> imp (512, 0.f); imp[0] = 0.1f; g.run (imp); CHECK (std::fabs (imp[0]) > 0.05f, "%-7s responds at sample 0", m); }
        { Rig g; for (const char* m : mods) g.on (m); g.apply (); const int lag = lagOf (g); CHECK (std::abs (lag) <= 3 && g.e.latencySamples() == 0, "whole main chain: cross-correlation peak at %d samples (min-phase filter delay only), reported latency %d", lag, g.e.latencySamples()); }
        { Rig g; g.on ("at_byp"); g.set ("at_byp", 0); g.set ("at_input", 1); g.apply (); const int lag = lagOf (g);
          CHECK (lag == g.e.latencySamples() && lag > 0, "autotune (Alto/Tenor): measured lag %d = reported %d samples (%.1f ms)", lag, g.e.latencySamples(), 1000.0 * lag / fs); }
        for (int t = 0; t < 3; ++t) { Rig g; g.set ("at_byp", 0).set ("at_input", t); g.apply (); std::printf ("  info  autotune input type %d reports %d samples = %.1f ms @48k\n", t, g.e.latencySamples(), 1000.0 * g.e.latencySamples() / fs); }
        { Rig g; g.set ("at_byp", 1); g.apply (); CHECK (g.e.latencySamples() == 0, "autotune bypassed reports 0 latency"); }
    }

    std::printf ("5. Autotune accuracy\n");
    {
        struct Case { const char* name; double inHz, outHz; int scale, key, mode; double retune, trans; std::vector<float> (*gen)(double, double, int); };
        auto sn = [] (double f, double s, int n) { return sine (s, f, 0.3, n); };
        auto vc = [] (double f, double s, int n) { return voice (s, f, n); };
        (void) sn; (void) vc;
        struct C2 { const char* name; bool useVoice; double inHz, outHz; int scale, key, mode; double retune, trans; double tolCents; } cs[] = {
            { "sine 225 Hz -> A3 220 (chromatic, hard)", false, 225.0, 220.0, 0, 0, 1, 0, 0, 12 },
            { "voice 178 Hz -> F3 174.6 (chromatic, hard)", true, 178.0, 174.614, 0, 0, 1, 0, 0, 15 },
            { "voice 213 Hz -> A3 220 (A natural minor, hard)", true, 213.0, 220.0, 2, 9, 1, 0, 0, 15 },
            { "sine 270 Hz in C major -> C4 261.6 (C#/not allowed)", false, 270.0, 261.626, 1, 0, 1, 0, 0, 12 },
            { "voice 190 Hz natural, 30 ms retune -> F#3 185", true, 190.0, 184.997, 0, 0, 0, 30, 0, 20 },
            { "voice 200 Hz -> G3 196 then transpose +5 st", true, 200.0, 196.0 * std::pow (2.0, 5.0 / 12.0), 0, 0, 1, 0, 5, 20 } };
        for (auto& c : cs)
        {
            Rig g; g.set ("at_byp", 0).set ("at_scale", c.scale).set ("at_key", c.key).set ("at_mode", c.mode).set ("at_retune", c.retune).set ("at_trans", c.trans).set ("at_input", 1); g.apply ();
            auto x = c.useVoice ? voice (fs, c.inHz, (int) (fs * 2.0)) : sine (fs, c.inHz, 0.3, (int) (fs * 2.0)); auto y = x; g.run (y);
            const double f = estimateHz (y, fs, (size_t) (fs * 1.3), 6000, 80, 600);
            CHECK (finite (y) && std::fabs (cents (f, c.outHz)) < c.tolCents, "%s -> measured %.2f Hz (%.1f cents from target)", c.name, f, cents (f, c.outHz));
        }
        { Rig g; g.set ("at_byp", 0).set ("at_mode", 0).set ("at_retune", 30); g.apply (); auto x = noise ((int) fs, 0.2f, 3); auto y = x; g.run (y);
          const int D = g.e.latencySamples(); double d = 0; for (size_t i = 12000; i + D < y.size(); ++i) d = std::max (d, (double) std::fabs (y[i + D] - x[i]));
          CHECK (d < 1e-6, "unvoiced noise passes through untouched (delayed by the reported latency, diff %.2g)", d); }
        { Rig g; g.set ("at_byp", 0).set ("at_scale", 0).set ("at_mode", 1).set ("at_retune", 0); g.apply (); auto x = voice (fs, 220.0, (int) fs); auto y = x; g.run (y); const double f = estimateHz (y, fs, 30000, 6000); CHECK (std::fabs (cents (f, 220.0)) < 8, "already-in-tune input stays in tune (%.1f cents)", cents (f, 220.0)); }
    }

    std::printf ("6. Vocal Rider\n");
    {
        Rig g; g.on ("rd_byp").set ("rd_speed", 60).set ("rd_range", 12).set ("rd_up", 12).set ("rd_down", 12).set ("rd_target", -18).set ("rd_sens", 0.8); g.apply ();
        std::vector<float> x ((size_t) (fs * 4)); for (size_t i = 0; i < x.size(); ++i) { const bool loud = ((i / 48000) % 2) == 1; x[i] = (float) ((loud ? 0.35 : 0.06) * std::sin (2 * kPi * 300.0 * i / fs)); }
        auto y = x; g.run (y);
        const double dIn = linToDb (rms (x, 48000 + 30000, 96000)) - linToDb (rms (x, 20000, 48000 - 1000)), dOut = linToDb (rms (y, 48000 + 30000, 96000)) - linToDb (rms (y, 28000 + 48000 * 2, 96000 + 48000 - 1000));
        const double lo = linToDb (rms (y, 2 * 48000 + 30000, 3 * 48000 - 1000));
        std::printf ("  info  passage difference %.1f dB in -> %.1f dB out (soft passage rode to %.1f dBFS rms)\n", dIn, linToDb (rms (y, 48000 + 30000, 96000)) - lo, lo);
        CHECK (std::fabs (linToDb (rms (y, 48000 + 30000, 96000)) - lo) < 0.5 * std::fabs (dIn), "level difference between soft and loud phrases reduced by more than half");
        (void) dOut;
    }

    std::printf ("7. De-essers (high and low)\n");
    {
        auto makeS = [&] (double fc)
        {
            std::vector<float> v ((size_t) (fs * 1.5)); auto nz = noise ((int) v.size(), 1.0f, 5);
            Biquad a, b, c2, d2; auto h = filt::highpass (fs, fc * 0.7, 0.7071), l = filt::lowpass (fs, fc * 1.4, 0.7071);
            for (size_t i = 0; i < v.size(); ++i) { double vow = 0.25 * std::sin (2 * kPi * 200 * i / fs) + 0.1 * std::sin (2 * kPi * 600 * i / fs);
                double s = d2.process (c2.process (b.process (a.process (nz[i], h), l), h), l) * 0.9; const bool in = i > (size_t) (0.5 * fs) && i < (size_t) fs; v[i] = (float) (vow + (in ? s : 0.0)); }
            return v;
        };
        for (int which = 0; which < 2; ++which)
        {
            const char* pre = which ? "dl" : "dh"; const double fc = which ? 3800 : 7000;
            auto x = makeS (fc);
            auto band = [&] (const std::vector<float>& s, double a, double b) { Biquad h, l; auto c1 = filt::highpass (fs, fc * 0.6, 0.7071), c2 = filt::lowpass (fs, fc * 1.6, 0.7071); double sum = 0; int n = 0; for (size_t i = 0; i < s.size(); ++i) { double y = l.process (h.process (s[i], c1), c2); if (i >= (size_t) (a * fs) && i < (size_t) (b * fs)) { sum += y * y; ++n; } } return std::sqrt (sum / n); };
            Rig on; on.on ((std::string (pre) + "_byp").c_str()); on.set ((std::string (pre) + "_freq").c_str(), fc).set ((std::string (pre) + "_range").c_str(), 8).set ((std::string (pre) + "_sens").c_str(), 0.9).set ((std::string (pre) + "_thr").c_str(), -40); on.apply ();
            Rig off; off.apply ();
            auto yo = x, yf = x; on.run (yo); off.run (yf);
            const double red = 20 * std::log10 (band (yf, 0.6, 0.95) / band (yo, 0.6, 0.95)), redV = 20 * std::log10 (band (yf, 0.1, 0.4) / band (yo, 0.1, 0.4));
            CHECK (red > 3.0 && red <= 8.4, "%s: sibilance reduced by %.2f dB (range 8 dB)", pre, red);
            CHECK (std::fabs (redV) < 0.3, "%s: non-sibilant vowel unchanged (%.3f dB)", pre, redV);
            Rig zero; zero.on ((std::string (pre) + "_byp").c_str()); zero.set ((std::string (pre) + "_range").c_str(), 0); zero.set ((std::string (pre) + "_freq").c_str(), fc); zero.apply (); auto yz = x; zero.run (yz);
            CHECK (maxDiff (yz, yf) < 1e-9, "%s: range 0 dB = transparent", pre);
            Rig ls; ls.on ((std::string (pre) + "_byp").c_str()); ls.set ((std::string (pre) + "_listen").c_str(), 1).set ((std::string (pre) + "_freq").c_str(), fc); ls.apply (); auto yl = x; ls.run (yl);
            CHECK (goertzel (yl, fs, 200, 24000) < 0.1 * goertzel (x, fs, 200, 24000) && peak (yl, 30000) > 0.02, "%s: LISTEN monitors only the detected band", pre);
            Rig st; st.on ((std::string (pre) + "_byp").c_str()); st.set ((std::string (pre) + "_freq").c_str(), fc).set ((std::string (pre) + "_range").c_str(), 8).set ((std::string (pre) + "_thr").c_str(), -40).set ((std::string (pre) + "_sens").c_str(), 0.9); st.apply ();
            auto L = x, R = x; for (auto& s : R) s *= 0.5f; st.run (L, &R);
            double err = 0; for (size_t i = 0; i < L.size(); ++i) err = std::max (err, (double) std::fabs (R[i] - 0.5f * L[i]));
            CHECK (err < 1e-5, "%s: linked stereo gain keeps balance (err %.2g)", pre, err);
        }
    }

    std::printf ("8. Compressors\n");
    {
        auto x = sine (fs, 1000, 0.5, N);
        for (int r = 0; r < 4; ++r) { Rig g; g.on ("fet_byp").set ("fet_in", 12).set ("fet_out", 0).set ("fet_ratio", r).set ("fet_att", 0.2).set ("fet_color", 0); g.apply (); auto y = x; g.run (y);
            const double gr = linToDb (rms (x, 30000, N)) - linToDb (rms (y, 30000, N)) + 12; std::printf ("  info  1176 ratio %d: input +12 dB -> net %.1f dB (gain reduction %.1f dB)\n", r, linToDb (rms (y, 30000, N)) - linToDb (rms (x, 30000, N)), gr); }
        { Rig a; a.on ("fet_byp").set ("fet_in", 12).set ("fet_out", 0).set ("fet_ratio", 0).set ("fet_color", 0); a.apply (); auto y4 = x; a.run (y4);
          Rig b; b.on ("fet_byp").set ("fet_in", 12).set ("fet_out", 0).set ("fet_ratio", 3).set ("fet_color", 0); b.apply (); auto y20 = x; b.run (y20);
          CHECK (rms (y20, 30000, N) < rms (y4, 30000, N), "1176: 20:1 compresses more than 4:1"); }
        { Rig g; g.on ("la_byp").set ("la_pr", 0.8).set ("la_gain", 0); g.apply (); auto y = x; g.run (y); const double r = linToDb (rms (x, 30000, N)) - linToDb (rms (y, 30000, N)); CHECK (r > 3.0, "LA-2A: peak reduction 80%% reduces level by %.1f dB", r); }
        { Rig g; g.on ("la_byp").set ("la_pr", 0.8).set ("la_gain", 0); g.apply (); std::vector<float> y ((size_t) (fs * 3), 0.f); for (size_t i = 0; i < 48000; ++i) y[i] = (float) (0.5 * std::sin (2 * kPi * 1000.0 * i / fs));
          for (size_t i = 48000; i < y.size(); ++i) y[i] = (float) (0.05 * std::sin (2 * kPi * 1000.0 * i / fs)); auto z = y; g.run (z);
          const double early = rms (z, 48000 + 2400, 48000 + 7200) / rms (y, 48000 + 2400, 48000 + 7200), late = rms (z, 48000 * 2 + 20000, 48000 * 3 - 100) / rms (y, 48000 * 2 + 20000, 48000 * 3 - 100);
          CHECK (late > early, "LA-2A: program-dependent release recovers gradually (gain %.2f at 50-150 ms -> %.2f at 1-2 s)", early, late); }
        { Rig g; g.set ("ny_on", 1).set ("ny_mix", 1).set ("ny_thr", -40).set ("ny_ratio", 12).set ("ny_makeup", 0); g.apply (); auto y = x; g.run (y);
          CHECK (finite (y) && peak (y, 30000) > 0.1, "New York compression return is active"); }
    }

    std::printf ("9. EQs: response matches the controls\n");
    {
        { Rig g; g.on ("eq_byp").set ("eq3_freq", 1000).set ("eq3_gain", 6).set ("eq3_q", 1.0); g.apply (); const double a = gainAt (g, 1000); CHECK (std::fabs (a - 6.0) < 0.4, "EQ8 bell +6 dB at 1 kHz: measured %.2f dB", a); }
        { Rig g; g.on ("eq_byp").set ("eq7_freq", 2000).set ("eq7_gain", 9); g.apply (); const double a = gainAt (g, 12000); CHECK (a > 6.5, "EQ8 high shelf at 2 kHz (extended low range) +%.1f dB at 12 kHz", a); }
        { Rig g; g.on ("eq_byp").set ("eq6_freq", 20000).set ("eq6_gain", 6).set ("eq6_q", 1); g.apply (); const double a = gainAt (g, 18500); CHECK (std::isfinite (a) && a > 1.5, "EQ8 bell at 20 kHz works (+%.1f dB at 18.5 kHz)", a); }
        { Rig g; g.on ("eq_byp"); g.set ("eq1_freq", 200).set ("eq1_on", 1).set ("eq1_type", 3); g.apply (); const double a = gainAt (g, 50); CHECK (a < -10, "EQ8 high-pass at 200 Hz: %.1f dB at 50 Hz", a); }
        { Rig g; g.on ("ssl_byp").set ("ssl_hf_gain", 9).set ("ssl_hf_freq", 8000); g.apply (); const double a = gainAt (g, 14000); CHECK (a > 6.0, "SSL HF shelf +9 dB: %.1f dB at 14 kHz", a); }
        { Rig g; g.on ("ssl_byp").set ("ssl_hmf_gain", 6).set ("ssl_hmf_freq", 3000).set ("ssl_hmf_q", 1.0); g.apply (); const double a = gainAt (g, 3000); CHECK (std::fabs (a - 6) < 0.5, "SSL HMF +6 dB at 3 kHz: %.2f dB", a); }
        { Rig g; g.on ("ssl_byp").set ("ssl_hpf_on", 1).set ("ssl_hpf_freq", 120); g.apply (); const double a = gainAt (g, 40), b = gainAt (g, 1000); CHECK (a < -15 && std::fabs (b) < 0.3, "SSL 18 dB/oct high-pass at 120 Hz: %.1f dB at 40 Hz, %.2f dB at 1 kHz", a, b); }
        { Rig g; g.on ("mg_byp").set ("mg_air", 10).set ("mg_airf", 2); g.apply (); const double a = gainAt (g, 15000), b = gainAt (g, 1000); CHECK (a > 4 && a <= 10.5 && std::fabs (b) < 1.0, "Maag air band +10 dB @10k setting: %.1f dB at 15 kHz, %.2f dB at 1 kHz", a, b); }
        { Rig g; g.on ("mg_byp").set ("mg_160", 8); g.apply (); const double a = gainAt (g, 160); CHECK (std::fabs (a - 8) < 0.7, "Maag 160 Hz band +8 dB: %.2f dB", a); }
    }

    std::printf ("10. Parallel chains\n");
    {
        { // reverb decay
            auto tail = [&] (double decay) { Rig g; g.set ("rvb_on", 1).set ("rvb_mix", 1).set ("rvb_decay", decay).set ("rvb_pre", 0).set ("rvb_lc", 20).set ("rvb_hc", 20000).set ("rvb_damp", 0.2); g.apply ();
                std::vector<float> x ((size_t) (fs * 4), 0.f); x[0] = 0.5f; g.run (x); return std::make_pair (rms (x, 48000, 72000), finite (x) ? 1 : 0); };
            const auto a = tail (0.6), b = tail (5.0);
            CHECK (a.second && b.second && b.first > 3 * a.first && b.first > 1e-5, "reverb: longer DECAY gives a longer tail (%.2g vs %.2g rms at 1-1.5 s)", b.first, a.first);
            Rig g; g.set ("rvb_on", 1).set ("rvb_mix", 1).set ("rvb_decay", 10).set ("rvb_size", 2).set ("rvb_mod", 1).set ("rvb_damp", 0); g.apply (); auto x = noise ((int) (fs * 3), 0.8f, 2); std::vector<float> st ((size_t) (fs * 10), 0.f); g.run (x); g.run (st);
            CHECK (finite (x) && finite (st) && rms (st, 400000, 480000) < 1e-3, "reverb: stable at maximum decay/size (tail after 8 s: %.2g)", rms (st, 400000, 480000)); }
        { Rig g; g.set ("dly_on", 1).set ("dly_mix", 1).set ("dly_sync", 0).set ("dly_time", 250).set ("dly_fb", 0).set ("dly_pp", 0).set ("dly_analog", 0).set ("dly_hp", 20).set ("dly_lp", 20000).set ("dly_mod", 0); g.apply ();           std::vector<float> x ((size_t) fs, 0.f); x[100] = 0.8f; g.e.reset(); g.run (x); size_t pk = 0; for (size_t i = 0; i < x.size(); ++i) if (std::fabs (x[i]) > std::fabs (x[pk]) && i > 1000) pk = i;
          CHECK (std::abs ((int) pk - (100 + 12000)) <= 3, "delay: echo at %.1f ms (set 250 ms)", 1000.0 * ((double) pk - 100) / fs); }
        { Rig g; g.set ("dly_on", 1).set ("dly_mix", 1).set ("dly_sync", 1).set ("dly_div", 1); g.e.setTempo (120); g.apply (); std::vector<float> x ((size_t) fs, 0.f); x[100] = 0.8f; g.run (x); size_t pk = 0; for (size_t i = 1000; i < x.size(); ++i) if (std::fabs (x[i]) > std::fabs (x[pk])) pk = i;
          CHECK (std::abs ((int) pk - (100 + 24000)) <= 700, "delay: 1/4 at 120 BPM lands at ~500 ms (%.0f ms; includes filter group delay)", 1000.0 * ((double) pk - 100) / fs); }
        { Rig g; g.set ("dbl_on", 1).set ("dbl_mix", 1).set ("dbl_spread", 1).set ("dbl_detl", -10).set ("dbl_detr", 10); g.apply (); auto L = voice (fs, 200, N), R = L; g.run (L, &R);
          double c = 0; for (size_t i = 24000; i < L.size(); ++i) c += (double) L[i] * R[i]; const double cn = c / (rms (L, 24000, N) * rms (R, 24000, N) * (N - 24000));
          CHECK (finite (L) && rms (L, 24000, N) > 0.02 && cn < 0.99, "doubler: two decorrelated voices, L/R correlation %.2f (1.0 = mono)", cn); }
        { auto mk_ = [] (double amt) { auto p = std::make_unique<Rig>(); Rig& a = *p; a.set ("eq4_gain", 6).set ("eq4_freq", 1000).set ("eq4_q", 1.0); a.set ("br_on", 1).set ("br_mix", 1).set ("br_eqamt", amt).set ("br_drive", 0).set ("br_harm", 0).set ("br_midair", 0).set ("br_hiair", 0); a.apply (); return p; };
          auto pa = mk_ (1.0), pb = mk_ (0.0);
          const double ga = gainAt (*pa, 1000), gb = gainAt (*pb, 1000); CHECK (ga - gb > 3.5, "brightness chain follows the main EQ: +%.1f dB at 1 kHz with EQ AMOUNT 100%% vs 0%% (main EQ bypassed so only the copy acts)", ga - gb); }
        { Rig g; g.set ("tb_on", 1).set ("tb_mix", 1).set ("tb_drive", 0.8).set ("tb_harm", 1).set ("tb_char", 0.2); g.apply (); auto x = sine (fs, 500, 0.3, N); auto y = x; g.run (y);
          const double h2 = goertzel (y, fs, 1000, 24000), h3 = goertzel (y, fs, 1500, 24000), f1 = goertzel (y, fs, 500, 24000); CHECK (h2 > 0.01 * f1 && h3 > 0.003 * f1, "tube: adds harmonics (H2 %.1f dB, H3 %.1f dB rel. fundamental)", 20 * std::log10 (h2 / f1), 20 * std::log10 (h3 / f1)); }
    }

    std::printf ("11. Block sizes, mono/stereo, determinism\n");
    {
        int n; const auto* pr = factoryPresets (n);
        auto x = voice (fs, 200, N);
        std::vector<float> ref;
        for (int blk : { 32, 64, 128, 256, 480 })
        {
            Rig g (fs, false); for (int k = 0; k < pr[3].count; ++k) g.set (pr[3].values[k].id, pr[3].values[k].value); g.apply ();
            auto y = x; g.run (y, nullptr, blk);
            if (ref.empty()) ref = y; else CHECK (maxDiff (ref, y) < 1e-6, "block %d: identical result to block 32 (max diff %.2g)", blk, maxDiff (ref, y));
        }
        { Rig g (fs, false); for (int k = 0; k < pr[2].count; ++k) g.set (pr[2].values[k].id, pr[2].values[k].value); g.apply (); auto L = x, R = x; g.run (L, &R); auto M = x; Rig h (fs, false); for (int k = 0; k < pr[2].count; ++k) h.set (pr[2].values[k].id, pr[2].values[k].value); h.apply (); h.run (M);
          CHECK (finite (L) && finite (R) && finite (M) && peak (M, 24000) > 0.05, "stereo and mono (single-channel) paths both work on the URBAN TRAP preset"); }
        { Rig g; g.on ("hw_byp").on ("eq_byp").on ("fet_byp"); g.apply (); auto L = x, R = x; for (auto& s : R) s = 0.f; g.run (L, &R); CHECK (peak (R) == 0.0, "no crosstalk: silent right channel stays silent through the main chain"); }
    }

    std::printf ("12. Stability fuzzing (random + extreme parameters, all sample rates)\n");
    {
        bool ok = true; double worst = 0; std::mt19937 g (42);
        for (double sr : { 44100.0, 48000.0, 88200.0, 96000.0 })
            for (int trial = 0; trial < 40; ++trial)
            {
                Rig r (sr, false); std::uniform_real_distribution<float> u (0, 1);
                for (int i = 0; i < kNumParams; ++i)
                {
                    const auto& s = paramSpecs()[i]; float val;
                    const int mode = trial < 4 ? trial % 2 : 2; // trials 0..3: all-min / all-max extremes
                    if (s.kind == K_BOOL) val = (trial < 2) ? (trial == 0 ? 0.f : 1.f) : (u (g) > 0.4f ? 1.f : 0.f);
                    else if (s.kind == K_CHOICE) val = (float) (int) (u (g) * choiceCount (s.choices) * 0.999f);
                    else val = mode == 0 ? s.lo : mode == 1 ? s.hi : s.lo + u (g) * (s.hi - s.lo);
                    r.v[i] = val;
                }
                if (trial < 2) for (const char* b : { "at_byp", "rd_byp", "hw_byp", "u47_byp", "rv_byp", "eq_byp", "dh_byp", "dl_byp", "fet_byp", "la_byp", "ssl_byp", "mg_byp" }) r.set (b, 0);
                if (trial < 2) for (const char* b : { "rvb_on", "dly_on", "ny_on", "br_on", "dbl_on", "tb_on" }) r.set (b, 1);
                r.apply ();
                auto x = (trial & 1) ? noise ((int) (sr * 0.5), 0.9f, (unsigned) trial) : voice (sr, 150 + trial * 5, (int) (sr * 0.5)); auto y = x, z = x;
                const int blk = 1 + (int) (u (g) * 300); r.run (y, &z, blk);
                ok = ok && finite (y) && finite (z); worst = std::max (worst, peak (y));
                // automation slam
                for (int k = 0; k < 20; ++k) { for (int i = 0; i < kNumParams; ++i) { const auto& s = paramSpecs()[i]; if (s.kind == K_FLOAT && u (g) > 0.7f) r.v[i] = s.lo + u (g) * (s.hi - s.lo); } auto t = noise (257, 0.8f, (unsigned) k); r.run (t, nullptr, 64); ok = ok && finite (t); }
            }
        CHECK (ok, "no NaN/Inf over 160 randomised sessions incl. all-min / all-max, 4 sample rates, odd block sizes, automation slamming (largest peak seen %.1f)", worst);
    }

    std::printf ("13. Clicks when switching modules on/off\n");
    {
        const char* mods[] = { "rd_byp", "hw_byp", "u47_byp", "rv_byp", "eq_byp", "dh_byp", "fet_byp", "la_byp", "ssl_byp", "mg_byp", "at_byp" }; const char* par[] = { "rvb_on", "dly_on", "ny_on", "br_on", "dbl_on", "tb_on" };
        bool ok = true; double worst = 0;
        for (int i = 0; i < 17; ++i)
        {
            Rig g (fs, i >= 0); if (i < 11) { g.on (mods[i]); if (i == 3) g.set ("rv_shim", 1); } else g.set (par[i - 11], 1);
            g.set ("fet_in", 14).set ("la_pr", 0.6); g.apply ();
            auto x = sine (fs, 440, 0.3, (int) fs); std::vector<float> y; double prev = 0, mx = 0; int pos = 0;
            for (int b = 0; b < (int) fs / 64; ++b)
            {
                if (b == 300) { if (i < 11) g.set (mods[i], 1); else g.set (par[i - 11], 0); }
                if (b == 500) { if (i < 11) g.set (mods[i], 0); else g.set (par[i - 11], 1); }
                std::vector<float> t (x.begin() + pos, x.begin() + pos + 64); g.run (t, nullptr, 64); pos += 64;
                for (float v : t) { if (b > 100) mx = std::max (mx, std::fabs (v - prev)); prev = v; }
            }
            worst = std::max (worst, mx); if (mx > 0.09) { ok = false; std::printf ("  note  %s max step %.3f\n", i < 11 ? mods[i] : par[i - 11], mx); }
        }
        CHECK (ok, "largest sample-to-sample step while toggling any module: %.3f (a clean 440 Hz sine at this level steps up to %.3f)", worst, 0.3 * 2 * kPi * 440 / fs);
    }
    }

    std::printf ("14. CPU (single core of this build machine, 48 kHz stereo, ALL modules and ALL parallel chains active)\n");
    {
        for (int blk : { 32, 64, 128, 256 })
        {
            Rig g (fs, false); for (const char* b : { "at_byp", "u47_byp", "rv_byp", "dl_byp" }) g.set (b, 0); for (const char* b : { "rvb_on", "dly_on", "ny_on", "br_on", "dbl_on", "tb_on" }) g.set (b, 1);
            g.set ("at_mode", 1).set ("at_retune", 0).set ("at_scale", 0).set ("rvb_decay", 4).set ("dly_sync", 0); g.apply ();
            auto x = voice (fs, 200, (int) (fs * 10)); auto y = x, z = x;
            const auto t0 = std::chrono::steady_clock::now(); g.run (y, &z, blk); const auto t1 = std::chrono::steady_clock::now();
            const double sec = std::chrono::duration<double> (t1 - t0).count();
            std::printf ("  info  block %3d: %.2f s of audio in %.3f s = %.1f %% of one core\n", blk, 10.0, sec, 100.0 * sec / 10.0);
            if (blk == 64) CHECK (sec / 10.0 < 0.5, "worst-case all-on chain uses %.1f %% of one core (< 50 %% required)", 100.0 * sec / 10.0);
        }
    }
    if (fails)
    {
        std::cerr << "\n[DSP TEST] " << fails << " check(s) failed:\n";
        for (const auto& s : failedChecks) std::cerr << "  - " << s << "\n";
        std::cerr.flush();
    }
    std::printf ("\n%s (%d failure%s)\n", fails ? "FAILED" : "ALL CHECKS PASSED", fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
