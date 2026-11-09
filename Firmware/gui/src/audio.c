/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

#include <pulse/pulseaudio.h>

#include "lvgl/lvgl.h"
#include "audio.h"
#include "dsp.h"
#include "mic.h"
#include "fpga/adc.h"
#include "settings/options.h"

#define PLAY_RATE_MS    100
#define CAPTURE_RATE_MS 10

static pa_threaded_mainloop *mloop;
static pa_mainloop_api      *mlapi;
static pa_context           *ctx;

static pa_stream            *play_stm;
static pa_stream            *capture_stm;

static pa_stream            *adc_play_stm;

typedef struct {
    audio_device_list_t *list;
    bool                ok;
} device_query_t;

static void on_state_change(pa_context *c, void *userdata) {
    pa_threaded_mainloop_signal(mloop, 0);
}

static void stream_state_cb(pa_stream *s, void *userdata) {
    pa_threaded_mainloop_signal(mloop, 0);
}

static void read_callback(pa_stream *s, size_t nbytes, void *udata) {
    const void *buf = NULL;

    if (pa_stream_peek(s, &buf, &nbytes) < 0)
        return;
    if (buf)
        mic_put_audio_samples(nbytes / sizeof(int16_t), (int16_t *)buf);
    if (nbytes)
        pa_stream_drop(s);
}

/* Called with the mainloop locked. All callbacks wake the waiting caller. */
static bool wait_operation(pa_operation *op) {
    if (!op)
        return false;

    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING &&
           pa_context_get_state(ctx) == PA_CONTEXT_READY)
        pa_threaded_mainloop_wait(mloop);

    bool ok = pa_operation_get_state(op) == PA_OPERATION_DONE;

    if (!ok)
        pa_operation_cancel(op);

    pa_operation_unref(op);
    return ok;
}

void audio_free_devices(audio_device_list_t *list) {
    for (size_t i = 0; i < list->count; i++) {
        free(list->items[i].name);
        free(list->items[i].description);
    }

    free(list->items);
    *list = (audio_device_list_t){0};
}

static void append_device(device_query_t *query, const char *name, const char *description) {
    if (!query->ok)
        return;

    audio_device_t item = {
        .name = strdup(name),
        .description = strdup(description && *description ? description : name)
    };

    audio_device_list_t *list = query->list;
    audio_device_t      *items = NULL;

    if (item.name && item.description)
        items = realloc(list->items, (list->count + 1) * sizeof(*items));

    if (!items) {
        free(item.name);
        free(item.description);
        query->ok = false;
        return;
    }

    list->items = items;
    list->items[list->count++] = item;
}

static void sink_info_cb(pa_context *c, const pa_sink_info *info, int eol, void *userdata) {
    device_query_t *query = userdata;

    if (eol) {
        if (eol < 0)
            query->ok = false;

        pa_threaded_mainloop_signal(mloop, 0);
    } else if (info) {
        append_device(query, info->name, info->description);
    }
}

static void source_info_cb(pa_context *c, const pa_source_info *info, int eol, void *userdata) {
    device_query_t *query = userdata;

    if (eol) {
        if (eol < 0)
            query->ok = false;

        pa_threaded_mainloop_signal(mloop, 0);
    } else if (info) {
        append_device(query, info->name, info->description);
    }
}

bool audio_get_devices(bool capture, audio_device_list_t *list) {
    audio_free_devices(list);

    if (!mloop || !ctx)
        return false;

    device_query_t query = { .list = list, .ok = true };

    pa_threaded_mainloop_lock(mloop);

    bool ok = false;

    if (pa_context_get_state(ctx) == PA_CONTEXT_READY) {
        pa_operation *op = capture
            ? pa_context_get_source_info_list(ctx, source_info_cb, &query)
            : pa_context_get_sink_info_list(ctx, sink_info_cb, &query);

        ok = wait_operation(op) && query.ok;
    }

    pa_threaded_mainloop_unlock(mloop);

    if (!ok)
        audio_free_devices(list);

    return ok;
}

