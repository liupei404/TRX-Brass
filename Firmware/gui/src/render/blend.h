/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#pragma once

#include "lvgl/lvgl.h"
#include "lvgl/src/draw/sw/lv_draw_sw_blend.h"

void blend(struct _lv_draw_ctx_t * draw_ctx, const lv_draw_sw_blend_dsc_t * dsc);
