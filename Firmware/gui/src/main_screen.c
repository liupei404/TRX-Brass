/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include "main_screen.h"
#include "styles.h"
#include "util.h"
#include "radio.h"
#include "events.h"
#include "msg.h"
#include "msg_tiny.h"
#include "dsp.h"
#include "mfk.h"
#include "vol.h"
#include "main.h"
#include "pannel.h"
#include "rtty.h"
#include "screenshot.h"
#include "keyboard.h"
#include "dialog.h"
#include "dialog_settings.h"
#include "dialog_audio_settings.h"
#include "dialog_rf_settings.h"
#include "dialog_freq.h"
#include "dialog_msg_cw.h"
#include "dialog_msg_voice.h"
#include "dialog_swrscan.h"
#include "dialog_ft8.h"
#include "dialog_gps.h"
#include "dialog_qth.h"
#include "dialog_recorder.h"
#include "dialog_callsign.h"
#include "dialog_bluetooth.h"
#include "dialog_sstv.h"
#include "backlight.h"
#include "power.h"
#include "buttons.h"
#include "recorder.h"
#include "voice.h"
#include "python/python.h"
#include "msgs.h"
#include "settings/bands.h"
#include "settings/modes.h"
#include "settings/rf.h"
#include "bands/bands.h"
#include "smeter.h"

static lv_obj_t     *obj = NULL;
static bool         freq_lock = false;
static bool         mode_lock = false;
static bool         band_lock = false;

static void freq_shift(int16_t diff);
static void next_freq_step(bool up);
static uint64_t freq_update();
static void band_changed_cb(void *s, brass_msg_t *m);

void mem_load(uint16_t id) {
    /* REWRITE

    params_memory_load(id);

    if (params_bands_find(params_band.freq_rx, &params.freq_band)) {
        if (params.freq_band.type != 0) {
            params.band = params.freq_band.id;
        } else {
            params.band = -1;
        }
    } else {
        params.band = -1;
    }

    radio_freq_update();
    settings_mode_update(op_work->mode);

    radio_load_atu();

    dsp_auto_clear();
    freq_update();

    if (strlen(params_band.label) > 0) {
        msg_set_text_fmt("%s", params_band.label);
    } else if (id <= MEM_NUM) {
        msg_set_text_fmt("Loaded from memory %i", id);
    }
    */
}

void mem_save(uint16_t id) {
    /* REWRITE
    params_memory_save(id);

    if (id <= MEM_NUM) {
        msg_set_text_fmt("Saved in memory %i", id);
    }
    */
}

/* * */

static void next_freq_step(bool up) {
    switch (op_mode->freq_step) {
        case 10:
            op_mode->freq_step = up ? 100 : 5000;
            break;

        case 100:
            op_mode->freq_step = up ? 500 : 10;
            break;

        case 500:
            op_mode->freq_step = up ? 1000 : 100;
            break;

        case 1000:
            op_mode->freq_step = up ? 5000 : 500;
            break;

        case 5000:
            op_mode->freq_step = up ? 10 : 1000;
            break;

        default:
            break;
    }

    msg_set_text_fmt("Freq step: %i Hz", op_mode->freq_step);
    voice_say_text_fmt("Frequency step %i herz", op_mode->freq_step);
}

static void apps_disable() {
    dialog_destruct();

    rtty_set_state(RTTY_OFF);
}

void main_screen_dialog_deleted_cb() {
    buttons_unload_page();
    buttons_load_page(PAGE_VOL_1);
}

