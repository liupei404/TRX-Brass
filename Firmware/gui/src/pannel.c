/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pannel.h"
#include "styles.h"
#include "util.h"
#include "events.h"
#include "radio.h"
#include "rtty.h"
#include "msgs.h"
#include "queue.h"
#include "settings/op_work.h"
#include "settings/options.h"

static lv_obj_t     *obj;
static char         buf[1024];
static char         tmp_buf[1024];
static char         *last_line;

static lv_timer_t   *timer = NULL;
static lv_anim_t    fade;
static bool         fade_run = false;

static bool visible();

static void msg_timer(lv_timer_t *t) {
    if (visible()) {
        lv_anim_set_values(&fade, lv_obj_get_style_opa_layered(obj, 0), LV_OPA_COVER);
        lv_anim_start(&fade);
    }

    timer = NULL;
}

static void fade_anim(void * obj, int32_t v) {
    lv_obj_set_style_opa_layered(obj, v, 0);
}

static void fade_ready(lv_anim_t * a) {
    fade_run = false;
}

static void check_lines() {
    char        *second_line = NULL;
    char        *ptr = (char *) &buf;
    uint16_t    count = 0;

    while (*ptr) {
        if (*ptr == '\n') {
            count++;

            if (count == 1) {
                second_line = ptr + 1;
            }
        }
        ptr++;
    }

    if (count > 6) {
        strcpy(tmp_buf, second_line);
        strcpy(buf, tmp_buf);
    }

    ptr = (char *) &buf;

    while (*ptr) {
        if (*ptr == '\n') {
            last_line = ptr + 1;
        }
        ptr++;
    }

    *last_line = '\0';
}

static void pannel_update_cb(lv_event_t * e) {
    lv_point_t line_size;
    lv_point_t text_size;

    char *text = lv_event_get_param(e);

    if (strcmp(text, "\n") == 0) {
        if (last_line[strlen(last_line) - 1] != '\n') {
            strcat(last_line, text);
            check_lines();
        }
    } else {
        lv_draw_label_dsc_t dsc_label;

        lv_draw_label_dsc_init(&dsc_label);
        lv_obj_init_draw_label_dsc(obj, LV_PART_MAIN, &dsc_label);

        lv_txt_get_size(&line_size, last_line, dsc_label.font, 0, 0, LV_COORD_MAX, 0);
        lv_txt_get_size(&text_size, text, dsc_label.font, 0, 0, LV_COORD_MAX, 0);

        if (line_size.x + text_size.x > (lv_obj_get_width(obj) - 40)) {
            strcat(last_line, "\n");
            check_lines();
        }
        strcat(last_line, text);
    }

    lv_label_set_text_static(obj, buf);
}

static void mode_changed_cb(void *s, brass_msg_t *m) {
    pannel_visible();
}

lv_obj_t * pannel_init(lv_obj_t *parent) {
    obj = lv_label_create(parent);

    lv_obj_add_style(obj, pannel_style, LV_PART_MAIN);
    lv_obj_add_event_cb(obj, pannel_update_cb, EVENT_PANNEL_UPDATE, NULL);
    lv_obj_set_style_opa_layered(obj, LV_OPA_TRANSP, 0);

    brass_msg_subscribe(MSG_MODE_CHANGED, mode_changed_cb, NULL);

    /* Fade */

    lv_anim_init(&fade);
    lv_anim_set_var(&fade, obj);
    lv_anim_set_time(&fade, 250);
    lv_anim_set_exec_cb(&fade, fade_anim);
    lv_anim_set_ready_cb(&fade, fade_ready);

    return obj;
}

void pannel_add_text(const char * text) {
    queue_send(obj, EVENT_PANNEL_UPDATE, strdup(text));
}

static bool visible() {
    switch (op_work->mode) {
        case RADIO_MODE_CW:
        case RADIO_MODE_CWR:
            return options->cw.decoder;

        case RADIO_MODE_RTTY:
        case RADIO_MODE_OLIVIA:
            return true;

        default:
            break;
    }

    return false;
}

void pannel_visible() {
    if (visible()) {
        strcpy(buf, "");
        last_line = (char *) &buf;
        lv_label_set_text_static(obj, buf);
        lv_obj_set_style_opa_layered(obj, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_opa_layered(obj, LV_OPA_TRANSP, 0);
    }
}

void pannel_fade() {
    if (!visible()) {
        return;
    }

    if (!fade_run) {
        fade_run = true;
        lv_anim_set_values(&fade, lv_obj_get_style_opa_layered(obj, 0), LV_OPA_TRANSP);
        lv_anim_start(&fade);
    }

    if (timer) {
        lv_timer_reset(timer);
    } else {
        timer = lv_timer_create(msg_timer, 1000, NULL);
        lv_timer_set_repeat_count(timer, 1);
    }
}
