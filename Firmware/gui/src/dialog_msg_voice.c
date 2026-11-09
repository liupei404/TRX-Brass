/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <time.h>
#include <sys/time.h>
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
#include "main.h"
#include "audio.h"
#include "dialog_msg_voice.h"
#include "styles.h"
#include "settings/options.h"
#include "util.h"
#include "keyboard.h"
#include "textarea_window.h"
#include "msg.h"
#include "msgs.h"
#include "mic.h"
#include "buttons.h"
#include "queue.h"
#include "dsp.h"
#include "fpga/dac.h"
#include "smeter.h"

#define DECIM       441
#define INTER       128

#define BUF_SIZE    1024

typedef enum {
    VOICE_BEACON_OFF = 0,
    VOICE_BEACON_PLAY,
    VOICE_BEACON_IDLE,
} voice_beacon_t;

static _Atomic msg_voice_state_t state = MSG_VOICE_OFF;
static _Atomic voice_beacon_t    beacon = VOICE_BEACON_OFF;
static lv_timer_t           *beacon_timer = NULL;
static char                 *path = "/mnt/msg";

static lv_obj_t             *table;
static int16_t              table_rows = 0;
static SNDFILE              *file = NULL;

static char                 *prev_filename;
static pthread_t            thread;
static bool                 thread_joinable = false;
static bool                 worker_sending = false;
static int16_t              samples_buf[BUF_SIZE];

static uint8_t              buttons_page = 0;

/* For send */

static cbufferf             out_buf;
static rresamp_rrrf         resamp;
static float                resamp_buf[INTER];
static pthread_mutex_t      mux;
static pthread_cond_t       cond;
static pthread_mutex_t      file_mux;

static void construct_cb(lv_obj_t *parent);
static void destruct_cb();
static bool keypad_cb(event_keypad_t *keypad);
static bool modulate_state_cb();
static size_t modulate_cb(float complex *data, size_t max_size, radio_mode_t mode);

static void send_cb(lv_event_t * e);
static void beacon_cb(lv_event_t * e);
static void rec_cb(lv_event_t * e);
static void play_cb(lv_event_t * e);
static void rename_cb(lv_event_t * e);
static void delete_cb(lv_event_t * e);
static void beacon_period_cb(lv_event_t * e);

static void send_stop_cb(lv_event_t * e);
static void beacon_stop_cb(lv_event_t * e);
static void rec_stop_cb(lv_event_t * e);
static void play_stop_cb(lv_event_t * e);

static bool send_file();

static void join_worker() {
    if (thread_joinable) {
        pthread_join(thread, NULL);
        thread_joinable = false;
    }
}

static button_item_t button_send            = { .label = "Send",            .press = send_cb };
static button_item_t button_becon           = { .label = "Beacon",          .press = beacon_cb };
static button_item_t button_rec             = { .label = "Rec",             .press = rec_cb };
static button_item_t button_play            = { .label = "Play",            .press = play_cb };
static button_item_t button_rename          = { .label = "Rename",          .press = rename_cb };
static button_item_t button_delete          = { .label = "Delete",          .press = delete_cb };

static button_item_t button_becon_period    = { .label = "Beacon\nPeriod",  .press = beacon_period_cb };

static button_item_t button_send_stop       = { .label = "Send\nStop",      .press = send_stop_cb };
static button_item_t button_beacon_stop     = { .label = "Beacon\nStop",    .press = beacon_stop_cb };
static button_item_t button_rec_stop        = { .label = "Rec\nStop",       .press = rec_stop_cb };
static button_item_t button_play_stop       = { .label = "Play\nStop",      .press = play_stop_cb };

static dialog_t             dialog = {
    .run = false,
    .construct_cb = construct_cb,
    .destruct_cb = destruct_cb,
    .keypad_cb = keypad_cb,
    .modulate_state_cb = modulate_state_cb,
    .modulate_cb = modulate_cb,
    .audio_cb = NULL,
    .buttons = true,
    .key_cb = NULL
};

dialog_t *dialog_msg_voice = &dialog;

static void load_page(uint8_t i) {
    switch (i) {
        case 0:
            buttons_load(0, &button_send);
            buttons_load(1, &button_becon);
            buttons_load(2, &button_rec);
            buttons_load(3, &button_play);
            buttons_load(4, &button_rename);
            buttons_load(5, &button_delete);
            break;

        default:
            buttons_load(0, &button_becon_period);
            break;
    }

    buttons_page = i;
}

