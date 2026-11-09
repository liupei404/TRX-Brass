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
#include "lv_spectrum3d.h"
#include "lvgl/src/core/lv_obj_class_private.h"
#include "lvgl/src/core/lv_obj_private.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_spectrum3d_class

typedef struct {
    lv_obj_t obj;
    uint16_t depth;
    uint16_t data_size;
    uint8_t * data_buf;
    uint8_t * line_buf;
    lv_coord_t * top_buf;
    uint32_t top_buf_size;
    lv_color_t palette[256];
    int16_t min;
    int16_t max;
    int32_t span;
    int16_t scroll_surplus;
    int32_t scroll;
} lv_spectrum3d_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
 
static void lv_spectrum3d_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_spectrum3d_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_spectrum3d_event(const lv_obj_class_t * class_p, lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_spectrum3d_class  = {
    .constructor_cb = lv_spectrum3d_constructor,
    .destructor_cb = lv_spectrum3d_destructor,
    .event_cb = lv_spectrum3d_event,
    .base_class = &lv_obj_class,
    .instance_size = sizeof(lv_spectrum3d_t),
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_spectrum3d_create(lv_obj_t * parent) {
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);

    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_spectrum3d_set_grad(lv_obj_t * obj, lv_grad_dsc_t * grad) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum3d_t * spectrum3d = (lv_spectrum3d_t *)obj;

    if (grad == NULL || grad->stops_count == 0) return;

    uint8_t stop = 0;
    for (uint16_t i = 0; i < 256; i++) {
        if (i <= grad->stops[0].frac) {
            spectrum3d->palette[i] = grad->stops[0].color;
            continue;
        }
        while (stop + 1 < grad->stops_count && i > grad->stops[stop + 1].frac) stop++;

        if (stop + 1 == grad->stops_count) {
            spectrum3d->palette[i] = grad->stops[stop].color;
        } else {
            const lv_grad_stop_t * from = &grad->stops[stop];
            const lv_grad_stop_t * to = &grad->stops[stop + 1];
            uint8_t range = to->frac - from->frac;
            uint8_t mix = range == 0 ? 255 : (uint8_t)(((i - from->frac) * 255U) / range);
            spectrum3d->palette[i] = lv_color_mix(to->color, from->color, mix);
        }
    }
}

void lv_spectrum3d_set_data_size(lv_obj_t * obj, uint16_t size, uint16_t depth) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum3d_t * spectrum3d = (lv_spectrum3d_t *)obj;

    if (size == 0 || depth == 0) return;
    lv_obj_update_layout(obj);
    uint32_t width = lv_obj_get_width(obj);
    if (width == 0) return;

    uint8_t * data_buf = lv_malloc((size_t)size * depth);
    uint8_t * line_buf = lv_malloc(size);
    lv_coord_t * top_buf = lv_malloc(width * sizeof(*top_buf));
    if (data_buf == NULL || line_buf == NULL || top_buf == NULL) {
        lv_free(data_buf);
        lv_free(line_buf);
        lv_free(top_buf);
        return;
    }

    lv_free(spectrum3d->data_buf);
    lv_free(spectrum3d->line_buf);
    lv_free(spectrum3d->top_buf);

    spectrum3d->data_buf = data_buf;
    spectrum3d->line_buf = line_buf;
    spectrum3d->top_buf = top_buf;
    spectrum3d->top_buf_size = width;
    spectrum3d->data_size = size;
    spectrum3d->depth = depth;
    lv_spectrum3d_clear_data(obj);
}

void lv_spectrum3d_clear_data(lv_obj_t * obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum3d_t * spectrum3d = (lv_spectrum3d_t *)obj;

    spectrum3d->scroll_surplus = 0;
    if (spectrum3d->data_buf != NULL) memset(spectrum3d->data_buf, 0, (size_t)spectrum3d->data_size * spectrum3d->depth);
    if (spectrum3d->line_buf != NULL) memset(spectrum3d->line_buf, 0, spectrum3d->data_size);
    lv_obj_invalidate(obj);
}

