/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

#include "lvgl/lvgl.h"
#include "util.h"
#include "radio.h"
#include "dsp.h"
#include "dmr.h"
#include "main_screen.h"
#include "hkey.h"
#include "events.h"
#include "dialog_swrscan.h"
#include "voice.h"
#include "fpga/control.h"
#include "msg.h"
#include "msgs.h"
#include "bands/bands.h"
#include "settings/modes.h"
#include "settings/rf.h"
#include "settings/op_work.h"
#include "hw/gpio.h"
#include "hw/xvrt.h"
#include "hw/pa_bias.h"
#include "hw/iio.h"

#define FLOW_RESTART_TIMOUT     300

static lv_obj_t                 *main_obj;

static radio_state_t            state = RADIO_RX;
static tx_band_item_t           *current_tx_band = NULL;

bool check_freq(uint64_t freq, uint32_t *shift, int32_t *corr);

bool radio_tick() {
/*
    if (x6100_flow_read(pack)) {
        prev_time = now_time;

        static uint8_t delay = 0;

        if (delay++ > 10) {
            delay = 0;
            clock_update_power(pack->vext * 0.1f, pack->vbat*0.1f, pack->batcap);
        }

        dsp_samples(pack->samples, RADIO_SAMPLES);

        switch (state) {
            case RADIO_RX:
                if (pack->flag.tx) {
                    state = RADIO_TX;
                    event_send(main_obj, EVENT_RADIO_TX, NULL);
                }
                break;

            case RADIO_TX:
                if (!pack->flag.tx) {
                    state = RADIO_RX;
                    event_send(main_obj, EVENT_RADIO_RX, NULL);
                } else {
                    tx_info_update(pack->tx_power * 0.1f, pack->vswr * 0.1f, pack->alc_level * 0.1f);
                }
                break;

            case RADIO_ATU_START:
                radio_lock();
                x6100_control_atu_tune(true);
                radio_unlock();
                state = RADIO_ATU_WAIT;
                break;
                
            case RADIO_ATU_WAIT:
                if (pack->flag.tx) {
                    event_send(main_obj, EVENT_RADIO_TX, NULL);
                    state = RADIO_ATU_RUN;
                }
                break;
                
            case RADIO_ATU_RUN:
                if (!pack->flag.tx) {
                    params_atu_save(pack->atu_params);
                    radio_lock();
                    x6100_control_atu_tune(false);
                    radio_unlock();
                    event_send(main_obj, EVENT_RADIO_RX, NULL);
                    
                    if (params.atu) {
                        radio_lock();
                        x6100_control_cmd(x6100_atu_network, pack->atu_params);
                        radio_unlock();
                        params.atu_loaded = true;
                        event_send(main_obj, EVENT_ATU_UPDATE, NULL);
                    }
                    state = RADIO_RX;
                } else {
                    tx_info_update(pack->tx_power * 0.1f, pack->vswr * 0.1f, pack->alc_level * 0.1f);
                }
                break;

            case RADIO_SWRSCAN:
                dialog_swrscan_update(pack->vswr * 0.1f);
                break;
                
            case RADIO_POWEROFF:
                x6100_control_poweroff();
                state = RADIO_OFF;
                break;
                
            case RADIO_OFF:
                break;
        }
        
        hkey_put(pack->hkey);
    } else {
        if (d > FLOW_RESTART_TIMOUT) {
            LV_LOG_WARN("Flow reset");
            prev_time = now_time;
            x6100_flow_restart();
            dsp_reset();
        }
        return true;
    }
    */
    return false;
}

void radio_load_bpf() {
    uint64_t freq = op_work->rx - op_work->shift;

    for (uint8_t i = 0; i < rf->rx_band_count; i++) {
        rx_band_item_t *item = &rf->rx_band[i];

        if (freq < item->to) {
            gpio_set_rf_bpf(item->bpf);
            return;
        }
    }
    LV_LOG_WARN("BPF now found for %llu", freq);
}