void main_screen_app(app_t app) {
    apps_disable();
    buttons_unload_page();

    switch (app) {
        case APP_SETTINGS:
            dialog_construct(dialog_settings, obj);
            voice_say_text_fmt("Settings window");
            break;

        case APP_FT8:
            dialog_construct(dialog_ft8, obj);
            voice_say_text_fmt("FT8 window");
            break;

        case APP_GPS:
            dialog_construct(dialog_gps, obj);
            voice_say_text_fmt("GPS window");
            break;

        case APP_RECORDER:
            dialog_construct(dialog_recorder, obj);
            voice_say_text_fmt("Audio recorder window");
            break;

        case APP_MSG_VOICE:
            dialog_construct(dialog_msg_voice, obj);
            voice_say_text_fmt("Voice messages window");
            break;

        case APP_AUDIO_SETTINGS:
            dialog_construct(dialog_audio_settings, obj);
            voice_say_text_fmt("Audio settings window");
            break;

        case APP_RF_SETTINGS:
            dialog_construct(dialog_rf_settings, obj);
            voice_say_text_fmt("R F settings window");
            break;

        case APP_BLUETOOTH:
            dialog_construct(dialog_bluetooth, obj);
            voice_say_text_fmt("BlueTooth window");
            break;

        case APP_SSTV:
            dialog_construct(dialog_sstv, obj);
            voice_say_text_fmt("S S T V window");
            break;

        default:
            break;
    }
}

void main_screen_action(options_action_t action) {
    switch (action) {
        case ACTION_NONE:
            break;

        case ACTION_SCREENSHOT:
            screenshot_take();
            break;

        case ACTION_RECORDER:
            if (recorder_is_on()) {
                recorder_set_on(false);
                voice_say_text_fmt("Audio recorder off");
            } else {
                voice_say_text_fmt("Audio recorder on");
                recorder_set_on(true);
            }
            break;

        case ACTION_MUTE:
            dsp_change_mute();
            break;

        case ACTION_VOICE_MODE:
            voice_change_mode();
            break;

        case ACTION_STEP_UP:
            next_freq_step(true);
            break;

        case ACTION_STEP_DOWN:
            next_freq_step(false);
            break;

        case ACTION_APP_FT8:
            main_screen_app(APP_FT8);
            break;

        case ACTION_APP_SETTINGS:
            main_screen_app(APP_SETTINGS);
            break;

        case ACTION_APP_RECORDER:
            main_screen_app(APP_RECORDER);
            break;

        case ACTION_APP_QTH:
            dialog_construct(dialog_qth, obj);
            voice_say_text_fmt("QTH window");
            break;

        case ACTION_APP_CALLSIGN:
            dialog_construct(dialog_callsign, obj);
            voice_say_text_fmt("Callsign window");
            break;

        case ACTION_APP_FREQ:
            dialog_construct(dialog_freq, obj);
            voice_say_text_fmt("Enter frequency");
            break;
    }
}

