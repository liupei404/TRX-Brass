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

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "agc.h"

#define MAX_SAMPLE_RATE     (32000.0)
#define MAX_N_TAU           (8)
#define MAX_TAU_ATTACK      (0.01)
#define RB_SIZE	            (int)(MAX_SAMPLE_RATE * MAX_N_TAU * MAX_TAU_ATTACK + 1)

static void agc_load(agc_t *a);

void agc_calc(agc_t *a) {
    a->ring_buffsize = RB_SIZE;
    a->in_index = 0;
    a->out_index = 1;
    a->ring_max = 0.0;
    a->volts = 0.0;
    a->save_volts = 0.0;
    a->fast_backaverage = 0.0;
    a->hang_backaverage = 0.0;
    a->hang_counter = 0;
    a->decay_type = 0;
    a->state = 0;
    a->ring = (float *) calloc(RB_SIZE, sizeof(float));
    a->abs_ring = (float *) calloc(RB_SIZE, sizeof(float));

    agc_load(a);
}

void agc_decalc(agc_t *a) {
    free(a->abs_ring);
    free(a->ring);
}

agc_t * agc_create(
    int mode, int sample_rate, float tau_attack, float tau_decay, int n_tau,
    float max_gain, float var_gain, float fixed_gain, float max_input, float out_targ, float tau_fast_backaverage, float tau_fast_decay,
    float pop_ratio, int hang_enable, float tau_hang_backmult, float hangtime, float hang_thresh, float tau_hang_decay) {

    agc_t *a = (agc_t *) malloc(sizeof(agc_t));

    a->mode = mode;
    a->sample_rate = (float)sample_rate;
    a->tau_attack = tau_attack;
    a->tau_decay = tau_decay;
    a->n_tau = n_tau;
    a->max_gain = max_gain;
    a->var_gain = var_gain;
    a->fixed_gain = fixed_gain;
    a->max_input = max_input;
    a->out_targ = out_targ;
    a->tau_fast_backaverage = tau_fast_backaverage;
    a->tau_fast_decay = tau_fast_decay;
    a->pop_ratio = pop_ratio;
    a->hang_enable = hang_enable;
    a->tau_hang_backmult = tau_hang_backmult;
    a->hangtime = hangtime;
    a->hang_thresh = hang_thresh;
    a->tau_hang_decay = tau_hang_decay;
    agc_calc(a);

    return a;
}

static void agc_load(agc_t *a) {
    float tmp;

    a->attack_buffsize = (int) ceil(a->sample_rate * a->n_tau * a->tau_attack);
    a->in_index = a->attack_buffsize + a->out_index;
    a->attack_mult = 1.0f - expf(-1.0f / (a->sample_rate * a->tau_attack));
    a->decay_mult = 1.0f - expf(-1.0f / (a->sample_rate * a->tau_decay));
    a->fast_decay_mult = 1.0f - expf(-1.0f / (a->sample_rate * a->tau_fast_decay));
    a->fast_backmult = 1.0f - expf(-1.0f / (a->sample_rate * a->tau_fast_backaverage));
    a->onemfast_backmult = 1.0f - a->fast_backmult;

    a->out_target = a->out_targ * (1.0f - expf(-(float)a->n_tau)) * 0.9999f;
    a->min_volts = a->out_target / (a->var_gain * a->max_gain);
    a->inv_out_target = 1.0f / a->out_target;

    tmp = log10f(a->out_target / (a->max_input * a->var_gain * a->max_gain));

    if (tmp == 0.0) {
        tmp = 1e-16;
    }

    a->slope_constant = (a->out_target * (1.0f - 1.0f / a->var_gain)) / tmp;

    a->inv_max_input = 1.0f / a->max_input;

    tmp = powf(10.0f, (a->hang_thresh - 1.0f) / 0.125f);
    a->hang_level = (a->max_input * tmp + (a->out_target / (a->var_gain * a->max_gain)) * (1.0f - tmp)) * 0.637f;

    a->hang_backmult = 1.0f - expf(-1.0f / (a->sample_rate * a->tau_hang_backmult));
    a->onemhang_backmult = 1.0f - a->hang_backmult;

    a->hang_decay_mult = 1.0f - expf(-1.0f / (a->sample_rate * a->tau_hang_decay));
}

void agc_destroy(agc_t *a) {
    agc_decalc(a);
    free(a);
}

