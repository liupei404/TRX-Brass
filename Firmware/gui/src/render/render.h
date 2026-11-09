/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include "lvgl/lvgl.h"
#include "lvgl/src/draw/sw/lv_draw_sw.h"

typedef lv_draw_sw_ctx_t brass_draw_ctx_t;

void brass_draw_ctx_init(lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx);
