/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#include "blend.h"
#include <arm_neon.h>

static inline lv_color_t fast_color_mix(lv_color_t c1, lv_color_t c2, uint8_t mix) {
    lv_color_t  ret;
    uint8_t     nmix = 255 - mix;

    ret.ch.red = (c1.ch.red * mix + c2.ch.red * nmix) / 256;
    ret.ch.green = (c1.ch.green * mix + c2.ch.green * nmix) / 256;
    ret.ch.blue = (c1.ch.blue * mix + c2.ch.blue * nmix) / 256;

    return ret;
}

void fill_normal(
    lv_color_t *__restrict__ dest_buf, const lv_area_t *__restrict__ dest_area,
    lv_coord_t dest_stride, lv_color_t color, lv_opa_t opa,
    const lv_opa_t *__restrict__ mask, lv_coord_t mask_stride);

void map_normal(
    lv_color_t *__restrict__ dest_buf, const lv_area_t *__restrict__ dest_area,
    lv_coord_t dest_stride, const lv_color_t *__restrict__ src_buf,
    lv_coord_t src_stride, lv_opa_t opa, const lv_opa_t *__restrict__ mask,
    lv_coord_t mask_stride);

void blend(struct _lv_draw_ctx_t * draw_ctx, const lv_draw_sw_blend_dsc_t * dsc) {
    lv_opa_t * mask;

    if (dsc->mask_buf == NULL) mask = NULL;

    if (dsc->mask_buf && dsc->mask_res == LV_DRAW_MASK_RES_TRANSP) {
        return;
    } else if (dsc->mask_res == LV_DRAW_MASK_RES_FULL_COVER) {
        mask = NULL;
    } else {
        mask = dsc->mask_buf;
    }

    lv_coord_t dest_stride = lv_area_get_width(draw_ctx->buf_area);

    lv_area_t blend_area;

    if (!_lv_area_intersect(&blend_area, dsc->blend_area, draw_ctx->clip_area)) return;

    lv_disp_t * disp = _lv_refr_get_disp_refreshing();
    lv_color_t * dest_buf = draw_ctx->buf;

    dest_buf += dest_stride * (blend_area.y1 - draw_ctx->buf_area->y1) + (blend_area.x1 - draw_ctx->buf_area->x1);

    const lv_color_t * src_buf = dsc->src_buf;
    lv_coord_t src_stride;

    if (src_buf) {
        src_stride = lv_area_get_width(dsc->blend_area);
        src_buf += src_stride * (blend_area.y1 - dsc->blend_area->y1) + (blend_area.x1 - dsc->blend_area->x1);
    } else {
        src_stride = 0;
    }

    lv_coord_t mask_stride;

    if (mask) {
        mask_stride = lv_area_get_width(dsc->mask_area);
        mask += mask_stride * (blend_area.y1 - dsc->mask_area->y1) + (blend_area.x1 - dsc->mask_area->x1);
    } else {
        mask_stride = 0;
    }

    lv_area_move(&blend_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);

    if (dsc->src_buf == NULL) {
        fill_normal(dest_buf, &blend_area, dest_stride, dsc->color, dsc->opa, mask, mask_stride);
    } else {
        map_normal(dest_buf, &blend_area, dest_stride, src_buf, src_stride, dsc->opa, mask, mask_stride);
    }
}

/* * */

