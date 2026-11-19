# MAKETEO parameter reference

All 209 parameters are host-automatable. Percent parameters are stored 0..1 and shown 0..100 %.

## I/O

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `in_trim` | Input | -24 .. 24 | 0 | dB |
| `out_trim` | Output | -24 .. 24 | 0 | dB |

## Autotune Pro

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `at_byp` | Bypass | off / on | on |  |
| `at_key` | Key | C|C#|D|D#|E|F|F#|G|G#|A|A#|B | C |  |
| `at_scale` | Scale | Chromatic|Major|Minor|Custom | Major |  |
| `at_mode` | Mode | Natural|Hard Tune | Natural |  |
| `at_retune` | Retune Speed | 0 .. 400 | 30 | ms |
| `at_human` | Humanize | 0 .. 1 | 0.3 | % |
| `at_flex` | Flex Tune | 0 .. 1 | 0.2 | % |
| `at_vib` | Natural Vibrato | 0 .. 1 | 0.7 | % |
| `at_fcorr` | Formant Correction | off / on | on |  |
| `at_fshift` | Formant Shift | -12 .. 12 | 0 | st |
| `at_trans` | Transpose | -12 .. 12 | 0 | st |
| `at_input` | Input Type | Soprano|Alto / Tenor|Low Male | Alto / Tenor |  |
| `at_n0` | Custom C | off / on | on |  |
| `at_n1` | Custom C# | off / on | off |  |
| `at_n2` | Custom D | off / on | on |  |
| `at_n3` | Custom D# | off / on | off |  |
| `at_n4` | Custom E | off / on | on |  |
| `at_n5` | Custom F | off / on | on |  |
| `at_n6` | Custom F# | off / on | off |  |
| `at_n7` | Custom G | off / on | on |  |
| `at_n8` | Custom G# | off / on | off |  |
| `at_n9` | Custom A | off / on | on |  |
| `at_n10` | Custom A# | off / on | off |  |
| `at_n11` | Custom B | off / on | on |  |

## Vocal Rider

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `rd_byp` | Bypass | off / on | off |  |
| `rd_target` | Target Level | -36 .. -6 | -18 | dB |
| `rd_sens` | Sensitivity | 0 .. 1 | 0.5 | % |
| `rd_speed` | Speed | 20 .. 1000 (log) | 250 | ms |
| `rd_range` | Range | 0 .. 18 | 9 | dB |
| `rd_up` | Upward Gain | 0 .. 12 | 6 | dB |
| `rd_down` | Downward Gain | 0 .. 12 | 6 | dB |
| `rd_man` | Manual Gain | -12 .. 12 | 0 | dB |

## Hollywood Preamp

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `hw_byp` | Bypass | off / on | off |  |
| `hw_in` | Input | -12 .. 12 | 0 | dB |
| `hw_drive` | Drive | 0 .. 1 | 0.25 | % |
| `hw_sat` | Saturation | 0 .. 1 | 0.3 | % |
| `hw_out` | Output | -12 .. 12 | 0 | dB |

## U47 Microphone

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `u47_byp` | Bypass | off / on | on |  |
| `u47_prox` | Proximity | 0 .. 1 | 0.3 | % |
| `u47_body` | Body | 0 .. 1 | 0.3 | % |
| `u47_pres` | Presence | 0 .. 1 | 0.3 | % |
| `u47_color` | Color | 0 .. 1 | 0.3 | % |
| `u47_amt` | Intensity | 0 .. 1 | 1 | % |

## Revival

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `rv_byp` | Bypass | off / on | on |  |
| `rv_shim` | Shimmer | 0 .. 1 | 0.3 | % |
| `rv_thick` | Thickness | 0 .. 1 | 0.3 | % |
| `rv_amt` | Intensity | 0 .. 1 | 0.7 | % |

