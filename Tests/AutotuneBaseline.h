// Frozen copy of the v1 Autotune (before the latency optimisation) used only for A/B tests.
// MAKETEO - AUTOTUNE PRO module: causal pitch correction.
//
// Architecture (no lookahead beyond a fixed, reported latency):
//   * YIN pitch detector running on a 12 kHz decimated copy of the input.
//   * Scale/key target selection with hysteresis, retune-speed smoothing, humanize, flex-tune dead zone and vibrato preservation.
//   * Pitch-synchronous overlap-add (PSOLA) resynthesis. Grains are two periods long and are placed at the corrected period,
//     which preserves formants (Formant Correction on). With Formant Correction off the grain is resampled so formants follow pitch.
//   * A fixed input delay D (reported to the host) gives the grain extractor enough past/future signal. D depends on the Input Type.
#pragma once
#include "DSP/Common.h"
#include "Params.h"

namespace mkbase
{
using namespace mk;
class Autotune
{
public:
    // GUI meters
    double detectedHz = 0.0, shiftSt = 0.0, targetNote = 0.0;
    bool voiced = false;

    static void rangeFor (int inputType, double& fmin, double& fmax)
    {
        static const double mn[3] = { 170.0, 95.0, 65.0 }, mx[3] = { 1100.0, 800.0, 600.0 };
        fmin = mn[inputType]; fmax = mx[inputType];
    }
    static int latencyFor (double fs, int inputType)
    {
        double fmin, fmax; rangeFor (inputType, fmin, fmax);
        return (int) std::ceil (2.8 * fs / fmin) + 16;
    }
    int latencySamples() const { return D; }
    int inputType() const { return type; }

    void prepare (double sr)
    {
        fs = sr; dec = std::max (1, (int) std::lround (fs / 12000.0)); fsd = fs / dec;
        int n = 1; while (n < (int) (fs * 0.5)) n <<= 1; // ring size: ~0.5 s
        N = n; mask = n - 1;
        inL.assign ((size_t) N, 0.0); inR.assign ((size_t) N, 0.0); outL.assign ((size_t) N, 0.0); outR.assign ((size_t) N, 0.0);
        yin.assign (4096, 0.0); win.assign (4096, 0.0); dd.assign (512, 0.0); cm.assign (512, 0.0);
        cLP = filt::lowpass (fs, 2400.0, 0.7071);
        hopSec = (double) hopD * dec / fs; aBlend = msToCoef (6.0, fs);
        configure (type, true);
    }
    void configure (int inputType, bool force = false)
    {
        inputType = (int) clampd (inputType, 0, 2);
        if (! force && inputType == type) return;
        type = inputType; rangeFor (type, fmin, fmax); D = latencyFor (fs, type);
        tauMin = std::max (2, (int) std::floor (fsd / fmax)); tauMax = std::min (300, (int) std::ceil (fsd / fmin));
        W = 2 * tauMax; reset();
    }
    void reset()
    {
        std::fill (inL.begin(), inL.end(), 0.0); std::fill (inR.begin(), inR.end(), 0.0);
        std::fill (outL.begin(), outL.end(), 0.0); std::fill (outR.begin(), outR.end(), 0.0);
        std::fill (yin.begin(), yin.end(), 0.0);
        lp1.reset(); lp2.reset(); n = 0; decCount = 0; yinW = 0; hopCount = 0;
        voiced = false; voicedHold = 0; slowM = 0; target = -1000; shiftSm = 0; shiftSt = 0; held = 0; blend = 0.0;
        T0 = 200.0; ratio = 1.0; f0hist[0] = f0hist[1] = f0hist[2] = 0.0; detectedHz = 0.0; sNext = 0.0;
    }

    void update (const double* P)
    {
        keyPc = (int) P[AT_KEY]; scale = (int) P[AT_SCALE];
        hard = P[AT_MODE] > 0.5;
        retuneMs = P[AT_RETUNE]; human = P[AT_HUMAN]; flex = P[AT_FLEX]; vib = P[AT_VIB];
        fcorr = P[AT_FCORR] > 0.5; fshift = P[AT_FSHIFT]; trans = P[AT_TRANS];
        for (int i = 0; i < 12; ++i) custom[i] = P[AT_N0 + i] > 0.5;
        if (hard) { retuneMs = std::min (retuneMs, 3.0); human = 0.0; flex = 0.0; vib = 0.0; }
    }

    // While bypassed the module still records its input history, so switching it on is a smooth cross-fade (no silent gap).
    inline void feed (double l, double r)
    {
        const size_t wi = (size_t) (n & mask);
        inL[wi] = l; inR[wi] = r; outL[wi] = 0.0; outR[wi] = 0.0; ++n;
        blend = 0.0;
    }

