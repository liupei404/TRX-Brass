/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>

void control_init();
void control_update();

void control_set_rx_freq(uint64_t freq);
void control_set_rx_rate(uint32_t rate);
void control_set_tx_freq(uint64_t freq);
void control_set_fft_freq(uint64_t freq);
void control_set_fft_rate(uint32_t rate);

void control_rx_enable();
void control_fft_enable();
void control_mb_enable();
