// MAKETEO - engine: MAIN VOCAL ENGINE (serial) + PARALLEL PROCESSING ENGINE (six independent chains) + routing.
//
//   MIC IN -> in trim -> AUTOTUNE -> RIDER -> HOLLYWOOD -> U47 -> REVIVAL -> EQ8 -> DE-ESS HIGH -> DE-ESS LOW
//          -> 1176 -> LA-2A -> SSL EQ -> MAAG EQ4 -> MAIN VOCAL OUTPUT --+--> REVERB / DELAY / NY / BRIGHTNESS / DOUBLER / TUBE
//   OUT = (MAIN + sum(parallel returns)) * out trim; a parallel chain never changes the main signal.
#pragma once
#include <atomic>
#include "Autotune.h"
#include "Dynamics.h"
#include "Tone.h"
#include "TimeFx.h"

namespace mk
{
enum MeterId { MT_RIDER, MT_DH, MT_DL, MT_FET, MT_LA, MT_NY, MT_PAR0, MT_PAR_LAST = MT_PAR0 + 5, MT_AT_HZ, MT_AT_SHIFT, MT_AT_NOTE, MT_AT_VOICED, MT_COUNT };

class Engine
{
public:
    static constexpr int kAnaSize = 8192;
    std::atomic<float> meters[MT_COUNT];
    std::atomic<uint32_t> anaWrite { 0 };
    float ana[kAnaSize] = {};   // analyzer tap (post EQ), read by the GUI
    Engine() { for (auto& m : meters) m.store (0.f); }

    void prepare (double sr)
    {
        fs = sr > 8000.0 ? sr : 48000.0;
        const auto* specs = paramSpecs();
        for (int i = 0; i < kNumParams; ++i) { tgt[i] = sp[i] = specs[i].def; isFloat[i] = specs[i].kind == K_FLOAT; }
        aSm = std::exp (-(double) kControlRate / (0.02 * fs));
        at.prepare (fs); rider.prepare (fs); u47.prepare (fs); rv.prepare (fs); eq.prepare (fs); eqBr.prepare (fs);
        dh.prepare (fs); dl_.prepare (fs); fet.prepare (fs); la.prepare (fs); ssl.prepare (fs); mg.prepare (fs);
        reverb.prepare (fs); delay.prepare (fs); ny.prepare (fs); exc.prepare (fs); air.prepare (fs); dbl.prepare (fs); tube.prepare (fs);
        fadeStep = (double) kControlRate / (0.01 * fs);
        reset();
    }

    void reset()
    {
        at.reset(); rider.reset(); hw.reset(); u47.reset(); rv.reset(); eq.reset(); eqBr.reset(); dh.reset(); dl_.reset(); fet.reset(); la.reset();
        ssl.reset(); mg.reset(); reverb.reset(); delay.reset(); ny.reset(); exc.reset(); air.reset(); dbl.reset(); tube.reset();
        for (auto& s : mainSlot) { s.w.snap (0); s.target = 0; }
        for (auto& s : parSlot) { s.w.snap (0); s.target = 0; }
        listenRamp.snap (0); tick = 0;
        for (int i = 0; i < kNumParams; ++i) sp[i] = tgt[i];
        controlTick (true);
    }

    // Plain parameter values (kNumParams floats). Call once per host block. snap=true applies instantly (state restore, tests).
    void setParams (const float* v, bool snap = false)
    {
        for (int i = 0; i < kNumParams; ++i) tgt[i] = (double) v[i];
        if (snap) controlTick (true);
    }
    void setTempo (double bpm) { delay.setTempo (bpm); }

    // Latency reported to the host: only AUTOTUNE needs any, and only while it is active.
    int latencySamples() const
    {
        return tgt[AT_BYP] > 0.5 ? 0 : Autotune::latencyFor (fs, (int) clampd (tgt[AT_INPUT], 0, 2));
    }

