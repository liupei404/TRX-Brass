/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include "dmr.h"
#include "dmr/src/phy.h"
#include "dmr/src/voice.h"
#include "dmr/src/vocoder.h"
#include "dmr/src/embedded.h"
#include "dmr/src/data.h"
#include "mic.h"
#include "msg.h"
#include "radio.h"
#include "main.h"
#include "fpga/adc.h"
#include "fpga/dac.h"
#include "settings/modes.h"
#include "dialog_num.h"

#include <liquid/liquid.h>
#include <math.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* One burst carries 3 x 20 ms AMBE frames. RF occupies 30 ms of each 60 ms.
 * Streaming rational resamplers retain history, including the silent slot. */

enum {
    PCM_BURST = 480,
    IQ_PERIOD = 1152,
    DAC_PERIOD = 768,
    AUDIO_QUEUE = 3072,
    MIC_BLOCK = 128
};

_Static_assert(ADC_RATE == 12800 && DAC_RATE == 12800, "DMR frontend rate");

static bool             ready;
static bool             rx_debug;
static atomic_uint      rx_epoch;
static atomic_bool      tx_active, tx_stop;

static struct {
    dmr_phy_rx_t    *phy;
    rresamp_crcf    iq_resamp;
    rresamp_rrrf    audio_resamp;

    struct {
        vocoder_state_t             *codec;
        dmr_embedded_lc_decoder_t   *lc;
        uint64_t                    last;
        bool                        voice, allowed;
        uint32_t                    source_id, target_id;
        bool                        ids_shown, group_call;
    } slot[2];

    unsigned        epoch;
    bool            active;
    int             playing_slot;
    float complex   input[2];
    size_t          fill;
    uint64_t        samples;
    float           audio[AUDIO_QUEUE];
    float           audio_gain;
    size_t          read, count, play_delay;
    uint64_t        report_at;
    unsigned        syncs, frames, emb_errors, fec_errors, underflows;
    double          raw_power;
    unsigned        raw_samples;
} rx;

static struct {
    dmr_phy_tx_t    *phy;
    vocoder_state_t *codec;
    rresamp_crcf    iq_resamp;
    rresamp_rrrf    audio_resamp;
    uint8_t         lc[9];
    dmr_emb_t       fragments[4];
    unsigned        sequence, headers;
    bool            active, ending, terminated;
    float           input[8];
    size_t          fill;
    float           pcm[PCM_BURST * 2];
    size_t          read, count;
    float complex   output[DAC_PERIOD];
    size_t          pos;
} tx;

void dmr_reset(void) {
    atomic_fetch_add(&rx_epoch, 1);
}

static void reset_slot(unsigned slot) {
    vocoder_state_reset(rx.slot[slot].codec);
    dmr_embedded_lc_decoder_reset(rx.slot[slot].lc);

    rx.slot[slot].voice = rx.slot[slot].allowed = false;
    rx.slot[slot].last = 0;
    rx.slot[slot].ids_shown = false;

    if (rx.playing_slot == (int)slot) {
        rx.playing_slot = -1;
        rx.audio_gain = 8.0f;
    }
}

static uint32_t id24(const uint8_t *p) {
    return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
}

static bool accept_lc(const uint8_t lc[9]) {
    if (op_modes->dmr.config.monitor)
        return true;

    /* Standard group/private voice only. Reject protected/encrypted LC and
     * proprietary feature sets; never feed data or privacy audio to AMBE. */

    if ((lc[0] & 0xc0) || lc[1] || (lc[2] & 0x40))
        return false;

    if ((lc[0] & 0x3f) == 0)
        return !op_modes->dmr.config.private_call && id24(lc + 3) == op_modes->dmr.config.target_id;

    if ((lc[0] & 0x3f) == 3)
        return op_modes->dmr.config.source_id && id24(lc + 3) == op_modes->dmr.config.source_id;

    return false;
}