static void load_table() {
    lv_table_set_row_cnt(table, 1);
    lv_table_set_cell_value(table, 0, 0, "");

    table_rows = 0;

    DIR             *dp;
    struct dirent   *ep;

    dp = opendir(path);

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

static bool create_file() {
    SF_INFO sfinfo;

    memset(&sfinfo, 0, sizeof(sfinfo));

    sfinfo.samplerate = AUDIO_CAPTURE_RATE;
    sfinfo.channels = 1;

    bool mp3 = options->audio.rec_format == REC_FORMAT_MP3;

    sfinfo.format = mp3 ? SF_FORMAT_MPEG | SF_FORMAT_MPEG_LAYER_III :
                          SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    char        filename[64];
    time_t      now = time(NULL);
    struct tm   *t = localtime(&now);

    snprintf(filename, sizeof(filename),
        "%s/MSG_%04i%02i%02i_%02i%02i%02i.%s",
        path, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, mp3 ? "mp3" : "wav"
    );

    file = sf_open(filename, SFM_WRITE, &sfinfo);

    if (file == NULL) {
        LV_LOG_ERROR("Problem with create file - %s", sf_strerror(NULL));
        return false;
    }

    return true;
}

static void open_file() {
    const char *item = get_item();

    if (!item) {
        return;
    }

    char filename[64];

    strcpy(filename, path);
    strcat(filename, "/");
    strcat(filename, item);

    SF_INFO sfinfo;

    memset(&sfinfo, 0, sizeof(sfinfo));

    file = sf_open(filename, SFM_READ, &sfinfo);

    if (file && (sfinfo.samplerate != AUDIO_CAPTURE_RATE || sfinfo.channels != 1)) {
        LV_LOG_ERROR("Voice message must be mono, 44100 Hz (got %d Hz, %d channels)",
                     sfinfo.samplerate, sfinfo.channels);
        sf_close(file);
        file = NULL;
    }
}

static void close_file() {
    pthread_mutex_lock(&file_mux);

    if (file != NULL) {
        sf_close(file);
        file = NULL;
    }

    pthread_mutex_unlock(&file_mux);
}

static void * play_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    open_file();

    if (file == NULL) {
        state = MSG_VOICE_OFF;
        if (dialog.run) queue_send(dialog.obj, EVENT_VOICE_DONE, NULL);
        return NULL;
    }

    while (state == MSG_VOICE_PLAY) {
        int res = sf_read_short(file, samples_buf, BUF_SIZE);

        if (res > 0) {
            audio_play(samples_buf, res);
        } else {
            state = MSG_VOICE_OFF;
        }
    }

    close_file();
    audio_play_wait();

    if (dialog.run) queue_send(dialog.obj, EVENT_VOICE_DONE, NULL);

    return NULL;
}

static void beacon_timer_cb(lv_timer_t *t) {
    beacon_timer = NULL;
    beacon = VOICE_BEACON_PLAY;

    send_file();
}

static void * send_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    pthread_mutex_lock(&mux);
    cbufferf_reset(out_buf);
    pthread_mutex_unlock(&mux);
    rresamp_rrrf_reset(resamp);

    bool        eof = false;
    unsigned    tail = 0;
    float       buf[DECIM];

    while (state == MSG_VOICE_SEND) {
        pthread_mutex_lock(&mux);

        while (state == MSG_VOICE_SEND &&
               ((eof && !tail) ? cbufferf_size(out_buf) > 0 : cbufferf_space_available(out_buf) < INTER)) {
            pthread_cond_wait(&cond, &mux);
        }

        bool done = (eof && !tail) || state != MSG_VOICE_SEND;

        pthread_mutex_unlock(&mux);

        if (done) break;

        unsigned count;

        if (tail) {
            memset(buf, 0, sizeof(buf));
            count = tail;
            tail = 0;
        } else {
            int res = sf_read_float(file, buf, DECIM);

            if (res < 0) 
                res = 0;

            memset(buf + res, 0, (DECIM - res) * sizeof(float));

            eof = res < DECIM;
            /* Drain the full 128-input-sample FIR tail at EOF as well as
             * the final partial file block before releasing PTT. */
            count = ((res + (eof ? 128 : 0)) * INTER + DECIM - 1) / DECIM;

            if (count > INTER) {
                tail = count - INTER;
                count = INTER;
            }
        }
        rresamp_rrrf_execute(resamp, buf, resamp_buf);
        pthread_mutex_lock(&mux);
        cbufferf_write(out_buf, resamp_buf, count);
        pthread_mutex_unlock(&mux);
    }

    close_file();

    state = MSG_VOICE_OFF;

    if (dialog.run) 
        queue_send(dialog.obj, EVENT_VOICE_DONE, NULL);

    return NULL;
}

