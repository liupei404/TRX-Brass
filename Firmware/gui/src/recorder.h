/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

extern char *recorder_path;

void recorder_init();

void recorder_set_on(bool on);
bool recorder_is_on();
void recorder_put_audio_samples(float *data, uint16_t samples);