static void move_cb(pa_context *c, int success, void *userdata) {
    *(bool *)userdata = success != 0;

    pa_threaded_mainloop_signal(mloop, 0);
}

static bool move_stream(pa_stream *stream, bool capture, const char *name) {
    if (!stream || pa_stream_get_state(stream) != PA_STREAM_READY)
        return false;

    bool success = false;
    pa_operation *op = capture
        ? pa_context_move_source_output_by_name(ctx, pa_stream_get_index(stream), name, move_cb, &success)
        : pa_context_move_sink_input_by_name(ctx, pa_stream_get_index(stream), name, move_cb, &success);

    return wait_operation(op) && success;
}

bool audio_set_device(bool capture, const char *name) {
    if (!mloop || !ctx)
        return false;

    if (!name || !*name)
        name = capture ? "@DEFAULT_SOURCE@" : "@DEFAULT_SINK@";

    pa_threaded_mainloop_lock(mloop);

    bool ok = false;

    if (pa_context_get_state(ctx) == PA_CONTEXT_READY) {
        if (capture) {
            ok = move_stream(capture_stm, true, name);
        } else if (play_stm && pa_stream_get_state(play_stm) == PA_STREAM_READY) {
            /* Keep both playback streams on the same sink, also on failure. */
            const char  *old = pa_stream_get_device_name(play_stm);
            char        *previous = old ? strdup(old) : NULL;

            if (previous && move_stream(play_stm, false, name)) {
                ok = move_stream(adc_play_stm, false, name);

                if (!ok && !move_stream(play_stm, false, previous))
                    LV_LOG_ERROR("Cannot restore playback device");
            }
            free(previous);
        }
    }

    if (!ok)
        LV_LOG_WARN("Cannot select audio device %s: %s", name, pa_strerror(pa_context_errno(ctx)));

    pa_threaded_mainloop_unlock(mloop);
    return ok;
}

static pa_stream *connect_stream(const char *label, const pa_sample_spec *spec,
                                 const pa_buffer_attr *attr, bool capture, const char *device) {
    if (device && !*device)
        device = NULL;

    for (;;) {
        pa_stream *stream = pa_stream_new(ctx, label, spec, NULL);

        if (!stream)
            return NULL;

        pa_stream_set_state_callback(stream, stream_state_cb, NULL);

        if (capture)
            pa_stream_set_read_callback(stream, read_callback, NULL);

        int result = capture
            ? pa_stream_connect_record(stream, device, attr, PA_STREAM_ADJUST_LATENCY)
            : pa_stream_connect_playback(stream, device, attr, PA_STREAM_ADJUST_LATENCY, NULL, NULL);

        if (result >= 0) {
            while (pa_stream_get_state(stream) == PA_STREAM_CREATING &&
                   pa_context_get_state(ctx) == PA_CONTEXT_READY)
                pa_threaded_mainloop_wait(mloop);

            if (pa_stream_get_state(stream) == PA_STREAM_READY)
                return stream;
        }

        LV_LOG_WARN("Cannot connect %s to %s: %s", label, device ? device : "default",
                    pa_strerror(pa_context_errno(ctx)));

        pa_stream_set_read_callback(stream, NULL, NULL);
        pa_stream_set_state_callback(stream, NULL, NULL);
        pa_stream_disconnect(stream);
        pa_stream_unref(stream);

        if (!device || pa_context_get_state(ctx) != PA_CONTEXT_READY)
            return NULL;

        /* A saved device may be unplugged. Keep audio working via the default. */
        device = NULL;
    }
}

