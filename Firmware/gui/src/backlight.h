/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

void backlight_init();
void backlight_tick();

void backlight_set_brightness(int16_t value);

void backlight_switch();
bool backlight_is_on();