static void main_screen_keypad_cb(lv_event_t * e) {
    event_keypad_t *keypad = lv_event_get_param(e);

    if (dialog_is_run()) {
        if (dialog_keypad(keypad)) {
            return;
        }
    }

    switch (keypad->key) {
        case KEYPAD_POWER:
            if (keypad->state == KEYPAD_RELEASE) {
                backlight_switch();
            } else if (keypad->state == KEYPAD_LONG) {
                voice_say_text_fmt("Power off");
                msg_set_text_fmt("Power off");
                radio_poweroff();
                power_shutdown();       /* 真正执行断电，短按仍然只切换背光 */
            }
            break;

        case KEYPAD_VOL:
            if (keypad->state == KEYPAD_RELEASE) {
                buttons_vol();
            } else if (keypad->state == KEYPAD_LONG) {
                main_screen_action(options->control.long_vol);
            }
            break;

        case KEYPAD_MFK:
            if (keypad->state == KEYPAD_RELEASE) {
                buttons_mfk();
            } else if (keypad->state == KEYPAD_LONG) {
                main_screen_action(options->control.long_mfk);
            }
            break;

        case KEYPAD_APP:
            if (keypad->state == KEYPAD_RELEASE) {
                if (dialog_is_run()) {
                    dialog_keypad(keypad);
                } else {
                    buttons_app();
                }
            } else if (keypad->state == KEYPAD_LONG) {
                main_screen_action(options->control.long_app);
            }
            break;

        case KEYPAD_BAND_UP:
            if (keypad->state == KEYPAD_RELEASE) {
                if (!band_lock) {
                    bands_change(true);
                    dialog_send(EVENT_FREQ_UPDATE, NULL);
                }
                dialog_bands_change(true);
            } else if (keypad->state == KEYPAD_LONG) {
                main_screen_action(options->control.long_band_up);
            }
            break;

        case KEYPAD_BAND_DOWN:
            if (keypad->state == KEYPAD_RELEASE) {
                if (!band_lock) {
                    bands_change(false);
                    dialog_send(EVENT_FREQ_UPDATE, NULL);
                }
                dialog_bands_change(false);
            } else if (keypad->state == KEYPAD_LONG) {
                main_screen_action(options->control.long_band_down);
            }
            break;

        case KEYPAD_FST:
            if (keypad->state == KEYPAD_RELEASE) {
                next_freq_step(true);
            } else if (keypad->state == KEYPAD_LONG) {
                next_freq_step(false);
            }
            break;

        case KEYPAD_MODE:
            if (mode_lock) {
                break;
            }

            if (keypad->state == KEYPAD_RELEASE) {
                radio_change_mode(RADIO_MODE_NEXT);
            } else if (keypad->state == KEYPAD_LONG) {
                radio_change_mode(RADIO_MODE_SUBSET);
            }

            /* FIXME
            if (options->mag.info) {
                msg_tiny_set_text_fmt("%s", info_params_mode());
            }
            */
            break;

        case KEYPAD_F1:
            if (keypad->state == KEYPAD_RELEASE) {
                buttons_press(0, false);
            } else if (keypad->state == KEYPAD_LONG) {
                buttons_press(0, true);
            }
            break;

        case KEYPAD_F2:
            if (keypad->state == KEYPAD_RELEASE) {
                buttons_press(1, false);
            } else if (keypad->state == KEYPAD_LONG) {
                buttons_press(1, true);
            }
            break;

        case KEYPAD_F3:
            if (keypad->state == KEYPAD_RELEASE) {
                buttons_press(2, false);
            } else if (keypad->state == KEYPAD_LONG) {
                buttons_press(2, true);
            }
            break;

        case KEYPAD_F4:
            if (keypad->state == KEYPAD_RELEASE) {
                buttons_press(3, false);
            } else if (keypad->state == KEYPAD_LONG) {
                buttons_press(3, true);
            }
            break;

        case KEYPAD_F5:
            if (keypad->state == KEYPAD_RELEASE) {
                buttons_press(4, false);
            } else if (keypad->state == KEYPAD_LONG) {
                buttons_press(4, true);
            }
            break;

        case KEYPAD_F6:
            if (keypad->state == KEYPAD_RELEASE) {
                buttons_press(5, false);
            } else if (keypad->state == KEYPAD_LONG) {
                buttons_press(5, true);
            }
            break;

        case KEYPAD_PTT:
            switch (keypad->state) {
                case KEYPAD_PRESS:
                    radio_set_ptt(true);

                    switch (op_work->mode) {
                        case RADIO_MODE_CW:
                        case RADIO_MODE_CWR:
                            radio_set_morse_key(true);
                            break;
                    }
                    break;

                case KEYPAD_RELEASE:
                case KEYPAD_LONG_RELEASE:
                    switch (op_work->mode) {
                        case RADIO_MODE_CW:
                        case RADIO_MODE_CWR:
                            radio_set_morse_key(false);
                            break;
                    }

                    radio_set_ptt(false);
                    break;
            }
            break;

        default:
            LV_LOG_WARN("Unsuported key");
            break;
    }
}

