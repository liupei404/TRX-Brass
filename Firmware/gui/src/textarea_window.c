/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "events.h"
#include "keyboard.h"
#include "textarea_window.h"
#include "styles.h"
#include "dsp.h"
#include "main.h"

static lv_obj_t             *window = NULL;
static lv_obj_t             *text = NULL;
static lv_obj_t             *keyboard = NULL;

static textarea_window_cb_t ok_cb = NULL;
static textarea_window_cb_t cancel_cb = NULL;

static void ok() {
    if (ok_cb) {
        ok_cb();
        ok_cb = NULL;
    }

    textarea_window_close();
}

static void cancel() {
    if (cancel_cb) {
        cancel_cb();
        cancel_cb = NULL;
    }

    textarea_window_close();
}

static void text_cb(lv_event_t * e) {
    uint32_t key = *((uint32_t *)lv_event_get_param(e));

    switch (key) {
        case HKEY_FINP:
        case LV_KEY_ENTER:
        case LV_KEY_END:
            ok();
            break;

        case LV_KEY_ESC:
            cancel();
            break;

        case KEY_VOL_LEFT_EDIT:
        case KEY_VOL_LEFT_SELECT:
            dsp_change_vol(-1);
            break;

        case KEY_VOL_RIGHT_EDIT:
        case KEY_VOL_RIGHT_SELECT:
            dsp_change_vol(1);
            break;
    }
}

static void keyboard_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    switch (code) {
        case LV_EVENT_KEY: {
            uint32_t key = *((uint32_t *)lv_event_get_param(e));

            switch (key) {
                case KEY_VOL_LEFT_EDIT:
                case KEY_VOL_LEFT_SELECT:
                    dsp_change_vol(-1);
                    break;

                case KEY_VOL_RIGHT_EDIT:
                case KEY_VOL_RIGHT_SELECT:
                    dsp_change_vol(1);
                    break;

                case LV_KEY_ESC:
                    cancel();
                    break;
            }
        } break;

        case LV_EVENT_READY:
            ok();
            break;

        case LV_EVENT_CANCEL:
            cancel();
            break;
    }
}

lv_obj_t * textarea_window_open(textarea_window_cb_t ok, textarea_window_cb_t cancel) {
    ok_cb = ok;
    cancel_cb = cancel;

    window = lv_obj_create(lv_scr_act());

    lv_obj_remove_style_all(window);
    lv_obj_add_style(window, msg_style, 0);
    lv_obj_clear_flag(window, LV_OBJ_FLAG_SCROLLABLE);

    text = lv_textarea_create(window);

    lv_obj_remove_style_all(text);
    lv_obj_add_style(text, msg_label_normal_style, LV_PART_MAIN);
    lv_obj_add_style(text, msg_label_select_style, LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_add_style(text, dialog_item_focus_style, LV_PART_CURSOR);
    lv_obj_clear_flag(text, LV_OBJ_FLAG_SCROLLABLE);

    lv_textarea_set_one_line(text, true);
    lv_textarea_set_max_length(text, 40);

    if (ok || cancel) {
        lv_obj_add_event_cb(text, text_cb, LV_EVENT_KEY, NULL);
    }

    if (!keyboard_ready()) {
        keyboard = lv_keyboard_create(lv_scr_act());

        lv_keyboard_set_textarea(keyboard, text);
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_UPPER);

        lv_obj_add_event_cb(keyboard, keyboard_cb, LV_EVENT_READY, NULL);
        lv_obj_add_event_cb(keyboard, keyboard_cb, LV_EVENT_CANCEL, NULL);
        lv_obj_add_event_cb(keyboard, keyboard_cb, LV_EVENT_KEY, NULL);

        lv_obj_remove_style_all(keyboard);
        lv_obj_add_style(keyboard, keyboard_style, LV_PART_MAIN);
        lv_obj_add_style(keyboard, dialog_item_style, LV_PART_ITEMS);
        lv_obj_add_style(keyboard, dialog_item_focus_style, LV_STATE_FOCUSED | LV_PART_ITEMS);

        lv_group_add_obj(keyboard_group, keyboard);
    } else {
        keyboard = NULL;
    }

    lv_group_add_obj(keyboard_group, text);

    return window;
}

void textarea_window_close() {
    if (keyboard) {
        lv_obj_del(keyboard);
        keyboard = NULL;
    }

    if (window) {
        lv_obj_del(window);
        window = NULL;
    }
}

const char* textarea_window_get() {
    return lv_textarea_get_text(text);
}

void textarea_window_set(const char *val) {
    lv_textarea_set_text(text, val);
}

lv_obj_t * textarea_window_text() {
    return text;
}