## Parametric EQ (8 band)

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `eq_byp` | Bypass | off / on | off |  |
| `eq1_on` | Band 1 On | off / on | on |  |
| `eq1_type` | Band 1 Type | Bell|Low Shelf|High Shelf|High Pass|Low Pass|Notch | High Pass |  |
| `eq1_freq` | Band 1 Freq | 20 .. 20000 (log) | 80 | Hz |
| `eq1_gain` | Band 1 Gain | -24 .. 24 | 0 | dB |
| `eq1_q` | Band 1 Q | 0.1 .. 18 (log) | 0.71 |  |
| `eq2_on` | Band 2 On | off / on | on |  |
| `eq2_type` | Band 2 Type | Bell|Low Shelf|High Shelf|High Pass|Low Pass|Notch | Low Shelf |  |
| `eq2_freq` | Band 2 Freq | 20 .. 20000 (log) | 200 | Hz |
| `eq2_gain` | Band 2 Gain | -24 .. 24 | 0 | dB |
| `eq2_q` | Band 2 Q | 0.1 .. 18 (log) | 1 |  |
| `eq3_on` | Band 3 On | off / on | on |  |
| `eq3_type` | Band 3 Type | Bell|Low Shelf|High Shelf|High Pass|Low Pass|Notch | Bell |  |
| `eq3_freq` | Band 3 Freq | 20 .. 20000 (log) | 450 | Hz |
| `eq3_gain` | Band 3 Gain | -24 .. 24 | 0 | dB |
| `eq3_q` | Band 3 Q | 0.1 .. 18 (log) | 1 |  |
| `eq4_on` | Band 4 On | off / on | on |  |
| `eq4_type` | Band 4 Type | Bell|Low Shelf|High Shelf|High Pass|Low Pass|Notch | Bell |  |
| `eq4_freq` | Band 4 Freq | 20 .. 20000 (log) | 1000 | Hz |
| `eq4_gain` | Band 4 Gain | -24 .. 24 | 0 | dB |
| `eq4_q` | Band 4 Q | 0.1 .. 18 (log) | 1 |  |
| `eq5_on` | Band 5 On | off / on | on |  |
| `eq5_type` | Band 5 Type | Bell|Low Shelf|High Shelf|High Pass|Low Pass|Notch | Bell |  |
| `eq5_freq` | Band 5 Freq | 20 .. 20000 (log) | 3000 | Hz |
| `eq5_gain` | Band 5 Gain | -24 .. 24 | 0 | dB |
| `eq5_q` | Band 5 Q | 0.1 .. 18 (log) | 1 |  |
| `eq6_on` | Band 6 On | off / on | on |  |
| `eq6_type` | Band 6 Type | Bell|Low Shelf|High Shelf|High Pass|Low Pass|Notch | Bell |  |
| `eq6_freq` | Band 6 Freq | 20 .. 20000 (log) | 6000 | Hz |
| `eq6_gain` | Band 6 Gain | -24 .. 24 | 0 | dB |
| `eq6_q` | Band 6 Q | 0.1 .. 18 (log) | 1 |  |
| `eq7_on` | Band 7 On | off / on | on |  |
| `eq7_type` | Band 7 Type | Bell|Low Shelf|High Shelf|High Pass|Low Pass|Notch | High Shelf |  |
| `eq7_freq` | Band 7 Freq | 20 .. 20000 (log) | 10000 | Hz |
| `eq7_gain` | Band 7 Gain | -24 .. 24 | 0 | dB |
| `eq7_q` | Band 7 Q | 0.1 .. 18 (log) | 1 |  |
| `eq8_on` | Band 8 On | off / on | off |  |
| `eq8_type` | Band 8 Type | Bell|Low Shelf|High Shelf|High Pass|Low Pass|Notch | Low Pass |  |
| `eq8_freq` | Band 8 Freq | 20 .. 20000 (log) | 18000 | Hz |
| `eq8_gain` | Band 8 Gain | -24 .. 24 | 0 | dB |
| `eq8_q` | Band 8 Q | 0.1 .. 18 (log) | 0.71 |  |

## De-Esser High

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `dh_byp` | Bypass | off / on | off |  |
| `dh_freq` | Frequency | 3000 .. 12000 (log) | 7000 | Hz |
| `dh_thr` | Threshold | -60 .. 0 | -32 | dB |
| `dh_range` | Range | 0 .. 12 | 5 | dB |
| `dh_sens` | Sensitivity | 0 .. 1 | 0.5 | % |
| `dh_att` | Attack | 0.1 .. 20 (log) | 1 | ms |
| `dh_rel` | Release | 10 .. 400 (log) | 60 | ms |
| `dh_listen` | Listen | off / on | off |  |

## De-Esser Low

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `dl_byp` | Bypass | off / on | on |  |
| `dl_freq` | Frequency | 1500 .. 7000 (log) | 3800 | Hz |
| `dl_thr` | Threshold | -60 .. 0 | -32 | dB |
| `dl_range` | Range | 0 .. 12 | 5 | dB |
| `dl_sens` | Sensitivity | 0 .. 1 | 0.5 | % |
| `dl_att` | Attack | 0.1 .. 20 (log) | 1 | ms |
| `dl_rel` | Release | 10 .. 400 (log) | 60 | ms |
| `dl_listen` | Listen | off / on | off |  |