static void main_screen_hkey_cb(lv_event_t * e) {
    event_hkey_t *hkey = lv_event_get_param(e);

    switch (hkey->key) {
        case HKEY_1:
        case HKEY_2:
        case HKEY_3:
        case HKEY_4:
        case HKEY_5:
        case HKEY_6:
        case HKEY_7:
        case HKEY_8:
            if (hkey->state == HKEY_RELEASE) {
                mem_load(hkey->key - HKEY_1 + 1);
                voice_say_text_fmt("Memory %i loaded", hkey->key - HKEY_1 + 1);
            } else if (hkey->state == HKEY_LONG) {
                mem_save(hkey->key - HKEY_1 + 1);
                voice_say_text_fmt("Memory %i stored", hkey->key - HKEY_1 + 1);
            }
            break;

        case HKEY_UP:
            if (hkey->state == HKEY_RELEASE) {
                if (!freq_lock) {
                    freq_shift(+1);
                }
            } else if (hkey->state == HKEY_LONG) {
                if (!band_lock) {
                    bands_change(true);
                    dialog_send(EVENT_FREQ_UPDATE, NULL);
                }
                dialog_bands_change(true);
            }
            break;

        case HKEY_DOWN:
            if (hkey->state == HKEY_RELEASE) {
                if (!freq_lock) {
                    freq_shift(-1);
                }
            } else if (hkey->state == HKEY_LONG) {
                if (!band_lock) {
                    bands_change(false);
                    dialog_send(EVENT_FREQ_UPDATE, NULL);
                }
                dialog_bands_change(false);
            }
            break;

        case HKEY_P1 ... HKEY_P4:
            if (hkey->state == HKEY_RELEASE) {
                main_screen_action(options->hkeys.press_p[hkey->key - HKEY_P1]);
            } else if (hkey->state == HKEY_LONG) {
            }
            break;

        case HKEY_A ... HKEY_D:
            if (hkey->state == HKEY_RELEASE) {
                main_screen_action(options->hkeys.press_char[hkey->key - HKEY_A]);
            } else if (hkey->state == HKEY_LONG) {
            }
            break;

        default:
            break;
    }
}

static uint16_t freq_accel(uint16_t diff) {
    if (diff < 3) {
        return 1;
    }

    switch (options->freq.accel) {
        case FREQ_ACCEL_NONE:
            return 1;

        case FREQ_ACCEL_LITE:
            return (diff < 6) ? 5 : 10;

        case FREQ_ACCEL_STRONG:
            return (diff < 6) ? 10 : 30;
    }
}

static void freq_shift(int16_t diff) {
    if (freq_lock) {
        return;
    }

    uint64_t    prev_freq_rx = op_work->rx;
    uint64_t    prev_freq_tx = op_work->tx;
    uint64_t    prev_freq_fft = op_work->fft;

    int32_t     df = diff * op_mode->freq_step * freq_accel(abs(diff));

    uint64_t    freq_rx = align_long(prev_freq_rx + df, abs(df));
    uint64_t    freq_tx = align_long(prev_freq_tx + df, abs(df));
    uint64_t    freq_fft = align_long(prev_freq_fft + df, abs(df));
    int32_t     freq_shift = 0;

    switch (options->freq.mode) {
        case FREQ_MODE_JOIN:
            radio_set_freq_fft(freq_fft);
            radio_set_freqs(freq_rx, freq_tx);
            freq_shift = freq_fft - prev_freq_fft;
            brass_msg_send(MSG_FREQ_FFT_SHIFT, &freq_shift);
            voice_say_freq(freq_rx);
            break;

        case FREQ_MODE_SLIDE:
        {
            uint64_t tuned_freq = op_work->split == SPLIT_TX ? freq_tx : freq_rx;
            int32_t span = 100000 / op_mode->spectrum_factor;
            int32_t trigger = span * 45 / 100;
            int64_t offset = (int64_t)tuned_freq - (int64_t)prev_freq_fft;

            if (offset < -trigger) {
                freq_fft = tuned_freq + trigger;
                freq_shift = freq_fft - prev_freq_fft;
            } else if (offset > trigger) {
                freq_fft = tuned_freq - trigger;
                freq_shift = freq_fft - prev_freq_fft;
            }

            if (freq_shift != 0) {
                radio_set_freq_fft(freq_fft);
                brass_msg_send(MSG_FREQ_FFT_SHIFT, &freq_shift);
            }

            radio_set_freqs(freq_rx, freq_tx);
            break;
        }

        case FREQ_MODE_RX_ONLY:
            radio_set_freqs(freq_rx, freq_tx);
            break;

        case FREQ_MODE_FFT_ONLY:
            radio_set_freq_fft(freq_fft);
            freq_shift = freq_fft - prev_freq_fft;
            brass_msg_send(MSG_FREQ_FFT_SHIFT, &freq_shift);
            break;

        default:
            break;
    }

    if (options->mag.freq) {
        uint16_t    mhz, khz, hz;

        split_freq(freq_rx, &mhz, &khz, &hz);

        if (mhz < 100) {
            msg_tiny_set_text_fmt("%i.%03i.%03i", mhz, khz, hz);
        } else {
            msg_tiny_set_text_fmt("%i.%03i", mhz, khz);
        }
    }

    dialog_send(EVENT_FREQ_UPDATE, NULL);
    pannel_fade();
}

