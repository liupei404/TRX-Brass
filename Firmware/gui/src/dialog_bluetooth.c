/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <stdatomic.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "styles.h"
#include "pannel.h"
#include "buttons.h"
#include "dialog.h"
#include "keyboard.h"
#include "events.h"
#include "queue.h"
#include "msg.h"
#include "dbus/bluez.h"

typedef struct {
    char            *addr;
    char            *name;
    char            *path;
} bluetooth_msg_t;

static lv_obj_t             *table;
static int16_t              table_rows = 0;
static _Atomic bool         active = false;

static void construct_cb(lv_obj_t *parent);
static void destruct_cb();

static void scan_start_cb(lv_event_t * e);
static void scan_stop_cb(lv_event_t * e);
static void connect_cb(lv_event_t * e);
static void remove_cb(lv_event_t * e);

static void add_interface_cb(lv_event_t * e);

static button_item_t button_scan_start  = { .label = "Scan\nStart",     .press = scan_start_cb };
static button_item_t button_scan_stop   = { .label = "Scan\nStop",      .press = scan_stop_cb };
static button_item_t button_connect     = { .label = "Connect",         .press = connect_cb };
static button_item_t button_remove      = { .label = "Remove",          .press = remove_cb };
static button_item_t button_empty       = { .label = "",                .press = NULL };

static void free_msg(void *param) {
    bluetooth_msg_t *msg = param;

    if (msg == NULL) return;
    free(msg->addr);
    free(msg->name);
    free(msg->path);
    free(msg);
}

static void free_table_paths() {
    if (table == NULL) return;

    uint32_t rows = lv_table_get_row_count(table);
    for (uint32_t row = 0; row < rows; row++) {
        free(lv_table_get_cell_user_data(table, row, 0));
        lv_table_set_cell_user_data(table, row, 0, NULL);
    }
}

static dialog_t             dialog = {
    .run = false,
    .construct_cb = construct_cb,
    .destruct_cb = destruct_cb,
    .audio_cb = NULL,
    .buttons = true,
    .key_cb = NULL
};

dialog_t                    *dialog_bluetooth = &dialog;

