// MAKETEO - tonal modules: exciter, air, analog preamp (Hollywood / U47 / Revival), 8-band EQ, SSL-style EQ, Maag-style EQ, tube saturation.
#pragma once
#include "Common.h"
#include "../Params.h"

namespace mk
{
// ================================================================== band processor shared by every EQ
// One biquad per channel with a smooth wet ramp, so enabling/disabling a band never clicks.
struct BandProc
{
    Biquad bq[2];
    Coef c;
    Ramp w;
    double wTarget = 0.0;
    bool wasActive = false;
    void setTarget (double t) { wTarget = t; w.setTarget (t, kControlRate); }
    bool active() const { return wTarget > 0.0 || w.cur > 0.0; }
    void reset() { bq[0].reset(); bq[1].reset(); }
    inline void process (double& l, double& r)
    {
        const double wv = w.next();
        if (wv <= 0.0 && wTarget <= 0.0) { wasActive = false; return; }
        if (! wasActive) { reset(); wasActive = true; }
        l += wv * (bq[0].process (l, c) - l);
        r += wv * (bq[1].process (r, c) - r);
    }
};

// ================================================================== exciter (smooth harmonic generator, additive)
class Exciter
{
public:
    void prepare (double sr)
    {
        fs = sr; aAtt = msToCoef (1.0, fs); aRel = msToCoef (60.0, fs); reset();
    }
    void reset() { for (auto& c : ch) c = Chan {}; }
    void update (double amount, double character, double harmonics)
    {
        const double c = character;
        const double fcHP = 1800.0 * std::pow (2.0, 1.5 * c);
        const double fcPre = std::min (0.13 * fs, 7000.0), fcPost = std::min (0.42 * fs, 9000.0 + 8000.0 * c);
        cHP = filt::highpass (fs, fcHP, 0.7071); cLP = filt::lowpass (fs, std::max (fcPre, fcHP * 1.5), 0.7071);
        cPostHP = filt::highpass (fs, fcHP * 1.2, 0.7071); cPostLP = filt::lowpass (fs, fcPost, 0.7071);
        drive = 1.0 + 4.0 * harmonics; bias = (1.0 - c) * 0.35;
        const double tb = std::tanh (bias); norm = 1.0 / (drive * (1.0 - tb * tb)); amt = amount;
    }
    inline void process (double& l, double& r)
    {
        if (amt <= 0.0) { return; }
        l = one (ch[0], l); r = one (ch[1], r);
    }
private:
    struct Chan { Biquad hp, lp, phh, plp; double envB = 0, envF = 0; };
    inline double one (Chan& s, double x)
    {
        const double band = s.lp.process (s.hp.process (x, cHP), cLP), ab = std::fabs (band), af = std::fabs (x);
        s.envB = ab > s.envB ? ab + (s.envB - ab) * aAtt : ab + (s.envB - ab) * aRel;
        s.envF = af > s.envF ? af + (s.envF - af) * aAtt : af + (s.envF - af) * aRel;
        const double u = band / (s.envB + 1.0e-4);
        const double gen = s.envB * (std::tanh (drive * u + bias) - std::tanh (bias)) * norm;
        const double ratio = s.envB / (s.envF + 1.0e-4);
        const double prot = 1.0 - 0.7 * clampd ((ratio - 0.3) / 0.4, 0.0, 1.0); // already-bright input protection
        return x + amt * 0.6 * prot * s.plp.process (s.phh.process (gen, cPostHP), cPostLP);
    }
    Chan ch[2];
    Coef cHP, cLP, cPostHP, cPostLP;
    double fs = 48000, aAtt = 0, aRel = 0, drive = 1, bias = 0, norm = 1, amt = 0;
};

// ================================================================== air stage (shelf + presence + sibilance guard)
class AirStage
{
public:
    void prepare (double sr) { fs = sr; aSA = msToCoef (0.5, fs); aSR = msToCoef (40, fs); aFA = msToCoef (1, fs); aFR = msToCoef (60, fs); reset(); }
    void reset() { for (auto& c : ch) c = Chan {}; }
    void update (double air, double presence, double softness, double brightness)
    {
        cAir = filt::highShelfS (fs, 10000.0, 9.0 * air, 1.0 - 0.65 * softness);
        cPres = filt::peaking (fs, 3200.0, 0.9, 5.0 * presence);
        cBr = filt::highShelfS (fs, 5000.0, 6.0 * brightness, 0.6);
        cSib = filt::highpass (fs, 5500.0, 0.7071);
    }
    inline void process (double& l, double& r) { l = one (ch[0], l); r = one (ch[1], r); }
private:
    struct Chan { Biquad air, pres, br, sib; double envS = 0, envF = 0; };
    inline double one (Chan& s, double y)
    {
        double a = s.br.process (s.pres.process (s.air.process (y, cAir), cPres), cBr);
        const double sb = std::fabs (s.sib.process (y, cSib)), af = std::fabs (y);
        s.envS = sb > s.envS ? sb + (s.envS - sb) * aSA : sb + (s.envS - sb) * aSR;
        s.envF = af > s.envF ? af + (s.envF - af) * aFA : af + (s.envF - af) * aFR;
        const double guard = 1.0 - 0.5 * clampd ((s.envS / (s.envF + 1.0e-4) - 0.5) / 0.4, 0.0, 1.0);
        return y + guard * (a - y);
    }
    Chan ch[2];
    Coef cAir, cPres, cBr, cSib;
    double fs = 48000, aSA = 0, aSR = 0, aFA = 0, aFR = 0;
};

// ================================================================== analog preamp: Hollywood preamp
class HollywoodPreamp
{
public:
    void reset() { for (int i = 0; i < 2; ++i) { sat[i].reset(); dc[i].reset(); } }
    void update (const double* P)
    {
        gIn = dbToLin (P[HW_IN]); gOut = dbToLin (P[HW_OUT]);
        const double dr = P[HW_DRIVE], st = P[HW_SAT];
        drive = 1.0 + 6.0 * std::pow (dr, 1.5); bias = 0.35 * st; mixAmt = std::min (1.0, 0.15 + dr * 1.6 + st * 0.4);
    }
    inline void process (double& l, double& r)
    {
        l = one (0, l); r = one (1, r);
    }
private:
    inline double one (int c, double x)
    {
        x *= gIn;
        const double s = dc[c].process (sat[c].process (x, drive, bias));
        return (x + mixAmt * (s - x)) * gOut;
    }
    Saturator sat[2]; DcBlock dc[2];
    double gIn = 1, gOut = 1, drive = 1, bias = 0, mixAmt = 0.3;
};

// ================================================================== analog preamp: U47-style microphone voicing
class U47Mic
{
public:
    void prepare (double sr) { fs = sr; reset(); }
    void reset() { for (int i = 0; i < 2; ++i) { prox[i].reset(); body[i].reset(); pres[i].reset(); top[i].reset(); sat[i].reset(); dc[i].reset(); } }
    void update (const double* P)
    {
        cProx = filt::lowShelfQ (fs, 150.0, 5.0 * P[U47_PROX], 0.6);
        cBody = filt::peaking (fs, 240.0, 0.8, 3.0 * P[U47_BODY]);
        cPres = filt::peaking (fs, 7000.0, 0.9, 4.0 * P[U47_PRES]);
        cTop = filt::highShelfQ (fs, 14000.0, -1.5, 0.6); // gentle vintage top roll-off
        color = P[U47_COLOR]; amt = P[U47_AMT];
    }
    inline void process (double& l, double& r) { l = one (0, l); r = one (1, r); }
private:
    inline double one (int c, double x)
    {
        double y = top[c].process (pres[c].process (body[c].process (prox[c].process (x, cProx), cBody), cPres), cTop);
        if (color > 0.001) { const double m = std::min (1.0, color * 3.0); y += m * (dc[c].process (sat[c].process (y, 1.0 + 1.5 * color, 0.25 * color)) - y); }
        return x + amt * (y - x);
    }
    Biquad prox[2], body[2], pres[2], top[2]; Saturator sat[2]; DcBlock dc[2];
    Coef cProx, cBody, cPres, cTop;
    double fs = 48000, color = 0, amt = 1;
};

// ================================================================== analog preamp: Revival (shimmer + thickness)
class Revival
{
public:
    void prepare (double sr)
    {
        fs = sr; exc.prepare (fs); air.prepare (fs); cBP = filt::bandpass (fs, 250.0, 0.7); reset();
    }
    void reset() { exc.reset(); air.reset(); for (int i = 0; i < 2; ++i) { bp[i].reset(); sat[i].reset(); } }
    void update (const double* P)
    {
        const double sh = P[RV_SHIM];
        exc.update (sh, 0.8, 0.5); air.update (sh * 0.5, 0.0, 0.6, 0.0);
        thick = P[RV_THICK]; amt = P[RV_AMT];
    }
    inline void process (double& l, double& r)
    {
        double yl = l, yr = r;
        exc.process (yl, yr); air.process (yl, yr);
        if (thick > 0.001)
        {
            const double bl = bp[0].process (l, cBP), br = bp[1].process (r, cBP);
            yl += thick * 0.6 * sat[0].process (bl, 3.0, 0.15);
            yr += thick * 0.6 * sat[1].process (br, 3.0, 0.15);
        }
        l += amt * (yl - l); r += amt * (yr - r);
    }
private:
    Exciter exc; AirStage air; Biquad bp[2]; Saturator sat[2]; Coef cBP;
    double fs = 48000, thick = 0, amt = 1;
};

// ================================================================== 8-band parametric EQ (minimum phase, biquads)
class ParamEQ8
{
public:
    void prepare (double sr) { fs = sr; reset(); }
    void reset() { for (auto& b : band) { b.reset(); b.w.snap (0); } }
    // gainScale lets the Brightness chain reuse the very same settings at a different amount
    void update (const double* P, double gainScale = 1.0)
    {
        for (int i = 0; i < 8; ++i)
        {
            const int base = EQ1_ON + i * 5;
            const bool on = P[base] > 0.5;
            const int type = (int) clampd (P[base + 1], 0, 5);
            const double f = P[base + 2], g = P[base + 3] * gainScale, q = P[base + 4];
            auto& b = band[i];
            b.c = coefFor (fs, type, f, g, q);
            const bool identity = type <= 2 && std::fabs (g) < 0.005;
            b.setTarget (on && ! identity ? 1.0 : 0.0);
        }
    }
    inline void process (double& l, double& r) { for (auto& b : band) b.process (l, r); }
    // coefficient design shared with the GUI curve display
    static Coef coefFor (double fs, int type, double f, double g, double q)
    {
        switch (type)
        {
            case 0: return filt::peaking (fs, f, q, g);
            case 1: return filt::lowShelfQ (fs, f, g, clampd (q, 0.3, 1.2));
            case 2: return filt::highShelfQ (fs, f, g, clampd (q, 0.3, 1.2));
            case 3: return filt::highpass (fs, f, q);
            case 4: return filt::lowpass (fs, f, q);
            default: return filt::notch (fs, f, q);
        }
    }
    BandProc band[8];
private:
    double fs = 48000;
};

// ================================================================== SSL 4000 J-style console EQ
class SSLEQ
{
public:
    void prepare (double sr) { fs = sr; reset(); }
    void reset() { for (auto* b : { &hf, &hmf, &lmf, &lf, &hpf, &lpf }) { b->reset(); b->w.snap (0); } hpA.z = hpB.z = 0; hpOn.snap (0); }
    void update (const double* P)
    {
        hf.c = P[SSL_HF_BELL] > 0.5 ? filt::peaking (fs, P[SSL_HF_FREQ], 1.0, P[SSL_HF_GAIN]) : filt::highShelfQ (fs, P[SSL_HF_FREQ], P[SSL_HF_GAIN], 0.7);
        hmf.c = filt::peaking (fs, P[SSL_HMF_FREQ], P[SSL_HMF_Q], P[SSL_HMF_GAIN]);
        lmf.c = filt::peaking (fs, P[SSL_LMF_FREQ], P[SSL_LMF_Q], P[SSL_LMF_GAIN]);
        lf.c = P[SSL_LF_BELL] > 0.5 ? filt::peaking (fs, P[SSL_LF_FREQ], 1.0, P[SSL_LF_GAIN]) : filt::lowShelfQ (fs, P[SSL_LF_FREQ], P[SSL_LF_GAIN], 0.7);
        hpf.c = filt::highpass (fs, P[SSL_HPF_FREQ], 0.7071); lpf.c = filt::lowpass (fs, P[SSL_LPF_FREQ], 0.7071);
        hpCoef = OnePole::coefHz (P[SSL_HPF_FREQ], fs);
        auto on = [] (BandProc& b, double g) { b.setTarget (std::fabs (g) < 0.005 ? 0.0 : 1.0); };
        on (hf, P[SSL_HF_GAIN]); on (hmf, P[SSL_HMF_GAIN]); on (lmf, P[SSL_LMF_GAIN]); on (lf, P[SSL_LF_GAIN]);
        hpf.setTarget (P[SSL_HPF_ON] > 0.5 ? 1.0 : 0.0); lpf.setTarget (P[SSL_LPF_ON] > 0.5 ? 1.0 : 0.0);
        hpOn.setTarget (P[SSL_HPF_ON] > 0.5 ? 1.0 : 0.0, kControlRate);
    }
    inline void process (double& l, double& r)
    {
        hpf.process (l, r); // 12 dB/oct + a first-order section below = 18 dB/oct like the console
        const double w = hpOn.next();
        if (w > 0.0) { l -= w * hpA.lp (l, hpCoef); r -= w * hpB.lp (r, hpCoef); }
        else { hpA.z = hpB.z = 0; }
        lf.process (l, r); lmf.process (l, r); hmf.process (l, r); hf.process (l, r); lpf.process (l, r);
    }
private:
    BandProc hf, hmf, lmf, lf, hpf, lpf;
    OnePole hpA, hpB; Ramp hpOn; double hpCoef = 0.9, fs = 48000;
};

// ================================================================== Maag EQ4-style (with the wide Air Band)
class MaagEQ
{
public:
    void prepare (double sr) { fs = sr; reset(); }
    void reset() { for (auto& b : band) { b.reset(); b.w.snap (0); } }
    void update (const double* P)
    {
        static const double af[5] = { 2500, 5000, 10000, 20000, 40000 };
        const double airCorner = af[(int) clampd (P[MG_AIRF], 0, 4)] * 0.5;
        band[0].c = filt::lowShelfQ (fs, 30.0, P[MG_SUB], 0.5);
        const double fq[5] = { 40, 160, 650, 2500, 5000 };
        for (int i = 0; i < 5; ++i) band[1 + i].c = filt::peaking (fs, fq[i], 0.8, P[MG_40 + i]);
        band[6].c = filt::highShelfQ (fs, airCorner, P[MG_AIR], 0.4);
        const int idx[7] = { MG_SUB, MG_40, MG_160, MG_650, MG_2K5, MG_5K, MG_AIR };
        for (int i = 0; i < 7; ++i) band[i].setTarget (std::fabs (P[idx[i]]) < 0.005 ? 0.0 : 1.0);
        gOut = dbToLin (P[MG_OUT]);
    }
    inline void process (double& l, double& r) { for (auto& b : band) b.process (l, r); l *= gOut; r *= gOut; }
private:
    BandProc band[7]; double fs = 48000, gOut = 1;
};

// ================================================================== tube saturation (wet path)
class TubeSat
{
public:
    void prepare (double sr) { fs = sr; reset(); }
    void reset() { for (int i = 0; i < 2; ++i) { sat[i].reset(); dc[i].reset(); ws[i].reset(); ts[i].reset(); tl[i].reset(); lp[i].z = 0; } }
    void update (const double* P)
    {
        gDrive = dbToLin (18.0 * P[TB_DRIVE]); comp = 1.0 / std::pow (gDrive, 0.7);
        const double ch = P[TB_CHAR];
        d = 1.2 + 2.8 * ch; bias = 0.3 * (1.0 - ch) + 0.03; harm = P[TB_HARM];
        cWarm = filt::lowShelfQ (fs, 180.0, 6.0 * P[TB_WARM], 0.6);
        lpA = OnePole::coefHz (18000.0 - 10000.0 * P[TB_WARM], fs);
        cTS = filt::highShelfQ (fs, 2000.0, 6.0 * P[TB_TONE], 0.5); cTL = filt::lowShelfQ (fs, 2000.0, -6.0 * P[TB_TONE], 0.5);
        gOut = dbToLin (P[TB_OUT]);
    }
    inline void process (double l, double r, double& oL, double& oR) { oL = one (0, l); oR = one (1, r); }
private:
    inline double one (int c, double x)
    {
        const double pre = x * gDrive;
        double s = dc[c].process (sat[c].process (pre, d, bias));
        s = (x * gDrive + harm * (s - x * gDrive)) * comp;
        s = lp[c].lp (ws[c].process (s, cWarm), lpA);
        s = tl[c].process (ts[c].process (s, cTS), cTL);
        return s * gOut;
    }
    Saturator sat[2]; DcBlock dc[2]; Biquad ws[2], ts[2], tl[2]; OnePole lp[2];
    Coef cWarm, cTS, cTL;
    double fs = 48000, gDrive = 1, comp = 1, d = 1, bias = 0, harm = 0.5, lpA = 0.5, gOut = 1;
};
} // namespace mk
