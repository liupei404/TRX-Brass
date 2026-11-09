/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_hiding.h"
#include "lv_hiding_private.h"
#include "lvgl/src/core/lv_obj_class_private.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_hiding_class

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_hiding_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_hiding_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void msg_timer(lv_timer_t *t);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_hiding_class  = {
    .constructor_cb = lv_hiding_constructor,
    .destructor_cb = lv_hiding_destructor,
    .base_class = &lv_obj_class,
    .instance_size = sizeof(lv_hiding_t),
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_hiding_create(lv_obj_t * parent) {
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);

    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_hiding_set_timeout(lv_obj_t * obj, uint16_t timeout) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_hiding_t * hiding = (lv_hiding_t *) obj;

    hiding->timeout = timeout;
}

void lv_hiding_touch(lv_obj_t * obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_hiding_t * hiding = (lv_hiding_t *) obj;

    if (!hiding->fade_run) {
        hiding->fade_run = true;
        lv_anim_set_values(&hiding->fade, lv_obj_get_style_opa(obj, 0), LV_OPA_COVER);
        lv_anim_start(&hiding->fade);
    }

    if (hiding->timer) {
        lv_timer_reset(hiding->timer);
    } else {
        hiding->timer = lv_timer_create(msg_timer, hiding->timeout, hiding);
        lv_timer_set_repeat_count(hiding->timer, 1);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void msg_timer(lv_timer_t *t) {
    lv_hiding_t * hiding = lv_timer_get_user_data(t);

    lv_anim_set_values(&hiding->fade, lv_obj_get_style_opa((lv_obj_t *) hiding, 0), LV_OPA_TRANSP);
    lv_anim_start(&hiding->fade);
    hiding->timer = NULL;
}

static void fade_anim(void * obj, int32_t v) {
    lv_obj_set_style_opa(obj, v, 0);
}

static void fade_ready(lv_anim_t * a) {
    lv_hiding_t * hiding = (lv_hiding_t *) lv_anim_get_user_data(a);

    hiding->fade_run = false;
}

static void lv_hiding_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_hiding_t * hiding = (lv_hiding_t *)obj;

    hiding->timeout = 5000;
    hiding->timer = NULL;

    lv_anim_init(&hiding->fade);
    lv_anim_set_var(&hiding->fade, obj);
    lv_anim_set_time(&hiding->fade, 250);
    lv_anim_set_exec_cb(&hiding->fade, fade_anim);
    lv_anim_set_ready_cb(&hiding->fade, fade_ready);
    lv_anim_set_user_data(&hiding->fade, obj);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_hiding_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
    LV_UNUSED(class_p);
    lv_hiding_t * hiding = (lv_hiding_t *)obj;

    if (hiding->timer != NULL) {
        lv_timer_delete(hiding->timer);
        hiding->timer = NULL;
    }
    lv_anim_delete(obj, NULL);
}
