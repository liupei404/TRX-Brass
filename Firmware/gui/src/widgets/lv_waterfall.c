/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

/*********************
 *      INCLUDES
 *********************/

#include <stdlib.h>
#include <string.h>
#include "lv_waterfall.h"
#include "lvgl/src/core/lv_obj_class_private.h"
#include "lvgl/src/core/lv_obj_private.h"
#include "lvgl/src/misc/cache/instance/lv_image_cache.h"

struct lv_waterfall_t {
    lv_obj_t        obj;

    lv_obj_t        *img;
    lv_draw_buf_t   *draw_buf;

    uint32_t        line_len;
    uint16_t        height;
    uint16_t        head;

    lv_color_t      palette[256];

    int16_t         min;
    int16_t         max;
    int32_t         span;
    int64_t         scroll_numerator;
    bool            skip_next_line;
};

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_waterfall_class

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_waterfall_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_waterfall_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_waterfall_update_image(lv_waterfall_t * waterfall);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_waterfall_class  = {
    .constructor_cb = lv_waterfall_constructor,
    .destructor_cb = lv_waterfall_destructor,
    .base_class = &lv_obj_class,
    .instance_size = sizeof(lv_waterfall_t),
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_waterfall_create(lv_obj_t * parent) {
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);

    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_waterfall_set_grad(lv_obj_t * obj, lv_grad_dsc_t * grad) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_waterfall_t * waterfall = (lv_waterfall_t *)obj;

    if (grad == NULL || grad->stops_count == 0) return;

    uint8_t stop = 0;
    for (uint16_t i = 0; i < 256; i++) {
        if (i <= grad->stops[0].frac) {
            waterfall->palette[i] = grad->stops[0].color;
            continue;
        }
        while (stop + 1 < grad->stops_count && i > grad->stops[stop + 1].frac) stop++;

        if (stop + 1 == grad->stops_count) {
            waterfall->palette[i] = grad->stops[stop].color;
        } else {
            const lv_grad_stop_t *from = &grad->stops[stop];
            const lv_grad_stop_t *to = &grad->stops[stop + 1];
            uint8_t range = to->frac - from->frac;
            uint8_t mix = range == 0 ? 255 : (uint8_t) (((i - from->frac) * 255U) / range);
            waterfall->palette[i] = lv_color_mix(to->color, from->color, mix);
        }
    }
}

void lv_waterfall_set_data_size(lv_obj_t * obj, uint16_t size) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_obj_update_layout(obj);

    lv_waterfall_t  *waterfall = (lv_waterfall_t *)obj;
    lv_coord_t      height = lv_obj_get_height(obj);

    if (waterfall->img != NULL) {
        lv_obj_delete(waterfall->img);
        waterfall->img = NULL;
    }
    if (waterfall->draw_buf != NULL) {
        lv_image_cache_drop(waterfall->draw_buf);
        lv_draw_buf_destroy(waterfall->draw_buf);
    }

    waterfall->draw_buf = NULL;
    waterfall->height = 0;
    waterfall->head = 0;

    if (size == 0 || height <= 0 || height > UINT16_MAX / 2) return;

    /* Keep a second copy of every row.  A height-sized window starting at
     * head is then always contiguous, even when the ring wraps. */
    waterfall->draw_buf = lv_draw_buf_create(size, (uint32_t) height * 2, LV_COLOR_FORMAT_XRGB8888, 0);
    if (waterfall->draw_buf == NULL) return;
    memset(waterfall->draw_buf->data, 0, waterfall->draw_buf->data_size);

    waterfall->line_len = waterfall->draw_buf->header.stride;
    waterfall->height = height;

    waterfall->img = lv_image_create(obj);

    lv_image_set_src(waterfall->img, waterfall->draw_buf);
    lv_obj_align(waterfall->img, LV_ALIGN_TOP_MID, 0, 0);
    lv_image_cache_drop(waterfall->draw_buf);
}

void lv_waterfall_set_max(lv_obj_t * obj, int16_t db) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_waterfall_t * waterfall = (lv_waterfall_t *)obj;

    waterfall->max = db;
}

void lv_waterfall_set_min(lv_obj_t * obj, int16_t db) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_waterfall_t * waterfall = (lv_waterfall_t *)obj;

    waterfall->min = db;
}

void lv_waterfall_set_span(lv_obj_t * obj, int32_t hz) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_waterfall_t * waterfall = (lv_waterfall_t *)obj;

    waterfall->span = hz;
}

void lv_waterfall_clear_data(lv_obj_t * obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_waterfall_t * waterfall = (lv_waterfall_t *)obj;

    if (waterfall->draw_buf == NULL) return;

    /* Keep the current ring window in place. */
    memset(waterfall->draw_buf->data, 0, waterfall->draw_buf->data_size);
    waterfall->scroll_numerator = 0;
    waterfall->skip_next_line = false;
    lv_draw_buf_flush_cache(waterfall->draw_buf, NULL);
    lv_obj_invalidate(waterfall->img);
}