static void construct_cb(lv_obj_t *parent) {
    dialog_init(parent, &dialog);

    table = lv_table_create(dialog.obj);

    lv_obj_remove_style_all(table);
    lv_obj_add_style(table, dialog_item_focus_style, LV_PART_ITEMS | LV_STATE_EDITED);
    lv_obj_set_size(table, 775, 325);
    lv_obj_center(table);

    table_rows = 0;
    lv_table_set_col_cnt(table, 1);
    lv_table_set_col_width(table, 0, 770);

    lv_obj_add_event_cb(table, dialog_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(table, add_interface_cb, EVENT_BLUETOOTH_MSG, NULL);

    lv_group_add_obj(keyboard_group, table);
    lv_group_set_editing(keyboard_group, true);
    active = true;

    if (dbus_bluez_ok()) {
        lv_table_set_cell_value(table, 0, 0, "BlueTooth adapter ready");

        buttons_load(0, &button_scan_start);
        dbus_bluez_cached();
    } else {
        lv_table_set_cell_value(table, 0, 0, "BlueTooth adapter not found");
    }
}

static void destruct_cb() {
    active = false;
    dbus_bluez_scan(false);
    queue_cancel(table, EVENT_BLUETOOTH_MSG, free_msg);
    free_table_paths();
    table = NULL;
}

static void scan_start_cb(lv_event_t * e) {
    if (!dbus_bluez_scan(true)) {
        msg_set_text_fmt("Bluetooth scan failed");
        return;
    }

    if (table_rows == 0) {
        lv_table_set_row_cnt(table, 1);
        lv_table_set_cell_value(table, 0, 0, "Scanning...");
    }

    buttons_load(0, &button_scan_stop);
    dbus_bluez_cached();
}

static void scan_stop_cb(lv_event_t * e) {
    buttons_load(0, &button_scan_start);
    dbus_bluez_scan(false);
}

static void add_interface_cb(lv_event_t * e) {
    bluetooth_msg_t *msg = (bluetooth_msg_t *) lv_event_get_param(e);
    uint32_t        row = 0;
    uint32_t        col = 0;
    bool            scroll;

    for (uint32_t i = 0; i < (uint32_t)table_rows; i++) {
        const char *path = lv_table_get_cell_user_data(table, i, 0);

        if (path != NULL && strcmp(path, msg->path) == 0) {
            lv_table_set_cell_value_fmt(table, i, 0, "%s %s", msg->addr, msg->name);
            free(msg->addr);
            msg->addr = NULL;
            free(msg->name);
            msg->name = NULL;
            free(msg->path);
            msg->path = NULL;
            return;
        }
    }

    lv_table_get_selected_cell(table, &row, &col);
    scroll = table_rows == (row + 1);

    lv_table_set_cell_value_fmt(table, table_rows, 0, "%s %s", msg->addr, msg->name);
    lv_table_set_cell_user_data(table, table_rows, 0, msg->path);
    msg->path = NULL;

    if (scroll) {
        int32_t c = LV_KEY_DOWN;

        lv_obj_send_event(table, LV_EVENT_KEY, &c);
    }

    table_rows++;

    if (table_rows == 1) {
        buttons_load(1, &button_connect);
        buttons_load(2, &button_remove);
    }

    free(msg->addr);
    msg->addr = NULL;
    free(msg->name);
    msg->name = NULL;
}

static void connect_cb(lv_event_t * e) {
    uint32_t        row = 0;
    uint32_t        col = 0;

    lv_table_get_selected_cell(table, &row, &col);

    const char *path = lv_table_get_cell_user_data(table, row, col);

    msg_set_text_fmt("Connecting");
    dbus_bluez_connect(path);

    buttons_load(0, &button_scan_start);
}

static void remove_cb(lv_event_t * e) {
    uint32_t row = 0;
    uint32_t col = 0;

    if (table_rows <= 0) return;

    lv_table_get_selected_cell(table, &row, &col);
    if (row == LV_TABLE_CELL_NONE || row >= (uint32_t)table_rows || col != 0) return;

    char *path = lv_table_get_cell_user_data(table, row, 0);
    if (path == NULL) return;

    if (!dbus_bluez_remove(path)) {
        msg_set_text_fmt("Bluetooth remove failed");
        return;
    }

    free(path);

    for (uint32_t i = row; i + 1 < (uint32_t)table_rows; i++) {
        const char *text = lv_table_get_cell_value(table, i + 1, 0);
        void *next_path = lv_table_get_cell_user_data(table, i + 1, 0);

        lv_table_set_cell_value(table, i, 0, text);
        lv_table_set_cell_user_data(table, i, 0, next_path);
    }

    table_rows--;
    if (table_rows == 0) {
        lv_table_set_row_cnt(table, 1);
        lv_table_set_cell_value(table, 0, 0, "No devices");
        lv_table_set_cell_user_data(table, 0, 0, NULL);
        buttons_load(1, &button_empty);
        buttons_load(2, &button_empty);
    } else {
        lv_table_set_cell_user_data(table, table_rows, 0, NULL);
        lv_table_set_row_cnt(table, table_rows);
    }

    msg_set_text_fmt("Bluetooth device removed");
}

void dialog_bluetooth_interface_added(const gchar *addr, const gchar *name, const gchar *path) {
    if (!active || table == NULL) return;

    bluetooth_msg_t   *msg = malloc(sizeof(bluetooth_msg_t));

    if (msg == NULL) return;

    msg->addr = strdup(addr);
    msg->name = strdup(name);
    msg->path = strdup(path);

    if (msg->addr == NULL || msg->name == NULL || msg->path == NULL) {
        free_msg(msg);
        return;
    }

    queue_send(table, EVENT_BLUETOOTH_MSG, msg);
}
