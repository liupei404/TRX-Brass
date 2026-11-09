/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

/*********************
 *      INCLUDES
 *********************/

#include <stdlib.h>
#include "lv_spectrum.h"
#include "lvgl/src/core/lv_obj_class_private.h"
#include "lvgl/src/core/lv_obj_private.h"

struct lv_spectrum_t {
    lv_obj_t            obj;

    int16_t             min;
    int16_t             max;
    int32_t             span;
    int16_t             delta_surplus;
    bool                filled;
    bool                peak_on;
    uint16_t            peak_hold;
    float               peak_speed;

    uint16_t            data_size;
    float               *data_buf;
    lv_spectrum_peak_t  *peak_buf;
};

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_spectrum_class

/**********************
 *  STATIC PROTOTYPES
 **********************/
 
static void lv_spectrum_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_spectrum_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_spectrum_event(const lv_obj_class_t * class_p, lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_spectrum_class  = {
    .constructor_cb = lv_spectrum_constructor,
    .destructor_cb = lv_spectrum_destructor,
    .event_cb = lv_spectrum_event,
    .base_class = &lv_obj_class,
    .instance_size = sizeof(lv_spectrum_t),
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_spectrum_create(lv_obj_t * parent) {
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);

    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_spectrum_set_data_size(lv_obj_t * obj, uint16_t size) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum_t * spectrum = (lv_spectrum_t *)obj;

    spectrum->data_size = size;
    spectrum->data_buf = lv_realloc(spectrum->data_buf, size * sizeof(float));
    spectrum->peak_buf = lv_realloc(spectrum->peak_buf, size * sizeof(lv_spectrum_peak_t));
}

void lv_spectrum_clear_data(lv_obj_t * obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum_t * spectrum = (lv_spectrum_t *)obj;

    uint32_t now = lv_tick_get();
    
    for (uint16_t i = 0; i < spectrum->data_size; i++) {
        spectrum->peak_buf[i].val = spectrum->min;
        spectrum->peak_buf[i].time = now;
    }
    
    spectrum->delta_surplus = 0;
}

void lv_spectrum_scroll_data(lv_obj_t * obj, int32_t df) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum_t       *spectrum = (lv_spectrum_t *)obj;
    lv_spectrum_peak_t  *from, *to;
    uint32_t            now = lv_tick_get();

    if (spectrum->data_size == 0) return;
    uint16_t            div = spectrum->span / spectrum->data_size;
    if (div == 0) return;
    int16_t             surplus = df % div;
    int32_t             delta = df / div;
    
    if (surplus) {
        spectrum->delta_surplus += surplus;
    } else {
        spectrum->delta_surplus = 0;
    }
    
    if (abs(spectrum->delta_surplus) > 0) {
        delta += spectrum->delta_surplus / div;
        spectrum->delta_surplus = spectrum->delta_surplus % div;
    }

    if (delta == 0) {
        return;
    }

    if (delta > 0) {
        for (int16_t i = 0; i < spectrum->data_size - 1; i++) {
            to = &spectrum->peak_buf[i];
            
            if (i >= spectrum->data_size - delta) {
                to->val = spectrum->min;
                to->time = now;
            } else {
                from = &spectrum->peak_buf[i + delta];
                *to = *from;
            }
        }
    } else {
        delta = -delta;
        
        for (int16_t i = spectrum->data_size - 1; i > 0; i--) {
            to = &spectrum->peak_buf[i];
            
            if (i <= delta) {
                to->val = spectrum->min;
                to->time = now;
            } else {
                from = &spectrum->peak_buf[i - delta];
                *to = *from;
            }
        }
    }
}

void lv_spectrum_add_data(lv_obj_t * obj, float * data, size_t size) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum_t * spectrum = (lv_spectrum_t *)obj;

    uint32_t    now = lv_tick_get();
    
    for (uint16_t i = 0; i < spectrum->data_size; i++) {
        size_t index = i * size / spectrum->data_size;

        spectrum->data_buf[i] = data[index];

        if (spectrum->peak_on) {
            float               v = spectrum->data_buf[i];
            lv_spectrum_peak_t  *peak = &spectrum->peak_buf[i];
            
            if (v > peak->val) {
                peak->time = now;
                peak->val = v;
            } else {
                if (now - peak->time > spectrum->peak_hold) {
                    peak->val -= spectrum->peak_speed;
                }
            }
        }
    }
}

void lv_spectrum_set_span(lv_obj_t * obj, int32_t hz) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum_t * spectrum = (lv_spectrum_t *)obj;

    spectrum->span = hz;
}

void lv_spectrum_set_max(lv_obj_t * obj, int16_t db) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum_t * spectrum = (lv_spectrum_t *)obj;

    spectrum->max = db;
}