static void receive_lc(unsigned slot, const uint8_t lc[9]) {
    rx.slot[slot].allowed = accept_lc(lc);

    if (!rx.slot[slot].allowed)
        return;

    uint32_t source = id24(lc + 6);
    uint32_t target = id24(lc + 3);
    bool group_call = (lc[0] & 0x3f) == 0;

    if (source != rx.slot[slot].source_id || target != rx.slot[slot].target_id ||
        group_call != rx.slot[slot].group_call)
        rx.slot[slot].ids_shown = false;

    rx.slot[slot].source_id = source;
    rx.slot[slot].target_id = target;
    rx.slot[slot].group_call = group_call;
}

/* Speech PCM is much quieter than the analogue AGC output. Level each
 * 20-ms frame with bounded gain and peak headroom, before speaker volume.
 * Do not raise gain on silence; interpolate changes to avoid frame clicks. */

static void level_rx(const int16_t *pcm, float *audio, bool valid) {
    float power = 0, peak = 0;

    for (unsigned i = 0; i < DMR_VOCODER_PCM_SAMPLES; i++) {
        audio[i] = valid ? pcm[i] / 32768.0f : 0;
        power += audio[i] * audio[i];
        peak = fmaxf(peak, fabsf(audio[i]));
    }

    if (valid) {
        rx.raw_power += power;
        rx.raw_samples += DMR_VOCODER_PCM_SAMPLES;
    }

    float rms = sqrtf(power / DMR_VOCODER_PCM_SAMPLES);
    float gain = rx.audio_gain;

    if (rms > 1.0f / 32768.0f) {
        float wanted = fminf(128.0f, 0.20f / rms);

        gain += (wanted - gain) * (wanted < gain ? 0.5f : 0.2f);
    }

    float limit = peak > 0 ? 0.8f / peak : 128.0f;
    float start = fminf(rx.audio_gain, limit);

    gain = fminf(gain, limit);

    for (unsigned i = 0; i < DMR_VOCODER_PCM_SAMPLES; i++)
        audio[i] *= start + (gain - start) * (i + 1) / DMR_VOCODER_PCM_SAMPLES;

    rx.audio_gain = gain;
}

static void receive_burst(void *context, const dmr_burst_t *burst) {
    (void)context;

    if (burst->timeslot < 1 || burst->timeslot > 2)
        return;

    unsigned slot = burst->timeslot - 1;

    if (burst->has_sync)
        rx.syncs++;

    if (rx.slot[slot].last && (burst->first_sample_index < rx.slot[slot].last || burst->first_sample_index - rx.slot[slot].last > 2308)) {
        reset_slot(slot);
    }

    rx.slot[slot].last = burst->first_sample_index;

    if (burst->has_sync && (burst->sync_kind == DMR_SYNC_MS_DATA ||
                           burst->sync_kind == DMR_SYNC_BS_DATA)) {
        uint8_t     lc[9];
        unsigned    cc, type;

        reset_slot(slot);

        rx.slot[slot].last = burst->first_sample_index;

        if (dmr_full_lc_decode(burst, lc, &cc, &type) &&
            type == DMR_DATA_TYPE_VOICE_LC_HEADER && cc == op_modes->dmr.config.color_code) {
            receive_lc(slot, lc);
            rx.slot[slot].voice = rx.slot[slot].allowed;
        }

        return;
    }

    if (burst->has_sync) {
        if (burst->sync_kind != DMR_SYNC_MS_VOICE && burst->sync_kind != DMR_SYNC_BS_VOICE)
            return;

        rx.slot[slot].voice = true;
        dmr_embedded_lc_decoder_reset(rx.slot[slot].lc);
    } else {
        dmr_emb_t           emb;
        dmr_embedded_lc_t   lc;

        if (!rx.slot[slot].voice)
            return;

        if (!dmr_emb_decode(burst, &emb)) {
            /* A damaged QR field is not a new call. Keep previously checked
             * LC/CC and decode the independent voice payload. Without prior
             * authorisation the gate below still stays closed. */
            rx.emb_errors++;
            dmr_embedded_lc_decoder_reset(rx.slot[slot].lc);
        } else {
            if (emb.color_code != op_modes->dmr.config.color_code || emb.privacy) {
                reset_slot(slot);
                return;
            }

            if (dmr_embedded_lc_decoder_process(rx.slot[slot].lc, &emb, &lc))
                receive_lc(slot, lc.data);
        }
    }

    if (!rx.slot[slot].allowed)
        return;

    /* Direct calls have one occupied phase. Never mix unrelated calls. */

    if (rx.playing_slot >= 0 && rx.playing_slot != (int)slot)
        return;

    if (rx.playing_slot < 0) {
        /* One 20-ms reserve absorbs sync refinement/clock jitter instead of
         * playing each 60-ms burst queue down to exactly zero. */
        rx.play_delay = ADC_RATE / 50;
    }

    rx.playing_slot = (int)slot;

    if (!rx.slot[slot].ids_shown) {
        brass_lv_lock();
        msg_set_text_fmt("%s%u -> %u", rx.slot[slot].group_call ? "TG " : "", rx.slot[slot].source_id, rx.slot[slot].target_id);
        brass_lv_unlock();

        rx.slot[slot].ids_shown = true;
    }

    uint8_t frames[DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BYTES];

    if (!dmr_voice_burst_extract(burst, frames))
        return;

    for (unsigned f = 0; f < DMR_VOICE_FRAME_COUNT; f++) {
        int16_t pcm[DMR_VOCODER_PCM_SAMPLES];
        int     corrected = vocoder_state_decode_fec_checked(rx.slot[slot].codec, frames + f * DMR_VOICE_FRAME_BYTES, pcm);

        rx.frames++;

        if (corrected < 0)
            rx.fec_errors++;

        float audio[DMR_VOCODER_PCM_SAMPLES];

        level_rx(pcm, audio, corrected >= 0);

        for (unsigned i = 0; i < DMR_VOCODER_PCM_SAMPLES; i += 5) {
            float out[8];

            rresamp_rrrf_execute(rx.audio_resamp, audio + i, out);

            for (unsigned j = 0; j < 8; j++) {
                if (rx.count == AUDIO_QUEUE) {
                    rx.read = (rx.read + 1) % AUDIO_QUEUE;
                    rx.count--;
                }

                rx.audio[(rx.read + rx.count++) % AUDIO_QUEUE] = out[j];
            }
        }
    }
}

