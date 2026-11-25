// MAKETEO - time based parallel effects: FDN reverb (4 algorithms x 4 colour modes), stereo delay, vocal doubler.
#pragma once
#include "Common.h"
#include "../Params.h"

namespace mk
{
// ================================================================== REVERB: 8-line feedback delay network
class Reverb
{
public:
    void prepare (double sr)
    {
        fs = sr;
        for (auto& l : line) l.prepare ((int) (0.45 * fs));
        pd[0].prepare ((int) (0.3 * fs)); pd[1].prepare ((int) (0.3 * fs));
        static const double apMs[4] = { 4.7, 7.3, 11.1, 16.7 };
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 4; ++i) { ap[c][i].dl.prepare ((int) (0.03 * fs)); ap[c][i].len = std::max (2, (int) (apMs[i] * (c ? 1.13 : 1.0) * fs / 1000.0)); }
        for (int i = 0; i < 8; ++i) phase[i] = i * 0.125;
        reset();
    }
    void reset()
    {
        for (auto& l : line) l.reset();
        for (int c = 0; c < 2; ++c) { pd[c].reset(); for (auto& a : ap[c]) a.dl.reset(); hp[c].reset(); lp[c].reset(); sh[c].reset(); sl[c].reset(); }
        for (auto& d : damp) d.z = 0;
    }
    void update (const double* P)
    {
        static const double sets[4][8] = {
            { 11.3, 13.7, 17.1, 19.9, 23.3, 27.7, 31.1, 37.9 },      // Room
            { 23.1, 29.3, 37.7, 43.1, 53.3, 61.7, 71.3, 79.1 },      // Plate
            { 43.7, 53.1, 67.3, 79.9, 97.1, 113.3, 131.9, 149.3 },   // Hall
            { 17.9, 25.3, 33.1, 47.9, 59.3, 71.9, 83.3, 101.1 } };   // Chamber
        const int algo = (int) clampd (P[RVB_ALGO], 0, 3), cmode = (int) clampd (P[RVB_CMODE], 0, 3);
        const double size = P[RVB_SIZE], rt60 = P[RVB_DECAY];
        modDepth = P[RVB_MOD] * 0.002 * fs * (cmode == 3 ? 1.6 : 1.0);
        for (int i = 0; i < 8; ++i)
        {
            len[i] = clampd (sets[algo][i] * size * fs / 1000.0, 8.0, 0.4 * fs);
            gain[i] = std::min (0.9995, std::pow (10.0, -3.0 * len[i] / (fs * rt60)));
        }
        double fcDamp = 20000.0 * std::pow (0.02, P[RVB_DAMP]);
        fcDamp *= cmode == 1 ? 0.5 : cmode == 2 ? 1.6 : cmode == 3 ? 0.55 : 1.0;
        dampA = OnePole::coefHz (fcDamp, fs);
        preS = std::max (1.0, P[RVB_PRE] * fs / 1000.0);
        width = P[RVB_WIDTH]; out = dbToLin (P[RVB_OUT]);
        const double col = P[RVB_COLOR];
        cHP = filt::highpass (fs, P[RVB_LC], 0.7071); cLP = filt::lowpass (fs, P[RVB_HC], 0.7071);
        cSH = filt::highShelfQ (fs, 2000.0, 6.0 * col, 0.5); cSL = filt::lowShelfQ (fs, 2000.0, -4.0 * col, 0.5);
        apG = algo == 1 ? 0.7 : algo == 0 ? 0.5 : 0.6;
    }
    inline void process (double inL, double inR, double& oL, double& oR)
    {
        pd[0].push (inL); pd[1].push (inR);
        double x[2] = { pd[0].read (preS), pd[1].read (preS) };
        for (int c = 0; c < 2; ++c)
            for (auto& a : ap[c])
            {
                const double d = a.dl.tap (a.len), w = x[c] + apG * d;
                a.dl.push (w); x[c] = -apG * w + d;
            }
        double v[8], sum = 0.0;
        for (int i = 0; i < 8; ++i)
        {
            phase[i] += rates[i] / fs; if (phase[i] >= 1.0) phase[i] -= 1.0;
            const double dly = clampd (len[i] + modDepth * std::sin (2.0 * kPi * phase[i]), 2.0, 0.44 * fs);
            v[i] = damp[i].lp (line[i].read (dly), dampA);
            sum += v[i];
        }
        const double k = sum * 0.25; // Householder reflection: I - (2/N) * ones
        double a = 0.0, b = 0.0;
        for (int i = 0; i < 8; ++i)
        {
            line[i].push ((v[i] - k) * gain[i] + 0.35 * x[i & 1]);
            const double s = (i & 2) ? -v[i] : v[i];
            if (i & 1) b += s; else a += s;
        }
        a *= 0.5; b *= 0.5;
        const double m = 0.5 * (a + b), s = 0.5 * (a - b) * width;
        double yl = m + s, yr = m - s;
        yl = lp[0].process (hp[0].process (yl, cHP), cLP); yr = lp[1].process (hp[1].process (yr, cHP), cLP);
        yl = sl[0].process (sh[0].process (yl, cSH), cSL); yr = sl[1].process (sh[1].process (yr, cSH), cSL);
        oL = yl * out; oR = yr * out;
    }
private:
    struct AP { DelayLine dl; int len = 2; };
    DelayLine line[8], pd[2]; AP ap[2][4]; OnePole damp[8]; Biquad hp[2], lp[2], sh[2], sl[2]; Coef cHP, cLP, cSH, cSL;
    double fs = 48000, len[8] = {}, gain[8] = {}, phase[8] = {}, dampA = 0.5, modDepth = 0, preS = 100, width = 1, out = 1, apG = 0.6;
    const double rates[8] = { 0.13, 0.17, 0.23, 0.31, 0.37, 0.43, 0.51, 0.59 };
};

