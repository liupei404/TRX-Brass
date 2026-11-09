#pragma once

#include "lv_hiding.h"
#include "lvgl/src/core/lv_obj_private.h"

struct lv_hiding_t {
    lv_obj_t    obj;
    lv_timer_t  *timer;
    lv_anim_t   fade;
    bool        fade_run;
    uint16_t    timeout;
};