## 1176 Rev A

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `fet_byp` | Bypass | off / on | off |  |
| `fet_in` | Input | 0 .. 36 | 6 | dB |
| `fet_out` | Output | -24 .. 12 | 0 | dB |
| `fet_att` | Attack | 0 .. 1 | 0.3 |  |
| `fet_rel` | Release | 0 .. 1 | 0.5 |  |
| `fet_ratio` | Ratio | 4:1|8:1|12:1|20:1 | 4:1 |  |
| `fet_color` | Analog Color | 0 .. 1 | 0.3 | % |

## LA-2A

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `la_byp` | Bypass | off / on | off |  |
| `la_pr` | Peak Reduction | 0 .. 1 | 0.35 | % |
| `la_gain` | Gain | 0 .. 30 | 6 | dB |
| `la_mode` | Mode | Compress|Limit | Compress |  |
| `la_color` | Analog Color | 0 .. 1 | 0.3 | % |

## SSL 4000 J EQ

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `ssl_byp` | Bypass | off / on | off |  |
| `ssl_hf_freq` | HF Freq | 1500 .. 16000 (log) | 8000 | Hz |
| `ssl_hf_gain` | HF Gain | -15 .. 15 | 0 | dB |
| `ssl_hf_bell` | HF Bell | off / on | off |  |
| `ssl_hmf_freq` | HMF Freq | 600 .. 7000 (log) | 3500 | Hz |
| `ssl_hmf_gain` | HMF Gain | -15 .. 15 | 0 | dB |
| `ssl_hmf_q` | HMF Q | 0.5 .. 2.5 (log) | 1 |  |
| `ssl_lmf_freq` | LMF Freq | 200 .. 2500 (log) | 800 | Hz |
| `ssl_lmf_gain` | LMF Gain | -15 .. 15 | 0 | dB |
| `ssl_lmf_q` | LMF Q | 0.5 .. 2.5 (log) | 1 |  |
| `ssl_lf_freq` | LF Freq | 30 .. 450 (log) | 100 | Hz |
| `ssl_lf_gain` | LF Gain | -15 .. 15 | 0 | dB |
| `ssl_lf_bell` | LF Bell | off / on | off |  |
| `ssl_hpf_on` | High Pass On | off / on | off |  |
| `ssl_hpf_freq` | High Pass | 16 .. 350 (log) | 60 | Hz |
| `ssl_lpf_on` | Low Pass On | off / on | off |  |
| `ssl_lpf_freq` | Low Pass | 3000 .. 22000 (log) | 18000 | Hz |

## Maag EQ4

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `mg_byp` | Bypass | off / on | off |  |
| `mg_sub` | Sub | 0 .. 10 | 0 | dB |
| `mg_40` | 40 Hz | -10 .. 10 | 0 | dB |
| `mg_160` | 160 Hz | -10 .. 10 | 0 | dB |
| `mg_650` | 650 Hz | -10 .. 10 | 0 | dB |
| `mg_2k5` | 2.5 kHz | -10 .. 10 | 0 | dB |
| `mg_5k` | 5 kHz | -10 .. 10 | 0 | dB |
| `mg_air` | Air Band | 0 .. 10 | 0 | dB |
| `mg_airf` | Air Freq | 2.5 kHz|5 kHz|10 kHz|20 kHz|40 kHz | 10 kHz |  |
| `mg_out` | Output | -12 .. 12 | 0 | dB |

## Reverb

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `rvb_on` | On | off / on | off |  |
| `rvb_mix` | Mix | 0 .. 1 | 0.25 | % |
| `rvb_algo` | Algorithm | Room|Plate|Hall|Chamber | Plate |  |
| `rvb_cmode` | Color Mode | Neutral|Dark|Bright|Vintage | Neutral |  |
| `rvb_decay` | Decay | 0.2 .. 10 (log) | 1.8 | s |
| `rvb_pre` | Pre-Delay | 0 .. 250 | 20 | ms |
| `rvb_size` | Size | 0.5 .. 2 (log) | 1 | x |
| `rvb_damp` | Damping | 0 .. 1 | 0.5 | % |
| `rvb_hc` | High Cut | 1000 .. 20000 (log) | 12000 | Hz |
| `rvb_lc` | Low Cut | 20 .. 1000 (log) | 120 | Hz |
| `rvb_color` | Color | -1 .. 1 | 0 |  |
| `rvb_mod` | Modulation | 0 .. 1 | 0.3 | % |
| `rvb_width` | Width | 0 .. 2 | 1 | % |
| `rvb_out` | Output | -24 .. 12 | 0 | dB |

