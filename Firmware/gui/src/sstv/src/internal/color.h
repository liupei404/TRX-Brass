/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stdint.h>

void sstv_robot_to_rgb(uint8_t y, uint8_t ry, uint8_t by, uint8_t *r, uint8_t *g, uint8_t *b);
void sstv_pd_to_rgb(uint8_t y, uint8_t ry, uint8_t by, uint8_t *r, uint8_t *g, uint8_t *b);