int32_t lv_waterfall_scroll_data(lv_obj_t * obj, int32_t df) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_waterfall_t * waterfall = (lv_waterfall_t *)obj;

    int32_t width = lv_obj_get_width(obj);
    if (width <= 0 || waterfall->span <= 0 || waterfall->draw_buf == NULL) return 0;

    /* Preserve the fractional pixel remainder exactly. This avoids drift
     * when span / width is not an integer number of Hz per pixel. */
    waterfall->scroll_numerator += (int64_t)df * width;
    int32_t px = waterfall->scroll_numerator / waterfall->span;
    waterfall->scroll_numerator -= (int64_t)px * waterfall->span;
    waterfall->skip_next_line = true;

    /* Scroll */

    if (px) {
        lv_draw_buf_t   *dsc = waterfall->draw_buf;

        if (abs(px) > dsc->header.w) {
            lv_waterfall_clear_data(obj);
            return 0;
        }

        uint8_t         *ptr = dsc->data;
        uint32_t        line_len = waterfall->line_len;
        uint16_t        offset = abs(px) * 4;
        uint16_t        tail = (dsc->header.w - abs(px)) * 4;

        /* Horizontal scrolling is independent of the vertical ring order.
         * Update every unique row and then synchronize its duplicate. */
        for (uint16_t y = 0; y < waterfall->height; y++) {
            if (px > 0) {
                memmove(ptr, ptr + offset, tail);
                memset(ptr + tail, 0, offset);
            } else {
                memmove(ptr + offset, ptr, tail);
                memset(ptr, 0, offset);
            }

            memcpy(ptr + waterfall->height * line_len, ptr, line_len);
            ptr += line_len;
        }

        lv_waterfall_update_image(waterfall);
        lv_obj_invalidate(waterfall->img);
    }

    return (int64_t)px * waterfall->span / width;
}

void lv_waterfall_add_data(lv_obj_t * obj, float * data, uint16_t cnt) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_waterfall_t  *waterfall = (lv_waterfall_t *)obj;
    lv_draw_buf_t   *dsc = waterfall->draw_buf;

    if (dsc == NULL || waterfall->max == waterfall->min) return;

    if (cnt == 0 || waterfall->height == 0) return;

    /* The first accumulation interval after an FFT retune can contain data
     * from both center frequencies. Do not advance the vertical history with
     * that mixed line. Repeated retunes keep resetting this one-frame gate. */
    if (waterfall->skip_next_line) {
        waterfall->skip_next_line = false;
        return;
    }

    uint32_t    line_len = waterfall->line_len;

    /* Moving the window one row backwards makes the new row appear at the
     * top without moving the existing image. */
    waterfall->head = waterfall->head == 0 ? waterfall->height - 1 : waterfall->head - 1;
    uint8_t *ptr = dsc->data + waterfall->head * line_len;

    /* Paint */

    for (uint32_t x = 0; x < dsc->header.w; x++) {
        uint32_t    index = x * cnt / dsc->header.w;
        float       d = data[index];
        float       v = (d - waterfall->min) / (waterfall->max - waterfall->min);

        if (v < 0.0f) {
            v = 0.0f;
        } else if (v > 1.0f) {
            v = 1.0f;
        }

        uint8_t id = v * 255;

        ((uint32_t *) ptr)[x] = lv_color_to_u32(waterfall->palette[id]);
    }

    memcpy(ptr + waterfall->height * line_len, ptr, line_len);
    lv_waterfall_update_image(waterfall);
    lv_obj_invalidate(waterfall->img);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_waterfall_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_waterfall_t * waterfall = (lv_waterfall_t *)obj;

    waterfall->img = NULL;
    waterfall->draw_buf = NULL;
    waterfall->line_len = 0;
    waterfall->height = 0;
    waterfall->head = 0;
    waterfall->min = -40;
    waterfall->max = 0;
    waterfall->span = 100000;
    waterfall->scroll_numerator = 0;
    waterfall->skip_next_line = false;

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_waterfall_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);
    lv_waterfall_t * waterfall = (lv_waterfall_t *)obj;

    if (waterfall->draw_buf) {
        lv_image_cache_drop(waterfall->draw_buf);
        lv_draw_buf_destroy(waterfall->draw_buf);
    }
}

static void lv_waterfall_update_image(lv_waterfall_t * waterfall) {
    lv_area_t visible = {
        .x1 = 0,
        .y1 = waterfall->head,
        .x2 = waterfall->draw_buf->header.w - 1,
        .y2 = waterfall->head + waterfall->height - 1,
    };

    lv_draw_buf_flush_cache(waterfall->draw_buf, &visible);
    lv_obj_align(waterfall->img, LV_ALIGN_TOP_MID, 0, -(lv_coord_t) waterfall->head);
}
