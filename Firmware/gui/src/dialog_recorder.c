/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sndfile.h>
#include <dirent.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "audio.h"
#include "recorder.h"
#include "dialog_recorder.h"
#include "styles.h"
#include "settings/options.h"
#include "events.h"
#include "util.h"
#include "pannel.h"
#include "keyboard.h"
#include "textarea_window.h"
#include "msg.h"
#include "msgs.h"
#include "buttons.h"
#include "dsp.h"
#include "main.h"

#define BUF_SIZE 1024

static lv_obj_t             *table;
static int16_t              table_rows = 0;
static SNDFILE              *file = NULL;
static atomic_bool          play_state = false;

static char                 *prev_filename;
static pthread_t            thread;
static int16_t              samples_buf[BUF_SIZE];

static void construct_cb(lv_obj_t *parent);
static void destruct_cb();
static void rec_stop_cb(lv_event_t * e);
static void play_stop_cb(lv_event_t * e);
static void rec_start_cb(lv_event_t * e);
static void play_start_cb(lv_event_t * e);
static void rename_cb(lv_event_t * e);
static void delete_cb(lv_event_t * e);
static void format_cb(lv_event_t * e);

static button_item_t button_rec_start   = { .label = "Rec",         .press = rec_start_cb };
static button_item_t button_rename      = { .label = "Rename",      .press = rename_cb };
static button_item_t button_delete      = { .label = "Delete",      .press = delete_cb };
static button_item_t button_play_start  = { .label = "Play",        .press = play_start_cb };
static button_item_t button_format_wav  = { .label = "Format\nWav", .press = format_cb };
static button_item_t button_format_mp3  = { .label = "Format\nMP3", .press = format_cb };

static button_item_t button_rec_stop    = { .label = "Rec\nStop",   .press = rec_stop_cb };
static button_item_t button_play_stop   = { .label = "Play\nStop",  .press = play_stop_cb };

static dialog_t             dialog = {
    .run = false,
    .construct_cb = construct_cb,
    .destruct_cb = destruct_cb,
    .audio_cb = NULL,
    .buttons = true,
    .key_cb = NULL
};

dialog_t                    *dialog_recorder = &dialog;

static void buttons_format() {
    switch (options->audio.rec_format) {
        case REC_FORMAT_WAV:
            buttons_load(4, &button_format_wav);
            break;

        case REC_FORMAT_MP3:
            buttons_load(4, &button_format_mp3);
            break;

        default:
            break;
    }
}

static void buttons_default() {
    buttons_load(0, &button_rec_start);
    buttons_load(1, &button_rename);
    buttons_load(2, &button_delete);
    buttons_load(3, &button_play_start);
    buttons_format();
}

static void load_table() {
    lv_table_set_row_cnt(table, 1);
    lv_table_set_cell_value(table, 0, 0, "");

    table_rows = 0;

    DIR             *dp;
    struct dirent   *ep;

    dp = opendir(recorder_path);

    if (dp != NULL) {
        while ((ep = readdir(dp)) != NULL) {
            if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0) {
                continue;
            }

            lv_table_set_cell_value(table, table_rows++, 0, ep->d_name);
        }

        closedir(dp);
    }
}

static void close_file() {
    sf_close(file);
}

static const char* get_item() {
    if (table_rows == 0) {
        return NULL;
    }

    uint32_t    row = 0;
    uint32_t    col = 0;

    lv_table_get_selected_cell(table, &row, &col);

    if (row == LV_TABLE_CELL_NONE) {
        return NULL;
    }

    return lv_table_get_cell_value(table, row, col);
}

static void play_item() {
    const char *item = get_item();

    if (!item) {
        return;
    }

    char filename[64];

    strcpy(filename, recorder_path);
    strcat(filename, "/");
    strcat(filename, item);

    SF_INFO sfinfo;

    memset(&sfinfo, 0, sizeof(sfinfo));

    SNDFILE *file = sf_open(filename, SFM_READ, &sfinfo);

    if (!file) {
        return;
    }

    if (recorder_is_on()) {
        recorder_set_on(false);
    }

    while (atomic_load(&play_state)) {
        int res = sf_read_short(file, samples_buf, BUF_SIZE);

        if (res > 0) {
            audio_play(samples_buf, res);
        } else {
            atomic_store(&play_state, false);
        }
    }

    sf_close(file);
    audio_play_wait();
}

