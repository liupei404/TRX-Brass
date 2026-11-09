/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "render.h"
#include "blend.h"
#include "draw_img_decoded.h"

void brass_draw_ctx_init(lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx) {
    lv_draw_sw_init_ctx(drv, draw_ctx);

    brass_draw_ctx_t * brass_draw_ctx = (brass_draw_ctx_t *) draw_ctx;

    brass_draw_ctx->base_draw.draw_img_decoded = draw_img_decoded;
    brass_draw_ctx->blend = blend;
}