void agc_flush(agc_t *a) {
    memset(a->ring, 0, sizeof(float) * a->ring_buffsize);
    memset(a->abs_ring, 0, sizeof(float) * a->ring_buffsize);
    a->out_index = 1;
    a->in_index = a->attack_buffsize + a->out_index;
    a->ring_max = a->volts = a->save_volts = 0;
    a->fast_backaverage = a->hang_backaverage = 0;
    a->hang_counter = a->decay_type = a->state = 0;
}

float agc_apply(agc_t *a, float in) {
    int i, j, k;
    float mult;

    if (a->mode == 0) {
        return a->fixed_gain * in;
    }

    a->out_index = (a->out_index + 1) % a->ring_buffsize;
    a->in_index = (a->in_index + 1) % a->ring_buffsize;

    a->out_sample = a->ring[a->out_index];
    a->abs_out_sample = a->abs_ring[a->out_index];

    a->ring[a->in_index] = in;
    a->abs_ring[a->in_index] = fabs(in);

    a->fast_backaverage = a->fast_backmult * a->abs_out_sample + a->onemfast_backmult * a->fast_backaverage;
    a->hang_backaverage = a->hang_backmult * a->abs_out_sample + a->onemhang_backmult * a->hang_backaverage;

    if ((a->abs_out_sample >= a->ring_max) && (a->abs_out_sample > 0.0f)) {
        a->ring_max = 0.0f;
        k = a->out_index;

        for (j = 0; j < a->attack_buffsize; j++) {
            if (++k == a->ring_buffsize) {
                k = 0;
            }

            if (a->abs_ring[k] > a->ring_max) {
                a->ring_max = a->abs_ring[k];
            }
        }
    }

    if (a->abs_ring[a->in_index] > a->ring_max) {
        a->ring_max = a->abs_ring[a->in_index];
    }

    if (a->hang_counter > 0) {
        --a->hang_counter;
    }

    switch (a->state) {
        case 0:
            if (a->ring_max >= a->volts) {
                a->volts += (a->ring_max - a->volts) * a->attack_mult;
            } else {
                if (a->volts > a->pop_ratio * a->fast_backaverage) {
                    a->state = 1;
                    a->volts += (a->ring_max - a->volts) * a->fast_decay_mult;
                } else {
                    if (a->hang_enable && (a->hang_backaverage > a->hang_level)) {
                        a->state = 2;
                        a->hang_counter = (int)(a->hangtime * a->sample_rate);
                        a->decay_type = 1;
                    } else {
                        a->state = 3;
                        a->volts += (a->ring_max - a->volts) * a->decay_mult;
                        a->decay_type = 0;
                    }
                }
            }
            break;

        case 1:
            if (a->ring_max >= a->volts) {
                a->state = 0;
                a->volts += (a->ring_max - a->volts) * a->attack_mult;
            } else {
                if (a->volts > a->save_volts) {
                    a->volts += (a->ring_max - a->volts) * a->fast_decay_mult;
                } else {
                    if (a->hang_counter > 0) {
                        a->state = 2;
                    } else {
                        if (a->decay_type == 0) {
                            a->state = 3;
                            a->volts += (a->ring_max - a->volts) * a->decay_mult;
                        } else {
                            a->state = 4;
                            a->volts += (a->ring_max - a->volts) * a->hang_decay_mult;
                        }
                    }
                }
            }
            break;

        case 2:
            if (a->ring_max >= a->volts) {
                a->state = 0;
                a->save_volts = a->volts;
                a->volts += (a->ring_max - a->volts) * a->attack_mult;
            } else {
                if (a->hang_counter == 0) {
                    a->state = 4;
                    a->volts += (a->ring_max - a->volts) * a->hang_decay_mult;
                }
            }
            break;

        case 3:
            if (a->ring_max >= a->volts) {
                a->state = 0;
                a->save_volts = a->volts;
                a->volts += (a->ring_max - a->volts) * a->attack_mult;
            } else {
                a->volts += (a->ring_max - a->volts) * a->decay_mult;
            }
            break;

        case 4:
            if (a->ring_max >= a->volts) {
                a->state = 0;
                a->save_volts = a->volts;
                a->volts += (a->ring_max - a->volts) * a->attack_mult;
            } else {
                a->volts += (a->ring_max - a->volts) * a->hang_decay_mult;
            }
            break;
    }

    if (a->volts < a->min_volts) {
        a->volts = a->min_volts;
    }

    a->gain = a->volts * a->inv_out_target;
    mult = (a->out_target - a->slope_constant * fmin(0.0f, log10f(a->inv_max_input * a->volts))) / a->volts;

    return a->out_sample * mult;
}

