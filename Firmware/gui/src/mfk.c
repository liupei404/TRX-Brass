/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "lvgl/lvgl.h"
#include "mfk.h"
#include "main_screen.h"
#include "msg.h"
#include "msgs.h"
#include "dsp.h"
#include "radio.h"
#include "cw_key.h"
#include "cw.h"
#include "rtty.h"
#include "util.h"
#include "backlight.h"
#include "voice.h"
#include "dsp/agc.h"
#include "settings/modes.h"
#include "settings/options.h"
#include "settings/rf.h"
#include "olivia/olivia.h"

mfk_state_t  mfk_state = MFK_STATE_EDIT;
mfk_mode_t   mfk_mode = MFK_FILTER_LOW;

void mfk_update(int16_t diff, bool voice) {
    int32_t     i;
    char        *str;
    bool        b;
    float       f;
    bool        update;

    char        mode = mfk_state == MFK_STATE_EDIT ? '^' : '#';

    switch (mfk_mode) {
        case MFK_FILTER_LOW:
            i = radio_change_filter_low(diff);
            msg_set_text_fmt("%c Filter low: %i Hz", mode, i);

            if (diff) {
                voice_delay_say_text_fmt("%i", i);
            } else if (voice) {
                voice_say_text_fmt("Low filter limit");
            }
            break;

        case MFK_FILTER_HIGH:
            i = radio_change_filter_high(diff);
            msg_set_text_fmt("%c Filter high: %i Hz", mode, i);

            if (diff) {
                voice_say_int("High filter limit", i);
            } else if (voice) {
                voice_say_text_fmt("High filter limit");
            }
            break;

        case MFK_FILTER_TRANSITION:
            i = radio_change_filter_transition(diff);
            msg_set_text_fmt("%c Filter transition: %i Hz", mode, i);

            if (diff) {
                voice_say_int("Filter transition", i);
            } else if (voice) {
                voice_say_text_fmt("Filter transition");
            }
            break;

        case MFK_AGC:
            i = dsp_change_rx_agc(diff);

            switch (i) {
                case AGC_OFF:
                    str = "Off";
                    break;

                case AGC_LONG:
                    str = "Long";
                    break;

                case AGC_SLOW:
                    str = "Slow";
                    break;

                case AGC_MED:
                    str = "Medium";
                    break;

                case AGC_FAST:
                    str = "Fast";
                    break;

                case AGC_CUSTOM:
                    str = "Custom";
                    break;
            }

            msg_set_text_fmt("%c AGC mode: %s", mode, str);

            if (diff) {
                voice_say_text("AGC mode", str);
            } else if (voice) {
                voice_say_text_fmt("AGC mode");
            }
            break;

        case MFK_SPECTRUM_FACTOR:
            if (diff != 0) {
                update = false;

                op_mode->spectrum_factor += diff;

                if (op_mode->spectrum_factor < 1) {
                    op_mode->spectrum_factor = 1;
                } else if (op_mode->spectrum_factor > 8) {
                    op_mode->spectrum_factor = 8;
                } else {
                    update = true;
                }

                if (update) {
                    dsp_set_spectrum_factor(op_mode->spectrum_factor);
                    main_screen_update_range();
                }
            }
            msg_set_text_fmt("%c Spectrum zoom: x%i", mode, op_mode->spectrum_factor);

            if (diff) {
                voice_say_int("Spectrum zoom", op_mode->spectrum_factor);
            } else if (voice) {
                voice_say_text_fmt("Spectrum zoom");
            }
            break;

        case MFK_SPECTRUM_BETA:
            if (diff != 0) {
                f = options->spectrum.beta + diff * 0.05f;

                if (f < 0.0f) {
                    f = 0.0f;
                } else if (f > 0.9f) {
                    f = 0.9f;
                }
                options->spectrum.beta = f;
            }
            msg_set_text_fmt("%c Spectrum beta: %.2f", mode, options->spectrum.beta);

            if (diff) {
                voice_say_float("Spectrum beta", options->spectrum.beta);
            } else if (voice) {
                voice_say_text_fmt("Spectrum beta");
            }
            break;

        case MFK_SPECTRUM_FILL:
            if (diff != 0) {
                options->spectrum.filled = !options->spectrum.filled;
            }
            msg_set_text_fmt("%c Spectrum fill: %s", mode, options->spectrum.filled ? "On" : "Off");

            if (diff) {
                voice_say_bool("Spectrum fill", options->spectrum.filled);
            } else if (voice) {
                voice_say_text_fmt("Spectrum fill switcher");
            }
            break;

        case MFK_SPECTRUM_PEAK:
            if (diff != 0) {
                options->spectrum.peak = !options->spectrum.peak;
            }
            msg_set_text_fmt("%c Spectrum peak: %s", mode, options->spectrum.peak ? "On" : "Off");

            if (diff) {
                voice_say_bool("Spectrum peak", options->spectrum.peak);
            } else if (voice) {
                voice_say_text_fmt("Spectrum peak switcher");
            }
            break;

        case MFK_PEAK_HOLD:
            if (diff != 0) {
                options->spectrum.peak_hold = limit(options->spectrum.peak_hold + diff * 1000, 1000, 10000);
            }
            msg_set_text_fmt("%c Peak hold: %i s", mode, options->spectrum.peak_hold / 1000);

            if (diff) {
                voice_say_int("Peak hold time", options->spectrum.peak_hold / 1000);
            } else if (voice) {
                voice_say_text_fmt("Peak hold time");
            }
            break;

        case MFK_PEAK_SPEED:
            if (diff != 0) {
                f = options->spectrum.peak_speed + diff * 0.1f;

                if (f < 0.1f) {
                    f = 0.1f;
                } else if (f > 3.0f) {
                    f = 3.0f;
                }

                options->spectrum.peak_speed = f;
            }
            msg_set_text_fmt("%c Peak speed: %.1f dB", mode, options->spectrum.peak_speed);

            if (diff) {
                voice_say_float("Peak speed time", options->spectrum.peak_speed);
            } else if (voice) {
                voice_say_text_fmt("Peak speed time");
            }
            break;

        case MFK_KEY_SPEED:
            i = cw_key_change_speed(diff);
            msg_set_text_fmt("%c Key speed: %i wpm", mode, i);

            if (diff) {
                voice_say_int("CW key speed", i);
            } else if (voice) {
                voice_say_text_fmt("CW key speed");
            }
            break;

        case MFK_KEY_MODE:
            i = cw_key_change_mode(diff);

            switch (i) {
                case KEY_MODE_MANUAL:
                    str = "Manual";
                    break;

                case KEY_MODE_AUTO_LEFT:
                    str = "Auto-L";
                    break;

                case KEY_MODE_AUTO_RIGHT:
                    str = "Auto-R";
                    break;
            }
            msg_set_text_fmt("%c Key mode: %s", mode, str);

            if (diff) {
                voice_say_text("CW key mode", str);
            } else if (voice) {
                voice_say_text_fmt("CW key mode selector");
            }
            break;

        case MFK_IAMBIC_MODE:
            i = cw_key_change_iambic_mode(diff);

            switch (i) {
                case IAMBIC_A:
                    str = "A";
                    break;

                case IAMBIC_B:
                    str = "B";
                    break;
            }
            msg_set_text_fmt("%c Iambic mode: %s", mode, str);

            if (diff) {
                voice_say_text("Iambic mode", str);
            } else if (voice) {
                voice_say_text_fmt("Iambic mode selector");
            }
            break;

        case MFK_KEY_TONE:
            i = cw_key_change_tone(diff);
            msg_set_text_fmt("%c Key tone: %i Hz", mode, i);

            if (diff) {
                voice_say_int("CW key tone", i);
            } else if (voice) {
                voice_say_text_fmt("CW key tone");
            }
            break;

        case MFK_KEY_TRAIN:
            b = cw_key_change_train(diff);
            msg_set_text_fmt("%c Key train: %s", mode, b ? "On" : "Off");

            if (diff) {
                voice_say_bool("CW key train", b);
            } else if (voice) {
                voice_say_text_fmt("CW key train switcher");
            }
            break;

        case MFK_QSK_TIME:
            i = cw_key_change_qsk_time(diff);
            msg_set_text_fmt("%c QSK time: %i ms", mode, i);

            if (diff) {
                voice_say_int("CW key QSK time", i);
            } else if (voice) {
                voice_say_text_fmt("CW key QSK time");
            }
            break;

        case MFK_KEY_RATIO:
            i = cw_key_change_ratio(diff);
            msg_set_text_fmt("%c Key ratio: %.1f", mode, i * 0.1f);

            if (diff) {
                voice_say_float("CW key ratio", i * 0.1f);
            } else if (voice) {
                voice_say_text_fmt("CW key ratio");
            }
            break;

        case MFK_ANT:
            if (diff != 0) {
                rf->ant = limit(rf->ant + diff, 1, 5);

                brass_msg_send(MSG_ANT_CHANGED, &rf->ant);
            }
            msg_set_text_fmt("%c Antenna : %i", mode, rf->ant);

            if (diff) {
                voice_say_int("Antenna", rf->ant);
            } else if (voice) {
                voice_say_text_fmt("Antenna selector");
            }
            break;

        case MFK_DENOISE_MODE:
            i = dsp_change_denoise(diff);

            switch (i) {
                case DENOISE_OFF:
                    str = "Off";
                    break;

                case DENOISE_NR:
                    str = "NR";
                    break;

                case DENOISE_EMNR:
                    str = "EMNR";
                    break;
            }

            msg_set_text_fmt("%c Denoise: %s", mode, str);

            if (diff) {
                voice_say_text("Denoise mode", str);
            } else if (voice) {
                voice_say_text_fmt("Denoise mode selector");
            }
            break;

        case MFK_CW_DECODER:
            b = cw_change_decoder(diff);
            msg_set_text_fmt("%c CW decoder: %s", mode, b ? "On" : "Off");

            if (diff) {
                voice_say_bool("CW decoder", b);
            } else if (voice) {
                voice_say_text_fmt("CW decoder switcher");
            }
            break;

        case MFK_CW_DECODER_SNR:
            f = cw_change_snr(diff);
            msg_set_text_fmt("%c CW decoder SNR: %.1f dB", mode, f);

            if (diff) {
                voice_say_float("CW decoder SNR level", f);
            } else if (voice) {
                voice_say_text_fmt("CW decoder SNR level");
            }
            break;

        case MFK_CW_DECODER_PEAK_BETA:
            f = cw_change_peak_beta(diff);
            msg_set_text_fmt("%c CW decoder peak beta: %.2f", mode, f);

            if (diff) {
                voice_say_float("CW decoder peak beta", f);
            } else if (voice) {
                voice_say_text_fmt("CW decoder peak beta");
            }
            break;

        case MFK_CW_DECODER_NOISE_BETA:
            f = cw_change_noise_beta(diff);
            msg_set_text_fmt("%c CW decoder noise beta: %.2f", mode, f);

            if (diff) {
                voice_say_float("CW decoder noise beta", f);
            } else if (voice) {
                voice_say_text_fmt("CW decoder noise beta");
            }
            break;

        case MFK_RTTY_RATE:
            f = rtty_change_rate(diff);
            msg_set_text_fmt("%c RTTY rate: %.2f", mode, f);

            if (diff) {
                voice_say_float2("Teletype rate", f);
            } else if (voice) {
                voice_say_text_fmt("Teletype rate");
            }
            break;

        case MFK_RTTY_SHIFT:
            i = rtty_change_shift(diff);
            msg_set_text_fmt("%c RTTY shift: %i Hz", mode, i);

            if (diff) {
                voice_say_int("Teletype frequency shift", i);
            } else if (voice) {
                voice_say_text_fmt("Teletype frequency shift");
            }
            break;

        case MFK_RTTY_CENTER:
            i = rtty_change_center(diff);
            msg_set_text_fmt("%c RTTY center: %i Hz", mode, i);

            if (diff) {
                voice_say_int("Teletype frequency center", i);
            } else if (voice) {
                voice_say_text_fmt("Teletype frequency center");
            }
            break;

        case MFK_RTTY_REVERSE:
            b = rtty_change_reverse(diff);
            msg_set_text_fmt("%c RTTY reverse: %s", mode, b ? "On" : "Off");

            if (diff) {
                voice_say_bool("Teletype reverse", b);
            } else if (voice) {
                voice_say_text_fmt("Teletype reverse switcher");
            }
            break;

        case MFK_OLIVIA_TONES:
            i = olivia_change_tones(diff);
            msg_set_text_fmt("%c Olivia tones: %i", mode, i);

            if (diff) {
                voice_say_int("Olivia tones", i);
            } else if (voice) {
                voice_say_text_fmt("Olivia tones");
            }
            break;

        case MFK_OLIVIA_WIDTH:
            i = olivia_change_width(diff);
            msg_set_text_fmt("%c Olivia band width: %i Hz", mode, i);

            if (diff) {
                voice_say_int("Olivia band width", i);
            } else if (voice) {
                voice_say_text_fmt("Olivia band width");
            }
            break;

        default:
            break;
    }
}

void mfk_press(int16_t dir) {
    while (true) {
        if (dir > 0) {
            if (mfk_mode == MFK_LAST-1) {
                mfk_mode = 0;
            } else {
                mfk_mode++;
            }
        } else {
            if (mfk_mode == 0) {
                mfk_mode = MFK_LAST-1;
            } else {
                mfk_mode--;
            }
        }

        uint64_t mask = (uint64_t) 1L << mfk_mode;

        if (options->control.mfk & mask) {
            break;
        }
    }

    mfk_update(0, true);
}

void mfk_set_mode(mfk_mode_t mode) {
    mfk_mode = mode;
}
