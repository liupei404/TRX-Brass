/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <complex.h>
#include <stdint.h>
#include <stdlib.h>

void two_tone_update();
size_t two_tone_generator(float complex *data, size_t max_size);