bool dmr_init(void) {
    if (ready)
        return true;

    dmr_phy_rx_config_t cfg = { .burst_callback = receive_burst, .track_timeslots = 1 };

    rx.phy = dmr_phy_rx_create(&cfg);
    tx.phy = dmr_phy_tx_create(NULL);
    tx.codec = vocoder_state_create();

    for (unsigned i = 0; i < 2; i++) {
        rx.slot[i].codec = vocoder_state_create();
        rx.slot[i].lc = dmr_embedded_lc_decoder_create();
    }

    /* liquid 1.6 bandwidth is relative to input rate. Explicit scale gives
     * unity DC gain (the default is energy-normalised). */

    rx.iq_resamp = rresamp_crcf_create_kaiser(3, 2, 24, 5000.0f / 12800, 80);
    tx.iq_resamp = rresamp_crcf_create_kaiser(2, 3, 24, 5000.0f / 19200, 80);
    rx.audio_resamp = rresamp_rrrf_create_kaiser(8, 5, 16, 3400.0f / 8000, 80);
    tx.audio_resamp = rresamp_rrrf_create_kaiser(5, 8, 16, 3400.0f / 12800, 80);

    if (!rx.phy || !tx.phy || !tx.codec || !rx.iq_resamp || !tx.iq_resamp ||
        !rx.audio_resamp || !tx.audio_resamp || !rx.slot[0].codec ||
        !rx.slot[1].codec || !rx.slot[0].lc || !rx.slot[1].lc) {

        dmr_free();
        return false;
    }

    rresamp_crcf_set_scale(rx.iq_resamp, 10000.0f / 12800);
    rresamp_crcf_set_scale(tx.iq_resamp, 10000.0f / 19200);
    rresamp_rrrf_set_scale(rx.audio_resamp, 6800.0f / 8000);
    rresamp_rrrf_set_scale(tx.audio_resamp, 6800.0f / 12800);

    const char *debug = getenv("BRASS_DMR_RX_DEBUG");

    rx_debug = debug && strcmp(debug, "1") == 0;
    ready = true;

    return true;
}

