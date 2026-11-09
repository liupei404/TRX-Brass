/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "lvgl/lvgl.h"
#include <stdio.h>
#include <string.h>
#include "dialog.h"
#include "styles.h"
#include "main_screen.h"
#include "keyboard.h"
#include "events.h"
#include "main.h"
#include "buttons.h"
#include "dsp.h"

#define SMALL_PAD   5

#define SMALL_1     57
#define SMALL_2     (SMALL_1 * 2 + SMALL_PAD * 1)
#define SMALL_3     (SMALL_1 * 3 + SMALL_PAD * 2)
#define SMALL_4     (SMALL_1 * 4 + SMALL_PAD * 3)
#define SMALL_5     (SMALL_1 * 5 + SMALL_PAD * 4)
#define SMALL_6     (SMALL_1 * 6 + SMALL_PAD * 5)

#define SMALL_WIDTH 57

static lv_coord_t   col_dsc[] = { 740 - (SMALL_1 + SMALL_PAD) * 6, SMALL_1, SMALL_1, SMALL_1, SMALL_1, SMALL_1, SMALL_1, LV_GRID_TEMPLATE_LAST };
static lv_coord_t   row_dsc[64];

static lv_obj_t     *obj;
static dialog_t     *current_dialog = NULL;

void dialog_construct(dialog_t *dialog, lv_obj_t *parent) {
    if (dialog && !dialog->run) {
        main_screen_keys_enable(false);
        dialog->construct_cb(parent);

        if (!dialog->buttons) {
            buttons_visible(false);
        }

        if (dialog->grid) {
            row_dsc[dialog->row + 1] = LV_GRID_TEMPLATE_LAST;
            lv_obj_set_grid_dsc_array(dialog->grid, col_dsc, row_dsc);
        }

        dialog->run = true;
    }

    current_dialog = dialog;
}

void dialog_destruct() {
    if (current_dialog && current_dialog->run) {
        current_dialog->run = false;

        main_screen_keys_enable(true);
        main_screen_dialog_deleted_cb();

        if (current_dialog->destruct_cb) {
            current_dialog->destruct_cb();
        }

        if (current_dialog->obj) {
            lv_obj_del(current_dialog->obj);
            current_dialog->obj = NULL;
        }

        if (!current_dialog->buttons) {
            buttons_visible(true);
        }

        current_dialog = NULL;
    }
}

void dialog_send(lv_event_code_t event_code, void *param) {
    if (dialog_is_run()) {
        lv_obj_send_event(current_dialog->obj, event_code, param);
    }
}

bool dialog_keypad(event_keypad_t *keypad) {
    if (dialog_is_run() && current_dialog->keypad_cb) {
        return current_dialog->keypad_cb(keypad);
    }

    return false;
}

bool dialog_modulate_state() {
    if (dialog_is_run() && current_dialog->modulate_state_cb) {
        return current_dialog->modulate_state_cb();
    }

    return false;
}

size_t dialog_modulate(float complex *data, size_t max_size, radio_mode_t mode) {
    if (current_dialog->modulate_cb) {
        return current_dialog->modulate_cb(data, max_size, mode);
    }

    return 0;
}

void dialog_bands_change(bool up) {
    if (dialog_is_run() && current_dialog->bands_change_cb) {
        current_dialog->bands_change_cb(up);
    }
}

bool dialog_is_run() {
    return (current_dialog != NULL) && current_dialog->run;
}

void dialog_init(lv_obj_t *parent, dialog_t *dialog) {
    obj = lv_obj_create(parent);

    lv_obj_remove_style_all(obj);
    lv_obj_add_style(obj, dialog_style, LV_PART_MAIN);

    if (!dialog->buttons) {
        lv_obj_add_style(obj, dialog_no_buttons_style, LV_PART_MAIN);
    }

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    dialog->obj = obj;
    dialog->grid = NULL;
}

void dialog_grid(lv_obj_t *parent, dialog_t *dialog) {
    dialog_init(parent, dialog);

    lv_obj_t *grid = lv_obj_create(dialog->obj);

    lv_obj_add_style(grid, dialog_grid_style, LV_PART_MAIN);

    memset(row_dsc, 0, sizeof(row_dsc));
    row_dsc[0] = 1;
    row_dsc[1] = LV_GRID_TEMPLATE_LAST;
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);

    dialog->grid = grid;
    dialog->row = 0;
}