    inline void process (double& l, double& r)
    {
        const size_t wi = (size_t) (n & mask);
        inL[wi] = l; inR[wi] = r;
        detectStep (0.5 * (l + r));

        // place every grain whose window has started to overlap the current output sample
        const double Tout = std::max (20.0, T0 / ratio);
        double rs = fcorr ? 1.0 : ratio;
        rs = clampd (rs * std::pow (2.0, fshift / 12.0), 0.8, 1.25);
        const double L = std::max (8.0, T0 / rs);
        if (voiced) { while (sNext - L <= (double) n) { placeGrain (sNext, rs, L, Tout); sNext += std::max (20.0, T0 / ratio); } }
        else sNext = (double) n + L;

        const double tgt = (voiced && std::fabs (shiftSt + trans) > 0.03) ? 1.0 : 0.0;
        blend = tgt + (blend - tgt) * aBlend;
        const double dryL = inL[(size_t) ((n - D) & mask)], dryR = inR[(size_t) ((n - D) & mask)];
        const double oL = outL[wi], oR = outR[wi];
        outL[wi] = 0.0; outR[wi] = 0.0;
        l = dryL + blend * (oL - dryL); r = dryR + blend * (oR - dryR);
        ++n;
    }

private:
    // ---------------------------------------------------------------- detection
    inline void detectStep (double mono)
    {
        const double y = lp2.process (lp1.process (mono, cLP), cLP);
        if (++decCount < dec) return;
        decCount = 0;
        yin[(size_t) (yinW & 4095)] = y; ++yinW;
        if (++hopCount < hopD) return;
        hopCount = 0;
        double period = 0.0, conf = 0.0;
        const bool ok = yinDetect (period, conf);
        onHop (ok, period);
    }

    bool yinDetect (double& period, double& conf)
    {
        const int total = W + tauMax + 1;
        if (yinW < total) return false;
        double e = 0.0;
        for (int i = 0; i < total; ++i) win[(size_t) i] = yin[(size_t) ((yinW - total + i) & 4095)];
        for (int i = total - W; i < total; ++i) e += win[(size_t) i] * win[(size_t) i];
        if (std::sqrt (e / W) < 0.0018) return false; // below about -55 dBFS: unvoiced / silence
        double run = 0.0; cm[0] = 1.0;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            double d = 0.0; const double* a = win.data(); const double* b = win.data() + tau;
            for (int j = 0; j < W; ++j) { const double t = a[j] - b[j]; d += t * t; }
            dd[(size_t) tau] = d; run += d;
            cm[(size_t) tau] = run > 1.0e-12 ? d * tau / run : 1.0;
        }
        int tau = -1;
        for (int t = tauMin; t <= tauMax; ++t)
            if (cm[(size_t) t] < 0.15) { tau = t; while (tau + 1 <= tauMax && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau; break; }
        if (tau < 0)
        {
            int best = tauMin; for (int t = tauMin; t <= tauMax; ++t) if (cm[(size_t) t] < cm[(size_t) best]) best = t;
            if (cm[(size_t) best] > 0.32) return false;
            tau = best;
        }
        double tf = tau;
        if (tau > 1 && tau < tauMax)
        {
            const double a = cm[(size_t) tau - 1], b = cm[(size_t) tau], c = cm[(size_t) tau + 1], den = a - 2 * b + c;
            if (std::fabs (den) > 1.0e-12) tf += clampd (0.5 * (a - c) / den, -1.0, 1.0);
        }
        period = tf * dec; conf = 1.0 - cm[(size_t) tau];
        return true;
    }

    static bool allowed (int note, int key, int scale, const bool* custom)
    {
        const int pc = ((note % 12) + 12) % 12;
        static const bool maj[12] = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 }, mnr[12] = { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0 };
        const int rel = ((pc - key) % 12 + 12) % 12;
        switch (scale) { case 1: return maj[rel]; case 2: return mnr[rel]; case 3: return custom[pc]; default: return true; }
    }