static bool send_file() {
    join_worker();
    open_file();

    if (file == NULL) {
        msg_set_text_fmt("File problem");
        return false;
    }

    state = MSG_VOICE_SEND;
    msg_set_text_fmt("Sending message");
    worker_sending = true;
    dsp_set_mute(true);
    radio_set_ptt(true);

    if (pthread_create(&thread, NULL, send_thread, NULL) != 0) {
        radio_set_ptt(false);
        dsp_set_mute(false);
        worker_sending = false;
        state = MSG_VOICE_OFF;
        close_file();

        return false;
    }

    thread_joinable = true;
    return true;
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

        snprintf(prev, sizeof(prev), "%s/%s", path, prev_filename);
        snprintf(new, sizeof(new), "%s/%s", path, new_filename);

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
                if (beacon == VOICE_BEACON_IDLE) {
                    if (beacon_timer) {
                        lv_timer_del(beacon_timer);
                        beacon_timer = NULL;
                    }
                    buttons_unload_page();
                    load_page(0);
                }

                beacon = VOICE_BEACON_OFF;

                if (state == MSG_VOICE_SEND) {
                    state = MSG_VOICE_SEND_CANCEL;

                    pthread_mutex_lock(&mux);
                    pthread_cond_signal(&cond);
                    pthread_mutex_unlock(&mux);
                }
            } else {
                dsp_set_mute(false);
            }
        } break;

        default:
            break;
    }
}

static void worker_done_cb(lv_event_t *e) {
    join_worker();

    if (worker_sending) {
        radio_set_ptt(false);
        worker_sending = false;
    }

    dsp_set_mute(false);

    if (beacon == VOICE_BEACON_PLAY && state == MSG_VOICE_OFF) {
        beacon = VOICE_BEACON_IDLE;
        beacon_timer = lv_timer_create(beacon_timer_cb, options->msg.voice_period * 1000, NULL);

        if (beacon_timer != NULL) {
            lv_timer_set_repeat_count(beacon_timer, 1);
            msg_set_text_fmt("Beacon pause: %i s", options->msg.voice_period);
        } else {
            beacon = VOICE_BEACON_OFF;
        }
    } else {
        buttons_unload_page();
        load_page(0);
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

    lv_obj_add_event_cb(table, msg_cb, BRASS_EVENT_MSG_RECEIVED, NULL);
    lv_obj_add_event_cb(dialog.obj, worker_done_cb, EVENT_VOICE_DONE, NULL);
    brass_msg_subscribe_obj(MSG_PTT, table, NULL);

    lv_obj_add_event_cb(table, dialog_key_cb, LV_EVENT_KEY, NULL);

    lv_group_add_obj(keyboard_group, table);
    lv_group_set_editing(keyboard_group, true);

    /* * */

    pthread_mutex_init(&mux, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&file_mux, NULL);

    float bw = 5000.0f / AUDIO_CAPTURE_RATE;
    resamp = rresamp_rrrf_create_kaiser(INTER, DECIM, 64, bw, 80);
    rresamp_rrrf_set_scale(resamp, 2 * bw);
    out_buf = cbufferf_create(DAC_RATE / 4);

    mkdir(path, 0755);
    load_table();
    load_page(0);
}

static void destruct_cb() {
    if (beacon_timer != NULL) {
        lv_timer_delete(beacon_timer);
        beacon_timer = NULL;
    }
    beacon = VOICE_BEACON_OFF;

    switch (state) {
        case MSG_VOICE_RECORD:
            rec_stop_cb(NULL);
            break;

        case MSG_VOICE_PLAY:
        case MSG_VOICE_SEND:
        case MSG_VOICE_SEND_CANCEL:
            state = MSG_VOICE_OFF;
            pthread_mutex_lock(&mux);
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mux);
            break;

        default:
            break;
    }

    join_worker();
    queue_cancel(dialog.obj, EVENT_VOICE_DONE, NULL);

    if (worker_sending) {
        radio_set_ptt(false);
        worker_sending = false;
    }

    dsp_set_mute(false);

    textarea_window_close();
    rresamp_rrrf_destroy(resamp);
    cbufferf_destroy(out_buf);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mux);
    pthread_mutex_destroy(&file_mux);
}

