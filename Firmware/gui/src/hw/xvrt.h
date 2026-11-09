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

bool xvrt_update(const uint32_t freq);
void xvrt_load_tcxo_freq();
