# Generates Source/Params.h (single source of truth for DSP, host parameters, GUI and docs)
L=[]
def ff(x): return repr(float(x))+'f'
def F(E,i,lab,lo,hi,d,mp,unit,g,p=0): L.append(f'F_({E},"{i}","{lab}",{ff(lo)},{ff(hi)},{ff(d)},{mp},"{unit}",{g},{p})')
def B(E,i,lab,d,g,p=0): L.append(f'B_({E},"{i}","{lab}",{d},{g},{p})')
def C(E,i,lab,ch,d,g,p=0): L.append(f'C_({E},"{i}","{lab}","{ch}",{d},{g},{p})')
LIN,LOG='M_LIN','M_LOG'
F('IN_TRIM','in_trim','Input',-24,24,0,LIN,'dB','G_IO',1)
F('OUT_TRIM','out_trim','Output',-24,24,0,LIN,'dB','G_IO',1)
# Autotune
g='G_AT'
B('AT_BYP','at_byp','Bypass',1,g,1)
C('AT_KEY','at_key','Key','C|C#|D|D#|E|F|F#|G|G#|A|A#|B',0,g,1)
C('AT_SCALE','at_scale','Scale','Chromatic|Major|Minor|Custom',1,g,1)
C('AT_MODE','at_mode','Mode','Natural|Hard Tune',0,g,1)
F('AT_RETUNE','at_retune','Retune Speed',0,400,30,LIN,'ms',g,1)
F('AT_HUMAN','at_human','Humanize',0,1,0.3,LIN,'%',g,1)
F('AT_FLEX','at_flex','Flex Tune',0,1,0.2,LIN,'%',g,1)
F('AT_VIB','at_vib','Natural Vibrato',0,1,0.7,LIN,'%',g)
B('AT_FCORR','at_fcorr','Formant Correction',1,g)
F('AT_FSHIFT','at_fshift','Formant Shift',-12,12,0,LIN,'st',g)
F('AT_TRANS','at_trans','Transpose',-12,12,0,LIN,'st',g)
C('AT_INPUT','at_input','Input Type','Soprano|Alto / Tenor|Low Male',1,g)
for n,nm in enumerate('C C# D D# E F F# G G# A A# B'.split()):
    B(f'AT_N{n}',f'at_n{n}',f'Custom {nm}',1 if n in (0,2,4,5,7,9,11) else 0,g)