void radio_load_lpf() {
    uint64_t freq = op_work->tx - op_work->shift;

    for (uint8_t i = 0; i < rf->tx_band_count; i++) {
        tx_band_item_t *item = &rf->tx_band[i];

        if (freq < item->to) {
            current_tx_band = item;

            gpio_set_rf_lpf(item->lpf);
            iio_set_vref(item->vref);
            return;
        }
    }

    current_tx_band = NULL;
    LV_LOG_WARN("LPF now found for %llu", freq);
}

void radio_rf_route() {
    uint64_t freq = op_work->rx;

    if (freq < 62000000) {
        gpio_set_rf_route(RF_ROUTE_HF);
    } else if (freq < 150000000) {
        gpio_set_rf_route(RF_ROUTE_VHF);
    } else {
        gpio_set_rf_route(RF_ROUTE_UHF);
    }
}

void radio_freq_update() {
    dmr_reset();
    uint32_t    shift;
    int32_t     corr;
    uint64_t    freq = op_work->rx;

    check_freq(freq, &shift, &corr);
    xvrt_update(shift);
    op_work->shift = shift;

    control_set_rx_freq(freq - shift - corr);
    control_set_tx_freq(op_work->tx - shift - corr);
    control_set_fft_freq(op_work->fft - shift - corr);

    radio_load_bpf();
    radio_load_lpf();
    radio_rf_route();
}

static void radio_msg_cb(void *s, brass_msg_t *m) {
    switch (brass_msg_get_id(m)) {
        case MSG_PTT: {
            const int *on = brass_msg_get_payload(m);

            if (*on) {
                if (band_settings && band_settings->tx) {
                    radio_set_ptt(true);
                } else {
                    msg_set_text_fmt("TX not permitted");
                }
            } else {
                radio_set_ptt(false);
            }
        } break;

        default:
            break;
    }
}

void radio_init(lv_obj_t *obj) {
    main_obj = obj;

    control_init();
    dsp_set_spectrum_factor(op_mode->spectrum_factor);
    settings_mode_update(op_work->mode);

    radio_freq_update();
    radio_load_atu();

    brass_msg_subscribe(MSG_PTT, radio_msg_cb, NULL);
}

radio_state_t radio_get_state() {
    return state;
}

void radio_set_freq_rx(uint64_t freq) {
    uint32_t    shift = 0;
    int32_t     corr = 0;

    if (!check_freq(freq, &shift, &corr)) {
        LV_LOG_ERROR("Freq %llu incorrect", freq);
        return;
    }

    xvrt_update(shift);

    op_work->shift = shift;
    op_work->corr = corr;
    op_work->rx = freq;

    control_set_rx_freq(freq - shift - corr);
    radio_rf_route();
    radio_load_bpf();

    brass_msg_send(MSG_FREQ_RX_CHANGED, &op_work->rx);
}

void radio_set_freq_tx(uint64_t freq) {
    uint32_t    shift = 0;
    int32_t     corr = 0;

    if (!check_freq(freq, &shift, &corr)) {
        LV_LOG_ERROR("Freq %llu incorrect", freq);
        return;
    }

    xvrt_update(shift);

    op_work->shift = shift;
    op_work->corr = corr;
    op_work->tx = freq;

    control_set_tx_freq(freq - shift - corr);
    radio_load_lpf();
    radio_load_atu();

    brass_msg_send(MSG_FREQ_TX_CHANGED, &op_work->tx);
}

uint64_t radio_set_freqs(uint64_t rx, uint64_t tx) {
    uint64_t    ret = 0;

    switch (op_work->split) {
        case SPLIT_NONE:
            brass_msg_send(MSG_FREQ_RX_PRE_CHANGED, &rx);
            brass_msg_send(MSG_FREQ_TX_PRE_CHANGED, &tx);
            radio_set_freq_rx(rx);
            radio_set_freq_tx(tx);
            ret = rx;
            break;

        case SPLIT_RX:
            brass_msg_send(MSG_FREQ_RX_PRE_CHANGED, &rx);
            radio_set_freq_rx(rx);
            ret = rx;
            break;

        case SPLIT_TX:
            brass_msg_send(MSG_FREQ_TX_PRE_CHANGED, &tx);
            radio_set_freq_tx(tx);
            ret = tx;
    }

    voice_say_freq(ret);
    return ret;
}

