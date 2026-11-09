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

void draw_img_decoded(
    struct _lv_draw_ctx_t * draw_ctx,
    const lv_draw_img_dsc_t * draw_dsc,
    const lv_area_t * coords, const uint8_t * src_buf,
    lv_img_cf_t cf);
