/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>

#include "lvgl/lvgl.h"
#include "dialog.h"
#include "dialog_settings.h"
#include "styles.h"
#include "backlight.h"
#include "radio.h"
#include "events.h"
#include "keyboard.h"
#include "voice.h"
#include "dsp.h"
#include "hkey.h"
#include "settings/options.h"
#include "settings/hw.h"

static time_t       now;
struct tm           ts;

static lv_obj_t     *day;
static lv_obj_t     *month;
static lv_obj_t     *year;
static lv_obj_t     *hour;
static lv_obj_t     *min;
static lv_obj_t     *sec;

static void construct_cb(lv_obj_t *parent);

static dialog_t     dialog = {
    .run = false,
    .construct_cb = construct_cb,
    .destruct_cb = NULL,
    .audio_cb = NULL,
    .buttons = false,
    .key_cb = dialog_key_cb
};

dialog_t            *dialog_settings = &dialog;

/* Shared update */

static void bool_update_cb(lv_event_t * e) {
    lv_obj_t        *obj = lv_event_get_target(e);
    bool            *var = lv_event_get_user_data(e);

    *var = lv_obj_has_state(obj, LV_STATE_CHECKED);
}

static void uint8_spinbox_update_cb(lv_event_t * e) {
    lv_obj_t        *obj = lv_event_get_target(e);
    uint8_t         *var = lv_event_get_user_data(e);

    *var = lv_spinbox_get_value(obj);
}

static void uint8_dropdown_update_cb(lv_event_t * e) {
    lv_obj_t        *obj = lv_event_get_target(e);
    uint8_t         *var = lv_event_get_user_data(e);

    *var = lv_dropdown_get_selected(obj);
}

/* Shared create */

static lv_obj_t * switch_bool(lv_obj_t *parent, uint8_t span, bool *var) {
    lv_obj_t *obj = dialog_switch(&dialog, span);

    lv_obj_center(obj);
    lv_obj_add_event_cb(obj, bool_update_cb, LV_EVENT_VALUE_CHANGED, var);

    if (*var) {
        lv_obj_add_state(obj, LV_STATE_CHECKED);
    }

    return obj;
}

static lv_obj_t * spinbox_uint8(lv_obj_t *parent, uint8_t span, uint8_t *var, uint8_t min, uint8_t max) {
    lv_obj_t *obj = lv_spinbox_create(parent);

    dialog_item(&dialog, obj, span);

    lv_spinbox_set_value(obj, *var);
    lv_spinbox_set_range(obj, min, max);
    lv_obj_add_event_cb(obj, uint8_spinbox_update_cb, LV_EVENT_VALUE_CHANGED, var);

    return obj;
}

static lv_obj_t * dropdown_uint8(lv_obj_t *parent, uint8_t span, uint8_t *var, const char *options) {
    lv_obj_t *obj;

    obj = dialog_dropdown(&dialog, span);

    lv_obj_add_event_cb(obj, uint8_dropdown_update_cb, LV_EVENT_VALUE_CHANGED, var);

    lv_dropdown_set_options(obj, options);
    lv_dropdown_set_selected(obj, *var);

    return obj;
}

/* Datetime */

static void datetime_update_cb(lv_event_t * e) {
    ts.tm_mday = lv_spinbox_get_value(day);
    ts.tm_mon = lv_spinbox_get_value(month) - 1;
    ts.tm_year = lv_spinbox_get_value(year) - 1900;
    ts.tm_hour = lv_spinbox_get_value(hour);
    ts.tm_min = lv_spinbox_get_value(min);
    ts.tm_sec = lv_spinbox_get_value(sec);

    /* Set system */

    struct timespec tp;

    tp.tv_sec = mktime(&ts);
    tp.tv_nsec = 0;

    clock_settime(CLOCK_REALTIME, &tp);

    /* Set RTC */

    int rtc = open("/dev/rtc1", O_WRONLY);

    if (rtc > 0) {
        ioctl(rtc, RTC_SET_TIME, &ts);
        close(rtc);
    } else {
        LV_LOG_ERROR("Set RTC");
    }
}