static bool keypad_cb(event_keypad_t *keypad) {
    switch (keypad->key) {
        case KEYPAD_VOL:
        case KEYPAD_MFK:
            return true;

        case KEYPAD_APP:
            if (keypad->state == KEYPAD_RELEASE) {
                buttons_unload_page();

                switch (buttons_page) {
                    case 0:
                        load_page(1);
                        break;

                    case 1:
                        load_page(0);
                        break;
                }

                return true;
            }
            break;

        default:
            break;
    }

    return false;
}

static bool modulate_state_cb() {
    return state == MSG_VOICE_SEND;
}

static size_t modulate_cb(float complex *data, size_t max_size, radio_mode_t mode) {
    float samples[INTER];

    size_t size = 0;

    if (max_size > INTER) 
        max_size = INTER;

    pthread_mutex_lock(&mux);

    if (cbufferf_size(out_buf) > 0) {
        uint32_t    n;
        float       *buf;

        cbufferf_read(out_buf, max_size, &buf, &n);

        for (uint32_t i = 0; i < n; i++) {
            samples[i] = buf[i];
        }

        cbufferf_release(out_buf, n);
        size = n;
    }

    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mux);

    for (size_t i = 0; i < size; ++i) 
        data[i] = dsp_modulate(samples[i], mode);

    return size;
}

static void send_cb(lv_event_t * e) {
    if (state == MSG_VOICE_OFF) {
        if (send_file()) {
            buttons_unload_page();
            buttons_load(0, &button_send_stop);
        }
    }
}

static void send_stop_cb(lv_event_t * e) {
    state = MSG_VOICE_OFF;

    pthread_mutex_lock(&mux);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mux);
}

static void beacon_cb(lv_event_t * e) {
    beacon = VOICE_BEACON_PLAY;

    if (send_file()) {
        buttons_unload_page();
        buttons_load(1, &button_beacon_stop);
    }
}

static void beacon_stop_cb(lv_event_t * e) {
    switch (beacon) {
        case VOICE_BEACON_PLAY:
            state = MSG_VOICE_OFF;

            pthread_mutex_lock(&mux);
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mux);
            break;

        case VOICE_BEACON_IDLE:
            lv_timer_del(beacon_timer);
            beacon_timer = NULL;
            buttons_unload_page();
            load_page(0);
            break;
    }

    beacon = VOICE_BEACON_OFF;
}

static void beacon_period_cb(lv_event_t * e) {
    uint16_t period = options->msg.voice_period;

    switch (period) {
        case 10:
            period = 30;
            break;

        case 30:
            period = 60;
            break;

        case 60:
            period = 120;
            break;

        case 120:
            period = 10;
            break;

        default:
            period = 10;
            break;
    }

    options->msg.voice_period = period;
    msg_set_text_fmt("Beacon period: %i s", period);
}

static void rec_cb(lv_event_t * e) {
    if (state == MSG_VOICE_OFF) {
        if (create_file()) {
            dsp_set_mute(true);
            mic_enabled(true);
            state = MSG_VOICE_RECORD;

            buttons_unload_page();
            buttons_load(2, &button_rec_stop);
        }
    }
}

static void rec_stop_cb(lv_event_t * e) {
    buttons_unload_page();
    load_page(0);

    state = MSG_VOICE_OFF;
    dsp_set_mute(false);
    mic_enabled(false);
    close_file();
    load_table();
}

static void play_cb(lv_event_t * e) {
    if (state == MSG_VOICE_OFF) {
        join_worker();

        state = MSG_VOICE_PLAY;
        worker_sending = false;
        dsp_set_mute(true);

        if (pthread_create(&thread, NULL, play_thread, NULL) != 0) {
            state = MSG_VOICE_OFF;
            dsp_set_mute(false);
            return;
        }

        thread_joinable = true;

        buttons_unload_page();
        buttons_load(3, &button_play_stop);
    }
}

static void play_stop_cb(lv_event_t * e) {
    state = MSG_VOICE_OFF;
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

        strcpy(filename, path);
        strcat(filename, "/");
        strcat(filename, item);

        unlink(filename);
        load_table();
    }
}

msg_voice_state_t dialog_msg_voice_get_state() {
    return state;
}

void dialog_msg_voice_put_audio_samples(float *samples, size_t nsamples) {
    pthread_mutex_lock(&file_mux);

    if (state == MSG_VOICE_RECORD && file != NULL) {
        sf_write_float(file, samples, nsamples);
    }

    pthread_mutex_unlock(&file_mux);
}
