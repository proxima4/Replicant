#include "Presets.h"

// ============================================================================
//  Factory sounds. Each preset is a list of  parameter=value  pairs in real
//  units (Hz, seconds, 0..1 slider positions, choice index, 0/1 switches)
//  applied on top of the INIT panel. "c*_" sets both channels.
//    feet: 0=16' 1=8' 2=5 1/3' 3=4' 4=2 2/3' 5=2'
//    sub_func: 0 sine 1 saw 2 inv saw 3 square 4 S/H 5 noise
//    trem_mode: 0 off 1 chorus 1 2 chorus 2 3 tremolo
//    dly_sync: 0 free 1 1/16 2 1/8T 3 1/8 4 1/8D 5 1/4T 6 1/4 7 1/4D 8 1/2 9 1/1
//
//  0-21 recreate the CS-80's 22 preset tone selector voices; 22-41 are new
//  sounds in the style of the instrument's famous late-70s / early-80s film
//  score work (long releases, expressive brass, bells, huge halls).
// ============================================================================
namespace rep
{
namespace
{
struct Preset { const char* name; const char* label; const char* data; };

const Preset kPresets[] =
{
    // ---------------------------------------------------------------- tone selector
    { "STRINGS 1", "STR 1",
      "c*_saw=1 c2_square=1 c2_pw=0.3 c2_pwm=0.5 c2_pwm_speed=5.3 c*_hpf=170 c*_lpf=3400 c*_lpf_res=0.08 "
      "c*_al=0.1 c*_fa=0.3 c*_fd=1.2 c*_a=0.32 c*_d=1.0 c*_s=0.9 c*_r=0.95 c*_tab=0.25 c*_tal=0.3 c*_til=0.25 "
      "detune=7 sub_vco=0.12 sub_speed=5.6 trem_mode=2 trem_speed=0.55 trem_depth=0.7 hall_mix=0.3 master=1.00" },
    { "STRINGS 2", "STR 2",
      "c*_saw=1 c2_feet=3 c2_level=0.55 c*_hpf=240 c*_lpf=4200 c*_al=0.12 c*_fa=0.4 c*_a=0.45 c*_d=1.2 c*_s=0.9 "
      "c*_r=1.2 c*_tal=0.25 detune=5 sub_vco=0.1 sub_speed=6.0 trem_mode=1 trem_speed=0.7 trem_depth=0.6 hall_mix=0.32 master=0.63" },
    { "STRINGS 3", "STR 3",
      "c*_saw=1 c1_square=1 c1_pw=0.2 c1_pwm=0.6 c1_pwm_speed=3.1 c*_hpf=120 c*_lpf=2000 c*_il=0.12 c*_al=0.05 "
      "c*_fa=1.6 c*_fd=2.0 c*_fr=2.0 c*_a=1.1 c*_d=2.0 c*_s=0.95 c*_r=2.1 c*_tab=0.3 aft_vib=0.4 detune=9 "
      "trem_mode=2 trem_speed=0.4 trem_depth=0.8 hall_mix=0.38 hall_decay=6 master=0.77" },
    { "STRINGS 4", "STR 4",
      "c*_saw=1 c*_hpf=300 c*_lpf=1700 c*_lpf_res=0.2 c*_il=0.35 c*_al=0.25 c*_fa=0.5 c*_fd=1.0 c*_fr=0.9 c*_a=0.2 "
      "c*_d=0.9 c*_s=0.8 c*_r=0.8 c*_tib=0.3 c*_tab=0.35 detune=12 c2_feet=0 c2_level=0.5 trem_mode=1 trem_speed=0.9 hall_mix=0.3 master=1.00" },
    { "BRASS 1", "BRS 1",
      "c*_saw=1 c*_lpf=650 c*_lpf_res=0.25 c*_il=0.35 c*_al=0.45 c*_fa=0.12 c*_fd=0.65 c*_fr=0.3 c*_a=0.04 c*_d=0.8 "
      "c*_s=0.82 c*_r=0.35 c*_tib=0.35 c*_tab=0.4 c*_til=0.35 c*_tal=0.25 detune=9 init_bend=0.35 bend_time=0.12 hall_mix=0.24 master=0.57" },
    { "BRASS 2", "BRS 2",
      "c1_saw=1 c2_saw=0 c2_square=1 c2_pw=0.2 c2_feet=0 c2_level=0.6 c*_lpf=900 c*_lpf_res=0.18 c*_il=0.3 c*_al=0.55 "
      "c*_fa=0.09 c*_fd=0.5 c*_fr=0.25 c*_a=0.03 c*_s=0.85 c*_r=0.3 c*_tib=0.4 c*_tab=0.3 detune=6 init_bend=0.25 hall_mix=0.22 master=0.46" },
    { "BRASS 3", "BRS 3",
      "c*_saw=1 c*_lpf=480 c*_lpf_res=0.12 c*_il=0.25 c*_al=0.3 c*_fa=0.28 c*_fd=1.1 c*_fr=0.5 c*_a=0.09 c*_d=1.2 "
      "c*_s=0.85 c*_r=0.55 c*_tib=0.25 c*_tab=0.45 c*_tal=0.3 detune=8 sub_vco=0.08 hall_mix=0.3 master=0.61" },
    { "ORGAN 1", "ORG 1",
      "c*_saw=0 c*_square=1 c2_feet=3 c1_sine=0.4 c*_lpf=6500 c*_al=0 c*_a=0.003 c*_d=0.1 c*_s=1 c*_r=0.06 c*_til=0 "
      "sub_vco=0.13 sub_speed=6.4 trem_mode=3 trem_speed=6.5 trem_depth=0.22 hall_mix=0.22 master=0.25" },
    { "ORGAN 2", "ORG 2",
      "c*_saw=0 c*_square=1 c1_feet=0 c2_feet=3 c*_sine=0.55 c*_vcf_level=0.45 c*_lpf=3000 c*_al=0 c*_a=0.004 c*_s=1 "
      "c*_r=0.08 c*_til=0 trem_mode=1 trem_speed=5.8 trem_depth=0.5 hall_mix=0.25 master=0.32" },
    { "ELECTRIC PIANO", "E.PNO",
      "c*_saw=0 c*_square=1 c1_sine=0.8 c1_vcf_level=0.3 c2_feet=3 c2_sine=0.4 c2_vcf_level=0.2 c*_lpf=1400 c*_al=0.5 "
      "c*_fd=0.8 c*_fr=0.4 c*_a=0.002 c*_d=2.6 c*_s=0 c*_r=0.6 c*_til=0.6 c*_tib=0.3 trem_mode=3 trem_speed=4.6 trem_depth=0.35 hall_mix=0.2 master=0.62" },
    { "GUITAR", "GTR",
      "c*_saw=1 c*_lpf=850 c*_lpf_res=0.35 c*_al=0.7 c*_fd=0.35 c*_fr=0.2 c*_a=0.002 c*_d=1.4 c*_s=0.12 c*_r=0.3 "
      "c*_tib=0.4 c*_til=0.5 detune=4 c2_level=0.6 hall_mix=0.2 master=0.67" },
    { "GUITAR 2", "GTR 2",
      "c*_saw=0 c*_square=1 c*_pw=0.4 c*_hpf=200 c*_lpf=1100 c*_lpf_res=0.3 c*_al=0.6 c*_fd=0.25 c*_a=0.002 c*_d=1.0 "
      "c*_s=0.08 c*_r=0.25 c*_tib=0.45 c*_til=0.5 detune=5 trem_mode=1 hall_mix=0.2 master=0.92" },
    { "HARPSICHORD 1", "HPS 1",
      "c1_saw=1 c2_saw=0 c2_square=1 c2_feet=3 c2_level=0.6 c*_hpf=300 c*_lpf=1800 c*_lpf_res=0.2 c*_al=0.8 c*_fd=0.25 "
      "c*_fr=0.15 c*_a=0.001 c*_d=1.2 c*_s=0 c*_r=0.25 c*_til=0.15 hall_mix=0.25 master=1.00" },
    { "HARPSICHORD 2", "HPS 2",
      "c*_saw=0 c*_square=1 c*_pw=0.8 c2_feet=3 c*_hpf=420 c*_lpf=2600 c*_lpf_res=0.15 c*_al=0.7 c*_fd=0.3 c*_a=0.001 "
      "c*_d=1.5 c*_s=0 c*_r=0.3 c*_til=0.1 detune=4 hall_mix=0.25 master=0.65" },
    { "CLAVICHORD 1", "CLV 1",
      "c*_saw=0 c*_square=1 c*_pw=0.7 c*_hpf=500 c*_hpf_res=0.3 c*_lpf=1600 c*_al=0.6 c*_fd=0.2 c*_fr=0.1 c*_a=0.001 "
      "c*_d=0.7 c*_s=0 c*_r=0.15 c*_tib=0.4 c*_til=0.6 c2_level=0.4 hall_mix=0.12 master=1.00" },
    { "CLAVICHORD 2", "CLV 2",
      "c*_saw=0 c*_square=1 c*_pw=0.55 c*_hpf=380 c*_lpf=700 c*_lpf_res=0.55 c*_al=0.9 c*_fd=0.18 c*_fr=0.1 c*_a=0.001 "
      "c*_d=0.6 c*_s=0.1 c*_r=0.12 c*_tib=0.5 c*_til=0.6 detune=3 hall_mix=0.1 master=1.00" },
    { "FUNKY 1", "FNK 1",
      "c*_saw=1 c*_lpf=320 c*_lpf_res=0.7 c*_al=0.8 c*_fd=0.3 c*_fr=0.2 c*_a=0.003 c*_d=0.6 c*_s=0.6 c*_r=0.2 "
      "c*_tib=0.4 c2_level=0.5 detune=5 hall_mix=0.12 master=0.57" },
    { "FUNKY 2", "FNK 2",
      "c*_saw=1 c*_lpf=2200 c*_lpf_res=0.62 c*_il=0.6 c*_al=0 c*_fa=0.25 c*_fr=0.2 c*_a=0.005 c*_d=0.5 c*_s=0.8 "
      "c*_r=0.2 c*_tab=0.3 c2_square=1 c2_saw=0 c2_level=0.5 hall_mix=0.12 master=0.44" },
    { "FUNKY 3", "FNK 3",
      "c*_saw=1 c*_lpf=700 c*_lpf_res=0.6 c*_al=0.15 c*_a=0.004 c*_s=0.9 c*_r=0.2 sub_func=3 sub_speed=4.2 sub_vcf=0.55 "
      "c2_level=0.55 detune=4 hall_mix=0.14 master=0.45" },
    { "FUNKY 4", "FNK 4",
      "c*_saw=0 c*_square=1 c*_pw=0.3 c*_lpf=1400 c*_lpf_res=0.35 c*_al=0.5 c*_fd=0.3 c*_a=0.002 c*_d=0.5 c*_s=0.2 c*_r=0.2 "
      "ring_mod=0.6 ring_speed=180 ring_depth=0.5 ring_a=0.002 ring_d=0.4 hall_mix=0.14 master=0.52" },
    { "FLUTE", "FLT",
      "c1_saw=1 c1_vcf_level=0.25 c1_sine=0.75 c1_lpf=1300 c2_saw=0 c2_noise=0.35 c2_hpf=1500 c2_lpf=6000 c2_level=0.35 "
      "c*_al=0.15 c*_fa=0.08 c*_a=0.07 c*_d=0.5 c*_s=0.9 c*_r=0.25 c*_tab=0.3 sub_vco=0.1 sub_speed=5.2 aft_vib=0.4 hall_mix=0.28 master=0.82" },
    { "BASS", "BASS",
      "c1_feet=0 c*_saw=1 c2_saw=0 c2_square=1 c*_lpf=380 c*_lpf_res=0.3 c*_al=0.55 c*_fd=0.3 c*_fr=0.1 c*_a=0.002 "
      "c*_d=0.5 c*_s=0.7 c*_r=0.12 c*_tib=0.3 c*_til=0.4 kb_track=0.3 hall_mix=0.06 master=0.48" },

    // ---------------------------------------------------------------- film-score era
    { "RAIN-SOAKED BRASS", nullptr,
      "c*_saw=1 c*_lpf=600 c*_lpf_res=0.22 c*_il=0.4 c*_al=0.42 c*_fa=0.18 c*_fd=1.6 c*_fr=1.1 c*_a=0.06 c*_d=1.5 "
      "c*_s=0.85 c*_r=1.6 c*_tib=0.35 c*_tab=0.6 c*_til=0.3 c*_tal=0.35 aft_vib=0.35 sub_speed=5.4 detune=11 "
      "init_bend=0.5 bend_time=0.18 hall_mix=0.4 hall_decay=7.5 hall_size=0.85 master=0.48" },
    { "NEON HAZE PAD", nullptr,
      "c*_saw=1 c2_saw=0 c2_square=1 c2_pw=0.25 c2_pwm=0.6 c2_pwm_speed=2.4 c*_lpf=1400 c*_il=0.2 c*_al=0.1 c*_fa=2.0 "
      "c*_fd=3.0 c*_fr=3.0 c*_a=1.6 c*_d=3.0 c*_s=0.9 c*_r=3.5 c*_tab=0.35 aft_vib=0.3 detune=8 trem_mode=2 "
      "trem_speed=0.35 trem_depth=0.8 hall_mix=0.45 hall_decay=9 master=0.52" },
    { "SPINNER LIFT-OFF", nullptr,
      "c*_saw=1 c2_noise=0.35 c*_lpf=900 c*_lpf_res=0.35 c*_il=0.6 c*_al=0.6 c*_fa=3.0 c*_fd=4.0 c*_fr=3.0 c*_a=1.4 "
      "c*_d=4 c*_s=0.8 c*_r=3.0 glide_time=0.8 detune=14 sub_vco=0.06 dly_on=1 dly_sync=7 dly_fb=0.4 dly_mix=0.2 "
      "hall_mix=0.45 hall_decay=8 master=0.48" },
    { "SODIUM LIGHTS", nullptr,
      "c*_saw=0 c1_sine=0.8 c1_vcf_level=0.2 c1_square=1 c2_feet=4 c2_sine=0.5 c2_vcf_level=0 c*_lpf=2400 c*_al=0.4 "
      "c*_fd=0.6 c*_a=0.002 c*_d=3.2 c*_s=0 c*_r=3.0 c*_til=0.5 ring_mod=0.35 ring_speed=520 dly_on=1 dly_sync=6 "
      "dly_fb=0.35 dly_mix=0.22 hall_mix=0.5 hall_decay=8 master=0.82" },
    { "PYRAMID DAWN", nullptr,
      "c*_saw=1 c*_lpf=520 c*_lpf_res=0.15 c*_il=0.5 c*_al=0.2 c*_fa=2.5 c*_fd=3.0 c*_fr=2.0 c*_a=1.2 c*_d=3.0 "
      "c*_s=0.9 c*_r=2.5 c*_tab=0.5 c*_tal=0.3 detune=10 c2_feet=0 c2_level=0.55 sub_vco=0.07 hall_mix=0.45 hall_decay=8 master=0.77" },
    { "OFF-WORLD CHOIR", nullptr,
      "c*_saw=0 c*_square=1 c*_pw=0.15 c*_pwm=0.55 c1_pwm_speed=4.4 c2_pwm_speed=5.9 c*_hpf=350 c*_hpf_res=0.45 "
      "c*_lpf=1800 c*_lpf_res=0.35 c*_al=0.05 c*_a=0.6 c*_d=2.0 c*_s=0.95 c*_r=2.2 c*_tab=0.25 sub_vco=0.1 "
      "detune=6 trem_mode=2 trem_speed=0.5 hall_mix=0.42 hall_decay=7 master=0.39" },
    { "ENHANCE 224", nullptr,
      "c*_saw=0 c*_square=1 c*_pw=0.2 c*_lpf=3200 c*_al=0.3 c*_fd=0.2 c*_a=0.001 c*_d=0.25 c*_s=0 c*_r=0.2 "
      "ring_mod=0.8 ring_speed=900 ring_depth=0.4 ring_d=0.2 dly_on=1 dly_sync=4 dly_fb=0.45 dly_mix=0.35 hall_mix=0.3 master=0.45" },
    { "MEMORY IMPLANT", nullptr,
      "c1_saw=1 c1_vcf_level=0.4 c1_sine=0.6 c1_lpf=900 c2_saw=0 c2_square=1 c2_feet=3 c2_level=0.35 c*_al=0.4 c*_fd=1.2 "
      "c*_a=0.003 c*_d=2.5 c*_s=0.2 c*_r=1.4 c*_til=0.5 trem_mode=3 trem_speed=3.5 trem_depth=0.2 dly_on=1 "
      "dly_sync=6 dly_mix=0.2 hall_mix=0.38 master=1.00" },
    { "HOVERCAR BELLS", nullptr,
      "c*_saw=0 c1_square=1 c1_pw=0.6 c1_lpf=2000 c1_al=0.7 c1_fd=0.5 c2_feet=5 c2_sine=0.6 c2_vcf_level=0.1 c*_a=0.001 "
      "c*_d=2.8 c*_s=0 c*_r=2.6 c*_til=0.4 detune=-6 hall_mix=0.5 hall_decay=9 master=0.67" },
    { "SMOG BASS", nullptr,
      "c*_feet=0 c1_saw=1 c2_saw=0 c2_square=1 c2_pw=0 c*_lpf=260 c*_lpf_res=0.5 c*_al=0.4 c*_fd=0.5 c*_fr=0.2 c*_a=0.003 "
      "c*_d=0.8 c*_s=0.75 c*_r=0.25 c*_tib=0.35 c*_tab=0.3 glide_time=0.12 detune=7 kb_track=0.25 hall_mix=0.1 master=0.42" },
    { "CATHEDRAL OF GLASS", nullptr,
      "c*_saw=0 c*_square=0 c1_feet=3 c1_sine=0.6 c1_vcf_level=0 c2_feet=5 c2_square=1 c2_vcf_level=0.25 c2_lpf=4000 "
      "c*_a=0.25 c*_d=2 c*_s=0.9 c*_r=3 trem_mode=1 trem_speed=0.6 hall_mix=0.55 hall_decay=12 hall_size=1 master=0.84" },
    { "PAPER UNICORN", nullptr,
      "c1_saw=0 c1_square=1 c1_pw=0.35 c1_vcf_level=0.35 c1_sine=0.7 c1_lpf=1500 c2_saw=0 c2_noise=0.3 c2_hpf=1800 "
      "c2_level=0.3 c*_al=0.2 c*_fa=0.06 c*_a=0.05 c*_s=0.9 c*_r=0.6 glide_time=0.18 aft_vib=0.5 sub_speed=5.3 "
      "c*_tab=0.3 dly_on=1 dly_sync=7 dly_mix=0.22 hall_mix=0.42 hall_decay=7 master=0.41" },
    { "MONOLOGUE", nullptr,
      "c*_saw=1 c2_square=1 c2_pwm=0.4 c*_hpf=150 c*_lpf=1900 c*_il=0.15 c*_al=0.08 c*_fa=1.2 c*_fd=2.4 c*_a=0.9 "
      "c*_d=2.5 c*_s=0.85 c*_r=4.5 c*_tab=0.35 aft_vib=0.3 detune=7 trem_mode=2 trem_speed=0.45 hall_mix=0.5 hall_decay=11 master=0.87" },
    { "C-BEAM SHIMMER", nullptr,
      "c*_saw=1 c2_noise=0.25 c*_lpf=1600 c*_lpf_res=0.62 c*_a=0.5 c*_s=0.9 c*_r=2.5 sub_func=4 sub_speed=7 sub_vcf=0.4 "
      "c2_feet=3 dly_on=1 dly_sync=3 dly_fb=0.5 dly_mix=0.3 hall_mix=0.45 hall_decay=9 master=0.36" },
    { "EMPATHY TEST", nullptr,
      "c*_saw=1 c2_feet=0 c*_lpf=800 c*_lpf_res=0.3 c*_al=0.2 c*_a=0.01 c*_s=0.9 c*_r=0.8 sub_func=3 sub_speed=3.0 "
      "sub_vca=0.85 sub_vcf=0.15 detune=8 hall_mix=0.35 master=0.46" },
    { "NEXUS LEAD", nullptr,
      "voices=1 c*_saw=1 c2_square=1 c2_pw=0.3 c*_lpf=1100 c*_lpf_res=0.3 c*_il=0.25 c*_al=0.45 c*_fa=0.06 c*_fd=0.6 "
      "c*_a=0.01 c*_s=0.9 c*_r=0.5 c*_tib=0.3 c*_tab=0.5 aft_vib=0.55 sub_speed=5.6 glide_time=0.07 init_bend=0.3 "
      "detune=9 dly_on=1 dly_sync=7 dly_mix=0.18 hall_mix=0.35 master=0.95" },
    { "DRIFTING CITY", nullptr,
      "age=1 c*_saw=1 c2_square=1 c2_pwm=0.5 c*_lpf=1300 c*_il=0.1 c*_al=0.15 c*_fa=1.5 c*_fd=3 c*_a=1.0 c*_d=3 c*_s=0.9 "
      "c*_r=3 sub_vco=0.05 sub_speed=0.6 detune=16 spread=0.9 trem_mode=2 trem_speed=0.3 hall_mix=0.48 hall_decay=10 master=0.79" },
    { "UNDERCITY THUNDER", nullptr,
      "c1_feet=0 c1_saw=1 c1_lpf=180 c1_lpf_res=0.55 c2_saw=0 c2_noise=0.9 c2_lpf=240 c2_lpf_res=0.7 c*_al=0.25 c*_fa=1.5 "
      "c*_fd=3 c*_a=1.8 c*_d=3 c*_s=0.9 c*_r=4 sub_speed=0.18 sub_vcf=0.35 hall_mix=0.4 hall_decay=10 master=0.75" },
    { "NEON PLUCK", nullptr,
      "c*_saw=1 c2_saw=0 c2_square=1 c2_feet=3 c2_level=0.5 c*_lpf=700 c*_lpf_res=0.35 c*_al=0.8 c*_fd=0.2 c*_a=0.001 "
      "c*_d=0.45 c*_s=0 c*_r=0.35 c*_tib=0.4 c*_til=0.5 dly_on=1 dly_sync=3 dly_fb=0.5 dly_mix=0.3 hall_mix=0.3 master=0.80" },
    { "INIT PANEL", nullptr, " master=0.46" },
};
constexpr int kNum = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
} // namespace

int numFactoryPresets() { return kNum; }

juce::String factoryPresetName (int i)
{
    return i >= 0 && i < kNum ? juce::String (kPresets[i].name) : juce::String();
}

juce::String toneButtonLabel (int i)
{
    return i >= 0 && i < kNum && kPresets[i].label != nullptr ? juce::String (kPresets[i].label) : juce::String();
}

void applyFactoryPreset (int index, juce::AudioProcessorValueTreeState& apvts)
{
    if (index < 0 || index >= kNum) return;

    auto set = [&] (const juce::String& pid, float v)
    {
        if (auto* p = apvts.getParameter (pid))
            p->setValueNotifyingHost (p->convertTo0to1 (v));
        else
            jassertfalse;   // typo in a preset
    };

    // start from the INIT panel
    for (auto* prm : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    auto tokens = juce::StringArray::fromTokens (kPresets[index].data, " ", "");
    for (auto& t : tokens)
    {
        if (t.isEmpty()) continue;
        const auto key = t.upToFirstOccurrenceOf ("=", false, false);
        const float v = t.fromFirstOccurrenceOf ("=", false, false).getFloatValue();
        if (key.startsWith ("c*_"))
        {
            set ("c1_" + key.substring (3), v);
            set ("c2_" + key.substring (3), v);
        }
        else set (key, v);
    }
}
} // namespace rep