void radio_set_freq_fft(uint64_t freq) {
    op_work->fft = freq;

    control_set_fft_freq(freq - op_work->shift - op_work->corr);
    brass_msg_send(MSG_FREQ_FFT_CHANGED, &op_work->fft);
}

bool check_freq(uint64_t freq, uint32_t *shift, int32_t *corr) {
    if (freq <= 62000000) {
        *shift = 0;
        *corr = 0;

        return true;
    }

    for (uint8_t i = 0; i < rf->xvrt_count; i++) {
        xvrt_item_t *item = &rf->xvrt[i];

        if (freq >= item->start && freq <= item->stop) {
            *shift = item->shift;
            *corr = item->corr;

            return true;
        }
    }

    return false;
}

split_mode_t radio_change_split(int16_t d) {
    if (d == 0) {
        return op_work->split;
    }

    op_work->split = limit(op_work->split + d, 0, 2);

    return op_work->split;
}

bool radio_change_pre() {
    op_work->pre = !op_work->pre;

    voice_say_text_fmt("Preamplifier %s", op_work->pre ? "On" : "Off");
    return op_work->pre;
}

bool radio_change_att() {
    op_work->att = !op_work->att;

    voice_say_text_fmt("Attenuator %s", op_work->att ? "On" : "Off");
    return op_work->att;
}

void radio_filter_get(int32_t *from_freq, int32_t *to_freq) {
    radio_mode_t    mode = op_work->mode;

    switch (mode) {
        case RADIO_MODE_LSB:
            *from_freq = -op_mode->filter.high;
            *to_freq = -op_mode->filter.low;
            break;

        case RADIO_MODE_USB:
        case RADIO_MODE_RTTY:
        case RADIO_MODE_OLIVIA:
            *from_freq = op_mode->filter.low;
            *to_freq = op_mode->filter.high;
            break;

        case RADIO_MODE_CW:
            *from_freq = op_mode->filter.low;
            *to_freq = op_mode->filter.high;
            break;

        case RADIO_MODE_CWR:
            *from_freq = -op_mode->filter.high;
            *to_freq = -op_mode->filter.low;
            break;

        case RADIO_MODE_AM:
        case RADIO_MODE_NFM:
        case RADIO_MODE_DMR:
            *from_freq = -op_mode->filter.high;
            *to_freq = op_mode->filter.high;
            break;

        default:
            *from_freq = 0;
            *to_freq = 0;
    }
}

void radio_change_mode(radio_change_mode_t select) {
    radio_mode_t    mode = op_work->mode;

    switch (select) {
        case RADIO_MODE_NEXT:
            switch (mode) {
                case RADIO_MODE_CW:
                case RADIO_MODE_CWR:
                    mode = RADIO_MODE_LSB;
                    voice_say_text_fmt("L S B modulation");
                    break;

                case RADIO_MODE_LSB:
                case RADIO_MODE_USB:
                    mode = RADIO_MODE_AM;
                    voice_say_text_fmt("A M modulation");
                    break;

                case RADIO_MODE_AM:
                    mode = RADIO_MODE_NFM;
                    voice_say_text_fmt("N F M modulation");
                    break;

                case RADIO_MODE_NFM:
                    mode = RADIO_MODE_RTTY;
                    voice_say_text_fmt("R T T Y modulation");
                    break;

                case RADIO_MODE_RTTY:
                    mode = RADIO_MODE_OLIVIA;
                    voice_say_text_fmt("C W modulation");
                    break;

                case RADIO_MODE_OLIVIA:
                    mode = RADIO_MODE_DMR;
                    voice_say_text_fmt("Olivia modulation");
                    break;

                case RADIO_MODE_DMR:
                    mode = RADIO_MODE_CW;
                    voice_say_text_fmt("D M R modulation");
                    break;
            }
            break;

        case RADIO_MODE_SUBSET:
            switch (mode) {
                case RADIO_MODE_CW:
                    mode = RADIO_MODE_CWR;
                    voice_say_text_fmt("C W R modulation");
                    break;

                case RADIO_MODE_CWR:
                    mode = RADIO_MODE_CW;
                    voice_say_text_fmt("C W modulation");
                    break;

                case RADIO_MODE_LSB:
                    mode = RADIO_MODE_USB;
                    voice_say_text_fmt("U S B modulation");
                    break;

                case RADIO_MODE_USB:
                    mode = RADIO_MODE_LSB;
                    voice_say_text_fmt("L S B modulation");
                    break;
            }
            break;

        default:
            break;
    }

    op_work_set_mode(mode);
}