    // In-place processing of 1 or 2 channels (extra channels untouched)
    void process (float* const* io, int numCh, int numSamples)
    {
        numCh = std::min (numCh, 2);
        if (numCh < 1) return;
        double parPk[6] = { 0, 0, 0, 0, 0, 0 };
        for (int n = 0; n < numSamples; ++n)
        {
            if ((tick++ % kControlRate) == 0) controlTick (false);

            double l = io[0][n], r = numCh > 1 ? io[1][n] : l;
            if (! std::isfinite (l)) l = 0.0;
            if (! std::isfinite (r)) r = 0.0;
            l *= inGain; r *= inGain;

            // ---------------- MAIN VOCAL ENGINE
            { auto& s0 = mainSlot[0]; const double w0 = s0.w.next();
              if (w0 <= 0.0 && s0.target <= 0.0) at.feed (l, r);
              else { double a = l, b = r; at.process (a, b); l += w0 * (a - l); r += w0 * (b - r); } }
            run (mainSlot[1], rider, l, r);
            run (mainSlot[2], hw, l, r);
            run (mainSlot[3], u47, l, r);
            run (mainSlot[4], rv, l, r);
            run (mainSlot[5], eq, l, r);
            ana[anaWrite.load (std::memory_order_relaxed) & (kAnaSize - 1)] = (float) (0.5 * (l + r));
            anaWrite.store (anaWrite.load (std::memory_order_relaxed) + 1, std::memory_order_release);
            double lsL = 0, lsR = 0;
            runDe (mainSlot[6], dh, DH_LISTEN, l, r, lsL, lsR);
            runDe (mainSlot[7], dl_, DL_LISTEN, l, r, lsL, lsR);
            run (mainSlot[8], fet, l, r);
            run (mainSlot[9], la, l, r);
            run (mainSlot[10], ssl, l, r);
            run (mainSlot[11], mg, l, r);

            // ---------------- PARALLEL PROCESSING ENGINE
            double sL = 0, sR = 0, wl = 0, wr = 0;
            for (int i = 0; i < 6; ++i)
            {
                auto& s = parSlot[i];
                const double w = s.w.next();
                if (w <= 0.0 && s.target <= 0.0) continue;
                switch (i)
                {
                    case 0: reverb.process (l, r, wl, wr); break;
                    case 1: delay.process (l, r, wl, wr); break;
                    case 2: ny.process (l, r, wl, wr); break;
                    case 3: wl = l; wr = r; exc.process (wl, wr); air.process (wl, wr); eqBr.process (wl, wr); wl *= brOut; wr *= brOut; break;
                    case 4: dbl.process (l, r, wl, wr); break;
                    default: tube.process (l, r, wl, wr); break;
                }
                const double g = w * parGain[i];
                sL += g * wl; sR += g * wr;
                parPk[i] = std::max (parPk[i], std::max (std::fabs (g * wl), std::fabs (g * wr)));
            }

            double oL = (l + sL) * outGain, oR = (r + sR) * outGain;
            const double lr = listenRamp.next();
            if (lr > 0.0) { oL += lr * (lsL * outGain - oL); oR += lr * (lsR * outGain - oR); }
            if (! std::isfinite (oL)) oL = 0.0;
            if (! std::isfinite (oR)) oR = 0.0;
            if (numCh > 1) { io[0][n] = (float) oL; io[1][n] = (float) oR; }
            else io[0][n] = (float) (0.5 * (oL + oR));
        }
        meters[MT_RIDER].store ((float) rider.gainDb, std::memory_order_relaxed);
        meters[MT_DH].store ((float) dh.grDb, std::memory_order_relaxed);
        meters[MT_DL].store ((float) dl_.grDb, std::memory_order_relaxed);
        meters[MT_FET].store ((float) fet.grDb, std::memory_order_relaxed);
        meters[MT_LA].store ((float) la.grDb, std::memory_order_relaxed);
        meters[MT_NY].store ((float) ny.grDb, std::memory_order_relaxed);
        for (int i = 0; i < 6; ++i) meters[MT_PAR0 + i].store ((float) parPk[i], std::memory_order_relaxed);
        meters[MT_AT_HZ].store ((float) at.detectedHz, std::memory_order_relaxed);
        meters[MT_AT_SHIFT].store ((float) at.shiftSt, std::memory_order_relaxed);
        meters[MT_AT_NOTE].store ((float) at.targetNote, std::memory_order_relaxed);
        meters[MT_AT_VOICED].store (at.voiced ? 1.f : 0.f, std::memory_order_relaxed);
    }

private:
    struct Slot
    {
        Ramp w; double target = 0;
        bool active() const { return target > 0.0 || w.cur > 0.0; }
    };

    template <class M> inline void run (Slot& s, M& m, double& l, double& r)
    {
        const double w = s.w.next();
        if (w <= 0.0 && s.target <= 0.0) return;
        double a = l, b = r; m.process (a, b);
        l += w * (a - l); r += w * (b - r);
    }
    inline void runDe (Slot& s, DeEsser& m, int listenIdx, double& l, double& r, double& lsL, double& lsR)
    {
        const double w = s.w.next();
        if (w <= 0.0 && s.target <= 0.0) return;
        double a = l, b = r; m.process (a, b);
        l += w * (a - l); r += w * (b - r);
        if (sp[listenIdx] > 0.5) { lsL = m.listenL; lsR = m.listenR; }
    }

    // fade a slot towards "want" (0/1) over ~10 ms; returns true if it is being switched on from fully off
    bool fade (Slot& s, double want, bool snap)
    {
        const bool turningOn = want > 0.5 && s.w.cur <= 0.0 && s.target <= 0.0;
        s.target = want;
        if (snap) { s.w.snap (want); return false; }
        double nw = s.w.cur + clampd (want - s.w.cur, -fadeStep, fadeStep);
        s.w.setTarget (nw, kControlRate);
        return turningOn;
    }

