/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  Copyright (c) 2022-2023 Belousov Oleg aka R1CBU
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_finder.h"
#include "lvgl/src/core/lv_obj_class_private.h"
#include "lvgl/src/core/lv_obj_private.h"

struct lv_finder_t {
    lv_obj_t        obj;

    int16_t         offset_min;
    int16_t         offset_max;
    uint64_t        value;

    uint64_t        range_min;
    uint64_t        range_max;
    uint64_t        center;
    int32_t         span;

    uint8_t         cursor_num;
    int16_t         cursor[LV_FINDER_CURSORS];

    lv_coord_t      prev_x1;
    lv_coord_t      prev_x2;
};

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_finder_class

/**********************
 *  STATIC PROTOTYPES
 **********************/
 
static void lv_finder_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_finder_event(const lv_obj_class_t * class_p, lv_event_t * e);

static void finder_invalidate(lv_obj_t * obj);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_finder_class = {
    .constructor_cb = lv_finder_constructor,
    .base_class = &lv_obj_class,
    .event_cb = lv_finder_event,
    .instance_size = sizeof(lv_finder_t),
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_finder_create(lv_obj_t * parent) {
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);

    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_finder_set_range(lv_obj_t * obj, uint64_t freq_min, uint64_t freq_max) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_finder_t * finder = (lv_finder_t *)obj;

    finder->range_min = freq_min;
    finder->range_max = freq_max;
    finder->center = (freq_min + freq_max) / 2;
    finder->span = freq_max - freq_min;

    finder_invalidate(obj);
}

void lv_finder_set_span(lv_obj_t * obj, int32_t hz) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_finder_t * finder = (lv_finder_t *)obj;

    finder->span = hz;
    finder->range_min = finder->center - finder->span / 2;
    finder->range_max = finder->center + finder->span / 2;

    finder_invalidate(obj);
}

void lv_finder_set_center(lv_obj_t * obj, uint64_t hz) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_finder_t * finder = (lv_finder_t *)obj;

    finder->center = hz;
    finder->range_min = finder->center - finder->span / 2;
    finder->range_max = finder->center + finder->span / 2;

    finder_invalidate(obj);
}

void lv_finder_set_cursor_num(lv_obj_t * obj, uint8_t num) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_finder_t * finder = (lv_finder_t *)obj;

    if (num > 0 && num <= LV_FINDER_CURSORS) {
        finder->cursor_num = num;
    }
}

void lv_finder_set_cursor(lv_obj_t * obj, uint8_t index, int16_t hz) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_finder_t * finder = (lv_finder_t *)obj;

    if (index > 0 && index <= LV_FINDER_CURSORS) {
        finder->cursor[index - 1] = hz;
    }

    finder_invalidate(obj);
}

void lv_finder_set_width(lv_obj_t * obj, uint16_t hz) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_finder_t * finder = (lv_finder_t *)obj;

    finder->offset_min = -hz/2;
    finder->offset_max = hz/2;

    finder_invalidate(obj);
}

void lv_finder_set_offsets(lv_obj_t * obj, int16_t min, int16_t max) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_finder_t * finder = (lv_finder_t *)obj;

    finder->offset_min = min;
    finder->offset_max = max;

    finder_invalidate(obj);
};