uint32_t radio_change_filter_low(int32_t df) {
    if (df == 0) {
        return op_mode->filter.low;
    }

    radio_mode_t    mode = op_work->mode;

    if (mode == RADIO_MODE_AM || mode == RADIO_MODE_NFM || mode == RADIO_MODE_DMR) {
        return 0;
    }

    op_mode->filter.low = limit(align_int(op_mode->filter.low + df * 50, 50), 0, op_mode->filter.high - 100);
    dsp_set_filter(&op_mode->filter);

    return op_mode->filter.low;
}

uint32_t radio_change_filter_high(int32_t df) {
    if (df == 0) {
        return op_mode->filter.high;
    }

    radio_mode_t    mode = op_work->mode;

    op_mode->filter.high = limit(align_int(op_mode->filter.high + df * 50, 50), op_mode->filter.low + 100, 6000);
    dsp_set_filter(&op_mode->filter);

    return op_mode->filter.high;
}

uint32_t radio_change_filter_transition(int32_t df) {
    if (df == 0) {
        return op_mode->filter.transition;
    }

    op_mode->filter.transition = limit(align_int(op_mode->filter.transition + df * 2, 2), 50, 200);
    dsp_set_filter(&op_mode->filter);

    return op_mode->filter.transition;
}

void radio_start_atu() {
    if (state == RADIO_RX) {
        state = RADIO_ATU_START;
    }
}

bool radio_start_swrscan() {
    if (state != RADIO_RX) {
        return false;
    }

    state = RADIO_SWRSCAN;
    return true;
}

void radio_stop_swrscan() {
    if (state == RADIO_SWRSCAN) {
        state = RADIO_RX;
    }
}

void radio_load_atu() {
/* FIXME
    if (params.atu) {
        if (op_work->shift) {
            info_atu_update();

            return;
        }

        uint32_t atu = params_atu_load(&params.atu_loaded);

        if (state != RADIO_SWRSCAN) {
            info_atu_update();
        }
    }
*/
}

float radio_change_pwr(int16_t d) {
    if (d == 0) {
        return rf->pwr;
    }

    rf->pwr += d * 0.1f;

    if (rf->pwr > rf->ref_pwr) {
        rf->pwr = rf->ref_pwr;
    } else if (rf->pwr < 0.1f) {
        rf->pwr = 0.1f;
    }

    return rf->pwr;
}

void radio_poweroff() {
    state = RADIO_POWEROFF;
}

void radio_set_ptt(bool on) {
    if (!dmr_ptt(on, op_work->mode == RADIO_MODE_DMR && rf->mode == RF_NORMAL)) {
        if (on) msg_set_text_fmt("Set DMR source_id in modes.yaml before TX");
        return;
    }
    if (on) {
        if (current_tx_band) {
            iio_set_vref(current_tx_band->vref);
        }

        pa_bias_update();
        gpio_set_preamp(false);
        gpio_set_tx(true);

        state = RADIO_TX;
        brass_msg_send(MSG_TX, NULL);
    } else {
        gpio_set_preamp(true);
        gpio_set_tx(false);

        state = RADIO_RX;
        brass_msg_send(MSG_RX, NULL);
    }
}

void radio_stop_tx() {
    if (state == RADIO_TX) {
        state = RADIO_STOP_TX;
    }
}

void radio_set_morse_key(bool on) {
}

void radio_update_pa_meter(int mv, int ma) {
    static float    avr_ma = 0;
    static uint64_t delay = 0;

    if (state == RADIO_TX) {
        uint64_t now = get_time();

        if (rf->mode == RF_SILENCE && now > delay) {
            if (avr_ma < 100) {
                avr_ma = ma;
            } else {
                avr_ma = 0.5f * avr_ma + 0.5f * ma;
            }

            msg_set_text_fmt("PA current: %.0f mA", avr_ma);
            delay = now + 200;
        }
    } else {
        avr_ma = 0;
    }
}