void dmr_free(void) {
    if (rx.phy)
        dmr_phy_rx_destroy(rx.phy);

    if (tx.phy)
        dmr_phy_tx_destroy(tx.phy);

    if (tx.codec)
        vocoder_state_destroy(tx.codec);

    for (unsigned i = 0; i < 2; i++) {
        if (rx.slot[i].codec)
            vocoder_state_destroy(rx.slot[i].codec);

        if (rx.slot[i].lc)
            dmr_embedded_lc_decoder_destroy(rx.slot[i].lc);
    }

    if (rx.iq_resamp)
        rresamp_crcf_destroy(rx.iq_resamp);

    if (tx.iq_resamp)
        rresamp_crcf_destroy(tx.iq_resamp);

    if (rx.audio_resamp)
        rresamp_rrrf_destroy(rx.audio_resamp);

    if (tx.audio_resamp)
        rresamp_rrrf_destroy(tx.audio_resamp);

    memset(&rx, 0, sizeof(rx));
    memset(&tx, 0, sizeof(tx));

    tx_active = tx_stop = false;
    ready = false;
}

static void reset_rx(unsigned epoch) {
    dmr_phy_rx_reset(rx.phy);
    rresamp_crcf_reset(rx.iq_resamp);
    rresamp_rrrf_reset(rx.audio_resamp);

    rx.fill = rx.read = rx.count = rx.play_delay = 0;
    rx.report_at = 19200;
    rx.syncs = rx.frames = rx.emb_errors = rx.fec_errors = rx.underflows = 0;
    rx.raw_power = 0;
    rx.raw_samples = 0;
    rx.samples = 0;
    rx.audio_gain = 8.0f;
    rx.playing_slot = -1;
    reset_slot(0);
    reset_slot(1);
    rx.epoch = epoch;
    rx.active = true;
}

void dmr_receive(bool active, const float complex *iq, float *pcm, size_t count) {
    if (!active) {
        rx.active = false;
        return;
    }

    if (!ready) {
        memset(pcm, 0, count * sizeof(*pcm));
        return;
    }

    unsigned epoch = atomic_load(&rx_epoch);

    if (!rx.active || rx.epoch != epoch)
        reset_rx(epoch);

    /* Interleave processing and playback so latency is independent of the
     * caller's block boundaries, including odd-sized input blocks. */

    for (size_t i = 0; i < count; i++) {
        if (!isfinite(crealf(iq[i])) || !isfinite(cimagf(iq[i]))) {
            reset_rx(epoch);
            pcm[i] = 0;
            continue;
        }

        rx.input[rx.fill++] = iq[i];

        if (rx.fill == 2) {
            float complex converted[3];

            rresamp_crcf_execute(rx.iq_resamp, rx.input, converted);
            rx.samples += 3;

            for (unsigned slot = 0; slot < 2; slot++) {
                if (rx.slot[slot].last && rx.samples > rx.slot[slot].last + 5760)
                    reset_slot(slot);
            }

            dmr_phy_rx_process(rx.phy, converted, 3);
            rx.fill = 0;
        }

        if (rx.play_delay) {
            rx.play_delay--;
            pcm[i] = 0;
            continue;
        }

        if (!rx.count && rx.playing_slot >= 0)
            rx.underflows++;

        pcm[i] = rx.count ? rx.audio[rx.read] : 0;

        if (rx.count) {
            rx.read = (rx.read + 1) % AUDIO_QUEUE;
            rx.count--;
        }
    }

    if (rx_debug && rx.samples >= rx.report_at) {
        fprintf(stderr, "DMR RX: sync=%u frames=%u EMBerr=%u FECerr=%u gaps=%u PCM_RMS=%.1f gain=%.1f queue=%zu\n",
                rx.syncs, rx.frames, rx.emb_errors, rx.fec_errors, rx.underflows,
                rx.raw_samples ? 32768 * sqrt(rx.raw_power / rx.raw_samples) : 0,
                rx.audio_gain, rx.count);
        rx.report_at = rx.samples + 19200;
        rx.syncs = rx.frames = rx.emb_errors = rx.fec_errors = rx.underflows = 0;
        rx.raw_power = 0;
        rx.raw_samples = 0;
    }
}