static void * play_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    play_item();

    brass_lv_lock();
    if (dialog.run) {
        buttons_unload_page();
        buttons_default();
    }
    brass_lv_unlock();

    return NULL;
}

static void textarea_window_close_cb() {
    lv_group_add_obj(keyboard_group, table);
    lv_group_set_editing(keyboard_group, true);

    free(prev_filename);
    prev_filename = NULL;
}

static void textarea_window_edit_ok_cb() {
    const char *new_filename = textarea_window_get();

    if (strcmp(prev_filename, new_filename) != 0) {
        char prev[64];
        char new[64];

        snprintf(prev, sizeof(prev), "%s/%s", recorder_path, prev_filename);
        snprintf(new, sizeof(new), "%s/%s", recorder_path, new_filename);

        if (rename(prev, new) == 0) {
            load_table();
            textarea_window_close_cb();
        }
    } else {
        free(prev_filename);
        prev_filename = NULL;
    }
}

static void msg_cb(lv_event_t * e) {
    brass_msg_t *m = brass_event_get_msg(e);

    switch (brass_msg_get_id(m)) {
        case MSG_PTT: {
            const int *on = brass_msg_get_payload(m);

            if (*on) {
                if (atomic_load(&play_state)) {
                    atomic_store(&play_state, false);

                    buttons_unload_page();
                    buttons_default();
                }
            }
        } break;
    }
}

static void construct_cb(lv_obj_t *parent) {
    dialog_init(parent, &dialog);

    table = lv_table_create(dialog.obj);

    lv_obj_remove_style_all(table);
    lv_obj_add_style(table, dialog_item_focus_style, LV_PART_ITEMS | LV_STATE_EDITED);
    lv_obj_set_size(table, 775, 325);
    lv_obj_center(table);

    lv_table_set_col_cnt(table, 1);
    lv_table_set_col_width(table, 0, 770);

    lv_obj_add_event_cb(table, dialog_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(table, msg_cb, BRASS_EVENT_MSG_RECEIVED, NULL);
    brass_msg_subscribe_obj(MSG_PTT, table, NULL);

    lv_group_add_obj(keyboard_group, table);
    lv_group_set_editing(keyboard_group, true);

    mkdir(recorder_path, 0755);
    load_table();

    if (recorder_is_on()) {
        buttons_unload_page();
        buttons_load(0, &button_rec_stop);
    } else {
        buttons_default();
    }
}

static void destruct_cb() {
    atomic_store(&play_state, false);
    textarea_window_close();
}

static void rec_start_cb(lv_event_t * e) {
    recorder_set_on(true);
}

static void rec_stop_cb(lv_event_t * e) {
    recorder_set_on(false);
    load_table();
}

static void play_start_cb(lv_event_t * e) {
    atomic_store(&play_state, true);
    if (pthread_create(&thread, NULL, play_thread, NULL) != 0) {
        atomic_store(&play_state, false);
        return;
    }

    buttons_unload_page();
    buttons_load(3, &button_play_stop);
}

static void play_stop_cb(lv_event_t * e) {
    atomic_store(&play_state, false);
    buttons_unload_page();
    buttons_default();
}

static void rename_cb(lv_event_t * e) {
    const char *item = get_item();
    prev_filename = item ? strdup(item) : NULL;

    if (prev_filename) {
        lv_group_remove_obj(table);
        textarea_window_open(textarea_window_edit_ok_cb, textarea_window_close_cb);
        textarea_window_set(prev_filename);
    }
}

static void delete_cb(lv_event_t * e) {
    const char *item = get_item();

    if (item) {
        char filename[64];

        strcpy(filename, recorder_path);
        strcat(filename, "/");
        strcat(filename, item);

        unlink(filename);
        load_table();
    }
}

static void format_cb(lv_event_t * e) {
    rec_format_t format = options->audio.rec_format;

    switch (format) {
        case REC_FORMAT_WAV:
            format = REC_FORMAT_MP3;
            break;

        case REC_FORMAT_MP3:
            format = REC_FORMAT_WAV;
            break;

        default:
            break;
    }

    options->audio.rec_format = format;
    buttons_format();
}

void dialog_recorder_set_on(bool on) {
    if (!dialog.run) {
        return;
    }

    buttons_unload_page();

    if (on) {
        buttons_load(0, &button_rec_stop);
    } else {
        buttons_default();
        load_table();
    }
}
