/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#ifndef LV_HIDING_H
#define LV_HIDING_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "lvgl/lvgl.h"

/**********************
 *      TYPEDEFS
 **********************/

typedef struct lv_hiding_t lv_hiding_t;

extern const lv_obj_class_t lv_hiding_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t * lv_hiding_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

void lv_hiding_set_timeout(lv_obj_t * obj, uint16_t timeout);
void lv_hiding_touch(lv_obj_t * obj);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