// ================================================================== DELAY
class Delay
{
public:
    void prepare (double sr)
    {
        fs = sr; for (auto& d : dl) d.prepare ((int) (2.3 * fs)); reset();
    }
    void reset()
    {
        for (auto& d : dl) d.reset();
        for (int c = 0; c < 2; ++c) { hp[c].reset(); lp[c].reset(); ahp[c].reset(); alp[c].reset(); sat[c].reset(); hold[c] = 0; cnt[c] = 0; }
        phase = 0; timeSm = -1;
    }
    void setTempo (double b) { bpm = b > 20.0 ? b : 120.0; }
    void update (const double* P)
    {
        static const double fac[8] = { 2.0, 1.0, 1.5, 2.0 / 3.0, 0.5, 0.75, 1.0 / 3.0, 0.25 };
        double ms = P[DLY_TIME];
        if (P[DLY_SYNC] > 0.5) ms = 60000.0 / bpm * fac[(int) clampd (P[DLY_DIV], 0, 7)];
        targetS = clampd (ms * fs / 1000.0, 2.0, 2.0 * fs);
        if (timeSm < 0) timeSm = targetS;
        timeSm += (targetS - timeSm) * (1.0 - std::exp (-(double) kControlRate / (0.12 * fs))); // tape-like glide, no clicks
        fb = P[DLY_FB]; pp = P[DLY_PP] > 0.5; analog = P[DLY_ANALOG] > 0.5; lofi = P[DLY_LOFI];
        const double lpHz = analog ? std::min (P[DLY_LP], 4500.0) : P[DLY_LP];
        cHP = filt::highpass (fs, P[DLY_HP], 0.7071); cLP = filt::lowpass (fs, lpHz, 0.7071);
        modDepth = P[DLY_MOD] * 0.004 * fs + (analog ? 0.0006 * fs : 0.0);
        width = P[DLY_WIDTH]; out = dbToLin (P[DLY_OUT]);
        lofiK = 1 + (int) std::lround (lofi * 7.0);
        bits = 12.0 - 6.0 * lofi; // 12 -> 6 bits
    }
    inline void process (double inL, double inR, double& oL, double& oR)
    {
        phase += (analog ? 0.45 : 0.6) / fs; if (phase >= 1.0) phase -= 1.0;
        const double lfo = std::sin (2.0 * kPi * phase);
        const double dL = clampd (timeSm + modDepth * lfo, 2.0, 2.2 * fs), dR = clampd (timeSm - modDepth * lfo, 2.0, 2.2 * fs);
        double tL = dl[0].read (dL), tR = dl[1].read (dR);
        double fL = tL, fR = tR;
        for (int c = 0; c < 2; ++c) { double& f = c ? fR : fL; f = lp[c].process (hp[c].process (f, cHP), cLP); }
        if (analog) { fL = sat[0].process (fL, 1.6, 0.0); fR = sat[1].process (fR, 1.6, 0.0); }
        double wL, wR;
        if (pp) { const double mono = 0.5 * (inL + inR); wL = mono + fb * fR; wR = fL; }
        else { wL = inL + fb * fL; wR = inR + fb * fR; }
        if (lofi > 0.001) { wL = crush (0, wL); wR = crush (1, wR); }
        dl[0].push (wL); dl[1].push (wR);
        const double m = 0.5 * (fL + fR), s = 0.5 * (fL - fR) * width;
        oL = (m + s) * out; oR = (m - s) * out;
    }
private:
    inline double crush (int c, double x) // sample-rate and bit-depth reduction inside the feedback loop
    {
        if ((cnt[c]++ % lofiK) == 0) { const double q = std::pow (2.0, bits - 1.0); hold[c] = std::round (x * q) / q; }
        return hold[c];
    }
    DelayLine dl[2]; Biquad hp[2], lp[2], ahp[2], alp[2]; Saturator sat[2]; Coef cHP, cLP;
    double fs = 48000, bpm = 120, targetS = 1000, timeSm = -1, fb = 0.3, lofi = 0, bits = 12, modDepth = 0, width = 1, out = 1, phase = 0, hold[2] = {};
    int lofiK = 1; int64_t cnt[2] = { 0, 0 }; bool pp = true, analog = true;
};

