/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 *
 *  Based on WDSP lib
 *  Copyright (C) 2011 - 2017 Warren Pratt, NR0V
 */

#pragma once

typedef enum {
    AGC_OFF = 0,
    AGC_LONG,
    AGC_SLOW,
    AGC_MED,
    AGC_FAST,
    AGC_CUSTOM
} agc_mode_t;

typedef struct {
    int     mode;
    float   sample_rate;

    float   tau_attack;
    float   tau_decay;
    int     n_tau;
    float   max_gain;
    float   var_gain;
    float   fixed_gain;
    float   min_volts;
    float   max_input;
    float   out_targ;
    float   out_target;
    float   inv_max_input;
    float   slope_constant;

    float   gain;
    float   inv_out_target;

    int     out_index;
    int     in_index;
    int     attack_buffsize;

    float*  ring;
    float*  abs_ring;
    int     ring_buffsize;
    float   ring_max;

    float   attack_mult;
    float   decay_mult;
    float   volts;
    float   save_volts;
    float   out_sample;
    float   abs_out_sample;
    int     state;

    float   tau_fast_backaverage;
    float   fast_backmult;
    float   onemfast_backmult;
    float   fast_backaverage;
    float   tau_fast_decay;
    float   fast_decay_mult;
    float   pop_ratio;

    int     hang_enable;
    float   hang_backaverage;
    float   tau_hang_backmult;
    float   hang_backmult;
    float   onemhang_backmult;
    int     hang_counter;
    float   hangtime;
    float   hang_thresh;
    float   hang_level;

    float   tau_hang_decay;
    float   hang_decay_mult;
    int     decay_type;
} agc_t;

agc_t * agc_create(
    int mode,
    int sample_rate,
    float tau_attack,
    float tau_decay,
    int n_tau,
    float max_gain,
    float var_gain,
    float fixed_gain,
    float max_input,
    float out_targ,
    float tau_fast_backaverage,
    float tau_fast_decay,
    float pop_ratio,
    int hang_enable,
    float tau_hang_backmult,
    float hangtime,
    float hang_thresh,
    float tau_hang_decay
);

float agc_apply(agc_t *a, float in);
void agc_destroy(agc_t *a);
void agc_flush(agc_t *a);
void agc_set_mode(agc_t *a, int mode);

/*
void SetRXAAGCFixed (int channel, float fixed_agc);
void SetRXAAGCAttack (int channel, int attack);
void SetRXAAGCDecay (int channel, int decay);
void SetRXAAGCHang (int channel, int hang);
void GetRXAAGCHangLevel(int channel, float *hangLevel);
void SetRXAAGCHangLevel(int channel, float hangLevel);
void GetRXAAGCHangThreshold(int channel, int *hangthreshold);
void SetRXAAGCHangThreshold (int channel, int hangthreshold);
void GetRXAAGCTop(int channel, float *max_agc);
void SetRXAAGCTop (int channel, float max_agc);
void SetRXAAGCSlope (int channel, int slope);
void SetRXAAGCThresh(int channel, float thresh, float size, float rate);
void GetRXAAGCThresh(int channel, float *thresh, float size, float rate);

void SetTXAALCSt (int channel, int state);
void SetTXAALCAttack (int channel, int attack);
void SetTXAALCDecay (int channel, int decay);
void SetTXAALCHang (int channel, int hang);
void SetTXALevelerSt (int channel, int state);
void SetTXALevelerAttack (int channel, int attack);
void SetTXALevelerDecay (int channel, int decay);
void SetTXALevelerHang (int channel, int hang);
void SetTXALevelerTop (int channel, float maxgain);
*/