static void main_screen_rotary_cb(lv_event_t * e) {
    int32_t     diff = (int32_t) (intptr_t) lv_event_get_param(e);

    freq_shift(diff);
    dialog_rotary(diff);
}

static void main_screen_key_cb(lv_event_t * e) {
    uint32_t key = *((uint32_t *)lv_event_get_param(e));

    switch (key) {
        case '-':
            if (!freq_lock) {
                freq_shift(-1);
            }
            break;

        case '=':
            if (!freq_lock) {
                freq_shift(+1);
            }
            break;

        case '_':
            next_freq_step(false);
            break;

        case '+':
            next_freq_step(true);
            break;

        case KEY_VOL_LEFT_EDIT:
        case '[':
            vol_update(-1, false);
            break;

        case KEY_VOL_RIGHT_EDIT:
        case ']':
            vol_update(+1, false);
            break;

        case KEY_VOL_LEFT_SELECT:
        case '{':
            vol_press(-1);
            break;

        case KEY_VOL_RIGHT_SELECT:
        case '}':
            vol_press(+1);
            break;

        case LV_KEY_LEFT:
            switch (mfk_state) {
                case MFK_STATE_EDIT:
                    mfk_update(-1, false);
                    break;

                case MFK_STATE_SELECT:
                    mfk_press(-1);
                    break;
            }
            break;

        case LV_KEY_RIGHT:
            switch (mfk_state) {
                case MFK_STATE_EDIT:
                    mfk_update(+1, false);
                    break;

                case MFK_STATE_SELECT:
                    mfk_press(+1);
                    break;
            }
            break;

        case LV_KEY_ESC:
            if (!dialog_is_run()) {
                switch (vol->mode) {
                    case VOL_EDIT:
                        vol->mode = VOL_SELECT;
                        voice_say_text_fmt("Selection mode");
                        break;

                    case VOL_SELECT:
                        vol->mode = VOL_EDIT;
                        voice_say_text_fmt("Edit mode");
                        break;
                }
                vol_update(0, false);
            }
            break;

        case KEYBOARD_PRINT:
        case KEYBOARD_PRINT_SCR:
            screenshot_take();
            break;

        case KEYBOARD_SCRL_LOCK:
            freq_lock = !freq_lock;
            break;

        case KEYBOARD_PGUP:
            if (!band_lock) {
                bands_change(true);
                dialog_send(EVENT_FREQ_UPDATE, NULL);
            }
            dialog_bands_change(true);
            break;

        case KEYBOARD_PGDN:
            if (!band_lock) {
                bands_change(false);
                dialog_send(EVENT_FREQ_UPDATE, NULL);
            }
            dialog_bands_change(false);
            break;

        case KEYBOARD_F9:
            dialog_construct(dialog_freq, obj);
            voice_say_text_fmt("Enter frequency");
            break;

        case KEYBOARD_F10:
            main_exit();
            break;

        default:
            break;
    }
}

static void main_screen_pressed_cb(lv_event_t * e) {
    switch (mfk_state) {
        case MFK_STATE_EDIT:
            mfk_state = MFK_STATE_SELECT;
            voice_say_text_fmt("Selection mode");
            break;

        case MFK_STATE_SELECT:
            mfk_state = MFK_STATE_EDIT;
            voice_say_text_fmt("Edit mode");
            break;
    }
    mfk_update(0, false);
}

