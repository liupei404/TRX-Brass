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
#include "lv_xmeter.h"
#include "lvgl/src/core/lv_obj_class_private.h"
#include "lvgl/src/core/lv_obj_private.h"

/*********************
 *      DEFINES
 *********************/

#define MY_CLASS    &lv_xmeter_class

typedef struct {
    char text[32];
    float value;
} lv_xmeter_label_t;

typedef struct {
    lv_obj_t obj;
    float min;
    float max;
    float value;
    float slice_value;
    float part[3];
    lv_xmeter_label_t labels[LV_SMETER_LABELS];
} lv_xmeter_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_xmeter_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_xmeter_event(const lv_obj_class_t * class_p, lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_xmeter_class  = {
    .constructor_cb = lv_xmeter_constructor,
    .base_class = &lv_obj_class,
    .event_cb = lv_xmeter_event,
    .instance_size = sizeof(lv_xmeter_t),
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_xmeter_create(lv_obj_t * parent) {
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);

    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_xmeter_set_value(lv_obj_t * obj, float value) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_xmeter_t * meter = (lv_xmeter_t *)obj;

    if (value < meter->min) {
        meter->value = meter->min;
    } else if (value > meter->max) {
        meter->value = meter->max;
    } else {
        meter->value = value;
    }

    lv_obj_invalidate(obj);
}

void lv_xmeter_set_part(lv_obj_t * obj, uint8_t index, float value) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_xmeter_t * meter = (lv_xmeter_t *)obj;

    if (index >= 3) return;
    meter->part[index] = value;
    lv_obj_invalidate(obj);
}

void lv_xmeter_set_slice(lv_obj_t * obj, float value) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_xmeter_t * meter = (lv_xmeter_t *)obj;

    meter->slice_value = value;
    lv_obj_invalidate(obj);
}

void lv_xmeter_set_label(lv_obj_t * obj, uint8_t index, const char *text, float value) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_xmeter_t         *meter = (lv_xmeter_t *)obj;
    if (index >= LV_SMETER_LABELS || text == NULL) return;
    lv_xmeter_label_t   *label = &meter->labels[index];

    label->value = value;
    strncpy(label->text, text, 31);
    label->text[31] = '\0';
    lv_obj_invalidate(obj);
}

void lv_xmeter_set_range(lv_obj_t * obj, float min, float max) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_xmeter_t *meter = (lv_xmeter_t *)obj;

    meter->min = min;
    meter->max = max;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_xmeter_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);

    lv_xmeter_t * meter = (lv_xmeter_t *)obj;

    meter->value = 0.0f;
    meter->slice_value = 1.0f;

    meter->min = 0.0f;
    meter->max = 1.0f;

    meter->part[0] = 0.0f;
    meter->part[1] = 0.5f;
    meter->part[2] = 0.1f;

    for (uint8_t i = 0; i < LV_SMETER_LABELS; i++)
        meter->labels[i].text[0] = 0;
}

static void lv_xmeter_event(const lv_obj_class_t * class_p, lv_event_t * e) {
    LV_UNUSED(class_p);

    lv_res_t res = lv_obj_event_base(MY_CLASS, e);

    if (res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t        *obj = lv_event_get_target(e);

    if (code == LV_EVENT_DRAW_MAIN_END) {
        lv_xmeter_t         *meter = (lv_xmeter_t *) obj;
        lv_layer_t          *layer = lv_event_get_layer(e);
        lv_draw_rect_dsc_t  rect_dsc;
        lv_draw_label_dsc_t label_dsc;
        lv_area_t           area;

        lv_draw_label_dsc_init(&label_dsc);

        int16_t     slice_width = lv_obj_get_style_width(obj, LV_PART_INDICATOR);
        int16_t     slice_pad = lv_obj_get_style_pad_column(obj, LV_PART_INDICATOR);

        lv_area_t obj_coords;
        lv_obj_get_coords(obj, &obj_coords);
        lv_coord_t  x1 = obj_coords.x1 + 18;
        lv_coord_t  y1 = obj_coords.y1;

        lv_coord_t  w = lv_obj_get_width(obj);
        lv_coord_t  h = lv_obj_get_height(obj);

        float range = meter->max - meter->min;
        if (range == 0.0f || meter->slice_value <= 0.0f || slice_width <= 0) return;
        lv_coord_t  len = slice_width * range / meter->slice_value;

        /* Rects */

        area.x1 = x1;
        area.y1 = y1;
        area.y2 = area.y1 + h;

        float slice_value = meter->labels[0].value;

        lv_draw_rect_dsc_init(&rect_dsc);

        while (slice_value <= meter->value) {
            if (slice_value <= meter->part[0]) {
                lv_obj_init_draw_rect_dsc(obj, LV_PART_METER_1, &rect_dsc);
            } else if (slice_value < meter->part[1]) {
                lv_obj_init_draw_rect_dsc(obj, LV_PART_METER_2, &rect_dsc);
            } else if (slice_value < meter->part[2]) {
                lv_obj_init_draw_rect_dsc(obj, LV_PART_METER_3, &rect_dsc);
            } else {
                lv_obj_init_draw_rect_dsc(obj, LV_PART_METER_4, &rect_dsc);
            }

            area.x2 = area.x1 + slice_width - slice_pad;

            lv_draw_rect(layer, &rect_dsc, &area);

            slice_value += meter->slice_value;
            area.x1 += slice_width;
        }

        /* Labels */

        lv_obj_init_draw_label_dsc(obj, LV_PART_MAIN, &label_dsc);

        lv_point_t label_size;

        for (uint8_t i = 0; i < LV_SMETER_LABELS; i++) {
            char    *text = meter->labels[i].text;

            if (*text) {
                float   value = meter->labels[i].value;

                lv_txt_get_size(&label_size, text, label_dsc.font, 0, 0, LV_COORD_MAX, 0);

                area.x1 = x1 + len * (value - meter->min) / (meter->max - meter->min) - (label_size.x / 2);
                area.y1 = y1 + h / 2 - label_size.y / 2;
                area.x2 = area.x1 + label_size.x;
                area.y2 = area.y1 + label_size.y;

                label_dsc.text = text;
                label_dsc.text_size = label_size;
                lv_draw_label(layer, &label_dsc, &area);
            }
        }
    }
}
