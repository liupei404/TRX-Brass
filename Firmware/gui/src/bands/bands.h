/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include "list.h"
#include "src/settings/bands.h"

extern band_settings_t  *band_settings;

void bands_init();
band_list_t * bands_list();

void bands_activate(band_t *band, bool touch_freq);
void bands_change(bool up);