# Rider
g='G_RD'
B('RD_BYP','rd_byp','Bypass',0,g,1)
F('RD_TARGET','rd_target','Target Level',-36,-6,-18,LIN,'dB',g,1)
F('RD_SENS','rd_sens','Sensitivity',0,1,0.5,LIN,'%',g,1)
F('RD_SPEED','rd_speed','Speed',20,1000,250,LOG,'ms',g,1)
F('RD_RANGE','rd_range','Range',0,18,9,LIN,'dB',g,1)
F('RD_UP','rd_up','Upward Gain',0,12,6,LIN,'dB',g)
F('RD_DOWN','rd_down','Downward Gain',0,12,6,LIN,'dB',g)
F('RD_MAN','rd_man','Manual Gain',-12,12,0,LIN,'dB',g,1)
# Preamp
g='G_HW'
B('HW_BYP','hw_byp','Bypass',0,g,1)
F('HW_IN','hw_in','Input',-12,12,0,LIN,'dB',g,1)
F('HW_DRIVE','hw_drive','Drive',0,1,0.25,LIN,'%',g,1)
F('HW_SAT','hw_sat','Saturation',0,1,0.3,LIN,'%',g,1)
F('HW_OUT','hw_out','Output',-12,12,0,LIN,'dB',g,1)
g='G_U47'
B('U47_BYP','u47_byp','Bypass',1,g,1)
F('U47_PROX','u47_prox','Proximity',0,1,0.3,LIN,'%',g,1)
F('U47_BODY','u47_body','Body',0,1,0.3,LIN,'%',g,1)
F('U47_PRES','u47_pres','Presence',0,1,0.3,LIN,'%',g,1)
F('U47_COLOR','u47_color','Color',0,1,0.3,LIN,'%',g,1)
F('U47_AMT','u47_amt','Intensity',0,1,1,LIN,'%',g,1)
g='G_RV'
B('RV_BYP','rv_byp','Bypass',1,g,1)
F('RV_SHIM','rv_shim','Shimmer',0,1,0.3,LIN,'%',g,1)
F('RV_THICK','rv_thick','Thickness',0,1,0.3,LIN,'%',g,1)
F('RV_AMT','rv_amt','Intensity',0,1,0.7,LIN,'%',g,1)
# EQ
g='G_EQ'
B('EQ_BYP','eq_byp','Bypass',0,g,1)
types=[3,1,0,0,0,0,2,4]; freqs=[80,200,450,1000,3000,6000,10000,18000]; ons=[1,1,1,1,1,1,1,0]
for b in range(8):
    n=b+1
    B(f'EQ{n}_ON',f'eq{n}_on',f'Band {n} On',ons[b],g)
    C(f'EQ{n}_TYPE',f'eq{n}_type',f'Band {n} Type','Bell|Low Shelf|High Shelf|High Pass|Low Pass|Notch',types[b],g)
    F(f'EQ{n}_FREQ',f'eq{n}_freq',f'Band {n} Freq',20,20000,freqs[b],LOG,'Hz',g)
    F(f'EQ{n}_GAIN',f'eq{n}_gain',f'Band {n} Gain',-24,24,0,LIN,'dB',g)
    F(f'EQ{n}_Q',f'eq{n}_q',f'Band {n} Q',0.1,18,0.71 if b in (0,7) else 1.0,LOG,'',g)
# De-essers
for pre,g,fl,fh,fd,byp in (('DH','G_DH',3000,12000,7000,0),('DL','G_DL',1500,7000,3800,1)):
    p=pre.lower()
    B(f'{pre}_BYP',f'{p}_byp','Bypass',byp,g,1)
    F(f'{pre}_FREQ',f'{p}_freq','Frequency',fl,fh,fd,LOG,'Hz',g,1)
    F(f'{pre}_THR',f'{p}_thr','Threshold',-60,0,-32,LIN,'dB',g,1)
    F(f'{pre}_RANGE',f'{p}_range','Range',0,12,5,LIN,'dB',g,1)
    F(f'{pre}_SENS',f'{p}_sens','Sensitivity',0,1,0.5,LIN,'%',g,1)
    F(f'{pre}_ATT',f'{p}_att','Attack',0.1,20,1,LOG,'ms',g)
    F(f'{pre}_REL',f'{p}_rel','Release',10,400,60,LOG,'ms',g)
    B(f'{pre}_LISTEN',f'{p}_listen','Listen',0,g,1)
