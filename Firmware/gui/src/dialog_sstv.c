/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include <complex.h>
#include <dirent.h>
#include <errno.h>
#include <png.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lvgl/lvgl.h"
#include "lvgl/src/misc/cache/instance/lv_image_cache.h"
#include "liquid/liquid.h"

#include "audio.h"
#include "buttons.h"
#include "dialog.h"
#include "dialog_sstv.h"
#include "events.h"
#include "fpga/adc.h"
#include "keyboard.h"
#include "msg.h"
#include "queue.h"
#include "settings/sstv.h"
#include "sstv/decoder.h"
#include "sstv/modes.h"
#include "util.h"

#define SSTV_PATH          "/mnt/sstv"
#define CARD_WIDTH         350
#define IMAGE_MAX_WIDTH    320
#define IMAGE_MAX_HEIGHT   240
#define AUDIO_CHUNK        1024

typedef struct sstv_card {
    struct sstv_card *next;
    lv_obj_t         *obj;
    lv_obj_t         *image;
    lv_draw_buf_t    *draw_buf;
    char              filename[256];
    bool              live;
} sstv_card_t;

typedef struct {
    int width;
    int height;
    char mode[16];
} vis_event_t;

typedef struct {
    int row;
    int width;
    uint8_t rgb[];
} line_event_t;

typedef struct {
    bool saved;
    bool complete;
    int percent;
    char filename[256];
} frame_event_t;

static lv_obj_t             *gallery;
static sstv_card_t          *cards;
static sstv_card_t          *selected;
static sstv_card_t          *live_card;

static pthread_t            worker;
static pthread_mutex_t      audio_mutex;
static pthread_cond_t       audio_cond;
static cbuffercf            audio_buffer;
static atomic_bool          worker_stop;
static bool                 worker_started;

static sstv_decoder_t       *decoder;
static const sstv_mode_t    *rx_mode;
static uint8_t              *rx_rgb;
static int                  rx_width;
static int                  rx_height;
static int                  rx_rows;

static void construct_cb(lv_obj_t *parent);
static void destruct_cb(void);
static void audio_cb(float complex *samples, size_t n);

static button_item_t delete_button;
static button_item_t threshold_button;

static dialog_t dialog = {
    .run = false,
    .construct_cb = construct_cb,
    .destruct_cb = destruct_cb,
    .audio_cb = audio_cb,
    .rotary_cb = NULL,
    .buttons = true,
    .key_cb = dialog_key_cb,
};

dialog_t *dialog_sstv = &dialog;