void fill_normal(
    lv_color_t *__restrict__        dest_buf,
    const lv_area_t *__restrict__   dest_area,
    lv_coord_t                      dest_stride,
    lv_color_t                      color,
    lv_opa_t                        opa,
    const lv_opa_t *__restrict__    mask,
    lv_coord_t                      mask_stride)
{
    int32_t w = lv_area_get_width(dest_area);
    int32_t h = lv_area_get_height(dest_area);

    int32_t x;
    int32_t y;

    if (mask == NULL) {
        if (opa >= LV_OPA_MAX) {
            for (y = 0; y < h; y++) {
                for (int i = 0; i < w; i++)
                    dest_buf[i] = color;

                dest_buf += dest_stride;
            }
        } else {
            lv_color_t last_dest_color = lv_color_black();
            lv_color_t last_res_color = fast_color_mix(color, last_dest_color, opa);

            uint16_t color_premult[3];
            lv_color_premult(color, opa, color_premult);
            lv_opa_t opa_inv = 255 - opa;

            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    if (last_dest_color.full != dest_buf[x].full) {
                        last_dest_color = dest_buf[x];
                        last_res_color = lv_color_mix_premult(color_premult, dest_buf[x], opa_inv);
                    }
                    dest_buf[x] = last_res_color;
                }
                dest_buf += dest_stride;
            }
        }
    } else {
        if (opa >= LV_OPA_MAX) {
            for (y = 0; y < h; y++) {
                for (x = 0; x < (w & ~3); x += 4) {
                    uint32_t mask32 = *((uint32_t *)mask);

                    if (mask32 == 0xFFFFFFFF) {
                        /* NEONize me */

                        dest_buf[0] = color;
                        dest_buf[1] = color;
                        dest_buf[2] = color;
                        dest_buf[3] = color;
                    } else if (mask32) {
                        /* NEONize me */

                        dest_buf[0] = fast_color_mix(color, dest_buf[0], mask[0]);
                        dest_buf[1] = fast_color_mix(color, dest_buf[1], mask[1]);
                        dest_buf[2] = fast_color_mix(color, dest_buf[2], mask[2]);
                        dest_buf[3] = fast_color_mix(color, dest_buf[3], mask[3]);
                    }

                    dest_buf += 4;
                    mask += 4;
                }

                for (; x < w ; x++) {
                    *dest_buf = fast_color_mix(color, *dest_buf, *mask);
                    mask++;
                    dest_buf++;
                }

                dest_buf += (dest_stride - w);
                mask += (mask_stride - w);
            }
        } else {
            lv_color_t last_dest_color;
            lv_color_t last_res_color;
            lv_opa_t last_mask = LV_OPA_TRANSP;
            last_dest_color.full = dest_buf[0].full;
            last_res_color.full = dest_buf[0].full;
            lv_opa_t opa_tmp = LV_OPA_TRANSP;

            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    if (*mask) {
                        if (*mask != last_mask) opa_tmp = *mask == LV_OPA_COVER ? opa :
                                                             (uint32_t)((uint32_t)(*mask) * opa) >> 8;
                        if (*mask != last_mask || last_dest_color.full != dest_buf[x].full) {
                            if (opa_tmp == LV_OPA_COVER) {
                                last_res_color = color;
                            } else {
                                last_res_color = fast_color_mix(color, dest_buf[x], opa_tmp);
                            }

                            last_mask = *mask;
                            last_dest_color.full = dest_buf[x].full;
                        }
                        dest_buf[x] = last_res_color;
                    }
                    mask++;
                }
                dest_buf += dest_stride;
                mask += (mask_stride - w);
            }
        }
    }
}

void map_normal(
    lv_color_t *__restrict__        dest_buf,
    const lv_area_t *__restrict__   dest_area,
    lv_coord_t                      dest_stride,
    const lv_color_t *__restrict__  src_buf,
    lv_coord_t                      src_stride,
    lv_opa_t                        opa,
    const lv_opa_t *__restrict__    mask,
    lv_coord_t                      mask_stride)
{
    int32_t w = lv_area_get_width(dest_area);
    int32_t h = lv_area_get_height(dest_area);

    int32_t x;
    int32_t y;

    if (mask == NULL) {
        if (opa >= LV_OPA_MAX) {
            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++)
                    dest_buf[x] = src_buf[x];

                dest_buf += dest_stride;
                src_buf += src_stride;
            }
        } else {
            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++)
                    dest_buf[x] = fast_color_mix(src_buf[x], dest_buf[x], opa);

                dest_buf += dest_stride;
                src_buf += src_stride;
            }
        }
    } else {
        if (opa > LV_OPA_MAX) {
            for (y = 0; y < h; y++) {
                for (x = 0; x < (w & ~3); x += 4) {
                    uint32_t mask32 = *((uint32_t *) mask);

                    if (mask32) {
                        if (mask32 == 0xFFFFFFFF) {
                            /* NEONize me */

                            dest_buf[0] = src_buf[0];
                            dest_buf[1] = src_buf[1];
                            dest_buf[2] = src_buf[2];
                            dest_buf[3] = src_buf[3];
                        } else {
                            /* NEONize me */

                            dest_buf[0] = fast_color_mix(src_buf[0], dest_buf[0], mask[0]);
                            dest_buf[1] = fast_color_mix(src_buf[1], dest_buf[1], mask[1]);
                            dest_buf[2] = fast_color_mix(src_buf[2], dest_buf[2], mask[2]);
                            dest_buf[3] = fast_color_mix(src_buf[3], dest_buf[3], mask[3]);
                        }
                    }
                    dest_buf += 4;
                    src_buf += 4;
                    mask += 4;
                }

                for (; x < w ; x++) {
                    *dest_buf = fast_color_mix(*src_buf, *dest_buf, *mask);
                    dest_buf++;
                    src_buf++;
                    mask++;
                }

                dest_buf += dest_stride;
                src_buf += src_stride;
                mask += mask_stride;
            }
        } else {
            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    if (mask[x]) {
                        lv_opa_t opa_tmp = mask[x] >= LV_OPA_MAX ? opa : ((opa * mask[x]) >> 8);
                        dest_buf[x] = fast_color_mix(src_buf[x], dest_buf[x], opa_tmp);
                    }
                }

                dest_buf += dest_stride;
                src_buf += src_stride;
                mask += mask_stride;
            }
        }
    }
}