# 1176
g='G_FET'
B('FET_BYP','fet_byp','Bypass',0,g,1)
F('FET_IN','fet_in','Input',0,36,6,LIN,'dB',g,1)
F('FET_OUT','fet_out','Output',-24,12,0,LIN,'dB',g,1)
F('FET_ATT','fet_att','Attack',0,1,0.3,LIN,'',g,1)
F('FET_REL','fet_rel','Release',0,1,0.5,LIN,'',g,1)
C('FET_RATIO','fet_ratio','Ratio','4:1|8:1|12:1|20:1',0,g,1)
F('FET_COLOR','fet_color','Analog Color',0,1,0.3,LIN,'%',g,1)
# LA-2A
g='G_LA'
B('LA_BYP','la_byp','Bypass',0,g,1)
F('LA_PR','la_pr','Peak Reduction',0,1,0.35,LIN,'%',g,1)
F('LA_GAIN','la_gain','Gain',0,30,6,LIN,'dB',g,1)
C('LA_MODE','la_mode','Mode','Compress|Limit',0,g,1)
F('LA_COLOR','la_color','Analog Color',0,1,0.3,LIN,'%',g,1)
# SSL
g='G_SSL'
B('SSL_BYP','ssl_byp','Bypass',0,g,1)
F('SSL_HF_FREQ','ssl_hf_freq','HF Freq',1500,16000,8000,LOG,'Hz',g,1)
F('SSL_HF_GAIN','ssl_hf_gain','HF Gain',-15,15,0,LIN,'dB',g,1)
B('SSL_HF_BELL','ssl_hf_bell','HF Bell',0,g)
F('SSL_HMF_FREQ','ssl_hmf_freq','HMF Freq',600,7000,3500,LOG,'Hz',g,1)
F('SSL_HMF_GAIN','ssl_hmf_gain','HMF Gain',-15,15,0,LIN,'dB',g,1)
F('SSL_HMF_Q','ssl_hmf_q','HMF Q',0.5,2.5,1.0,LOG,'',g)
F('SSL_LMF_FREQ','ssl_lmf_freq','LMF Freq',200,2500,800,LOG,'Hz',g,1)
F('SSL_LMF_GAIN','ssl_lmf_gain','LMF Gain',-15,15,0,LIN,'dB',g,1)
F('SSL_LMF_Q','ssl_lmf_q','LMF Q',0.5,2.5,1.0,LOG,'',g)
F('SSL_LF_FREQ','ssl_lf_freq','LF Freq',30,450,100,LOG,'Hz',g,1)
F('SSL_LF_GAIN','ssl_lf_gain','LF Gain',-15,15,0,LIN,'dB',g,1)
B('SSL_LF_BELL','ssl_lf_bell','LF Bell',0,g)
B('SSL_HPF_ON','ssl_hpf_on','High Pass On',0,g)
F('SSL_HPF_FREQ','ssl_hpf_freq','High Pass',16,350,60,LOG,'Hz',g)
B('SSL_LPF_ON','ssl_lpf_on','Low Pass On',0,g)
F('SSL_LPF_FREQ','ssl_lpf_freq','Low Pass',3000,22000,18000,LOG,'Hz',g)
# Maag
g='G_MG'
B('MG_BYP','mg_byp','Bypass',0,g,1)
F('MG_SUB','mg_sub','Sub',0,10,0,LIN,'dB',g,1)
F('MG_40','mg_40','40 Hz',-10,10,0,LIN,'dB',g,1)
F('MG_160','mg_160','160 Hz',-10,10,0,LIN,'dB',g,1)
F('MG_650','mg_650','650 Hz',-10,10,0,LIN,'dB',g,1)
F('MG_2K5','mg_2k5','2.5 kHz',-10,10,0,LIN,'dB',g,1)
F('MG_5K','mg_5k','5 kHz',-10,10,0,LIN,'dB',g,1)
F('MG_AIR','mg_air','Air Band',0,10,0,LIN,'dB',g,1)
C('MG_AIRF','mg_airf','Air Freq','2.5 kHz|5 kHz|10 kHz|20 kHz|40 kHz',2,g,1)
F('MG_OUT','mg_out','Output',-12,12,0,LIN,'dB',g,1)
# Parallel
def par(pre,g):
    B(f'{pre}_ON',f'{pre.lower()}_on','On',0,g,1)
    F(f'{pre}_MIX',f'{pre.lower()}_mix','Mix',0,1,0.25,LIN,'%',g,1)