    void controlTick (bool snap)
    {
        for (int i = 0; i < kNumParams; ++i)
        {
            if (isFloat[i]) { sp[i] = tgt[i] + (sp[i] - tgt[i]) * aSm; if (snap || std::fabs (sp[i] - tgt[i]) < 1.0e-7) sp[i] = tgt[i]; }
            else sp[i] = tgt[i];
        }
        inGain = dbToLin (sp[IN_TRIM]); outGain = dbToLin (sp[OUT_TRIM]);

        static const int byp[12] = { AT_BYP, RD_BYP, HW_BYP, U47_BYP, RV_BYP, EQ_BYP, DH_BYP, DL_BYP, FET_BYP, LA_BYP, SSL_BYP, MG_BYP };
        bool on[12];
        for (int i = 0; i < 12; ++i) on[i] = fade (mainSlot[i], sp[byp[i]] < 0.5 ? 1.0 : 0.0, snap);
                if (on[1]) rider.reset();
        if (on[2]) hw.reset();
        if (on[3]) u47.reset();
        if (on[4]) rv.reset();
        if (on[5]) eq.reset();
        if (on[6]) dh.reset();
        if (on[7]) dl_.reset();
        if (on[8]) fet.reset();
        if (on[9]) la.reset();
        if (on[10]) ssl.reset();
        if (on[11]) mg.reset();

        if (mainSlot[0].active()) { at.configure ((int) sp[AT_INPUT]); at.update (sp); }
        if (mainSlot[1].active()) rider.update (sp);
        if (mainSlot[2].active()) hw.update (sp);
        if (mainSlot[3].active()) u47.update (sp);
        if (mainSlot[4].active()) rv.update (sp);
        if (mainSlot[5].active() || parSlot[3].active()) eq.update (sp);
        if (mainSlot[6].active()) dh.update (sp, DH_FREQ, DH_THR, DH_RANGE, DH_SENS, DH_ATT, DH_REL, 1.2);
        if (mainSlot[7].active()) dl_.update (sp, DL_FREQ, DL_THR, DL_RANGE, DL_SENS, DL_ATT, DL_REL, 1.0);
        if (mainSlot[8].active()) fet.update (sp);
        if (mainSlot[9].active()) la.update (sp);
        if (mainSlot[10].active()) ssl.update (sp);
        if (mainSlot[11].active()) mg.update (sp);

        const bool listen = (sp[DH_LISTEN] > 0.5 && sp[DH_BYP] < 0.5) || (sp[DL_LISTEN] > 0.5 && sp[DL_BYP] < 0.5);
        { const double want = listen ? 1.0 : 0.0; listenRamp.setTarget (snap ? want : listenRamp.cur + clampd (want - listenRamp.cur, -fadeStep, fadeStep), kControlRate); if (snap) listenRamp.snap (want); }

        static const int pOn[6] = { RVB_ON, DLY_ON, NY_ON, BR_ON, DBL_ON, TB_ON }, pMix[6] = { RVB_MIX, DLY_MIX, NY_MIX, BR_MIX, DBL_MIX, TB_MIX };
        bool pon[6];
        for (int i = 0; i < 6; ++i) { pon[i] = fade (parSlot[i], sp[pOn[i]] > 0.5 ? 1.0 : 0.0, snap); parGain[i] = std::pow (clampd (sp[pMix[i]], 0.0, 1.0), 1.5); }
        if (pon[0]) reverb.reset();
        if (pon[1]) delay.reset();
        if (pon[2]) ny.reset();
        if (pon[3]) { exc.reset(); air.reset(); eqBr.reset(); }
        if (pon[4]) dbl.reset();
        if (pon[5]) tube.reset();
        if (parSlot[0].active()) reverb.update (sp);
        if (parSlot[1].active()) delay.update (sp);
        if (parSlot[2].active()) ny.update (sp);
        if (parSlot[3].active())
        {
            exc.update (sp[BR_HARM], 0.6, sp[BR_DRIVE]); air.update (sp[BR_HIAIR], sp[BR_MIDAIR], 0.5, 0.0);
            eqBr.update (sp, sp[BR_EQAMT]); brOut = dbToLin (sp[BR_OUT]);
        }
        if (parSlot[4].active()) dbl.update (sp);
        if (parSlot[5].active()) tube.update (sp);
    }

    double fs = 48000, aSm = 0.9, fadeStep = 0.01, inGain = 1, outGain = 1, brOut = 1, parGain[6] = {};
    double tgt[kNumParams] = {}, sp[kNumParams] = {}; bool isFloat[kNumParams] = {};
    uint32_t tick = 0;
    Slot mainSlot[12], parSlot[6]; Ramp listenRamp;

    Autotune at; VocalRider rider; HollywoodPreamp hw; U47Mic u47; Revival rv; ParamEQ8 eq, eqBr;
    DeEsser dh, dl_; FET1176 fet; LA2A la; SSLEQ ssl; MaagEQ mg;
    Reverb reverb; Delay delay; NYComp ny; Exciter exc; AirStage air; Doubler dbl; TubeSat tube;
};
} // namespace mk
