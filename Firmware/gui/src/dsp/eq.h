/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EQ_FFT_SIZE 512
#define EQ_HOP_SIZE 256
#define EQ_TAPS 257
#define EQ_MAX_POINTS 5

typedef struct {
    int16_t freq;
    int16_t gain;
} eq_point_t;

typedef struct {
    double sample_rate;
    double low;                  /* stopband ends here */
    double high;                 /* stopband begins here */
    double transition;           /* transitions lie inside [low, high] */
    size_t count;
    eq_point_t points[EQ_MAX_POINTS];
} eq_config_t;

typedef struct eq eq_t;

bool eq_valid_config(const eq_config_t *config);
bool eq_design(const eq_config_t *config, double taps[EQ_TAPS]);
double eq_target(const eq_config_t *config, double frequency);
eq_t *eq_create(const eq_config_t *config);
void eq_destroy(eq_t *eq);
bool eq_update(eq_t *eq, const eq_config_t *config);
void eq_reset(eq_t *eq);
size_t eq_process(eq_t *eq, const float *input, size_t count, float *output, size_t capacity, size_t *consumed);
