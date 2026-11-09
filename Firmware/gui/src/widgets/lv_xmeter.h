/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#ifndef LV_XMETER_H
#define LV_XMETER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/

#define LV_SMETER_LABELS 7

enum {
    LV_PART_METER_1     = LV_PART_CUSTOM_FIRST,
    LV_PART_METER_2     = LV_PART_CUSTOM_FIRST + 0x10000,
    LV_PART_METER_3     = LV_PART_CUSTOM_FIRST + 0x20000,
    LV_PART_METER_4     = LV_PART_CUSTOM_FIRST + 0x30000
};

extern const lv_obj_class_t lv_xmeter_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

lv_obj_t * lv_xmeter_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

void lv_xmeter_set_value(lv_obj_t * obj, float value);
void lv_xmeter_set_slice(lv_obj_t * obj, float value);
void lv_xmeter_set_part(lv_obj_t * obj, uint8_t index, float value);
void lv_xmeter_set_label(lv_obj_t * obj, uint8_t index, const char *text, float value);
void lv_xmeter_set_range(lv_obj_t * obj, float min, float max);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
