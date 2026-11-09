/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "vol.h"
#include "msg.h"
#include "msgs.h"
#include "radio.h"
#include "main.h"
#include "voice.h"
#include "dsp.h"
#include "util.h"
#include "settings/options.h"
#include "settings/op_work.h"

static vol_mode_t   vol_mode = VOL_VOL;

void vol_update(int16_t diff, bool voice) {
    int32_t     i;
    float       f;
    char        *str;
    bool        b;

    char        mode = vol->mode == VOL_EDIT ? '^' : '#';

    switch (vol_mode) {
        case VOL_VOL:
            i = dsp_change_vol(diff);
            msg_set_text_fmt("%c Volume: %i", mode, i);

            if (diff) {
                voice_say_int("Audio level", i);
            } else if (voice) {
                voice_say_text_fmt("Audio level");
            }
            break;

        case VOL_PWR:
            f = radio_change_pwr(diff);
            msg_set_text_fmt("%c Power: %0.1f W", mode, f);

            if (diff) {
                voice_say_float("Transmit power", f);
            } else if (voice) {
                voice_say_text_fmt("Transmit power");
            }
            break;

        case VOL_VOICE_LANG:
            str = voice_change(diff);
            msg_set_text_fmt("%c Voice: %s", mode, str);

            if (diff) {
                voice_say_lang();
            } else if (voice) {
                voice_say_text_fmt("Voice selector");
            }
            break;

        case VOL_VOICE_RATE:
            i = limit(options->voice.rate + diff, 50, 150);
            msg_set_text_fmt("%c Voice rate: %i", mode, i);
            options->voice.rate = i;

            if (diff == 0 && voice) {
///                voice_say_text_fmt(params.voice_rate.voice);
            }
            break;

        case VOL_VOICE_PITCH:
            i = limit(options->voice.pitch + diff, 50, 150);
            msg_set_text_fmt("%c Voice pitch: %i", mode, i);
            options->voice.pitch = i;

            if (diff == 0 && voice) {
///                voice_say_text_fmt(params.voice_pitch.voice);
            }
            break;

        case VOL_VOICE_VOLUME:
            i = limit(options->voice.volume + diff, 50, 150);
            msg_set_text_fmt("%c Voice volume: %i", mode, i);
            options->voice.volume = i;

            if (diff == 0 && voice) {
///                voice_say_text_fmt(params.voice_volume.voice);
            }
            break;

        case VOL_FREQ_MODE:
            i = limit(options->freq.mode + diff, FREQ_MODE_JOIN, FREQ_MODE_FFT_ONLY);
            options->freq.mode = i;

            switch (i) {
                case FREQ_MODE_JOIN:
                    str = "join";
                    break;

                case FREQ_MODE_SLIDE:
                    str = "slide";
                    break;

                case FREQ_MODE_RX_ONLY:
                    str = "RX only";
                    break;

                case FREQ_MODE_FFT_ONLY:
                    str = "spectrum only";
                    break;
            }

            msg_set_text_fmt("%c Freq mode: %s", mode, str);

            if (diff) {
                voice_say_text("Frequency mode", str);
            } else if (voice) {
                voice_say_text_fmt("Frequency mode");
            }
            break;

        case VOL_SPLIT:
            i = radio_change_split(diff);

            switch (i) {
                case SPLIT_NONE:
                    str = "RX/TX";
                    break;

                case SPLIT_RX:
                    str = "RX";
                    break;

                case SPLIT_TX:
                    str = "TX";
                    break;
            }
            msg_set_text_fmt("%c Split: %s", mode, str);

            if (diff) {
                brass_msg_send(MSG_SPLIT_CHANGED, &op_work->split);
                voice_say_text("Split mode", str);
            } else if (voice) {
                voice_say_text_fmt("Split mode selector");
            }
            break;

        default:
            break;
    }
}

void vol_press(int16_t dir) {
    while (true) {
        if (dir > 0) {
            if (vol_mode == VOL_LAST-1) {
                vol_mode = 0;
            } else {
                vol_mode++;
            }
        } else {
            if (vol_mode == 0) {
                vol_mode = VOL_LAST-1;
            } else {
                vol_mode--;
            }
        }

        if (options->control.vol & (1 << vol_mode)) {
            break;
        }
    }

    vol_update(0, true);
}

void vol_set_mode(vol_mode_t mode) {
    vol_mode = mode;
}