bool dmr_ptt(bool on, bool dmr_mode) {
    if (on) {
        tx_stop = false;

        return !dmr_mode || (ready && op_modes->dmr.config.source_id != 0);
    }

    if (atomic_load(&tx_active)) {
        tx_stop = true;

        return false;
    }

    return true;
}

void dmr_tx_set_active(bool active) {
    active = active && ready;

    if (active && !tx.active) {
        vocoder_state_reset(tx.codec);
        dmr_phy_tx_reset(tx.phy);
        rresamp_crcf_reset(tx.iq_resamp);
        rresamp_rrrf_reset(tx.audio_resamp);
        tx.fill = tx.read = tx.count = tx.sequence = 0;

        /* Two headers allow the asynchronous mic EQ/AGC to prime and leave
         * a small jitter reserve before the first 60-ms voice deadline. */

        tx.headers = 2;
        tx.pos = DAC_PERIOD;
        tx.ending = tx.terminated = false;
        memset(tx.lc, 0, sizeof(tx.lc));
        tx.lc[0] = op_modes->dmr.config.private_call ? 3 : 0;

        for (unsigned i = 0; i < 3; i++) {
            tx.lc[3 + i] = op_modes->dmr.config.target_id >> (16 - 8 * i);
            tx.lc[6 + i] = op_modes->dmr.config.source_id >> (16 - 8 * i);
        }

        dmr_embedded_lc_encode(tx.lc, op_modes->dmr.config.color_code, 0, tx.fragments);
    }

    tx.active = active;
    tx_active = active;
}

void dmr_tx_audio(float sample) {
    if (!tx.active)
        return;

    tx.input[tx.fill++] = isfinite(sample) ? sample : 0;

    if (tx.fill != 8)
        return;

    float out[5];

    rresamp_rrrf_execute(tx.audio_resamp, tx.input, out);
    tx.fill = 0;

    for (unsigned i = 0; i < 5; i++) {
        float x = out[i];

        if (tx.count == PCM_BURST * 2) {
            tx.read = (tx.read + 1) % (PCM_BURST * 2);
            tx.count--;
        }

        tx.pcm[(tx.read + tx.count++) % (PCM_BURST * 2)] = x;
    }
}

static void prepare_tx_frame(int16_t pcm[DMR_VOCODER_PCM_SAMPLES]) {
    float audio[DMR_VOCODER_PCM_SAMPLES], peak = 0;

    for (unsigned i = 0; i < DMR_VOCODER_PCM_SAMPLES; i++) {
        audio[i] = tx.count ? tx.pcm[tx.read] : 0;
        peak = fmaxf(peak, fabsf(audio[i]));

        if (tx.count) {
            tx.read = (tx.read + 1) % (PCM_BURST * 2);
            tx.count--;
        }
    }

    /* mic_modulate supplies the same EQ/AGC output as analogue modes.
     * Preserve that level; only limit resampler overshoot to PCM16 range.
     * There is no additional DMR microphone AGC or fixed attenuation. */

    const float full_scale = 32767.0f / 32768.0f;
    float       gain = full_scale / fmaxf(full_scale, peak);

    for (unsigned i = 0; i < DMR_VOCODER_PCM_SAMPLES; i++)
        pcm[i] = (int16_t)(audio[i] * gain * 32768.0f);
}