static void mode_changed_cb(void *s, brass_msg_t *m) {
    main_screen_update_finder();
}

void main_screen_keys_enable(bool value) {
    if (value) {
        lv_group_add_obj(keyboard_group, obj);
        lv_group_set_editing(keyboard_group, true);
    } else {
        lv_group_remove_obj(obj);
        lv_group_set_editing(keyboard_group, false);
    }
}

void main_screen_lock_freq(bool lock) {
    freq_lock = lock;
}

void main_screen_lock_band(bool lock) {
    band_lock = lock;
}

void main_screen_lock_mode(bool lock) {
    mode_lock = lock;
    /* FIXME info lock mode */
}

void main_screen_set_freq(uint64_t freq) {
    radio_set_freqs(freq, freq);
    radio_set_freq_fft(freq);
    dsp_auto_clear();
}

lv_obj_t * main_screen() {
    obj = python_main_screen_get();

    lv_obj_add_event_cb(obj, main_screen_rotary_cb, EVENT_ROTARY, NULL);
    lv_obj_add_event_cb(obj, main_screen_keypad_cb, EVENT_KEYPAD, NULL);
    lv_obj_add_event_cb(obj, main_screen_hkey_cb, EVENT_HKEY, NULL);

    lv_obj_add_event_cb(obj, main_screen_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(obj, main_screen_pressed_cb, LV_EVENT_PRESSED, NULL);

    brass_msg_subscribe(MSG_MODE_CHANGED, mode_changed_cb, NULL);
    brass_msg_subscribe(MSG_BAND_CHANGED, band_changed_cb, NULL);

    main_screen_keys_enable(true);

    buttons_init();
    pannel_init(obj);

    brass_msg_send(MSG_MODE_CHANGED, &op_work->mode);
    brass_msg_send(MSG_FREQ_RX_CHANGED, &op_work->rx);
    brass_msg_send(MSG_FREQ_TX_CHANGED, &op_work->tx);
    brass_msg_send(MSG_FREQ_FFT_CHANGED, &op_work->fft);

    brass_msg_send(MSG_AGC_CHANGED, &op_mode->agc);
    brass_msg_send(MSG_ANT_CHANGED, &rf->ant);
    brass_msg_send(MSG_SPLIT_CHANGED, &op_work->split);

    dsp_auto_clear();

    msg_set_text_fmt("TRX Brass " VERSION);

    return obj;
}

bool main_screen_ready() {
    return obj != NULL;
}

static void band_changed_cb(void *s, brass_msg_t *m) {
    brass_msg_send(MSG_ANT_CHANGED, &rf->ant);
    brass_msg_send(MSG_SPLIT_CHANGED, &op_work->split);

    dsp_auto_clear();

    switch (options->freq.mode) {
        case FREQ_MODE_SLIDE:
            if (op_work) {
                radio_set_freq_fft(op_work->fft);
            }
            break;

        default:
            break;
    }
}

void main_screen_update_range() {
    op_work->fft = op_work->rx;
    radio_set_freq_fft(op_work->fft);
}

void main_screen_update_finder() {
    msg_finder_cursor_t msg;

    msg.cursor[0] = 0;

    switch (op_work->mode) {
        case RADIO_MODE_LSB:
        case RADIO_MODE_USB:
        case RADIO_MODE_AM:
        case RADIO_MODE_NFM:
        case RADIO_MODE_CW:
        case RADIO_MODE_CWR:
        case RADIO_MODE_DMR:
            msg.num = 2;
            msg.cursor[1] = 0;
            break;

        case RADIO_MODE_RTTY:
            msg.num = 3;
            msg.cursor[1] = options->rtty.center - options->rtty.shift / 2;
            msg.cursor[2] = options->rtty.center + options->rtty.shift / 2;
            break;

        case RADIO_MODE_OLIVIA:
            msg.num = 3;
            msg.cursor[1] = options->olivia.band_lower;
            msg.cursor[2] = options->olivia.band_lower + options->olivia.band_width;
            break;
    }

    brass_msg_send(MSG_FINDER_CURSOR, &msg);
}
