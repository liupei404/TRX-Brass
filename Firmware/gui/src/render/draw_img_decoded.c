/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "draw_img_decoded.h"

#define MAX_BUF_SIZE (uint32_t) lv_disp_get_hor_res(_lv_refr_get_disp_refreshing())

static void convert_cb(
    const lv_area_t * dest_area, const void * src_buf, lv_coord_t src_w, lv_coord_t src_h,
    lv_coord_t src_stride, const lv_draw_img_dsc_t * draw_dsc, lv_img_cf_t cf, lv_color_t * cbuf, lv_opa_t * abuf);

void LV_ATTRIBUTE_FAST_MEM draw_img_decoded(struct _lv_draw_ctx_t * draw_ctx,
                                                  const lv_draw_img_dsc_t * draw_dsc,
                                                  const lv_area_t * coords, const uint8_t * src_buf,
                                                  lv_img_cf_t cf)
{
    /*Use the clip area as draw area*/
    lv_area_t draw_area;
    lv_area_copy(&draw_area, draw_ctx->clip_area);

    bool mask_any = lv_draw_mask_is_any(&draw_area);

    lv_area_t blend_area;
    lv_draw_sw_blend_dsc_t blend_dsc;

    lv_memset_00(&blend_dsc, sizeof(lv_draw_sw_blend_dsc_t));
    blend_dsc.opa = draw_dsc->opa;
    blend_dsc.blend_mode = draw_dsc->blend_mode;
    blend_dsc.blend_area = &blend_area;

    /*The simplest case just copy the pixels into the draw_buf*/
    if (!mask_any && cf == LV_IMG_CF_TRUE_COLOR && draw_dsc->recolor_opa == LV_OPA_TRANSP) {

        blend_dsc.src_buf = (const lv_color_t *)src_buf;

        blend_dsc.blend_area = coords;
        lv_draw_sw_blend(draw_ctx, &blend_dsc);
    } else {
        blend_area.x1 = draw_ctx->clip_area->x1;
        blend_area.x2 = draw_ctx->clip_area->x2;
        blend_area.y1 = draw_ctx->clip_area->y1;
        blend_area.y2 = draw_ctx->clip_area->y2;

        lv_coord_t src_w = lv_area_get_width(coords);
        lv_coord_t src_h = lv_area_get_height(coords);
        lv_coord_t blend_h = lv_area_get_height(&blend_area);
        lv_coord_t blend_w = lv_area_get_width(&blend_area);

        uint32_t max_buf_size = MAX_BUF_SIZE;
        uint32_t blend_size = lv_area_get_size(&blend_area);
        uint32_t buf_h;
        uint32_t buf_w = blend_w;

        if (blend_size <= max_buf_size) {
            buf_h = blend_h;
        } else {
            /*Round to full lines*/
            buf_h = max_buf_size / blend_w;
        }

        /*Create buffers and masks*/
        uint32_t buf_size = buf_w * buf_h;

        lv_color_t * rgb_buf = lv_mem_buf_get(buf_size * sizeof(lv_color_t));
        lv_opa_t * mask_buf = lv_mem_buf_get(buf_size);

        blend_dsc.mask_buf = mask_buf;
        blend_dsc.mask_area = &blend_area;
        blend_dsc.mask_res = LV_DRAW_MASK_RES_CHANGED;
        blend_dsc.src_buf = rgb_buf;

        lv_coord_t y_last = blend_area.y2;
        blend_area.y2 = blend_area.y1 + buf_h - 1;

        lv_draw_mask_res_t mask_res_def = (cf != LV_IMG_CF_TRUE_COLOR || draw_dsc->angle ||
                                           draw_dsc->zoom != LV_IMG_ZOOM_NONE) ?
                                          LV_DRAW_MASK_RES_CHANGED : LV_DRAW_MASK_RES_FULL_COVER;
        blend_dsc.mask_res = mask_res_def;

        while (blend_area.y1 <= y_last) {
            /*Apply transformations if any or separate the channels*/
            lv_area_t transform_area;
            lv_area_copy(&transform_area, &blend_area);
            lv_area_move(&transform_area, -coords->x1, -coords->y1);

            convert_cb(&transform_area, src_buf, src_w, src_h, src_w, draw_dsc, cf, rgb_buf, mask_buf);

            /*Blend*/
            lv_draw_sw_blend(draw_ctx, &blend_dsc);

            /*Go the the next lines*/
            blend_area.y1 = blend_area.y2 + 1;
            blend_area.y2 = blend_area.y1 + buf_h - 1;
            if (blend_area.y2 > y_last) blend_area.y2 = y_last;
        }

        lv_mem_buf_release(mask_buf);
        lv_mem_buf_release(rgb_buf);
    }
}

static void convert_cb(const lv_area_t * dest_area, const void * src_buf, lv_coord_t src_w, lv_coord_t src_h,
                       lv_coord_t src_stride, const lv_draw_img_dsc_t * draw_dsc, lv_img_cf_t cf, lv_color_t * cbuf, lv_opa_t * abuf)
{
    LV_UNUSED(draw_dsc);
    LV_UNUSED(src_h);
    LV_UNUSED(src_w);

    const uint8_t * src_tmp8 = (const uint8_t *)src_buf;
    lv_coord_t y;
    lv_coord_t x;

    /* LV_IMG_CF_TRUE_COLOR_ALPHA only */

    src_tmp8 += (src_stride * dest_area->y1 * LV_IMG_PX_SIZE_ALPHA_BYTE) + dest_area->x1 * LV_IMG_PX_SIZE_ALPHA_BYTE;

    lv_coord_t src_new_line_step_px = (src_stride - lv_area_get_width(dest_area));
    lv_coord_t src_new_line_step_byte = src_new_line_step_px * LV_IMG_PX_SIZE_ALPHA_BYTE;

    lv_coord_t dest_h = lv_area_get_height(dest_area);
    lv_coord_t dest_w = lv_area_get_width(dest_area);

    for (y = 0; y < dest_h; y++) {
        for (x = 0; x < dest_w; x++) {
            abuf[x] = src_tmp8[LV_IMG_PX_SIZE_ALPHA_BYTE - 1];
            cbuf[x] = *((lv_color_t *) src_tmp8);
            cbuf[x].ch.alpha = 0xff;
            src_tmp8 += LV_IMG_PX_SIZE_ALPHA_BYTE;

        }
        cbuf += dest_w;
        abuf += dest_w;
        src_tmp8 += src_new_line_step_byte;
    }
}