static void generate_period(void) {
    dmr_burst_t burst = {0};
    bool        stopping = atomic_load(&tx_stop);

    if ((stopping && tx.ending) || tx.headers) {
        unsigned type = tx.headers ? DMR_DATA_TYPE_VOICE_LC_HEADER : DMR_DATA_TYPE_TERMINATOR_LC;

        dmr_full_lc_encode(&burst, tx.lc, op_modes->dmr.config.color_code, type, DMR_SYNC_MS_DATA);
        tx.terminated = type == DMR_DATA_TYPE_TERMINATOR_LC;

        if (tx.headers)
            tx.headers--;
    } else {
        uint8_t frames[DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BYTES];

        for (unsigned f = 0; f < DMR_VOICE_FRAME_COUNT; f++) {
            int16_t pcm[DMR_VOCODER_PCM_SAMPLES];

            prepare_tx_frame(pcm);
            vocoder_state_encode_fec(tx.codec, frames + f * DMR_VOICE_FRAME_BYTES, pcm);
        }

        dmr_voice_burst_insert(&burst, frames);

        if (!tx.sequence) {
            dmr_phy_tx_insert_sync(burst.data, DMR_SYNC_MS_VOICE);
        } else {
            /* B..E carry LC; F has null embedded signalling. */
            dmr_emb_t emb = { .color_code = op_modes->dmr.config.color_code };

            if (tx.sequence <= 4)
                emb = tx.fragments[tx.sequence - 1];

            dmr_emb_encode(&burst, &emb);
        }

        tx.sequence = (tx.sequence + 1) % 6;
        tx.ending = stopping && tx.count == 0;
    }

    float complex iq[IQ_PERIOD] = {0};

    dmr_phy_tx_generate_slot(tx.phy, burst.data, iq);

    for (unsigned i = 0; i < IQ_PERIOD; i += 3)
        rresamp_crcf_execute(tx.iq_resamp, iq + i, tx.output + i * 2 / 3);

    tx.pos = 0;
}

size_t dmr_transmit(float complex *iq, size_t capacity) {
    if (!tx.active || !op_modes->dmr.config.source_id || !capacity)
        return 0;

    mic_on_air(!atomic_load(&tx_stop));

    if (tx.pos == DAC_PERIOD && tx.terminated) {
        /* PTT publishes synchronous LVGL messages. Serialize with UI/PTT
         * changes, including a new key-down arriving during the tail. */
        brass_lv_lock();
        dmr_tx_set_active(false);

        if (atomic_load(&tx_stop)) {
            mic_on_air(false);
            radio_set_ptt(false);
        } else {
            dmr_tx_set_active(true);
        }

        brass_lv_unlock();
        return 0;
    }

    /* Drain available capture chunks into the speech FIFO. Capture produces
     * 256 samples at a time and does not share the DAC clock. A temporarily
     * empty queue is not a missing 10-ms audio block: only conceal a genuine
     * shortage when a complete voice burst is due in generate_period().
     * mic_modulate drains the common mic EQ/AGC queue; dsp_modulate(DMR)
     * forwards those processed samples to dmr_tx_audio(). */

    float complex scratch[MIC_BLOCK];

    if (!atomic_load(&tx_stop)) {
        for (unsigned block = 0; block < 6; block++) {
            size_t room = ((PCM_BURST * 2 - tx.count) / 5) * 8;
            size_t want = room < MIC_BLOCK ? room : MIC_BLOCK;
            if (!want || mic_modulate(scratch, want, RADIO_MODE_DMR) < want)
                break;
        }
    }

    if (tx.pos == DAC_PERIOD)
        generate_period();

    size_t n = capacity < MIC_BLOCK ? capacity : MIC_BLOCK;

    if (n > DAC_PERIOD - tx.pos)
        n = DAC_PERIOD - tx.pos;

    memcpy(iq, tx.output + tx.pos, n * sizeof(*iq));
    tx.pos += n;

    return n;
}

void dmr_config(dmr_config_e x) {
    switch (x) {
        case DMR_CONFIG_SRC_ID:
            dialog_num_open(1, (1 << 24) - 1, &op_modes->dmr.config.source_id, "Source ID");
            break;

        case DMR_CONFIG_TARGET_ID:
            dialog_num_open(1, (1 << 24) - 1, &op_modes->dmr.config.target_id, "Target ID");
            break;

        case DMR_CONFIG_PRIVATE_CALL:
            op_modes->dmr.config.private_call = !op_modes->dmr.config.private_call;
            msg_set_text_fmt("Call is %s", op_modes->dmr.config.private_call ? "private" : "group");
            break;

        case DMR_CONFIG_COLOR_CODE:
            dialog_num_open(0, 15, &op_modes->dmr.config.color_code, "Color Code");
            break;
    }
}