void dialog_item(dialog_t *dialog, lv_obj_t *obj, uint8_t span) {
    lv_obj_remove_style_all(obj);

    lv_obj_add_style(obj, dialog_item_style, LV_STATE_DEFAULT);
    lv_obj_add_style(obj, dialog_item_focus_style, LV_STATE_FOCUSED);
    lv_obj_add_style(obj, dialog_item_edited_style, LV_STATE_EDITED);
    lv_obj_add_style(obj, dialog_item_cursor_style, LV_STATE_EDITED | LV_PART_CURSOR);

    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    lv_group_add_obj(keyboard_group, obj);

    if (dialog->key_cb) {
        lv_obj_add_event_cb(obj, dialog->key_cb, LV_EVENT_KEY, NULL);
    }

    lv_obj_set_size(obj, SMALL_1 * span + SMALL_PAD * (span - 1), 56);
    lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_START, dialog->col, span, LV_GRID_ALIGN_CENTER, dialog->row, 1);

    dialog->col += span;
}

lv_obj_t * dialog_dropdown(dialog_t *dialog, uint8_t span) {
    lv_obj_t *obj = lv_dropdown_create(dialog->grid);

    dialog_item(dialog, obj, span);
    lv_dropdown_set_symbol(obj, NULL);

    lv_obj_center(obj);

    lv_obj_t *list = lv_dropdown_get_list(obj);

    lv_obj_remove_style_all(list);

    lv_obj_add_style(list, dropdown_list_style, LV_PART_MAIN);
    lv_obj_add_style(list, dialog_item_focus_style, LV_PART_SELECTED);

    return obj;
}

lv_obj_t * dialog_slider(dialog_t *dialog, uint8_t span) {
    lv_obj_t *obj = lv_slider_create(dialog->grid);

    dialog_item(dialog, obj, span);
    lv_obj_center(obj);

    lv_obj_add_style(obj, slider_style, LV_PART_MAIN);

    lv_obj_add_style(obj, slider_indicator_style, LV_PART_INDICATOR);
    lv_obj_add_style(obj, slider_knob_style, LV_PART_KNOB);

    lv_obj_add_style(obj, slider_indicator_edited_style, LV_STATE_EDITED | LV_PART_INDICATOR);
    lv_obj_add_style(obj, slider_knob_edited_style, LV_STATE_EDITED | LV_PART_KNOB);

    return obj;
}

lv_obj_t * dialog_switch(dialog_t *dialog, uint8_t span) {
    lv_obj_t *obj = lv_switch_create(dialog->grid);

    dialog_item(dialog, obj, span);
    lv_obj_center(obj);

    lv_obj_add_style(obj, switch_style, LV_PART_MAIN);

    lv_obj_add_style(obj, switch_indicator_style, LV_PART_INDICATOR);
    lv_obj_add_style(obj, switch_knob_style, LV_PART_KNOB);

    lv_obj_add_style(obj, switch_indicator_checked_style, LV_STATE_CHECKED | LV_PART_INDICATOR);
    lv_obj_add_style(obj, switch_knob_checked_style, LV_STATE_CHECKED | LV_PART_KNOB);

    return obj;
}

void dialog_title(dialog_t *dialog, const char *text) {
    dialog->col = 1;
    dialog->row++;
    row_dsc[dialog->row] = LV_GRID_CONTENT;
    row_dsc[dialog->row + 1] = LV_GRID_TEMPLATE_LAST;

    lv_obj_t *obj = lv_label_create(dialog->grid);
    lv_obj_add_style(obj, dialog_title_style, LV_PART_MAIN);

    lv_label_set_text(obj, text);
    lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, dialog->row, 1);
}

void dialog_label(dialog_t *dialog, bool title, const char *fmt, ...) {
    char    buf[128];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    dialog->col = 1;
    dialog->row++;
    row_dsc[dialog->row] = LV_GRID_CONTENT;
    row_dsc[dialog->row + 1] = LV_GRID_TEMPLATE_LAST;

    lv_obj_t *obj = lv_label_create(dialog->grid);
    lv_obj_add_style(obj, title ? dialog_title_label_style : dialog_label_style, LV_PART_MAIN);

    lv_label_set_text(obj, buf);
    lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, dialog->row, 1);
}

void dialog_audio_samples(float complex *samples, size_t n) {
    if (dialog_is_run() && current_dialog->audio_cb) {
        current_dialog->audio_cb(samples, n);
    }
}

void dialog_rotary(int32_t diff) {
    if (dialog_is_run() && current_dialog->rotary_cb) {
        current_dialog->rotary_cb(diff);
    }
}

void dialog_key_cb(lv_event_t * e) {
    uint32_t    key = *((uint32_t *)lv_event_get_param(e));

    switch (key) {
        case HKEY_FINP:
             lv_group_set_editing(keyboard_group, !lv_group_get_editing((const lv_group_t*) keyboard_group));
             break;

        case LV_KEY_ESC:
            dialog_destruct();
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