void lv_finder_set_value(lv_obj_t * obj, uint64_t freq) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_finder_t * finder = (lv_finder_t *)obj;

    finder->value = freq;

    finder_invalidate(obj);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void finder_invalidate(lv_obj_t * obj) {
    lv_finder_t * finder = (lv_finder_t *)obj;
    lv_area_t area;

    lv_coord_t      x1 = obj->coords.x1;
    lv_coord_t      y1 = obj->coords.y1;
    lv_coord_t      w = lv_obj_get_width(obj);
    lv_coord_t      h = lv_obj_get_height(obj);
    uint16_t        border = lv_obj_get_style_border_width(obj, LV_PART_INDICATOR);

    int32_t         size_hz = finder->range_max - finder->range_min;
    int64_t         f = finder->value - finder->range_min;
    int64_t         f1 = w * (f + finder->offset_min) / size_hz;
    int64_t         f2 = w * (f + finder->offset_max) / size_hz;

    lv_coord_t      new_x1 = x1 + f1;
    lv_coord_t      new_x2 = x1 + f2;

    area.y1 = y1 + border;
    area.y2 = area.y1 + h - border * 2;

    if (finder->prev_x1 < new_x1) {
        area.x1 = finder->prev_x1;
    } else {
        area.x1 = new_x1;
    }

    if (finder->prev_x2 > new_x2) {
        area.x2 = finder->prev_x2;
    } else {
        area.x2 = new_x2;
    }

    lv_obj_invalidate_area(obj, &area);

    finder->prev_x1 = new_x1;
    finder->prev_x2 = new_x2;
}

static void lv_finder_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_finder_t * finder = (lv_finder_t *)obj;

    finder->value = 1000;
    finder->offset_min = -25;
    finder->offset_max = 25;
    finder->range_min = 50;
    finder->range_max = 3000;
    finder->cursor_num = 0;

    finder->center = (50 + 3000) / 2;
    finder->span = 3000 - 50;

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_finder_event(const lv_obj_class_t * class_p, lv_event_t * e) {
    LV_UNUSED(class_p);

    lv_res_t res = lv_obj_event_base(MY_CLASS, e);

    if (res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if (code == LV_EVENT_DRAW_MAIN_END) {
        lv_finder_t     *finder = (lv_finder_t *) obj;
        lv_layer_t      *layer = lv_event_get_layer(e);
        lv_area_t       area;

        lv_area_t       obj_coords;
        lv_obj_get_coords(obj, &obj_coords);
        lv_coord_t      x1 = obj_coords.x1;
        lv_coord_t      y1 = obj_coords.y1;
        lv_coord_t      w = lv_obj_get_width(obj);
        lv_coord_t      h = lv_obj_get_height(obj);
        uint16_t        border = lv_obj_get_style_border_width(obj, LV_PART_INDICATOR);

        int32_t         size_hz = finder->range_max - finder->range_min;
        int64_t         f = finder->value - finder->range_min;
        int64_t         f1 = w * (f + finder->offset_min) / size_hz;
        int64_t         f2 = w * (f + finder->offset_max) / size_hz;

        area.x1 = x1 + f1 + lv_obj_get_style_pad_left(obj, LV_PART_INDICATOR);
        area.y1 = y1 + border;
        area.x2 = x1 + f2 + lv_obj_get_style_pad_right(obj, LV_PART_INDICATOR);
        area.y2 = area.y1 + h - border * 2;

        /* Rectangle */

        lv_draw_rect_dsc_t  draw_dsc;

        lv_draw_rect_dsc_init(&draw_dsc);
        lv_obj_init_draw_rect_dsc(obj, LV_PART_INDICATOR, &draw_dsc);

        lv_draw_rect(layer, &draw_dsc, &area);

        /* Cursors */

        if (finder->cursor_num) {
            lv_draw_line_dsc_t  line_dsc;

            lv_draw_line_dsc_init(&line_dsc);
            lv_obj_init_draw_line_dsc(obj, LV_PART_INDICATOR, &line_dsc);

            lv_point_t a, b;

            for (uint8_t i = 0; i < finder->cursor_num; i++) {
                a.x = x1 + w * (f + finder->cursor[i]) / size_hz;
                a.y = area.y1;

                b.x = a.x;
                b.y = area.y2;

                line_dsc.p1.x = a.x;
                line_dsc.p1.y = a.y;
                line_dsc.p2.x = b.x;
                line_dsc.p2.y = b.y;
                lv_draw_line(layer, &line_dsc);
            }
        }
    }
}
