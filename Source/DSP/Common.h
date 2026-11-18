// MAKETEO - shared DSP primitives. Header-only, double precision, allocation-free in process paths.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace mk
{
constexpr double kPi = 3.14159265358979323846;
constexpr int kControlRate = 16; // control-rate tick (samples): smoothing and coefficient refresh

inline double clampd (double v, double lo, double hi) { return std::min (std::max (v, lo), hi); }
inline double dbToLin (double dB) { return std::pow (10.0, dB / 20.0); }
inline double linToDb (double x) { return 20.0 * std::log10 (std::max (x, 1.0e-12)); }
inline double msToCoef (double ms, double fs) { return ms <= 0.0 ? 0.0 : std::exp (-1.0 / (0.001 * ms * fs)); }
inline double softLimitSafe (double x) { return std::isfinite (x) ? x : 0.0; }

// ------------------------------------------------------------------ biquad
struct Coef { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

struct Biquad
{
    double z1 = 0, z2 = 0;
    inline double process (double x, const Coef& c)
    {
        const double y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        if (std::fabs (z1) < 1.0e-30) z1 = 0; // flush denormals
        if (std::fabs (z2) < 1.0e-30) z2 = 0;
        return y;
    }
    void reset() { z1 = z2 = 0; }
};

namespace filt
{
inline double clampF (double f, double fs) { return std::min (std::max (f, 10.0), 0.45 * fs); }
inline Coef norm (double b0, double b1, double b2, double a0, double a1, double a2)
{ Coef c; c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0; c.a1 = a1 / a0; c.a2 = a2 / a0; return c; }

inline Coef highpass (double fs, double f, double Q)
{ f = clampF (f, fs); const double w = 2 * kPi * f / fs, cw = std::cos (w), al = std::sin (w) / (2 * Q);
  return norm ((1 + cw) / 2, -(1 + cw), (1 + cw) / 2, 1 + al, -2 * cw, 1 - al); }
inline Coef lowpass (double fs, double f, double Q)
{ f = clampF (f, fs); const double w = 2 * kPi * f / fs, cw = std::cos (w), al = std::sin (w) / (2 * Q);
  return norm ((1 - cw) / 2, 1 - cw, (1 - cw) / 2, 1 + al, -2 * cw, 1 - al); }
inline Coef bandpass (double fs, double f, double Q) // 0 dB peak gain
{ f = clampF (f, fs); const double w = 2 * kPi * f / fs, cw = std::cos (w), al = std::sin (w) / (2 * Q);
  return norm (al, 0, -al, 1 + al, -2 * cw, 1 - al); }
inline Coef notch (double fs, double f, double Q)
{ f = clampF (f, fs); const double w = 2 * kPi * f / fs, cw = std::cos (w), al = std::sin (w) / (2 * Q);
  return norm (1, -2 * cw, 1, 1 + al, -2 * cw, 1 - al); }
inline Coef peaking (double fs, double f, double Q, double dB)
{ f = clampF (f, fs); const double A = std::pow (10.0, dB / 40.0);
  const double w = 2 * kPi * f / fs, cw = std::cos (w), al = std::sin (w) / (2 * Q);
  return norm (1 + al * A, -2 * cw, 1 - al * A, 1 + al / A, -2 * cw, 1 - al / A); }
inline Coef highShelfQ (double fs, double f, double dB, double Q)
{ f = clampF (f, fs); const double A = std::pow (10.0, dB / 40.0);
  const double w = 2 * kPi * f / fs, cw = std::cos (w), sw = std::sin (w), tsa = 2.0 * std::sqrt (A) * (sw / (2 * Q));
  return norm (A * ((A + 1) + (A - 1) * cw + tsa), -2 * A * ((A - 1) + (A + 1) * cw), A * ((A + 1) + (A - 1) * cw - tsa),
               (A + 1) - (A - 1) * cw + tsa, 2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - tsa); }
inline Coef lowShelfQ (double fs, double f, double dB, double Q)
{ f = clampF (f, fs); const double A = std::pow (10.0, dB / 40.0);
  const double w = 2 * kPi * f / fs, cw = std::cos (w), sw = std::sin (w), tsa = 2.0 * std::sqrt (A) * (sw / (2 * Q));
  return norm (A * ((A + 1) - (A - 1) * cw + tsa), 2 * A * ((A - 1) - (A + 1) * cw), A * ((A + 1) - (A - 1) * cw - tsa),
               (A + 1) + (A - 1) * cw + tsa, -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - tsa); }
inline Coef highShelfS (double fs, double f, double dB, double S)
{ f = clampF (f, fs); const double A = std::pow (10.0, dB / 40.0);
  const double w = 2 * kPi * f / fs, cw = std::cos (w), sw = std::sin (w);
  const double al = sw / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0), tsa = 2.0 * std::sqrt (A) * al;
  return norm (A * ((A + 1) + (A - 1) * cw + tsa), -2 * A * ((A - 1) + (A + 1) * cw), A * ((A + 1) + (A - 1) * cw - tsa),
               (A + 1) - (A - 1) * cw + tsa, 2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - tsa); }

// magnitude response in dB of a biquad at frequency f (for the GUI curve and tests)
inline double magDb (const Coef& c, double fs, double f)
{
    const double w = 2 * kPi * f / fs, c1 = std::cos (w), s1 = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
    const double nr = c.b0 + c.b1 * c1 + c.b2 * c2, ni = -(c.b1 * s1 + c.b2 * s2);
    const double dr = 1 + c.a1 * c1 + c.a2 * c2, di = -(c.a1 * s1 + c.a2 * s2);
    return 10.0 * std::log10 (std::max (1.0e-20, (nr * nr + ni * ni) / (dr * dr + di * di)));
}
} // namespace filt

// ------------------------------------------------------------------ smoothing helpers
struct OnePole
{
    double z = 0;
    inline double lp (double x, double a) { z = x + (z - x) * a; return z; } // a = exp(-2 pi fc/fs)
    static double coefHz (double fc, double fs) { return std::exp (-2.0 * kPi * std::min (fc, 0.45 * fs) / fs); }
};

struct Ramp // linear per-sample ramp towards a target set once per control tick
{
    double cur = 0, inc = 0;
    void snap (double v) { cur = v; inc = 0; }
    void setTarget (double t, int n) { inc = (t - cur) / (double) n; }
    inline double next() { cur += inc; return cur; }
};

// Smooth a value towards a target once per control tick (tau in ms)
struct TickSmoother
{
    double cur = 0, target = 0, a = 0;
    void setup (double fs, double ms) { a = std::exp (-(double) kControlRate / (0.001 * ms * fs)); }
    void snap (double v) { cur = target = v; }
    inline double step() { cur = target + (cur - target) * a; if (std::fabs (cur - target) < 1.0e-9) cur = target; return cur; }
};

// ------------------------------------------------------------------ nonlinearities
// First-order antiderivative anti-aliased tanh: reduces aliasing without oversampling or lookahead.
struct AdaaTanh
{
    double x1 = 0;
    static inline double logcosh (double x)
    { const double ax = std::fabs (x); return ax > 20.0 ? ax - 0.6931471805599453 : std::log (std::cosh (ax)); }
    inline double process (double x)
    {
        const double dx = x - x1; double y;
        if (std::fabs (dx) < 1.0e-5) y = std::tanh (0.5 * (x + x1));
        else y = (logcosh (x) - logcosh (x1)) / dx;
        x1 = x;
        return y;
    }
    void reset() { x1 = 0; }
};

// Asymmetric (even-harmonic capable), small-signal-normalised saturator. drive >= 1, bias 0..0.5
struct Saturator
{
    AdaaTanh t; bool primed = false;
    double process (double x, double drive, double bias)
    {
        if (! primed) { t.x1 = bias; primed = true; } // start from the silent operating point: no turn-on blip
        const double tb = std::tanh (bias), norm = 1.0 / (drive * (1.0 - tb * tb));
        return (t.process (drive * x + bias) - tb) * norm;
    }
    void reset() { t.reset(); primed = false; }
};

struct DcBlock
{
    double x1 = 0, y1 = 0;
    inline double process (double x) { const double y = x - x1 + 0.9995 * y1; x1 = x; y1 = y; return y; }
    void reset() { x1 = y1 = 0; }
};

// ------------------------------------------------------------------ delay line (pre-allocated)
struct DelayLine
{
    std::vector<double> buf;
    int mask = 0, w = 0;
    void prepare (int maxSamples)
    {
        int n = 4; while (n < maxSamples + 4) n <<= 1;
        buf.assign ((size_t) n, 0.0); mask = n - 1; w = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0); }
    inline void push (double x) { w = (w + 1) & mask; buf[(size_t) w] = x; }
    inline double tap (int d) const { return buf[(size_t) ((w - d) & mask)]; }
    inline double read (double d) const // linear interpolation, d in samples (>= 1 recommended)
    {
        const int i = (int) d; const double f = d - i;
        return buf[(size_t) ((w - i) & mask)] * (1.0 - f) + buf[(size_t) ((w - i - 1) & mask)] * f;
    }
};

inline int choiceCount (const char* s) { int n = 1; for (; *s; ++s) if (*s == '|') ++n; return n; }
} // namespace mk
