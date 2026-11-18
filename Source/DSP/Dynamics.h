// MAKETEO - dynamics modules: Vocal Rider, De-esser, 1176-style FET, LA-2A-style optical, New York compressor.
// All causal, zero-latency (no lookahead), stereo-linked detection.
#pragma once
#include "Common.h"
#include "../Params.h"

namespace mk
{
// Soft-knee gain computer. Returns gain reduction in dB (>= 0).
inline double gainReduction (double levelDb, double thr, double ratio, double knee)
{
    const double over = levelDb - thr, slope = 1.0 - 1.0 / ratio;
    if (knee <= 0.01) return over > 0 ? slope * over : 0.0;
    if (2.0 * over < -knee) return 0.0;
    if (2.0 * std::fabs (over) <= knee) { const double t = over + 0.5 * knee; return slope * t * t / (2.0 * knee); }
    return slope * over;
}

// ================================================================== VOCAL RIDER
class VocalRider
{
public:
    double gainDb = 0.0; // read by the GUI
    void prepare (double sr) { fs = sr; aRise = msToCoef (10, fs); aFall = msToCoef (120, fs); reset(); }
    void reset() { ms = 0; gainDb = 0; }
    void update (const double* P)
    {
        target = P[RD_TARGET]; sens = P[RD_SENS]; range = P[RD_RANGE]; man = P[RD_MAN];
        up = std::min (P[RD_UP], range); down = std::min (P[RD_DOWN], range);
        aUp = msToCoef (P[RD_SPEED] * 2.0, fs);   // riding up is slower (no noise pumping)
        aDown = msToCoef (P[RD_SPEED] * 0.5, fs); // riding down is faster (catch peaks)
    }
    inline void process (double& l, double& r)
    {
        const double p = std::max (l * l, r * r);
        ms = p > ms ? p + (ms - p) * aRise : p + (ms - p) * aFall;
        const double lvl = 10.0 * std::log10 (ms + 1.0e-12);
        const double gate = target - (8.0 + 40.0 * sens); // below this the rider holds (never lifts noise)
        const double desired = lvl > gate ? clampd (target - lvl, -down, up) : gainDb;
        const double a = desired > gainDb ? aUp : aDown;
        gainDb = desired + (gainDb - desired) * a;
        const double g = std::exp ((gainDb + man) * 0.11512925464970229);
        l *= g; r *= g;
    }
private:
    double fs = 48000, ms = 0, aRise = 0, aFall = 0, aUp = 0, aDown = 0, target = -18, sens = 0.5, range = 9, man = 0, up = 6, down = 6;
};

// ================================================================== DE-ESSER (dynamic split-band, linked stereo)
class DeEsser
{
public:
    double grDb = 0.0;       // GUI meter
    double listenL = 0, listenR = 0; // detected band of the last sample (for LISTEN)
    void prepare (double sr)
    {
        fs = sr; aEnvAtt = msToCoef (0.3, fs); aEnvRel = msToCoef (15, fs); aFullRel = msToCoef (60, fs); reset();
    }
    void reset() { for (auto& c : ch) c = Chan {}; depth = 0; grDb = 0; }
    void update (const double* P, int iFreq, int iThr, int iRange, int iSens, int iAtt, int iRel, double qBand)
    {
        cBP = filt::bandpass (fs, P[iFreq], qBand);
        thr = P[iThr]; range = P[iRange]; ratioThr = 0.55 - 0.45 * P[iSens];
        aAtt = msToCoef (P[iAtt], fs); aRel = msToCoef (P[iRel], fs);
    }
    inline void process (double& l, double& r)
    {
        double y[2] = { l, r }, band[2];
        double raw = 0.0;
        for (int c = 0; c < 2; ++c)
        {
            auto& s = ch[c];
            band[c] = s.bp.process (y[c], cBP);
            const double ab = std::fabs (band[c]), af = std::fabs (y[c]);
            s.env = ab > s.env ? ab + (s.env - ab) * aEnvAtt : ab + (s.env - ab) * aEnvRel;
            s.full = af > s.full ? af + (s.full - af) * aEnvAtt : af + (s.full - af) * aFullRel;
            const double ratio = s.env / (s.full + 1.0e-4);                 // relative sibilance measure
            const double absDepth = clampd ((linToDb (s.env) - thr) / 6.0, 0.0, 1.0); // absolute threshold
            const double w = clampd ((ratio - ratioThr) / 0.15, 0.0, 1.0);
            raw = std::max (raw, absDepth * w);
        }
        depth = raw > depth ? raw + (depth - raw) * aAtt : raw + (depth - raw) * aRel;
        grDb = range * depth;
        const double k = 1.0 - std::exp (-grDb * 0.11512925464970229);
        l = y[0] - k * band[0]; r = y[1] - k * band[1];
        listenL = band[0]; listenR = band[1];
    }
private:
    struct Chan { Biquad bp; double env = 0, full = 0; };
    Chan ch[2];
    Coef cBP;
    double fs = 48000, aEnvAtt = 0, aEnvRel = 0, aFullRel = 0, aAtt = 0, aRel = 0, thr = -30, range = 5, ratioThr = 0.3, depth = 0;
};

// ================================================================== 1176-style FET compressor
class FET1176
{
public:
    double grDb = 0.0;
    void prepare (double sr) { fs = sr; reset(); }
    void reset() { gr = 0; grDb = 0; sat[0].reset(); sat[1].reset(); }
    void update (const double* P)
    {
        gIn = dbToLin (P[FET_IN]); gOut = dbToLin (P[FET_OUT]);
        static const double ratios[4] = { 4, 8, 12, 20 };
        ratio = ratios[(int) clampd (P[FET_RATIO], 0, 3)];
        aAtt = msToCoef (0.02 * std::pow (40.0, P[FET_ATT]), fs);   // 20 us .. 800 us
        aRel = msToCoef (50.0 * std::pow (22.0, P[FET_REL]), fs);   // 50 .. 1100 ms
        color = P[FET_COLOR];
    }
    inline void process (double& l, double& r)
    {
        l *= gIn; r *= gIn;
        const double lvl = linToDb (std::max (std::fabs (l), std::fabs (r)));
        const double target = gainReduction (lvl, -10.0, ratio, 3.0);
        gr = target > gr ? target + (gr - target) * aAtt : target + (gr - target) * aRel;
        grDb = gr;
        const double g = std::exp (-gr * 0.11512925464970229);
        l *= g; r *= g;
        if (color > 0.001) // FET-style soft clipping with a touch of even harmonics
        {
            const double d = 1.0 + 3.0 * color, b = 0.12 * color, mixAmt = std::min (1.0, color * 3.0);
            l += mixAmt * (sat[0].process (l, d, b) - l);
            r += mixAmt * (sat[1].process (r, d, b) - r);
        }
        l *= gOut; r *= gOut;
    }
private:
    double fs = 48000, gIn = 1, gOut = 1, ratio = 4, aAtt = 0, aRel = 0, gr = 0, color = 0;
    Saturator sat[2];
};

// ================================================================== LA-2A-style optical compressor
class LA2A
{
public:
    double grDb = 0.0;
    void prepare (double sr)
    {
        fs = sr; aDet = msToCoef (5, fs);
        aFA = msToCoef (10, fs); aFR = msToCoef (70, fs);     // fast optical cell
        aSA = msToCoef (250, fs); aSR = msToCoef (2000, fs);  // slow "memory" cell
        reset();
    }
    void reset() { det = 0; grF = grS = 0; grDb = 0; sat[0].reset(); sat[1].reset(); }
    void update (const double* P)
    {
        thr = -2.0 - 40.0 * P[LA_PR]; ratio = P[LA_MODE] > 0.5 ? 10.0 : 3.0;
        gain = dbToLin (P[LA_GAIN]); color = P[LA_COLOR];
    }
    inline void process (double& l, double& r)
    {
        const double a = std::max (std::fabs (l), std::fabs (r));
        det = a + (det - a) * aDet;
        const double target = gainReduction (linToDb (det), thr, ratio, 12.0);
        grF = target > grF ? target + (grF - target) * aFA : target + (grF - target) * aFR;
        grS = target > grS ? target + (grS - target) * aSA : target + (grS - target) * aSR;
        const double gr = 0.6 * grF + 0.4 * grS; // program-dependent release
        grDb = gr;
        const double g = std::exp (-gr * 0.11512925464970229) * gain;
        l *= g; r *= g;
        if (color > 0.001)
        {
            const double d = 1.0 + 2.5 * color, b = 0.2 * color, m = std::min (1.0, color * 3.0);
            l += m * (sat[0].process (l, d, b) - l);
            r += m * (sat[1].process (r, d, b) - r);
        }
    }
private:
    double fs = 48000, det = 0, grF = 0, grS = 0, thr = -16, ratio = 3, gain = 1, color = 0;
    double aDet = 0, aFA = 0, aFR = 0, aSA = 0, aSR = 0;
    Saturator sat[2];
};

// ================================================================== New York parallel compressor (wet path)
class NYComp
{
public:
    double grDb = 0.0;
    void prepare (double sr) { fs = sr; reset(); }
    void reset() { gr = 0; grDb = 0; sat[0].reset(); sat[1].reset(); }
    void update (const double* P)
    {
        thr = P[NY_THR]; ratio = P[NY_RATIO]; knee = P[NY_KNEE]; makeup = dbToLin (P[NY_MAKEUP]);
        aAtt = msToCoef (P[NY_ATT], fs); aRel = msToCoef (P[NY_REL], fs); satAmt = P[NY_SAT]; out = dbToLin (P[NY_OUT]);
    }
    inline void process (double l, double r, double& oL, double& oR)
    {
        const double lvl = linToDb (std::max (std::fabs (l), std::fabs (r)));
        const double target = gainReduction (lvl, thr, ratio, knee);
        gr = target > gr ? target + (gr - target) * aAtt : target + (gr - target) * aRel;
        grDb = gr;
        const double g = std::exp (-gr * 0.11512925464970229) * makeup;
        oL = l * g; oR = r * g;
        if (satAmt > 0.001)
        {
            const double d = 1.0 + 2.5 * satAmt, m = std::min (1.0, satAmt * 3.0);
            oL += m * (sat[0].process (oL, d, 0.08) - oL);
            oR += m * (sat[1].process (oR, d, 0.08) - oR);
        }
        oL *= out; oR *= out;
    }
private:
    double fs = 48000, thr = -30, ratio = 10, knee = 6, makeup = 1, aAtt = 0, aRel = 0, gr = 0, satAmt = 0, out = 1;
    Saturator sat[2];
};
} // namespace mk