static void make_date() {
    lv_obj_t    *obj;

    dialog_label(&dialog, false, "Day, Month, Year");

    /* * */

    obj = lv_spinbox_create(dialog.grid);
    day = obj;

    dialog_item(&dialog, obj, 2);

    lv_spinbox_set_value(obj, ts.tm_mday);
    lv_spinbox_set_range(obj, 1, 31);
    lv_spinbox_set_digit_format(obj, 2, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, datetime_update_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* * */

    obj = lv_spinbox_create(dialog.grid);
    month = obj;

    dialog_item(&dialog, obj, 2);

    lv_spinbox_set_value(obj, ts.tm_mon + 1);
    lv_spinbox_set_range(obj, 1, 12);
    lv_spinbox_set_digit_format(obj, 2, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, datetime_update_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* * */

    obj = lv_spinbox_create(dialog.grid);
    year = obj;

    dialog_item(&dialog, obj, 2);

    lv_spinbox_set_value(obj, ts.tm_year + 1900);
    lv_spinbox_set_range(obj, 2020, 2038);
    lv_spinbox_set_digit_format(obj, 4, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, datetime_update_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void make_time() {
    lv_obj_t    *obj;

    dialog_label(&dialog, false, "Hour, Min, Sec");

    /* * */

    obj = lv_spinbox_create(dialog.grid);
    hour = obj;

    dialog_item(&dialog, obj, 2);

    lv_spinbox_set_value(obj, ts.tm_hour);
    lv_spinbox_set_range(obj, 0, 23);
    lv_spinbox_set_digit_format(obj, 2, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, datetime_update_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* * */

    obj = lv_spinbox_create(dialog.grid);
    min = obj;

    dialog_item(&dialog, obj, 2);

    lv_spinbox_set_value(obj, ts.tm_min);
    lv_spinbox_set_range(obj, 0, 59);
    lv_spinbox_set_digit_format(obj, 2, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, datetime_update_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* * */

    obj = lv_spinbox_create(dialog.grid);
    sec = obj;

    dialog_item(&dialog, obj, 2);

    lv_spinbox_set_value(obj, ts.tm_sec);
    lv_spinbox_set_range(obj, 0, 59);
    lv_spinbox_set_digit_format(obj, 2, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, datetime_update_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/* Long press actions */

typedef struct {
    char                *label;
    options_action_t    action;
} action_items_t;

static action_items_t long_action_items[] = {
    { .label = " None ", .action = ACTION_NONE },
    { .label = " Screenshot ", .action = ACTION_SCREENSHOT },
    { .label = " Recorder on/off ", .action = ACTION_RECORDER },
    { .label = " Mute ", .action = ACTION_MUTE },
    { .label = " Voice mode ", .action = ACTION_VOICE_MODE },
    { .label = " APP RTTY ", .action = ACTION_APP_RTTY },
    { .label = " APP FT8 ", .action = ACTION_APP_FT8 },
    { .label = " APP SWR Scan ", .action = ACTION_APP_SWRSCAN },
    { .label = " APP GPS ", .action = ACTION_APP_GPS },
    { .label = " APP Settings", .action = ACTION_APP_SETTINGS },
    { .label = " APP Recorder", .action = ACTION_APP_RECORDER },
    { .label = " QTH Grid", .action = ACTION_APP_QTH },
    { .label = " APP Freq", .action = ACTION_APP_FREQ },
    { .label = NULL, .action = ACTION_NONE }
};

static void long_action_update_cb(lv_event_t * e) {
    lv_obj_t    *obj = lv_event_get_target(e);
    uint32_t    *i = lv_event_get_user_data(e);
    uint8_t     val = long_action_items[lv_dropdown_get_selected(obj)].action;

    switch (*i) {
        case 0:
            options->control.long_vol = val;
            break;

        case 1:
            options->control.long_mfk = val;
            break;

        case 2:
            options->control.long_app = val;
            break;

        case 3:
            options->control.long_band_down = val;
            break;

        case 4:
            options->control.long_band_up = val;
            break;
    }
}

static void make_long_action() {
    char        *labels[] = { "VOL", "MFK", "APP", "Band down", "Band up" };
    lv_obj_t    *obj;

    dialog_title(&dialog, "Long press actions");

    for (uint8_t i = 0; i < 5; i++) {
        dialog_label(&dialog, true, labels[i]);

        obj = dialog_dropdown(&dialog, 6);

        uint8_t x;

        switch (i) {
            case 0: x = options->control.long_vol;          break;
            case 1: x = options->control.long_mfk;          break;
            case 2: x = options->control.long_app;          break;
            case 3: x = options->control.long_band_down;    break;
            case 4: x = options->control.long_band_up;      break;

            default:
                x = ACTION_NONE;
                break;
        }

        lv_dropdown_clear_options(obj);

        uint8_t n = 0;

        while (long_action_items[n].label) {
            lv_dropdown_add_option(obj, long_action_items[n].label, LV_DROPDOWN_POS_LAST);

            if (long_action_items[n].action == x) {
                lv_dropdown_set_selected(obj, n);
            }

            n++;
        }

        uint32_t *param = malloc(sizeof(uint32_t));
        *param = i;

        lv_obj_add_event_cb(obj, long_action_update_cb, LV_EVENT_VALUE_CHANGED, param);
    }
}

/* HMic */

static void hmic_update_cb(lv_event_t * e) {
    lv_obj_t    *obj = lv_event_get_target(e);
    char        *val = hw->hmic[lv_dropdown_get_selected(obj)].label;

    if (options->hkeys.hmic) {
        free(options->hkeys.hmic);
    }

    options->hkeys.hmic = strdup(val);
    hkey_mic_update();
}

static action_items_t hmic_action_items[] = {
    { .label = " None ", .action = ACTION_NONE },
    { .label = " Recorder on/off ", .action = ACTION_RECORDER },
    { .label = " Mute ", .action = ACTION_MUTE },
    { .label = " Step up ", .action = ACTION_STEP_UP },
    { .label = " Step down ", .action = ACTION_STEP_DOWN },
    { .label = " Voice mode ", .action = ACTION_VOICE_MODE },
    { .label = " APP Freq", .action = ACTION_APP_FREQ },
    { .label = NULL, .action = ACTION_NONE }
};

static void hmic_action_update_cb(lv_event_t * e) {
    lv_obj_t    *obj = lv_event_get_target(e);
    uint32_t    *i = lv_event_get_user_data(e);
    uint8_t     val = hmic_action_items[lv_dropdown_get_selected(obj)].action;

    switch (*i) {
        case 0 ... 3:
            options->hkeys.press_p[*i] = val;
            break;

        case 4 ... 7:
            options->hkeys.press_char[*i - 4] = val;
            break;
    }
}

static void make_hmic() {
    lv_obj_t    *obj;

    dialog_label(&dialog, false, "HMic");

    obj = dialog_dropdown(&dialog, 6);
    lv_dropdown_clear_options(obj);

    for (uint16_t i = 0; i < hw->hmic_count; i++) {
        hmic_item_t *item = &hw->hmic[i];

        lv_dropdown_add_option(obj, item->label, LV_DROPDOWN_POS_LAST);

        if (strcmp(item->label, options->hkeys.hmic) == 0) {
            lv_dropdown_set_selected(obj, i);
        }
    }

    lv_obj_add_event_cb(obj, hmic_update_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* * */

    char        *labels[] = { "P1", "P2", "P3", "P4", "A", "B", "C", "D" };

    dialog_title(&dialog, "HMic press actions");

    for (uint8_t i = 0; i < 8; i++) {
        dialog_label(&dialog, true, labels[i]);

        obj = dialog_dropdown(&dialog, 6);

        uint8_t x;

        switch (i) {
            case 0 ... 3:
                x = options->hkeys.press_p[i];
                break;

            case 4 ... 7:
                x = options->hkeys.press_char[i - 4];
                break;

            default:
                x = ACTION_NONE;
                break;
        }

        lv_dropdown_clear_options(obj);

        uint8_t n = 0;

        while (hmic_action_items[n].label) {
            lv_dropdown_add_option(obj, hmic_action_items[n].label, LV_DROPDOWN_POS_LAST);

            if (hmic_action_items[n].action == x) {
                lv_dropdown_set_selected(obj, n);
            }

            n++;
        }

        uint32_t *param = malloc(sizeof(uint32_t));
        *param = i;

        lv_obj_add_event_cb(obj, hmic_action_update_cb, LV_EVENT_VALUE_CHANGED, param);
    }
}

/* Voice */

static void make_voice() {
    lv_obj_t    *obj;

    dialog_title(&dialog, "Voice");

    dialog_label(&dialog, true, "Mode");

    obj = dropdown_uint8(dialog.grid, 6, &options->voice.mode, " Off \n When LCD off \n Always");

    /* * */

    dialog_label(&dialog, true, "Rate, pitch, volume");

    obj = spinbox_uint8(dialog.grid, 2, &options->voice.rate, 50, 150);

    lv_spinbox_set_digit_format(obj, 3, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    obj = spinbox_uint8(dialog.grid, 2, &options->voice.pitch, 50, 150);

    lv_spinbox_set_digit_format(obj, 3, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    obj = spinbox_uint8(dialog.grid, 2, &options->voice.volume, 50, 150);

    lv_spinbox_set_digit_format(obj, 3, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);
}

static void make_freq_accel() {
    lv_obj_t    *obj;

    dialog_label(&dialog, false, "Freq acceleration");

    obj = dropdown_uint8(dialog.grid, 6, &options->freq.accel, " None \n Lite \n Strong");
}

/* Backlight */

static void backlight_timeout_update_cb(lv_event_t * e) {
    lv_obj_t *obj = lv_event_get_target(e);

///    params.brightness_timeout = lv_spinbox_get_value(obj);

    backlight_tick();
}

static void backlight_brightness_update_cb(lv_event_t * e) {
    lv_obj_t *obj = lv_event_get_target(e);

///    params.brightness_normal = lv_slider_get_value(obj);
///    params.brightness_idle = lv_slider_get_left_value(obj);
///    backlight_set_brightness(params.brightness_normal);
}

static void make_backlight() {
    lv_obj_t    *obj;

    dialog_label(&dialog, false, "Timeout, Brightness");

    /* * */

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 2);

///    lv_spinbox_set_value(obj, params.brightness_timeout);
    lv_spinbox_set_range(obj, 5, 120);
    lv_spinbox_set_digit_format(obj, 3, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, backlight_timeout_update_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* * */

    obj = dialog_slider(&dialog, 4);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_slider_set_mode(obj, LV_SLIDER_MODE_RANGE);

///    lv_slider_set_value(obj, params.brightness_normal, LV_ANIM_OFF);
///    lv_slider_set_left_value(obj, params.brightness_idle, LV_ANIM_OFF);
    lv_slider_set_range(obj, 0, 9);
    lv_obj_add_event_cb(obj, backlight_brightness_update_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/* Mag Freq, Info, ALC */

static void make_mag() {
    lv_obj_t    *obj;

    dialog_label(&dialog, false, "Mag Freq, Info, ALC");

    obj = switch_bool(dialog.grid, 2, &options->mag.freq);
    obj = switch_bool(dialog.grid, 2, &options->mag.info);
    obj = switch_bool(dialog.grid, 2, &options->mag.alc);
}

/* Clock */

static void clock_view_update_cb(lv_event_t * e) {
    lv_obj_t *obj = lv_event_get_target(e);

/* FIXME    clock_set_view(lv_dropdown_get_selected(obj)); */
}

static void clock_time_timeout_update_cb(lv_event_t * e) {
    lv_obj_t *obj = lv_event_get_target(e);

/* FIXME    clock_set_time_timeout(lv_spinbox_get_value(obj)); */
}

static void clock_power_timeout_update_cb(lv_event_t * e) {
    lv_obj_t *obj = lv_event_get_target(e);

/* FIXME    clock_set_power_timeout(lv_spinbox_get_value(obj)); */
}

static void clock_tx_timeout_update_cb(lv_event_t * e) {
    lv_obj_t *obj = lv_event_get_target(e);

/* FIXME    clock_set_tx_timeout(lv_spinbox_get_value(obj)); */
}

static void make_clock() {
    lv_obj_t    *obj;

    dialog_title(&dialog, "Clock");

    dialog_label(&dialog, true, "View");

    /* * */

    obj = dialog_dropdown(&dialog, 6);

    lv_dropdown_set_options(obj, " Always Time \n Time and Power \n Always Power");
///    lv_dropdown_set_selected(obj, params.clock_view);
    lv_obj_add_event_cb(obj, clock_view_update_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* * */

    dialog_label(&dialog, true, "Timeout Clock, Power, TX");

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 2);

///    lv_spinbox_set_value(obj, params.clock_time_timeout);
    lv_spinbox_set_range(obj, 1, 59);
    lv_spinbox_set_digit_format(obj, 2, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, clock_time_timeout_update_cb, LV_EVENT_VALUE_CHANGED, NULL);

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 2);

///    lv_spinbox_set_value(obj, params.clock_power_timeout);
    lv_spinbox_set_range(obj, 1, 59);
    lv_spinbox_set_digit_format(obj, 2, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, clock_power_timeout_update_cb, LV_EVENT_VALUE_CHANGED, NULL);

    obj = lv_spinbox_create(dialog.grid);

    dialog_item(&dialog, obj, 2);

///    lv_spinbox_set_value(obj, params.clock_tx_timeout);
    lv_spinbox_set_range(obj, 0, 10);
    lv_spinbox_set_digit_format(obj, 2, 0);
    lv_spinbox_set_digit_step_direction(obj, LV_DIR_LEFT);

    lv_obj_add_event_cb(obj, clock_tx_timeout_update_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void construct_cb(lv_obj_t *parent) {
    dialog_grid(parent, &dialog);

    now = time(NULL);
    struct tm *t = localtime(&now);

    memcpy(&ts, t, sizeof(ts));

    make_date();
    make_time();
    make_freq_accel();
    make_backlight();
    make_mag();
    make_clock();
    make_long_action();
    make_hmic();
    make_voice();
}