    void onHop (bool ok, double period)
    {
        if (ok)
        {
            double f0 = fs / period;
            f0hist[2] = f0hist[1]; f0hist[1] = f0hist[0]; f0hist[0] = f0;
            if (f0hist[2] > 0.0) // median of 3 rejects single-hop octave errors
            {
                const double a = f0hist[0], b = f0hist[1], c = f0hist[2];
                f0 = std::max (std::min (a, b), std::min (std::max (a, b), c));
            }
            const double m = 69.0 + 12.0 * std::log2 (f0 / 440.0);
            if (! voiced) { voiced = true; slowM = m; target = -1000; shiftSm = 0.0; held = 0.0; sNext = (double) n; }
            voicedHold = 6;
            detectedHz = f0;
            T0 += 0.5 * ((fs / f0) - T0);

            slowM += (m - slowM) * (1.0 - std::exp (-hopSec / 0.08));
            const double vibDev = m - slowM;
            int best = (int) std::floor (slowM); double bd = 1.0e9;
            bool anyAllowed = false;
            for (int c = (int) std::floor (slowM) - 2; c <= (int) std::floor (slowM) + 3; ++c)
                if (allowed (c, keyPc, scale, custom)) { anyAllowed = true; const double d = std::fabs (c - slowM); if (d < bd) { bd = d; best = c; } }
            if (! anyAllowed) best = (int) std::lround (slowM);
            const bool prevOk = target > -999 && allowed ((int) target, keyPc, scale, custom);
            if (! (prevOk && std::fabs (slowM - target) <= bd + 0.3)) target = best; // hysteresis: avoid flicker between notes
            targetNote = target;

            double err = target - slowM;
            const double dead = 0.5 * flex;
            err = err > 0 ? std::max (0.0, err - dead) : std::min (0.0, err + dead);
            const double desired = clampd (err - (1.0 - vib) * vibDev, -3.0, 3.0);

            if (std::fabs (target - slowM) < 0.2) held = std::min (1.0, held + hopSec / 0.3); else held = std::max (0.0, held - hopSec / 0.1);
            const double tau = retuneMs * (1.0 + 3.0 * human * held);
            const double a = tau <= 0.5 ? 1.0 : 1.0 - std::exp (-hopSec / (tau * 0.001));
            shiftSm += (desired - shiftSm) * a;
        }
        else if (voicedHold > 0 && --voicedHold == 0) { voiced = false; detectedHz = 0.0; shiftSm = 0.0; }
        shiftSt = shiftSm;
        ratio = clampd (std::pow (2.0, (shiftSm + trans) / 12.0), 0.7, 1.4);
    }

    // ---------------------------------------------------------------- resynthesis
    inline double readRing (const std::vector<double>& b, double pos) const
    {
        const double fl = std::floor (pos); const int i = (int) fl; const double f = pos - fl;
        return b[(size_t) (i & mask)] * (1.0 - f) + b[(size_t) ((i + 1) & mask)] * f;
    }
    void placeGrain (double s, double rs, double Lf, double Tout)
    {
        const double c = s - D;                       // input time this grain represents
        const int ci = (int) std::lround (c), span = std::max (2, (int) (T0 * 0.5));
        int p = ci; double pv = -1.0e30;               // snap to the strongest local peak: keeps grains pitch-synchronous
        for (int k = -span; k <= span; ++k)
        {
            const size_t ix = (size_t) ((ci + k) & mask); const double v = inL[ix] + inR[ix];
            if (v > pv) { pv = v; p = ci + k; }
        }
        const int L = (int) std::ceil (Lf);
        const int o = (int) std::floor (s); const double fr = s - o;
        const double gain = Tout * rs / T0;
        for (int k = -L; k <= L; ++k)
        {
            const double u = ((double) k - fr) / Lf;
            if (u <= -1.0 || u >= 1.0) continue;
            const double w = 0.5 * (1.0 + std::cos (kPi * u)) * gain;
            const double pos = p + ((double) k - fr) * rs;
            const size_t oi = (size_t) ((o + k) & mask);
            outL[oi] += w * readRing (inL, pos); outR[oi] += w * readRing (inR, pos);
        }
    }

    // state
    std::vector<double> inL, inR, outL, outR, yin, win, dd, cm;
    Biquad lp1, lp2; Coef cLP;
    double fs = 48000, fsd = 12000, fmin = 95, fmax = 800, hopSec = 0.0027, aBlend = 0;
    int dec = 4, N = 32768, mask = 32767, D = 1000, type = 1, tauMin = 10, tauMax = 100, W = 200;
    int decCount = 0, hopCount = 0, hopD = 32, voicedHold = 0;
    int64_t n = 0, yinW = 0;
    double T0 = 200, ratio = 1, sNext = 0, slowM = 0, target = -1000, shiftSm = 0, held = 0, blend = 0, f0hist[3] = { 0, 0, 0 };
    // params
    int keyPc = 0, scale = 1; bool hard = false, fcorr = true, custom[12] = {}; double retuneMs = 30, human = 0.3, flex = 0.2, vib = 0.7, fshift = 0, trans = 0;
};
} // namespace mkbase
