/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  Copyright (c) 2022-2023 Belousov Oleg aka R1CBU
 */

#ifndef LV_FINDER_H
#define LV_FINDER_H

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

#define LV_FINDER_CURSORS    3

typedef struct lv_finder_t lv_finder_t;

extern const lv_obj_class_t lv_finder_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t * lv_finder_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

void lv_finder_set_range(lv_obj_t * obj, uint64_t freq_min, uint64_t freq_max);
void lv_finder_set_span(lv_obj_t * obj, int32_t hz);
void lv_finder_set_center(lv_obj_t * obj, uint64_t hz);

void lv_finder_set_cursor_num(lv_obj_t * obj, uint8_t num);
void lv_finder_set_cursor(lv_obj_t * obj, uint8_t index, int16_t hz);
void lv_finder_set_width(lv_obj_t * obj, uint16_t hz);
void lv_finder_set_offsets(lv_obj_t * obj, int16_t min, int16_t max);
void lv_finder_set_value(lv_obj_t * obj, uint64_t freq);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