void audio_init() {
    mloop = pa_threaded_mainloop_new();

    if (!mloop)
        return;

    mlapi = pa_threaded_mainloop_get_api(mloop);
    ctx = pa_context_new(mlapi, "Brass GUI");

    if (!ctx || pa_threaded_mainloop_start(mloop) < 0) {
        if (ctx)
            pa_context_unref(ctx);

        ctx = NULL;
        pa_threaded_mainloop_free(mloop);
        mloop = NULL;

        return;
    }

    pa_threaded_mainloop_lock(mloop);
    pa_context_set_state_callback(ctx, on_state_change, NULL);

    if (pa_context_connect(ctx, NULL, 0, NULL) < 0)
        goto failed;

    while (pa_context_get_state(ctx) != PA_CONTEXT_READY) {
        if (!PA_CONTEXT_IS_GOOD(pa_context_get_state(ctx)))
            goto failed;

        pa_threaded_mainloop_wait(mloop);
    }

    LV_LOG_INFO("Connected");

    pa_buffer_attr attr;
    pa_sample_spec spec = { .format = PA_SAMPLE_S16NE, .channels = 1 };

    memset(&attr, 0xff, sizeof(attr));

    spec.rate = AUDIO_PLAY_RATE;
    attr.fragsize = pa_usec_to_bytes(PLAY_RATE_MS * PA_USEC_PER_MSEC, &spec);
    attr.tlength = attr.fragsize * 8;
    play_stm = connect_stream("Brass GUI Play", &spec, &attr, false, options->audio.speaker.device);

    spec.rate = AUDIO_CAPTURE_RATE;
    attr.fragsize = attr.tlength = pa_usec_to_bytes(CAPTURE_RATE_MS * PA_USEC_PER_MSEC, &spec);
    capture_stm = connect_stream("Brass GUI Capture", &spec, &attr, true, options->audio.mic.device);

    spec.rate = ADC_RATE;
    attr.fragsize = ADC_SAMPLES * sizeof(int16_t);
    attr.tlength = attr.fragsize * 16;
    adc_play_stm = connect_stream("Brass GUI ADC", &spec, &attr, false, options->audio.speaker.device);

    pa_threaded_mainloop_unlock(mloop);

    return;

failed:
    LV_LOG_ERROR("PulseAudio connection failed: %s", pa_strerror(pa_context_errno(ctx)));
    pa_threaded_mainloop_unlock(mloop);
}

int audio_play(int16_t *samples_buf, size_t samples) {
    if (!play_stm)
        return -1;

    while (true) {
        size_t size;

        pa_threaded_mainloop_lock(mloop);

        if (pa_stream_get_state(play_stm) != PA_STREAM_READY) {
            pa_threaded_mainloop_unlock(mloop);
            return -1;
        }

        size = pa_stream_writable_size(play_stm);
        pa_threaded_mainloop_unlock(mloop);

        if (size == (size_t)-1)
            return -1;

        if (size >= (samples * 2)) {
            break;
        }

        usleep(1000);
    }

    pa_threaded_mainloop_lock(mloop);

    int res = pa_stream_write(play_stm, samples_buf, samples * 2, NULL, 0, PA_SEEK_RELATIVE);

    if (res < 0) {
        LV_LOG_ERROR("pa_stream_write() failed: %s", pa_strerror(pa_context_errno(ctx)));
    }

    pa_threaded_mainloop_unlock(mloop);
    return res;
}

void audio_play_wait() {
    if (!play_stm)
        return;

    pa_operation    *op;
    int             r;

    pa_threaded_mainloop_lock(mloop);
    op = pa_stream_drain(play_stm, NULL, NULL);
    pa_threaded_mainloop_unlock(mloop);

    if (!op)
        return;

    while (true) {
        pa_threaded_mainloop_lock(mloop);
        r = pa_operation_get_state(op);
        pa_threaded_mainloop_unlock(mloop);

        if (r == PA_OPERATION_DONE || r == PA_OPERATION_CANCELLED) {
            break;
        }

        usleep(1000);
    }

    pa_threaded_mainloop_lock(mloop);
    pa_operation_unref(op);
    pa_threaded_mainloop_unlock(mloop);
}

int audio_adc_play(int16_t *samples_buf, size_t samples) {
    if (!adc_play_stm)
        return -1;

    pa_threaded_mainloop_lock(mloop);

    int res = pa_stream_write(adc_play_stm, samples_buf, samples * 2, NULL, 0, PA_SEEK_RELATIVE);

    if (res < 0) {
        LV_LOG_ERROR("pa_stream_write() failed: %s", pa_strerror(pa_context_errno(ctx)));
    }

    pa_threaded_mainloop_unlock(mloop);

    return res;
}
