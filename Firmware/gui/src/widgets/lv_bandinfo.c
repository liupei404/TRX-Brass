/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

/*********************
 *      INCLUDES
 *********************/

#include <stdlib.h>
#include "lv_bandinfo.h"
#include "lv_hiding_private.h"
#include "lvgl/src/core/lv_obj_class_private.h"
#include "src/bands/bands.h"
#include "src/msgs.h"

struct lv_bandinfo_t {
    lv_hiding_t obj;
    uint64_t    center;
    int32_t     span;
};

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_bandinfo_class

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_bandinfo_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_bandinfo_event(const lv_obj_class_t * class_p, lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_bandinfo_class  = {
    .constructor_cb = lv_bandinfo_constructor,
    .base_class = &lv_hiding_class,
    .event_cb = lv_bandinfo_event,
    .instance_size = sizeof(lv_bandinfo_t),
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_bandinfo_create(lv_obj_t * parent) {
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);

    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_bandinfo_set_span(lv_obj_t * obj, int32_t hz) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_bandinfo_t * bandinfo = (lv_bandinfo_t *)obj;

    bandinfo->span = hz;
    lv_hiding_touch(obj);
}

void lv_bandinfo_set_center(lv_obj_t * obj, uint64_t hz) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_bandinfo_t * bandinfo = (lv_bandinfo_t *)obj;

    bandinfo->center = hz;
    lv_hiding_touch(obj);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_bandinfo_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_bandinfo_t * bandinfo = (lv_bandinfo_t *)obj;

    bandinfo->center = 0;
    bandinfo->span = 100000;

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_bandinfo_event(const lv_obj_class_t * class_p, lv_event_t * e) {
    LV_UNUSED(class_p);

    lv_res_t res = lv_obj_event_base(MY_CLASS, e);

    if (res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if (code == LV_EVENT_DRAW_MAIN_END) {
        band_list_t         *list = bands_list();

        if (!list) {
            return;
        }

        lv_bandinfo_t   *bandinfo = (lv_bandinfo_t *) obj;
        lv_layer_t      *layer = lv_event_get_layer(e);

        lv_area_t       obj_coords;
        lv_obj_get_coords(obj, &obj_coords);
        lv_coord_t      x1 = obj_coords.x1;
        lv_coord_t      y1 = obj_coords.y1;
        lv_coord_t      w = lv_obj_get_width(obj);
        lv_coord_t      h = lv_obj_get_height(obj) - 1;
        band_listtrav_t *trav = band_listtrav_new(list);

        for (const band_t *band = band_listtrav_first(trav); band != NULL; band = band_listtrav_next(trav)) {
            int32_t start = (int64_t)(band->settings.start - bandinfo->center) * w / bandinfo->span;
            int32_t stop = (int64_t)(band->settings.stop - bandinfo->center) * w / bandinfo->span;

            start += w / 2;
            stop += w / 2;

            if (start > w) {
                break;
            }

            if (start < 0) {
                start = 0;
            }

            if (stop > w) {
                stop = w;
            }

            lv_draw_rect_dsc_t  rect_dsc;
            lv_area_t           area;
            lv_draw_label_dsc_t dsc_label;

            lv_draw_label_dsc_init(&dsc_label);
            lv_draw_rect_dsc_init(&rect_dsc);

            if (band->settings.stop - band->settings.start >= 5000) {
                lv_obj_init_draw_rect_dsc(obj, LV_PART_INDICATOR, &rect_dsc);
                lv_obj_init_draw_label_dsc(obj, LV_PART_INDICATOR, &dsc_label);
            } else {
                lv_obj_init_draw_rect_dsc(obj, LV_PART_BANDINFO_MARKER, &rect_dsc);
                lv_obj_init_draw_label_dsc(obj, LV_PART_BANDINFO_MARKER, &dsc_label);
            }

            /* Rect */

            area.x1 = x1 + start + 2;
            area.y1 = y1;
            area.x2 = x1 + stop - 2;
            area.y2 = y1 + h;

            lv_draw_rect(layer, &rect_dsc, &area);

            /* Label */

            lv_point_t label_size;
            lv_txt_get_size(&label_size, band->settings.label, dsc_label.font, 0, 0, LV_COORD_MAX, 0);

            if (stop - start > label_size.x) {
                area.x1 = x1 + (start + stop) / 2 - label_size.x / 2;
                area.y1 = y1 + (h - label_size.y) / 2;
                area.x2 = x1 + (start + stop / 2 + label_size.x / 2);
                area.y2 = y1 + h;

                dsc_label.text = band->settings.label;
                lv_draw_label(layer, &dsc_label, &area);
            }
        }

        band_listtrav_delete(trav);
    }
}