void lv_spectrum_set_min(lv_obj_t * obj, int16_t db) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum_t * spectrum = (lv_spectrum_t *)obj;

    spectrum->min = db;
}

void lv_spectrum_set_filled(lv_obj_t * obj, bool on) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum_t * spectrum = (lv_spectrum_t *)obj;

    spectrum->filled = on;
}

void lv_spectrum_set_peak(lv_obj_t * obj, bool on) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum_t * spectrum = (lv_spectrum_t *)obj;

    spectrum->peak_on = on;
}

void lv_spectrum_set_peak_hold(lv_obj_t * obj, uint16_t ms) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum_t * spectrum = (lv_spectrum_t *)obj;
    
    spectrum->peak_hold = ms;
}

void lv_spectrum_set_peak_speed(lv_obj_t * obj, float db) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_spectrum_t * spectrum = (lv_spectrum_t *)obj;

    spectrum->peak_speed = db;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_spectrum_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_spectrum_t * spectrum = (lv_spectrum_t *)obj;

    spectrum->filled = false;
    spectrum->peak_on = false;
    spectrum->data_size = 0;
    spectrum->data_buf = NULL;
    spectrum->peak_buf = NULL;
    spectrum->min = -40;
    spectrum->max = 0;
    spectrum->span = 100000;
    spectrum->delta_surplus = 0;

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_spectrum_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);
    lv_spectrum_t * spectrum = (lv_spectrum_t *)obj;

    if (spectrum->data_buf) lv_free(spectrum->data_buf);
    if (spectrum->peak_buf) lv_free(spectrum->peak_buf);
}

static void lv_spectrum_event(const lv_obj_class_t * class_p, lv_event_t * e) {
    LV_UNUSED(class_p);

    lv_res_t res = lv_obj_event_base(MY_CLASS, e);

    if (res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if (code == LV_EVENT_DRAW_MAIN_END) {
        lv_spectrum_t   *spectrum = (lv_spectrum_t *) obj;
        lv_layer_t      *layer = lv_event_get_layer(e);

        lv_draw_line_dsc_t  main_line_dsc;
        lv_draw_line_dsc_t  peak_line_dsc;

        /* Lines */

        lv_draw_line_dsc_init(&main_line_dsc);
        lv_obj_init_draw_line_dsc(obj, LV_PART_INDICATOR, &main_line_dsc);

        if (spectrum->peak_on) {
            lv_draw_line_dsc_init(&peak_line_dsc);
            lv_obj_init_draw_line_dsc(obj, LV_PART_SPECTRUM_PEAK, &peak_line_dsc);
        }

        lv_area_t obj_coords;
        lv_obj_get_coords(obj, &obj_coords);
        lv_coord_t x1 = obj_coords.x1;
        lv_coord_t y1 = obj_coords.y1;

        lv_coord_t w = lv_obj_get_width(obj);
        lv_coord_t h = lv_obj_get_height(obj);

        lv_point_t main_a, main_b;
        lv_point_t peak_a, peak_b;

        if (!spectrum->filled) {
            main_b.x = x1;
            main_b.y = y1 + h;
        }

        peak_b.x = x1;
        peak_b.y = y1 + h;

        float range = spectrum->max - spectrum->min;

        for (uint16_t i = 0; i < spectrum->data_size; i++) {
            float       v = (spectrum->data_buf[i] - spectrum->min) / range;
            int32_t     x = i * w / spectrum->data_size;

            /* Peak */

            if (spectrum->peak_on) {
                float v_peak = (spectrum->peak_buf[i].val - spectrum->min) / range;

                peak_a.x = x1 + x;
                peak_a.y = y1 + (1.0f - v_peak) * h;

                peak_line_dsc.p1.x = peak_a.x;
                peak_line_dsc.p1.y = peak_a.y;
                peak_line_dsc.p2.x = peak_b.x;
                peak_line_dsc.p2.y = peak_b.y;
                lv_draw_line(layer, &peak_line_dsc);

                peak_b = peak_a;
            }

            /* Main */

            main_a.x = x1 + x;
            main_a.y = y1 + (1.0f - v) * h;

            if (spectrum->filled) {
                main_b.x = main_a.x;
                main_b.y = y1 + h;
            }

            main_line_dsc.p1.x = main_a.x;
            main_line_dsc.p1.y = main_a.y;
            main_line_dsc.p2.x = main_b.x;
            main_line_dsc.p2.y = main_b.y;
            lv_draw_line(layer, &main_line_dsc);

            if (!spectrum->filled) {
                main_b = main_a;
            }
        }
    }
}
