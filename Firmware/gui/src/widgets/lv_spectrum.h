/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  Copyright (c) 2022-2024 Belousov Oleg aka R1CBU
 */

#ifndef LV_SPECTRUM_H
#define LV_SPECTRUM_H

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

typedef struct {
    float       val;
    uint32_t    time;
} lv_spectrum_peak_t;

typedef struct lv_spectrum_t lv_spectrum_t;

enum {
    LV_PART_SPECTRUM_PEAK = LV_PART_CUSTOM_FIRST,
};

extern const lv_obj_class_t lv_spectrum_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t * lv_spectrum_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

void lv_spectrum_set_data_size(lv_obj_t * obj, uint16_t size);
void lv_spectrum_clear_data(lv_obj_t * obj);
void lv_spectrum_scroll_data(lv_obj_t * obj, int32_t df);
void lv_spectrum_add_data(lv_obj_t * obj, float * data, size_t size);
void lv_spectrum_set_span(lv_obj_t * obj, int32_t hz);
void lv_spectrum_set_max(lv_obj_t * obj, int16_t db);
void lv_spectrum_set_min(lv_obj_t * obj, int16_t db);
void lv_spectrum_set_filled(lv_obj_t * obj, bool on);
void lv_spectrum_set_peak(lv_obj_t * obj, bool on);
void lv_spectrum_set_peak_hold(lv_obj_t * obj, uint16_t ms);
void lv_spectrum_set_peak_speed(lv_obj_t * obj, float db);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