par('RVB','G_RVB')
C('RVB_ALGO','rvb_algo','Algorithm','Room|Plate|Hall|Chamber',1,'G_RVB',1)
C('RVB_CMODE','rvb_cmode','Color Mode','Neutral|Dark|Bright|Vintage',0,'G_RVB')
F('RVB_DECAY','rvb_decay','Decay',0.2,10,1.8,LOG,'s','G_RVB',1)
F('RVB_PRE','rvb_pre','Pre-Delay',0,250,20,LIN,'ms','G_RVB',1)
F('RVB_SIZE','rvb_size','Size',0.5,2,1,LOG,'x','G_RVB')
F('RVB_DAMP','rvb_damp','Damping',0,1,0.5,LIN,'%','G_RVB',1)
F('RVB_HC','rvb_hc','High Cut',1000,20000,12000,LOG,'Hz','G_RVB')
F('RVB_LC','rvb_lc','Low Cut',20,1000,120,LOG,'Hz','G_RVB')
F('RVB_COLOR','rvb_color','Color',-1,1,0,LIN,'','G_RVB')
F('RVB_MOD','rvb_mod','Modulation',0,1,0.3,LIN,'%','G_RVB')
F('RVB_WIDTH','rvb_width','Width',0,2,1,LIN,'%','G_RVB')
F('RVB_OUT','rvb_out','Output',-24,12,0,LIN,'dB','G_RVB')
par('DLY','G_DLY')
F('DLY_TIME','dly_time','Time',1,2000,375,LOG,'ms','G_DLY',1)
B('DLY_SYNC','dly_sync','BPM Sync',1,'G_DLY',1)
C('DLY_DIV','dly_div','Division','1/2|1/4|1/4D|1/4T|1/8|1/8D|1/8T|1/16',4,'G_DLY',1)
F('DLY_FB','dly_fb','Feedback',0,0.95,0.35,LIN,'%','G_DLY',1)
B('DLY_PP','dly_pp','Ping Pong',1,'G_DLY',1)
B('DLY_ANALOG','dly_analog','Analog',1,'G_DLY')
F('DLY_LOFI','dly_lofi','Lo-Fi',0,1,0,LIN,'%','G_DLY')
F('DLY_HP','dly_hp','High Pass',20,2000,150,LOG,'Hz','G_DLY')
F('DLY_LP','dly_lp','Low Pass',1000,20000,6000,LOG,'Hz','G_DLY')
F('DLY_MOD','dly_mod','Modulation',0,1,0.2,LIN,'%','G_DLY')
F('DLY_WIDTH','dly_width','Stereo Width',0,2,1,LIN,'%','G_DLY')
F('DLY_OUT','dly_out','Output',-24,12,0,LIN,'dB','G_DLY')
par('NY','G_NY')
F('NY_THR','ny_thr','Threshold',-60,0,-30,LIN,'dB','G_NY',1)
F('NY_RATIO','ny_ratio','Ratio',2,20,10,LOG,'x','G_NY',1)
F('NY_ATT','ny_att','Attack',0.1,50,2,LOG,'ms','G_NY',1)
F('NY_REL','ny_rel','Release',10,500,80,LOG,'ms','G_NY',1)
F('NY_KNEE','ny_knee','Knee',0,24,6,LIN,'dB','G_NY')
F('NY_MAKEUP','ny_makeup','Makeup Gain',0,24,6,LIN,'dB','G_NY',1)
F('NY_SAT','ny_sat','Saturation',0,1,0.2,LIN,'%','G_NY')
F('NY_OUT','ny_out','Output',-24,12,0,LIN,'dB','G_NY')
par('BR','G_BR')
F('BR_DRIVE','br_drive','Exciter Drive',0,1,0.4,LIN,'%','G_BR',1)
F('BR_HARM','br_harm','Harmonic Enhancement',0,1,0.4,LIN,'%','G_BR',1)
F('BR_MIDAIR','br_midair','Mid Air',0,1,0.3,LIN,'%','G_BR',1)
F('BR_HIAIR','br_hiair','High Air',0,1,0.4,LIN,'%','G_BR',1)
F('BR_EQAMT','br_eqamt','EQ Amount',0,1,1,LIN,'%','G_BR',1)
F('BR_OUT','br_out','Output',-24,12,0,LIN,'dB','G_BR')
par('DBL','G_DBL')
F('DBL_WIDTH','dbl_width','Width',0,2,1,LIN,'%','G_DBL',1)
F('DBL_DETL','dbl_detl','Detune Left',-30,30,-8,LIN,'ct','G_DBL',1)
F('DBL_DETR','dbl_detr','Detune Right',-30,30,8,LIN,'ct','G_DBL',1)
F('DBL_DLYL','dbl_dlyl','Delay Left',5,50,17,LIN,'ms','G_DBL',1)
F('DBL_DLYR','dbl_dlyr','Delay Right',5,50,24,LIN,'ms','G_DBL',1)
F('DBL_MOD','dbl_mod','Modulation',0,1,0.3,LIN,'%','G_DBL')
F('DBL_SPREAD','dbl_spread','Stereo Spread',0,1,0.8,LIN,'%','G_DBL')
F('DBL_OUT','dbl_out','Output',-24,12,0,LIN,'dB','G_DBL')
par('TB','G_TB')
F('TB_DRIVE','tb_drive','Drive',0,1,0.4,LIN,'%','G_TB',1)
F('TB_CHAR','tb_char','Tube Character',0,1,0.4,LIN,'%','G_TB',1)
F('TB_HARM','tb_harm','Harmonics',0,1,0.5,LIN,'%','G_TB',1)
F('TB_WARM','tb_warm','Warmth',0,1,0.4,LIN,'%','G_TB',1)
F('TB_TONE','tb_tone','Tone',-1,1,0,LIN,'','G_TB',1)
F('TB_OUT','tb_out','Output',-24,12,0,LIN,'dB','G_TB')