## Delay

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `dly_on` | On | off / on | off |  |
| `dly_mix` | Mix | 0 .. 1 | 0.25 | % |
| `dly_time` | Time | 1 .. 2000 (log) | 375 | ms |
| `dly_sync` | BPM Sync | off / on | on |  |
| `dly_div` | Division | 1/2|1/4|1/4D|1/4T|1/8|1/8D|1/8T|1/16 | 1/8 |  |
| `dly_fb` | Feedback | 0 .. 0.95 | 0.35 | % |
| `dly_pp` | Ping Pong | off / on | on |  |
| `dly_analog` | Analog | off / on | on |  |
| `dly_lofi` | Lo-Fi | 0 .. 1 | 0 | % |
| `dly_hp` | High Pass | 20 .. 2000 (log) | 150 | Hz |
| `dly_lp` | Low Pass | 1000 .. 20000 (log) | 6000 | Hz |
| `dly_mod` | Modulation | 0 .. 1 | 0.2 | % |
| `dly_width` | Stereo Width | 0 .. 2 | 1 | % |
| `dly_out` | Output | -24 .. 12 | 0 | dB |

## New York Compression

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `ny_on` | On | off / on | off |  |
| `ny_mix` | Mix | 0 .. 1 | 0.25 | % |
| `ny_thr` | Threshold | -60 .. 0 | -30 | dB |
| `ny_ratio` | Ratio | 2 .. 20 (log) | 10 | x |
| `ny_att` | Attack | 0.1 .. 50 (log) | 2 | ms |
| `ny_rel` | Release | 10 .. 500 (log) | 80 | ms |
| `ny_knee` | Knee | 0 .. 24 | 6 | dB |
| `ny_makeup` | Makeup Gain | 0 .. 24 | 6 | dB |
| `ny_sat` | Saturation | 0 .. 1 | 0.2 | % |
| `ny_out` | Output | -24 .. 12 | 0 | dB |

## Brightness Engine

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `br_on` | On | off / on | off |  |
| `br_mix` | Mix | 0 .. 1 | 0.25 | % |
| `br_drive` | Exciter Drive | 0 .. 1 | 0.4 | % |
| `br_harm` | Harmonic Enhancement | 0 .. 1 | 0.4 | % |
| `br_midair` | Mid Air | 0 .. 1 | 0.3 | % |
| `br_hiair` | High Air | 0 .. 1 | 0.4 | % |
| `br_eqamt` | EQ Amount | 0 .. 1 | 1 | % |
| `br_out` | Output | -24 .. 12 | 0 | dB |

## Doubler

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `dbl_on` | On | off / on | off |  |
| `dbl_mix` | Mix | 0 .. 1 | 0.25 | % |
| `dbl_width` | Width | 0 .. 2 | 1 | % |
| `dbl_detl` | Detune Left | -30 .. 30 | -8 | ct |
| `dbl_detr` | Detune Right | -30 .. 30 | 8 | ct |
| `dbl_dlyl` | Delay Left | 5 .. 50 | 17 | ms |
| `dbl_dlyr` | Delay Right | 5 .. 50 | 24 | ms |
| `dbl_mod` | Modulation | 0 .. 1 | 0.3 | % |
| `dbl_spread` | Stereo Spread | 0 .. 1 | 0.8 | % |
| `dbl_out` | Output | -24 .. 12 | 0 | dB |

## Tube Saturation

| ID | Name | Range | Default | Unit |
|---|---|---|---|---|
| `tb_on` | On | off / on | off |  |
| `tb_mix` | Mix | 0 .. 1 | 0.25 | % |
| `tb_drive` | Drive | 0 .. 1 | 0.4 | % |
| `tb_char` | Tube Character | 0 .. 1 | 0.4 | % |
| `tb_harm` | Harmonics | 0 .. 1 | 0.5 | % |
| `tb_warm` | Warmth | 0 .. 1 | 0.4 | % |
| `tb_tone` | Tone | -1 .. 1 | 0 |  |
| `tb_out` | Output | -24 .. 12 | 0 | dB |