void lv_spectrum3d_scroll_data(lv_obj_t * obj, int32_t df) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum3d_t * spectrum3d = (lv_spectrum3d_t *)obj;

    lv_coord_t width = lv_obj_get_width(obj);
    if (width <= 0 || spectrum3d->span <= 0 || spectrum3d->data_size == 0 || spectrum3d->data_buf == NULL) return;
    int32_t div = spectrum3d->span / width;
    if (div == 0) return;
    int16_t     surplus = df % div;

    spectrum3d->scroll += df / div;

    if (surplus) {
        spectrum3d->scroll_surplus += surplus;
    } else {
        spectrum3d->scroll_surplus = 0;
    }

    if (abs(spectrum3d->scroll_surplus) > div) {
        spectrum3d->scroll += spectrum3d->scroll_surplus / div;
        spectrum3d->scroll_surplus %= div;
    }

    /* Scroll */

    int16_t px = spectrum3d->scroll;

    if (px) {
        uint8_t         *ptr = spectrum3d->data_buf;
        uint32_t        line_len = spectrum3d->data_size;
        uint8_t         *line_buf = spectrum3d->line_buf;
        uint32_t offset = (uint32_t)abs(px);
        if (offset >= spectrum3d->data_size) {
            memset(spectrum3d->data_buf, 0, (size_t)spectrum3d->data_size * spectrum3d->depth);
            spectrum3d->scroll = 0;
            lv_obj_invalidate(obj);
            return;
        }
        uint16_t tail = spectrum3d->data_size - offset;

        for (int y = 0; y < spectrum3d->depth; y++) {
            if (px > 0) {
                memset(line_buf + tail, 0, offset);
                memcpy(line_buf, ptr + offset, tail);
                memcpy(ptr, line_buf, line_len);
            } else {
                memset(line_buf, 0, offset);
                memcpy(line_buf + offset, ptr, tail);
                memcpy(ptr, line_buf, line_len);
            }

            ptr += line_len;
        }

        spectrum3d->scroll = 0;
        lv_obj_invalidate(obj);
    }
}

void lv_spectrum3d_add_data(lv_obj_t * obj, float * data, uint16_t cnt) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum3d_t *spectrum3d = (lv_spectrum3d_t *)obj;

    if (data == NULL || cnt == 0 || spectrum3d->data_buf == NULL ||
        spectrum3d->data_size == 0 || spectrum3d->depth == 0 || spectrum3d->max == spectrum3d->min) {
        return;
    }

    /* Scroll down */

    uint16_t    data_size = spectrum3d->data_size;
    if (spectrum3d->depth > 1) {
        memmove(spectrum3d->data_buf + data_size, spectrum3d->data_buf,
                (size_t)data_size * (spectrum3d->depth - 1));
    }

    /* Fill */

    float    range = spectrum3d->max - spectrum3d->min;
    uint16_t shift = 0;

    if (cnt > data_size) {
        shift = (cnt - data_size) / 2;
    }

    for (uint16_t i = 0; i < data_size; i++) {
        uint16_t index = cnt >= data_size ? shift + i : (uint32_t)i * cnt / data_size;
        float   d = data[index];
        float   v = (d - spectrum3d->min) / range;

        if (v < 0.0f) {
            v = 0.0f;
        } else if (v > 1.0f) {
            v = 1.0f;
        }

        uint8_t id = v * 255;

        spectrum3d->data_buf[i] = id;
    }
    lv_obj_invalidate(obj);
}

void lv_spectrum3d_set_span(lv_obj_t * obj, int32_t hz) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum3d_t * spectrum3d = (lv_spectrum3d_t *)obj;

    spectrum3d->span = hz;
}

void lv_spectrum3d_set_max(lv_obj_t * obj, int16_t db) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum3d_t * spectrum3d = (lv_spectrum3d_t *)obj;

    spectrum3d->max = db;
}