groups=['G_IO','G_AT','G_RD','G_HW','G_U47','G_RV','G_EQ','G_DH','G_DL','G_FET','G_LA','G_SSL','G_MG','G_RVB','G_DLY','G_NY','G_BR','G_DBL','G_TB']
out='''// GENERATED by gen_params.py - single source of truth for DSP, host parameters, GUI and docs.
#pragma once
namespace mk
{
enum Group { %s, G_COUNT };
enum ParamKind { K_FLOAT, K_BOOL, K_CHOICE };
enum Mapping { M_LIN, M_LOG };
struct ParamSpec
{
    const char* id; const char* label; ParamKind kind; float lo, hi, def; Mapping map; const char* unit;
    Group group; bool primary; const char* choices;
};
#define MK_PARAMS(F_, B_, C_) \\
''' % ', '.join(groups)
out+=' \\\n'.join('    '+l for l in L)+'\n'
out+='''
#define MK_ENUM_F(E, id, lab, lo, hi, def, map, unit, grp, prim) E,
#define MK_ENUM_B(E, id, lab, def, grp, prim) E,
#define MK_ENUM_C(E, id, lab, ch, def, grp, prim) E,
enum ParamIndex { MK_PARAMS(MK_ENUM_F, MK_ENUM_B, MK_ENUM_C) kNumParams };
#define MK_SPEC_F(E, id, lab, lo, hi, def, map, unit, grp, prim) { id, lab, K_FLOAT, lo, hi, def, map, unit, grp, prim != 0, nullptr },
#define MK_SPEC_B(E, id, lab, def, grp, prim) { id, lab, K_BOOL, 0.0f, 1.0f, (float) (def), M_LIN, "", grp, prim != 0, nullptr },
#define MK_SPEC_C(E, id, lab, ch, def, grp, prim) { id, lab, K_CHOICE, 0.0f, 0.0f, (float) (def), M_LIN, "", grp, prim != 0, ch },
inline const ParamSpec* paramSpecs()
{
    static const ParamSpec s[] = { MK_PARAMS(MK_SPEC_F, MK_SPEC_B, MK_SPEC_C) };
    return s;
}
} // namespace mk
'''
open('/mnt/user-data/outputs/Maketeo/Source/Params.h','w').write(out)
print(len(L),'params')
