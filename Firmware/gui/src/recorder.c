/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <time.h>
#include <sys/time.h>
#include <sndfile.h>

#include "audio.h"
#include "dialog_recorder.h"
#include "recorder.h"
#include "msg.h"
#include "msgs.h"
#include "settings/options.h"
#include "fpga/adc.h"

#define DECIM       128
#define INTER       441

char                        *recorder_path = "/mnt/rec";

static bool                 on = false;
static SNDFILE              *file = NULL;

static cbufferf             in_buf;
static rresamp_rrrf         resamp;
static float                resamp_buf[INTER];

static bool create_file() {
    SF_INFO sfinfo;

    memset(&sfinfo, 0, sizeof(sfinfo));

    sfinfo.channels = 1;

    char        filename[64];
    time_t      now = time(NULL);
    struct tm   *t = localtime(&now);

    sfinfo.samplerate = AUDIO_PLAY_RATE;

    switch (options->audio.rec_format) {
        case REC_FORMAT_MP3:
            sfinfo.format = SF_FORMAT_MPEG | SF_FORMAT_MPEG_LAYER_III;

            snprintf(filename, sizeof(filename),
                "%s/REC_%04i%02i%02i_%02i%02i%02i.mp3",
                recorder_path, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec
            );
            break;

        case REC_FORMAT_WAV:
            sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

            snprintf(filename, sizeof(filename),
                "%s/REC_%04i%02i%02i_%02i%02i%02i.wav",
                recorder_path, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec
            );
            break;
    }

    file = sf_open(filename, SFM_WRITE, &sfinfo);

    if (file == NULL) {
        return false;
    }

    double q = 0.25;
    sf_command(file, SFC_SET_VBR_ENCODING_QUALITY, &q, sizeof(q));

    return true;
}

void recorder_init() {
    resamp = rresamp_rrrf_create_default(INTER, DECIM);     /* AUDIO_PLAY_RATE <- ADC_RATE */
    in_buf = cbufferf_create(ADC_RATE / 4);
}

void recorder_set_on(bool x) {
    if (x) {
        if (!create_file()) {
            msg_set_text_fmt("Problem with create file");
            return;
        } else {
            msg_set_text_fmt("Recorder is on");
        }
        on = true;
    } else {
        msg_set_text_fmt("Recorder is off");
        on = false;
        sf_close(file);
    }

    brass_msg_send(MSG_RECORDER, &on);
    dialog_recorder_set_on(on);
}

bool recorder_is_on() {
    return on;
}

void recorder_put_audio_samples(float *data, uint16_t samples) {
    cbufferf_write(in_buf, data, samples);

    if (cbufferf_size(in_buf) < ADC_RATE / 8) {
        return;
    }

    while (cbufferf_size(in_buf) > DECIM) {
        unsigned int    n;
        float*          buf;

        cbufferf_read(in_buf, DECIM, &buf, &n);
        rresamp_rrrf_execute(resamp, buf, resamp_buf);
        cbufferf_release(in_buf, n);

        sf_write_float(file, resamp_buf, INTER);
    }
}