void lv_spectrum3d_set_min(lv_obj_t * obj, int16_t db) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum3d_t * spectrum3d = (lv_spectrum3d_t *)obj;

    spectrum3d->min = db;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_spectrum3d_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_spectrum3d_t * spectrum3d = (lv_spectrum3d_t *)obj;

    spectrum3d->data_size = 0;
    spectrum3d->data_buf = NULL;
    spectrum3d->line_buf = NULL;
    spectrum3d->top_buf = NULL;
    spectrum3d->top_buf_size = 0;
    spectrum3d->depth = 150;

    spectrum3d->min = -40;
    spectrum3d->max = 0;
    spectrum3d->span = 100000;
    spectrum3d->scroll_surplus = 0;

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_spectrum3d_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);
    lv_spectrum3d_t * spectrum3d = (lv_spectrum3d_t *)obj;

    if (spectrum3d->data_buf) lv_free(spectrum3d->data_buf);
    if (spectrum3d->line_buf) lv_free(spectrum3d->line_buf);
    if (spectrum3d->top_buf) lv_free(spectrum3d->top_buf);
}

static void lv_spectrum3d_event(const lv_obj_class_t * class_p, lv_event_t * e) {
    LV_UNUSED(class_p);

    lv_res_t res = lv_obj_event_base(MY_CLASS, e);

    if (res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if (code == LV_EVENT_DRAW_MAIN_END) {
        lv_spectrum3d_t     *spectrum3d = (lv_spectrum3d_t *) obj;
        lv_layer_t          *layer = lv_event_get_layer(e);
        lv_draw_line_dsc_t  line_dsc;

        if (!spectrum3d->data_buf) return;
        if (!spectrum3d->top_buf) return;

        lv_draw_line_dsc_init(&line_dsc);

        line_dsc.width = 1;

        lv_area_t obj_coords;
        lv_obj_get_coords(obj, &obj_coords);
        lv_coord_t x1 = obj_coords.x1;
        lv_coord_t y1 = obj_coords.y1;

        lv_coord_t w = lv_obj_get_width(obj);
        lv_coord_t h = lv_obj_get_height(obj);

        if (w <= 0 || h <= 0 || spectrum3d->data_size == 0 || spectrum3d->depth == 0 ||
            (uint32_t)w > spectrum3d->top_buf_size) return;
        for (int i = 0; i < w; i++) {
            spectrum3d->top_buf[i] = y1 + h;
        }

        lv_point_t      a, b;
        float           scale = 1.0f;

        const uint8_t   *data = spectrum3d->data_buf;
        const uint16_t  data_size = spectrum3d->data_size;
        const uint16_t  depth = spectrum3d->depth;

        for (uint16_t d = 0; d < depth; d++) {
            lv_coord_t  y = y1 - d * h / depth;
            lv_coord_t  bottom = y + h;

            for (lv_coord_t x = 0; x < w; x++) {
                int32_t     index = data_size / 2 + (x - w / 2) / scale;

                if (index < 0 || index >= data_size) {
                    continue;
                }

                uint8_t     id = data[index];
                float       v = id / 255.0f * scale * 0.25f;
                lv_coord_t  prev_top = spectrum3d->top_buf[x];
                lv_coord_t  top = y + (1.0f - v) * h;

                if (top > prev_top) {
                    continue;
                }

                a.x = x1 + x;
                a.y = top;

                b.x = x1 + x;
                b.y = bottom > prev_top ? prev_top : bottom;

                line_dsc.color = lv_color_darken(spectrum3d->palette[id], 192 * d / depth);

                line_dsc.p1.x = a.x;
                line_dsc.p1.y = a.y;
                line_dsc.p2.x = b.x;
                line_dsc.p2.y = b.y;
                lv_draw_line(layer, &line_dsc);
                spectrum3d->top_buf[x] = top;
            }

            data += data_size;
            scale -= 4.0f / w;
        }
    }
}
