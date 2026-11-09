/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#ifndef LV_BANDINFO_H
#define LV_BANDINFO_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "lvgl/lvgl.h"
#include "lv_hiding.h"

/**********************
 *      TYPEDEFS
 **********************/

typedef struct lv_bandinfo_t lv_bandinfo_t;

enum {
    LV_PART_BANDINFO_MARKER = LV_PART_CUSTOM_FIRST,
};

extern const lv_obj_class_t lv_bandinfo_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t * lv_bandinfo_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

void lv_bandinfo_set_span(lv_obj_t * obj, int32_t hz);
void lv_bandinfo_set_center(lv_obj_t * obj, uint64_t hz);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
