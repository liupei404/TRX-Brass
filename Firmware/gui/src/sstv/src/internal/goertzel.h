/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#pragma once

#include <complex.h>

/* Computes two tone powers and, optionally, window energy in one pass.
 * Frequencies are not quantized to DFT bins. */
void sstv_goertzel_pair_power(const float complex *samples, int count, double frequency0, double frequency1, double sample_rate, double *power0, double *power1, double *signal_energy);