static void select_card(sstv_card_t *card) {
    if (selected == card) {
        return;
    }

    if (selected && selected->obj) {
        lv_obj_set_style_border_width(selected->obj, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(selected->obj, lv_color_hex(0x606060), LV_PART_MAIN);
    }

    selected = card;

    if (selected && selected->obj) {
        lv_obj_set_style_border_width(selected->obj, 3, LV_PART_MAIN);
        lv_obj_set_style_border_color(selected->obj, lv_color_hex(0x00b8ff), LV_PART_MAIN);
        lv_obj_scroll_to_view(selected->obj, LV_ANIM_ON);
    }
}

static void card_pressed_cb(lv_event_t *e) {
    select_card(lv_event_get_user_data(e));
}

static void card_focused_cb(lv_event_t *e) {
    select_card(lv_event_get_user_data(e));
}

static void free_card(sstv_card_t *card) {
    if (!card) {
        return;
    }

    if (card->draw_buf) {
        lv_image_cache_drop(card->draw_buf);
        lv_draw_buf_destroy(card->draw_buf);
    }

    free(card);
}

static void unlink_card(sstv_card_t *card) {
    sstv_card_t **link = &cards;

    while (*link && *link != card) {
        link = &(*link)->next;
    }

    if (*link) {
        *link = card->next;
    }
}

static sstv_card_t *create_card(const char *filename, int width, int height, bool live) {
    sstv_card_t *card = calloc(1, sizeof(*card));

    if (!card) {
        return NULL;
    }

    card->live = live;

    if (filename) {
        snprintf(card->filename, sizeof(card->filename), "%s", filename);
    }

    card->obj = lv_obj_create(gallery);

    lv_obj_set_width(card->obj, CARD_WIDTH);
    lv_obj_set_height(card->obj, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(card->obj, 7, LV_PART_MAIN);
    lv_obj_set_style_bg_color(card->obj, lv_color_hex(0x181818), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card->obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card->obj, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card->obj, lv_color_hex(0x606060), LV_PART_MAIN);
    lv_obj_set_flex_flow(card->obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card->obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(card->obj, card_pressed_cb, LV_EVENT_PRESSED, card);
    lv_obj_add_event_cb(card->obj, card_focused_cb, LV_EVENT_FOCUSED, card);
    lv_obj_add_event_cb(card->obj, dialog_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_flag(card->obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_group_add_obj(keyboard_group, card->obj);

    card->image = lv_image_create(card->obj);

    if (live) {
        card->draw_buf = lv_draw_buf_create((uint32_t) width, (uint32_t) height, LV_COLOR_FORMAT_RGB888, 0);

        if (!card->draw_buf) {
            lv_obj_delete(card->obj);
            free(card);
            return NULL;
        }

        memset(card->draw_buf->data, 0, card->draw_buf->data_size);
        lv_image_set_src(card->image, card->draw_buf);
        lv_image_cache_drop(card->draw_buf);
    } else {
        char lv_path[272];

        snprintf(lv_path, sizeof(lv_path), "A:%s", filename);
        lv_image_set_src(card->image, lv_path);
    }

    int shown_width = width;
    int shown_height = height;

    if (shown_width > IMAGE_MAX_WIDTH || shown_height > IMAGE_MAX_HEIGHT) {
        double scale_x = (double) IMAGE_MAX_WIDTH / shown_width;
        double scale_y = (double) IMAGE_MAX_HEIGHT / shown_height;
        double scale = scale_x < scale_y ? scale_x : scale_y;
        shown_width = (int) (shown_width * scale);
        shown_height = (int) (shown_height * scale);
    }

    lv_obj_set_size(card->image, shown_width, shown_height);
    lv_image_set_inner_align(card->image, LV_IMAGE_ALIGN_CONTAIN);

    card->next = cards;
    cards = card;
    lv_obj_move_to_index(card->obj, 0);

    return card;
}

static bool png_dimensions(const char *filename, int *width, int *height) {
    FILE *fp = fopen(filename, "rb");

    if (!fp) {
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop   info = png ? png_create_info_struct(png) : NULL;

    if (!png || !info || setjmp(png_jmpbuf(png))) {
        if (png) {
            png_destroy_read_struct(&png, info ? &info : NULL, NULL);
        }

        fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_read_info(png, info);
    *width = (int) png_get_image_width(png, info);
    *height = (int) png_get_image_height(png, info);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    return *width > 0 && *height > 0;
}

static int reverse_name_sort(const struct dirent **a, const struct dirent **b) {
    return strcmp((*b)->d_name, (*a)->d_name);
}

static void load_gallery(void) {
    struct dirent   **items = NULL;
    int             count = scandir(SSTV_PATH, &items, NULL, reverse_name_sort);

    for (int i = count - 1; i >= 0; --i) {
        const char  *name = items[i]->d_name;
        size_t      len = strlen(name);

        if (len > 4 && strcmp(name + len - 4, ".png") == 0) {
            char    path[256];
            int     width, height;

            snprintf(path, sizeof(path), "%s/%s", SSTV_PATH, name);

            if (png_dimensions(path, &width, &height)) {
                create_card(path, width, height, false);
            }
        }
        free(items[i]);
    }
    free(items);
}

static bool save_png(const char *filename, const uint8_t *rgb, int width, int height) {
    FILE *fp = fopen(filename, "wb");

    if (!fp) {
        return false;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop   info = png ? png_create_info_struct(png) : NULL;

    if (!png || !info || setjmp(png_jmpbuf(png))) {
        if (png) png_destroy_write_struct(&png, info ? &info : NULL);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    for (int y = 0; y < height; ++y) {
        png_write_row(png, (png_bytep) (rgb + (size_t) y * width * 3));
    }

    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);
    fclose(fp);

    return true;
}

static void decoder_vis(void *user, const sstv_mode_t *mode) {
    (void) user;

    free(rx_rgb);
    rx_mode = mode;
    rx_width = mode->width;
    rx_height = sstv_mode_height(mode);
    rx_rows = 0;
    rx_rgb = calloc((size_t) rx_width * rx_height, 3);

    if (!rx_rgb) {
        return;
    }

    vis_event_t *event = malloc(sizeof(*event));

    if (!event) {
        return;
    }

    event->width = rx_width;
    event->height = rx_height;
    snprintf(event->mode, sizeof(event->mode), "%s", mode->short_name);
    queue_send(dialog.obj, EVENT_SSTV_VIS, event);
}

static void decoder_line(void *user, int row, int width, const uint8_t *rgb) {
    (void) user;

    if (!rx_rgb || row < 0 || row >= rx_height || width != rx_width) {
        return;
    }

    memcpy(rx_rgb + (size_t) row * width * 3, rgb, (size_t) width * 3);

    if (row + 1 > rx_rows) {
        rx_rows = row + 1;
    }

    line_event_t *event = malloc(sizeof(*event) + (size_t) width * 3);

    if (!event) {
        return;
    }

    event->row = row;
    event->width = width;
    memcpy(event->rgb, rgb, (size_t) width * 3);
    queue_send(dialog.obj, EVENT_SSTV_LINE, event);
}

static void decoder_frame(void *user, sstv_frame_status_t status, int decoded_rows) {
    (void) user;

    if (!rx_rgb || !rx_mode || rx_height <= 0) {
        return;
    }

    int             percent = decoded_rows * 100 / rx_height;
    bool            complete = status == SSTV_FRAME_COMPLETE;
    bool            keep = complete || (status == SSTV_FRAME_TRUNCATED && settings_sstv && percent >= settings_sstv->partial_min_percent);
    frame_event_t   *event = calloc(1, sizeof(*event));

    if (!event) {
        return;
    }

    event->complete = complete;
    event->percent = percent;

    if (keep) {
        char timestamp[64];

        get_time_str(timestamp, sizeof(timestamp));
        snprintf(event->filename, sizeof(event->filename), "%s/%s_%s%s.png", SSTV_PATH,
                 timestamp, rx_mode->short_name, complete ? "" : "_partial");
        event->saved = save_png(event->filename, rx_rgb, rx_width, rx_height);
    }

    queue_send(dialog.obj, EVENT_SSTV_FRAME, event);

    free(rx_rgb);
    rx_rgb = NULL;
    rx_mode = NULL;
    rx_width = rx_height = rx_rows = 0;
}

static void vis_event_cb(lv_event_t *e) {
    vis_event_t *event = lv_event_get_param(e);

    if (live_card) {
        unlink_card(live_card);
        lv_obj_delete(live_card->obj);
        free_card(live_card);
    }

    live_card = create_card(NULL, event->width, event->height, true);
    select_card(live_card);

    if (live_card) {
        lv_group_focus_obj(live_card->obj);
    }

    msg_set_text_fmt("Receiving %s", event->mode);
}

static void line_event_cb(lv_event_t *e) {
    line_event_t *event = lv_event_get_param(e);

    if (!live_card || !live_card->draw_buf || event->width != live_card->draw_buf->header.w) {
        return;
    }

    if (event->row < 0 || event->row >= live_card->draw_buf->header.h) {
        return;
    }

    uint8_t *dst = live_card->draw_buf->data + (size_t) event->row * live_card->draw_buf->header.stride;

    /* LVGL's RGB888 draw buffer uses its native BGR byte order. */

    for (int x = 0; x < event->width; ++x) {
        dst[3 * x] = event->rgb[3 * x + 2];
        dst[3 * x + 1] = event->rgb[3 * x + 1];
        dst[3 * x + 2] = event->rgb[3 * x];
    }

    lv_image_cache_drop(live_card->draw_buf);
    lv_obj_invalidate(live_card->image);
}

static void frame_event_cb(lv_event_t *e) {
    frame_event_t *event = lv_event_get_param(e);

    if (!live_card) {
        return;
    }

    if (event->saved) {
        live_card->live = false;
        snprintf(live_card->filename, sizeof(live_card->filename), "%s", event->filename);
        msg_set_text_fmt("SSTV saved %i%%", event->percent);
        live_card = NULL;
    } else {
        sstv_card_t *discard = live_card;

        live_card = NULL;

        if (selected == discard) {
            selected = NULL;
        }

        unlink_card(discard);
        lv_obj_delete(discard->obj);
        free_card(discard);
        msg_set_text_fmt(event->complete ? "SSTV save error" : "SSTV discarded %i%%", event->percent);
    }
}

static void *decode_worker(void *arg) {
    (void) arg;

    float complex samples[AUDIO_CHUNK];

    while (!atomic_load(&worker_stop)) {
        pthread_mutex_lock(&audio_mutex);

        while (!atomic_load(&worker_stop) && cbuffercf_size(audio_buffer) == 0) {
            pthread_cond_wait(&audio_cond, &audio_mutex);
        }

        unsigned int count = cbuffercf_size(audio_buffer);

        if (count > AUDIO_CHUNK) {
            count = AUDIO_CHUNK;
        }

        for (unsigned int i = 0; i < count; ++i) {
            cbuffercf_pop(audio_buffer, &samples[i]);
        }

        pthread_mutex_unlock(&audio_mutex);

        if (count) {
            sstv_decoder_push_iq(decoder, samples, count);
        }
    }
    return NULL;
}

static void audio_cb(float complex *samples, size_t n) {
    pthread_mutex_lock(&audio_mutex);

    unsigned int space = cbuffercf_space_available(audio_buffer);

    if (n > space) {
        n = space;
    }

    if (n) {
        cbuffercf_write(audio_buffer, samples, (unsigned int) n);
    }

    pthread_cond_signal(&audio_cond);
    pthread_mutex_unlock(&audio_mutex);
}

static void delete_cb(lv_event_t *e) {
    (void) e;

    if (!selected || selected->live || selected->filename[0] == '\0') {
        return;
    }

    sstv_card_t *card = selected;
    sstv_card_t *next_selection = card->next;

    if (!next_selection && cards != card) {
        next_selection = cards;
    }

    if (unlink(card->filename) != 0) {
        msg_set_text_fmt("Delete error");
        return;
    }

    unlink_card(card);
    lv_obj_delete(card->obj);
    free_card(card);
    selected = NULL;
    select_card(next_selection);

    if (next_selection) {
        lv_group_focus_obj(next_selection->obj);
    }
}

static void threshold_cb(lv_event_t *e) {
    (void) e;

    if (!settings_sstv) {
        return;
    }

    settings_sstv->partial_min_percent += 5;

    if (settings_sstv->partial_min_percent > 100) {
        settings_sstv->partial_min_percent = 0;
    }

    static char label[32];

    snprintf(label, sizeof(label), "Partial\n%i%%", settings_sstv->partial_min_percent);
    threshold_button.label = label;
    buttons_load(1, &threshold_button);
}

static void construct_cb(lv_obj_t *parent) {
    mkdir(SSTV_PATH, 0755);
    settings_sstv_load();
    dialog_init(parent, &dialog);

    gallery = lv_obj_create(dialog.obj);

    lv_obj_set_size(gallery, 780, 384 - 40);
    lv_obj_set_pos(gallery, 10, 20);

    lv_obj_set_style_pad_all(gallery, 7, LV_PART_MAIN);
    lv_obj_set_style_pad_row(gallery, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_column(gallery, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(gallery, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(gallery, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(gallery, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(gallery, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(gallery, LV_DIR_VER);

    lv_obj_add_event_cb(dialog.obj, vis_event_cb, EVENT_SSTV_VIS, NULL);
    lv_obj_add_event_cb(dialog.obj, line_event_cb, EVENT_SSTV_LINE, NULL);
    lv_obj_add_event_cb(dialog.obj, frame_event_cb, EVENT_SSTV_FRAME, NULL);

    load_gallery();
    select_card(cards);

    if (cards) {
        lv_group_focus_obj(cards->obj);
    } else {
        lv_obj_add_event_cb(dialog.obj, dialog_key_cb, LV_EVENT_KEY, NULL);
        lv_group_add_obj(keyboard_group, dialog.obj);
    }
    msg_set_text_fmt("Waiting for SSTV");

    delete_button = (button_item_t) { .label = "Delete", .press = delete_cb };
    threshold_button = (button_item_t) { .press = threshold_cb };

    static char threshold_label[32];

    snprintf(threshold_label, sizeof(threshold_label), "Partial\n%i%%",
             settings_sstv ? settings_sstv->partial_min_percent : 25);

    threshold_button.label = threshold_label;
    buttons_load(0, &delete_button);
    buttons_load(1, &threshold_button);

    audio_buffer = cbuffercf_create(ADC_RATE * 3);
    pthread_mutex_init(&audio_mutex, NULL);
    pthread_cond_init(&audio_cond, NULL);

    sstv_callbacks_t callbacks = {
        .on_vis = decoder_vis,
        .on_line = decoder_line,
        .on_frame = decoder_frame,
    };

    decoder = sstv_decoder_create(&callbacks, NULL);
    atomic_store(&worker_stop, false);
    worker_started = decoder && pthread_create(&worker, NULL, decode_worker, NULL) == 0;

    if (!worker_started) {
        msg_set_text_fmt("SSTV decoder error");
    }
}

static void destruct_cb(void) {
    atomic_store(&worker_stop, true);
    pthread_mutex_lock(&audio_mutex);
    pthread_cond_signal(&audio_cond);
    pthread_mutex_unlock(&audio_mutex);

    if (worker_started) {
        pthread_join(worker, NULL);
    }

    worker_started = false;

    queue_cancel(dialog.obj, EVENT_SSTV_VIS, NULL);
    queue_cancel(dialog.obj, EVENT_SSTV_LINE, NULL);
    queue_cancel(dialog.obj, EVENT_SSTV_FRAME, NULL);

    if (decoder) {
        sstv_decoder_destroy(decoder);
    }

    decoder = NULL;

    if (audio_buffer) {
        cbuffercf_destroy(audio_buffer);
    }

    audio_buffer = NULL;
    pthread_cond_destroy(&audio_cond);
    pthread_mutex_destroy(&audio_mutex);

    free(rx_rgb);
    rx_rgb = NULL;
    rx_mode = NULL;

    if (gallery) {
        lv_obj_delete(gallery);
    }

    gallery = NULL;

    while (cards) {
        sstv_card_t *next = cards->next;
        free_card(cards);
        cards = next;
    }

    selected = live_card = NULL;
    settings_sstv_save();
}
