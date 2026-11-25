# Changelog

## v1.1
- UI: complete redesign (modern minimal, Evric Hunter palette, three pages VOCAL / FX / ADVANCED, large sliders, real meters, Spanish descriptions). Replaces the hardware-style editor.
- Autotune: latency bound derived from the grain geometry (-9 to -11 %), read guard, YIN detector 40 % cheaper; algorithm and quality unchanged (A/B tests in `Tests/dsp_test.cpp`, frozen v1 copy in `Tests/AutotuneBaseline.h`).
- Unchanged: all 209 parameters, IDs, ranges, defaults, APVTS, state, presets, routing and every other DSP module.

## Files modified
Source/PluginEditor.h, Source/PluginEditor.cpp, Source/DSP/Autotune.h, Tests/dsp_test.cpp, README.md.
New: Tests/AutotuneBaseline.h (test-only), CHANGELOG.md.