void agc_set_mode(agc_t *a, int mode) {
    switch (mode) {
        case AGC_OFF:
            a->mode = 0;
            agc_load(a);
            break;

        case AGC_LONG:
            a->mode = 1;
            a->hangtime = 2.000;
            a->tau_decay = 2.000;
            agc_load(a);
            break;

        case AGC_SLOW:
            a->mode = 2;
            a->hangtime = 1.000;
            a->tau_decay = 0.500;
            agc_load(a);
            break;

        case AGC_MED:
            a->mode = 3;
            a->hang_thresh = 1.0;
            a->hangtime = 0.000;
            a->tau_decay = 0.250;
            agc_load(a);
        break;

        case AGC_FAST:
            a->mode = 4;
            a->hang_thresh = 1.0;
            a->hangtime = 0.000;
            a->tau_decay = 0.050;
            agc_load(a);
            break;

        case AGC_CUSTOM:
            a->mode = 5;
            break;
    }
}

#if 0
PORT void
SetRXAAGCAttack (int channel, int attack)
{
	EnterCriticalSection (&ch[channel].csDSP);
	rxa[channel].agc.p->tau_attack = (float)attack / 1000.0;
	loadWcpAGC ( rxa[channel].agc.p );
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetRXAAGCDecay (int channel, int decay)
{
	EnterCriticalSection (&ch[channel].csDSP);
	rxa[channel].agc.p->tau_decay = (float)decay / 1000.0;
	loadWcpAGC ( rxa[channel].agc.p );
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetRXAAGCHang (int channel, int hang)
{
	EnterCriticalSection (&ch[channel].csDSP);
	rxa[channel].agc.p->hangtime = (float)hang / 1000.0;
	loadWcpAGC ( rxa[channel].agc.p );
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void				
GetRXAAGCHangLevel(int channel, float *hangLevel)
//for line on bandscope
{
	EnterCriticalSection (&ch[channel].csDSP);
	*hangLevel = 20.0 * log10( rxa[channel].agc.p->hang_level / 0.637 );
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void			
SetRXAAGCHangLevel(int channel, float hangLevel)
//for line on bandscope
{
	float convert, tmp;
	EnterCriticalSection (&ch[channel].csDSP);
	if (rxa[channel].agc.p->max_input > rxa[channel].agc.p->min_volts)
	{
		convert = pow (10.0, hangLevel / 20.0);
		tmp = max(1e-8, (convert - rxa[channel].agc.p->min_volts) / 
			(rxa[channel].agc.p->max_input - rxa[channel].agc.p->min_volts));
		rxa[channel].agc.p->hang_thresh = 1.0 + 0.125 * log10 (tmp);
	}
	else
		rxa[channel].agc.p->hang_thresh = 1.0;
	loadWcpAGC ( rxa[channel].agc.p );
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void				
GetRXAAGCHangThreshold(int channel, int *hangthreshold)
//for slider in setup
{
	EnterCriticalSection (&ch[channel].csDSP);
	*hangthreshold = (int)(100.0 * rxa[channel].agc.p->hang_thresh);
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetRXAAGCHangThreshold (int channel, int hangthreshold)
//For slider in setup
{
	EnterCriticalSection (&ch[channel].csDSP);
	rxa[channel].agc.p->hang_thresh = (float)hangthreshold / 100.0;
	loadWcpAGC ( rxa[channel].agc.p );
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void				
GetRXAAGCThresh(int channel, float *thresh, float size, float rate)
//for line on bandscope.
{
	float noise_offset;
	EnterCriticalSection (&ch[channel].csDSP);
	noise_offset = 10.0 * log10((rxa[channel].nbp0.p->fhigh - rxa[channel].nbp0.p->flow) 
		* size / rate);
	*thresh = 20.0 * log10( rxa[channel].agc.p->min_volts ) - noise_offset;
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void				
SetRXAAGCThresh(int channel, float thresh, float size, float rate)
//for line on bandscope
{
	float noise_offset;
	EnterCriticalSection (&ch[channel].csDSP);
	noise_offset = 10.0 * log10((rxa[channel].nbp0.p->fhigh - rxa[channel].nbp0.p->flow) 
		* size / rate);
	rxa[channel].agc.p->max_gain = rxa[channel].agc.p->out_target / 
		(rxa[channel].agc.p->var_gain * pow (10.0, (thresh + noise_offset) / 20.0));
	loadWcpAGC ( rxa[channel].agc.p );
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void			
GetRXAAGCTop(int channel, float *max_agc)
//for AGC Max Gain in setup
{
	EnterCriticalSection (&ch[channel].csDSP);
	*max_agc = 20 * log10 (rxa[channel].agc.p->max_gain);
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetRXAAGCTop (int channel, float max_agc)
//for AGC Max Gain in setup
{
	EnterCriticalSection (&ch[channel].csDSP);
	rxa[channel].agc.p->max_gain = pow (10.0, (float)max_agc / 20.0);
	loadWcpAGC ( rxa[channel].agc.p );
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetRXAAGCSlope (int channel, int slope)
{
	EnterCriticalSection (&ch[channel].csDSP);
	rxa[channel].agc.p->var_gain = pow (10.0, (float)slope / 20.0 / 10.0);
	loadWcpAGC ( rxa[channel].agc.p );
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetRXAAGCFixed (int channel, float fixed_agc)
{
	EnterCriticalSection (&ch[channel].csDSP);
	rxa[channel].agc.p->fixed_gain = pow (10.0, (float)fixed_agc / 20.0);
	loadWcpAGC ( rxa[channel].agc.p );
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetRXAAGCMaxInputLevel (int channel, float level)
{
	EnterCriticalSection (&ch[channel].csDSP);
	rxa[channel].agc.p->max_input = level;
	loadWcpAGC ( rxa[channel].agc.p );
	LeaveCriticalSection (&ch[channel].csDSP);
}

/********************************************************************************************************
*																										*
*											TXA Properties												*
*																										*
********************************************************************************************************/

PORT void
SetTXAALCSt (int channel, int state)
{
	EnterCriticalSection (&ch[channel].csDSP);
	txa[channel].alc.p->run = state;
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetTXAALCAttack (int channel, int attack)
{
	EnterCriticalSection (&ch[channel].csDSP);
	txa[channel].alc.p->tau_attack = (float)attack / 1000.0;
	loadWcpAGC(txa[channel].alc.p);
	LeaveCriticalSection (&ch[channel].csDSP);
}
PORT void
SetTXAALCDecay (int channel, int decay)
{
	EnterCriticalSection (&ch[channel].csDSP);
	txa[channel].alc.p->tau_decay = (float)decay / 1000.0;
	loadWcpAGC(txa[channel].alc.p);
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetTXAALCHang (int channel, int hang)
{
	EnterCriticalSection (&ch[channel].csDSP);
	txa[channel].alc.p->hangtime = (float)hang / 1000.0;
	loadWcpAGC(txa[channel].alc.p);
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetTXAALCMaxGain (int channel, float maxgain)
{
	EnterCriticalSection (&ch[channel].csDSP);
	txa[channel].alc.p->max_gain = pow (10.0,(float)maxgain / 20.0);
	loadWcpAGC(txa[channel].alc.p);
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetTXALevelerSt (int channel, int state)
{
	EnterCriticalSection (&ch[channel].csDSP);
	txa[channel].leveler.p->run = state;
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetTXALevelerAttack (int channel, int attack)
{
	EnterCriticalSection (&ch[channel].csDSP);
	txa[channel].leveler.p->tau_attack = (float)attack / 1000.0;
	loadWcpAGC(txa[channel].leveler.p);
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetTXALevelerDecay (int channel, int decay)
{
	EnterCriticalSection (&ch[channel].csDSP);
	txa[channel].leveler.p->tau_decay = (float)decay / 1000.0;
	loadWcpAGC(txa[channel].leveler.p);
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetTXALevelerHang (int channel, int hang)
{
	EnterCriticalSection (&ch[channel].csDSP);
	txa[channel].leveler.p->hangtime = (float)hang / 1000.0;
	loadWcpAGC(txa[channel].leveler.p);
	LeaveCriticalSection (&ch[channel].csDSP);
}

PORT void
SetTXALevelerTop (int channel, float maxgain)
{
	EnterCriticalSection (&ch[channel].csDSP);
	txa[channel].leveler.p->max_gain = pow (10.0,(float)maxgain / 20.0);
	loadWcpAGC(txa[channel].leveler.p);
	LeaveCriticalSection (&ch[channel].csDSP);
}
#endif