// ================================================================== DOUBLER (wet only, original stays untouched in the centre)
class Doubler
{
public:
    void prepare (double sr)
    {
        fs = sr; W = 0.016 * fs; dl.prepare ((int) (0.13 * fs)); reset();
    }
    void reset() { dl.reset(); ph[0] = 0.0; ph[1] = 0.37; lfoP[0] = 0.0; lfoP[1] = 0.5; }
    void update (const double* P)
    {
        for (int v = 0; v < 2; ++v)
        {
            const double det = P[v ? DBL_DETR : DBL_DETL], ms = P[v ? DBL_DLYR : DBL_DLYL];
            dph[v] = (1.0 - std::pow (2.0, det / 1200.0)) / W;
            base[v] = std::max (2.0, ms * fs / 1000.0 - 0.5 * W);
        }
        modDepth = P[DBL_MOD] * 0.0015 * fs; spread = P[DBL_SPREAD]; width = P[DBL_WIDTH]; out = dbToLin (P[DBL_OUT]);
    }
    inline void process (double inL, double inR, double& oL, double& oR)
    {
        dl.push (0.5 * (inL + inR));
        double vo[2];
        for (int v = 0; v < 2; ++v)
        {
            lfoP[v] += (v ? 0.47 : 0.35) / fs; if (lfoP[v] >= 1.0) lfoP[v] -= 1.0;
            ph[v] += dph[v]; ph[v] -= std::floor (ph[v]);
            const double md = modDepth * std::sin (2.0 * kPi * lfoP[v]);
            const double p2 = ph[v] + 0.5 - std::floor (ph[v] + 0.5);
            const double s1 = std::sin (kPi * ph[v]), s2 = std::sin (kPi * p2);
            vo[v] = dl.read (clampd (base[v] + md + ph[v] * W, 2.0, 0.125 * fs)) * s1 * s1
                  + dl.read (clampd (base[v] + md + p2 * W, 2.0, 0.125 * fs)) * s2 * s2;
        }
        const double a = 0.5 + 0.5 * spread, b = 0.5 - 0.5 * spread;
        const double l = vo[0] * a + vo[1] * b, r = vo[1] * a + vo[0] * b;
        const double m = 0.5 * (l + r), s = 0.5 * (l - r) * width;
        oL = (m + s) * out; oR = (m - s) * out;
    }
private:
    DelayLine dl; double fs = 48000, W = 768, dph[2] = {}, base[2] = { 100, 100 }, ph[2] = {}, lfoP[2] = {}, modDepth = 0, spread = 1, width = 1, out = 1;
};
} // namespace mk
