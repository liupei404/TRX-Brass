/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include "lvgl/lvgl.h"
#include "dialog.h"
#include "dialog_ft8.h"
#include "styles.h"
#include "dsp.h"
#include "audio.h"
#include "keyboard.h"
#include "events.h"
#include "radio.h"
#include "buttons.h"
#include "main_screen.h"
#include "qth.h"
#include "msg.h"
#include "util.h"
#include "recorder.h"
#include "main.h"
#include "memory.h"
#include "queue.h"
#include "settings/modes.h"
#include "settings/memory.h"
#include "settings/ft8.h"
#include "settings/options.h"

#include "widgets/lv_waterfall.h"
#include "widgets/lv_finder.h"

#include "ft8/unpack.h"
#include "ft8/pack.h"
#include "ft8/ldpc.h"
#include "ft8/decode.h"
#include "ft8/constants.h"
#include "ft8/encode.h"
#include "ft8/crc.h"
#include "gfsk.h"
#include "fpga/adc.h"

#define MIN_SCORE       10
#define MAX_CANDIDATES  120
#define LDPC_ITER       20
#define MAX_DECODED     50
#define FREQ_OSR        2
#define TIME_OSR        4

#define WIDTH           775

enum {
    LV_PART_RX_INFO     = LV_PART_CUSTOM_FIRST,
    LV_PART_RX_MSG      = LV_PART_CUSTOM_FIRST + 0x10000,
    LV_PART_RX_CQ       = LV_PART_CUSTOM_FIRST + 0x20000,
    LV_PART_RX_TO_ME    = LV_PART_CUSTOM_FIRST + 0x30000,
    LV_PART_TX_MSG      = LV_PART_CUSTOM_FIRST + 0x40000
};

typedef enum {
    NOT_READY = 0,
    IDLE,
    RX_PROCESS,
    TX_PROCESS,
    TX_STOP,
} ft8_state_t;

typedef enum {
    QSO_IDLE = 0,
    QSO_NEXT,
    QSO_ODD,
    QSO_EVEN,
} ft8_qso_t;

typedef enum {
    MSG_RX_INFO = 0,
    MSG_RX_MSG,
    MSG_RX_CQ,
    MSG_RX_TO_ME,
    MSG_TX_MSG
} ft8_msg_type_t;

typedef enum {
    MSG_TX_INVALID = 0,
    MSG_TX_CQ,
    MSG_TX_CALLING,
    MSG_TX_R_REPORT,

    MSG_TX_REPORT,
    MSG_TX_RR73,

    MSG_TX_DONE
} ft8_tx_msg_t;

typedef struct {
    ft8_msg_type_t  type;
    int16_t         snr;
    int16_t         dist;
    bool            odd;
} ft8_cell_t;

typedef struct {
    char            *msg;
    ft8_cell_t      *cell;
} ft8_msg_t;

typedef struct {
    char        remote_callsign[32];
    char        remote_qth[32];
    int16_t     remote_snr;

    char        local_callsign[32];
    char        local_qth[32];
    int16_t     local_snr;
} ft8_qso_item_t;

static memory_t             *memory_ft8 = NULL;
static memory_t             *memory_ft4 = NULL;
static const char           *memory_label;

static ft8_state_t          state = NOT_READY;
static ft8_qso_t            qso = QSO_IDLE;
static bool                 odd;

static char                 tx_msg[64] = "";
static ft8_qso_item_t       qso_item;

static lv_obj_t             *table;
static int16_t              table_rows;

static lv_timer_t           *timer = NULL;
static lv_anim_t            fade;
static bool                 fade_run = false;

static lv_obj_t             *finder;
static lv_obj_t             *waterfall;
static uint16_t             waterfall_nfft;
static spgramcf             waterfall_sg;
static float                *waterfall_psd;
static uint8_t              waterfall_fps_ms = (1000 / 5);
static uint64_t             waterfall_time;

static pthread_cond_t       audio_cond;
static pthread_mutex_t      audio_mutex;
static cbuffercf            audio_buf;
static float complex       *audio_frame;
static pthread_t            thread;

static complex float        *rx_window = NULL;
static complex float        *time_buf;
static complex float        *freq_buf;
static windowcf             frame_window;
static fftplan              fft;

static float                symbol_period;
static uint32_t             block_size;
static uint32_t             subblock_size;
static uint16_t             nfft;
static float                fft_norm;
static waterfall_t          wf;

static candidate_t          candidate_list[MAX_CANDIDATES];
static message_t            decoded[MAX_DECODED];
static message_t*           decoded_hashtable[MAX_DECODED];

static struct tm            timestamp;

static void construct_cb(lv_obj_t *parent);
static void destruct_cb();
static void audio_cb(float complex *samples, size_t n);
static void rotary_cb(int32_t diff);
static void band_cb(bool up);
static void * decode_thread(void *arg);

static void show_all_cb(lv_event_t * e);
static void show_cq_cb(lv_event_t * e);

static void mode_ft8_cb(lv_event_t * e);
static void mode_ft4_cb(lv_event_t * e);

static void tx_cq_dis_cb(lv_event_t * e);
static void tx_cq_en_cb(lv_event_t * e);
static void tx_cq_hold_cb(void * data);

static void tx_call_dis_cb(lv_event_t * e);
static void tx_call_en_cb(lv_event_t * e);

static void mode_auto_cb(lv_event_t * e);

static void make_tx_msg(ft8_tx_msg_t msg, int16_t snr);
static bool do_rx_msg(ft8_cell_t *cell, const char * msg, bool pressed);

static button_item_t button_show_all = { .label = "Show\nAll", .press = show_all_cb };
static button_item_t button_show_cq = { .label = "Show\nCQ", .press = show_cq_cb };

static button_item_t button_mode_ft8 = { .label = "Mode\nFT8", .press = mode_ft8_cb };
static button_item_t button_mode_ft4 = { .label = "Mode\nFT4", .press = mode_ft4_cb };

static button_item_t button_tx_cq_dis = { .label = "TX CQ\nDisabled", .press = tx_cq_dis_cb };
static button_item_t button_tx_cq_en = { .label = "TX CQ\nEnabled", .press = tx_cq_en_cb };

static button_item_t button_tx_call_dis = { .label = "TX Call\nDisabled", .press = tx_call_dis_cb, .hold = tx_cq_hold_cb };
static button_item_t button_tx_call_en = { .label = "TX Call\nEnabled", .press = tx_call_en_cb, .hold = tx_cq_hold_cb };

static button_item_t button_auto_dis = { .label = "Auto\nDisabled", .press = mode_auto_cb };
static button_item_t button_auto_en = { .label = "Auto\nEnabled", .press = mode_auto_cb };

static dialog_t             dialog = {
    .run = false,
    .construct_cb = construct_cb,
    .destruct_cb = destruct_cb,
    .audio_cb = audio_cb,
    .bands_change_cb = band_cb,
    .rotary_cb = rotary_cb,
    .buttons = true,
    .key_cb = dialog_key_cb
};

dialog_t                    *dialog_ft8 = &dialog;

static void reset() {
    wf.num_blocks = 0;
    state = IDLE;
}

static void init() {
    /* FT8 decoder */

    float   slot_time;

    switch (settings_ft8->protocol) {
        case PROTO_FT4:
            slot_time = FT4_SLOT_TIME;
            symbol_period = FT4_SYMBOL_PERIOD;
            break;

        case PROTO_FT8:
            slot_time = FT8_SLOT_TIME;
            symbol_period = FT8_SYMBOL_PERIOD;
            break;
    }

    block_size = ADC_RATE * symbol_period;
    subblock_size = block_size / TIME_OSR;
    nfft = block_size * FREQ_OSR;
    fft_norm = 2.0f / nfft;
    audio_frame = malloc(block_size * sizeof(*audio_frame));

    const uint32_t max_blocks = slot_time / symbol_period;
    const uint32_t num_bins = ADC_RATE * symbol_period / 2;

    size_t mag_size = max_blocks * TIME_OSR * FREQ_OSR * num_bins * sizeof(uint8_t);

    wf.max_blocks = max_blocks;
    wf.num_bins = num_bins;
    wf.time_osr = TIME_OSR;
    wf.freq_osr = FREQ_OSR;
    wf.block_stride = TIME_OSR * FREQ_OSR * num_bins;
    wf.mag = (uint8_t *) malloc(mag_size);
    wf.protocol = settings_ft8->protocol;

    /* FT8 DSP */

    time_buf = (float complex*) malloc(nfft * sizeof(float complex));
    freq_buf = (float complex*) malloc(nfft * sizeof(float complex));
    fft = fft_create_plan(nfft, time_buf, freq_buf, LIQUID_FFT_FORWARD, 0);

    frame_window = windowcf_create(nfft);

    rx_window = malloc(nfft * sizeof(complex float));

    for (uint16_t i = 0; i < nfft; i++)
        rx_window[i] = liquid_hann(i, nfft);

    float gain = 0.0f;

    for (uint16_t i = 0; i < nfft; i++)
        gain += rx_window[i] * rx_window[i];

    gain = 1.0f / sqrtf(gain);

    for (uint16_t i = 0; i < nfft; i++)
        rx_window[i] *= gain;

    qso = QSO_IDLE;

    reset();

    /* Waterfall */

    waterfall_nfft = block_size;

    waterfall_sg = spgramcf_create(waterfall_nfft, LIQUID_WINDOW_HANN, waterfall_nfft, waterfall_nfft / 4);
    waterfall_psd = (float *) malloc(waterfall_nfft * sizeof(float));
    waterfall_time = get_time();

    /* Worker */

    pthread_mutex_init(&audio_mutex, NULL);
    pthread_cond_init(&audio_cond, NULL);
    pthread_create(&thread, NULL, decode_thread, NULL);
}

static void done() {
    pthread_cancel(thread);
    pthread_join(thread, NULL);

    state = NOT_READY;

    free(wf.mag);
    windowcf_destroy(frame_window);

    free(time_buf);
    free(freq_buf);
    fft_destroy_plan(fft);

    spgramcf_destroy(waterfall_sg);
    free(waterfall_psd);

    free(rx_window);
    free(audio_frame);
    audio_frame = NULL;
}

static void send_info(const char * fmt, ...) {
    va_list     args;
    char        buf[128];
    ft8_msg_t   *msg = malloc(sizeof(ft8_msg_t));

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    msg->msg = strdup(buf);

    msg->cell = lv_malloc(sizeof(ft8_cell_t));
    msg->cell->type = MSG_RX_INFO;

    queue_send(table, EVENT_FT8_MSG, msg);
}

static const char * find_qth(const char *str) {
    char *ptr = rindex(str, ' ');

    if (ptr) {
        ptr++;

        if (strcmp(ptr, "RR73") != 0 && grid_check(ptr)) {
            return ptr;
        }
    }

    return NULL;
}

static bool to_me(const char * text) {
    int16_t         callsign_len = strlen(options->op.callsign);

    return (callsign_len > 0) && (strncasecmp(text, options->op.callsign, callsign_len) == 0);
}

static void send_rx_text(int16_t snr, const char * text) {
    ft8_msg_type_t  type;

    if (to_me(text)) {
        type = MSG_RX_TO_ME;
    } else if (strncmp(text, "CQ ", 3) == 0) {
        type = MSG_RX_CQ;
    } else {
        if (settings_ft8->show == FT8_SHOW_ALL) {
            type = MSG_RX_MSG;
        } else {
            return;
        }
    }

    ft8_msg_t *msg = malloc(sizeof(ft8_msg_t));

    msg->msg = strdup(text);

    msg->cell = lv_malloc(sizeof(ft8_cell_t));
    msg->cell->snr = snr;
    msg->cell->type = type;
    msg->cell->odd = odd;

    if (options->op.qth[0] != 0) {
        const char *qth = find_qth(text);

        msg->cell->dist = qth ? grid_dist(qth) : 0;
    } else {
        msg->cell->dist = 0;
    }

    queue_send(table, EVENT_FT8_MSG, msg);
}

static void send_tx_text(const char * text) {
    ft8_msg_t   *msg = malloc(sizeof(ft8_msg_t));

    msg->msg = strdup(text);

    msg->cell = lv_malloc(sizeof(ft8_cell_t));
    msg->cell->type = MSG_TX_MSG;

    queue_send(table, EVENT_FT8_MSG, msg);
}

static void free_pending_msg(void *param) {
    ft8_msg_t *msg = param;

    free(msg->msg);
    lv_free(msg->cell);
    free(msg);
}

static void free_table_cells() {
    if (table == NULL) return;

    uint32_t rows = lv_table_get_row_count(table);
    for (uint32_t row = 0; row < rows; row++) {
        ft8_cell_t *cell = lv_table_get_cell_user_data(table, row, 0);
        if (cell != NULL) {
            lv_free(cell);
            lv_table_set_cell_user_data(table, row, 0, NULL);
        }
    }
}

static void decode() {
    uint16_t    num_candidates = ft8_find_sync(&wf, MAX_CANDIDATES, candidate_list, MIN_SCORE);

    memset(decoded_hashtable, 0, sizeof(decoded_hashtable));
    memset(decoded, 0, sizeof(decoded));

    for (uint16_t idx = 0; idx < num_candidates; idx++) {
        const candidate_t *cand = &candidate_list[idx];

        if (cand->score < MIN_SCORE)
            continue;

        float freq_hz = (cand->freq_offset + (float) cand->freq_sub / wf.freq_osr) / symbol_period;
        float time_sec = (cand->time_offset + (float) cand->time_sub / wf.time_osr) * symbol_period;

        message_t       message;
        decode_status_t status;

        if (!ft8_decode(&wf, cand, &message, LDPC_ITER, &status)) {
            continue;
        }

        uint16_t    idx_hash = message.hash % MAX_DECODED;
        bool        found_empty_slot = false;
        bool        found_duplicate = false;

        do {
            if (decoded_hashtable[idx_hash] == NULL) {
                found_empty_slot = true;
            } else if (decoded_hashtable[idx_hash]->hash == message.hash && strcmp(decoded_hashtable[idx_hash]->text, message.text) == 0) {
                found_duplicate = true;
            } else {
                idx_hash = (idx_hash + 1) % MAX_DECODED;
            }
        } while (!found_empty_slot && !found_duplicate);

        if (found_empty_slot) {
            memcpy(&decoded[idx_hash], &message, sizeof(message));
            decoded_hashtable[idx_hash] = &decoded[idx_hash];

            send_rx_text(cand->snr, message.text);
        }
    }
}

void static waterfall_process(float complex *frame, const size_t size) {
    uint64_t now = get_time();

    spgramcf_write(waterfall_sg, frame, size);

    if (now - waterfall_time > waterfall_fps_ms) {
        uint32_t low_bin = waterfall_nfft / 2 + waterfall_nfft * op_mode->filter.low / ADC_RATE;
        uint32_t high_bin = waterfall_nfft / 2 + waterfall_nfft * op_mode->filter.high / ADC_RATE;

        spgramcf_get_psd(waterfall_sg, waterfall_psd);

        brass_lv_lock();
        lv_waterfall_add_data(waterfall, &waterfall_psd[low_bin], high_bin - low_bin);
        brass_lv_unlock();

        waterfall_time = now;
        spgramcf_reset(waterfall_sg);
    }
}

void static process(float complex *frame) {
    complex float   *frame_ptr;
    int             offset = wf.num_blocks * wf.block_stride;
    int             frame_pos = 0;

    for (int time_sub = 0; time_sub < wf.time_osr; time_sub++) {
        windowcf_write(frame_window, &frame[frame_pos], subblock_size);
        frame_pos += subblock_size;

        windowcf_read(frame_window, &frame_ptr);

        for (uint32_t pos = 0; pos < nfft; pos++)
            time_buf[pos] = rx_window[pos] * frame_ptr[pos];

        fft_execute(fft);

        for (int freq_sub = 0; freq_sub < wf.freq_osr; freq_sub++)
            for (int bin = 0; bin < wf.num_bins; bin++) {
                int             src_bin = (bin * wf.freq_osr) + freq_sub;
                complex float   freq = freq_buf[src_bin];
                float           v = crealf(freq * conjf(freq));
                float           db = 10.0f * log10f(v);
                int             scaled = (int16_t) (db * 2.0f + 240.0f);

                if (scaled < 0) {
                    scaled = 0;
                } else if (scaled > 255) {
                    scaled = 255;
                }

                wf.mag[offset] = scaled;
                offset++;
            }
    }

    wf.num_blocks++;
}

static bool do_start(bool *odd) {
    struct tm       *tm;
    time_t          now;
    bool            start = false;

    now = time(NULL);
    tm = localtime(&now);

    switch (settings_ft8->protocol) {
        case PROTO_FT4:
            switch (tm->tm_sec) {
                case 0 ... 1:
                case 15 ... 16:
                case 30 ... 31:
                case 45 ... 46:
                    *odd = true;
                    start = true;
                    break;

                case 7 ... 8:
                case 22 ... 23:
                case 37 ... 38:
                case 52 ... 53:
                    *odd = false;
                    start = true;
                    break;

                default:
                    start = false;
            }
            break;

        case PROTO_FT8:
            switch (tm->tm_sec) {
                case 0 ... 1:
                case 30 ... 31:
                    *odd = true;
                    start = true;
                    break;

                case 15 ... 16:
                case 45 ... 46:
                    *odd = false;
                    start = true;
                    break;

                default:
                    start = false;
            }
            break;
    }

    if (start) {
        timestamp = *tm;
    }

    return start;
}

static void rx_worker(bool sync) {
    unsigned int    n;
    float complex   *buf;

    pthread_mutex_lock(&audio_mutex);

    while (cbuffercf_size(audio_buf) < block_size) {
        pthread_cond_wait(&audio_cond, &audio_mutex);
    }

    while (cbuffercf_size(audio_buf) >= block_size) {
        cbuffercf_read(audio_buf, block_size, &buf, &n);
        memcpy(audio_frame, buf, block_size * sizeof(*audio_frame));
        cbuffercf_release(audio_buf, block_size);
        pthread_mutex_unlock(&audio_mutex);

        waterfall_process(audio_frame, block_size);

        if (sync) {
            process(audio_frame);

            if (wf.num_blocks >= wf.max_blocks) {
                decode();
                reset();
            }
        }

        pthread_mutex_lock(&audio_mutex);
    }

    pthread_mutex_unlock(&audio_mutex);
}

static void tx_worker() {
    uint8_t tones[FT8_NN];
    uint8_t packed[FTX_LDPC_K_BYTES];
    int     rc = pack77(tx_msg, packed);

    if (rc < 0) {
        LV_LOG_ERROR("Cannot parse message %i", rc);
        state = IDLE;
        return;
    }

    ft8_encode(packed, tones);

    int32_t     n_samples = 0;
    float       symbol_bt = (settings_ft8->protocol == PROTO_FT4) ? FT4_SYMBOL_BT : FT8_SYMBOL_BT;
    int16_t     *samples = gfsk_synth(tones, FT8_NN, settings_ft8->tx_freq, symbol_bt, symbol_period, &n_samples);
    int16_t     *ptr = samples;
    size_t      part = 1024 * 2;

    radio_set_ptt(true);

    while (true) {
        if (n_samples <= 0 || state != TX_PROCESS) {
            state = IDLE;
            break;
        }

        audio_play(ptr, part);

        n_samples -= part;
        ptr += part;
    }

    audio_play_wait();
    radio_set_ptt(false);
    free(samples);
}

static void * decode_thread(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (true) {
        switch (state) {
            case IDLE:
                if (do_start(&odd)) {
                    switch (qso) {
                        case QSO_IDLE:
                            state = RX_PROCESS;
                            break;

                        case QSO_NEXT:
                            state = TX_PROCESS;
                            qso = odd ? QSO_ODD : QSO_EVEN;
                            break;

                        case QSO_ODD:
                            state = odd ? TX_PROCESS : RX_PROCESS;
                            break;

                        case QSO_EVEN:
                            state = !odd ? TX_PROCESS : RX_PROCESS;
                            break;
                    }

                    switch (state) {
                        case RX_PROCESS:
                            if (qso == QSO_IDLE) {
                                send_info("RX %s %02i:%02i:%02i", memory_label, timestamp.tm_hour, timestamp.tm_min, timestamp.tm_sec);
                            }
                            rx_worker(true);
                            break;

                        case TX_PROCESS:
                            send_tx_text(tx_msg);
                            break;
                    }
                } else {
                    rx_worker(false);
                }
                break;
 
            case RX_PROCESS:
                rx_worker(true);
                break;

            case TX_PROCESS:
            case TX_STOP:
                tx_worker();
                break;

            default:
                break;
        }
    }

    return NULL;
}

static void add_msg_cb(lv_event_t * e) {
    ft8_msg_t   *msg = (ft8_msg_t *) lv_event_get_param(e);
    uint32_t    row = 0;
    uint32_t    col = 0;
    bool        scroll;

    lv_table_get_selected_cell(table, &row, &col);
    scroll = table_rows == (row + 1);

#ifdef MAX_TABLE_MSG
    if (table_rows > MAX_TABLE_MSG) {
        for (uint16_t i = 1; i < table_rows; i++)
            lv_table_set_cell_value(table, i-1, 0, lv_table_get_cell_value(table, i, 0));

        table_rows--;
    }
#endif

    lv_table_set_cell_value(table, table_rows, 0, msg->msg);

    if (msg->cell) {
        lv_table_set_cell_user_data(table, table_rows, 0, msg->cell);

        if (settings_ft8->tx_auto && (msg->cell->type == MSG_RX_TO_ME)) {
            do_rx_msg(msg->cell, msg->msg, false);
        }
    }

    if (scroll) {
        int32_t *c = malloc(sizeof(int32_t));
        *c = LV_KEY_DOWN;

        lv_obj_send_event(table, LV_EVENT_KEY, c);
    }

    table_rows++;
    free(msg->msg);
    msg->msg = NULL;
}

static void fill_style(lv_obj_t *obj, lv_draw_label_dsc_t *dsc, lv_style_selector_t part) {
    dsc->font = lv_obj_get_style_text_font(obj, part);
    dsc->color = lv_obj_get_style_text_color(obj, part);
    dsc->align = lv_obj_get_style_text_align(obj, part);
}

static void table_draw_task_cb(lv_event_t * e) {
    lv_obj_t *obj = lv_event_get_target(e);
    lv_draw_task_t *task = lv_event_get_draw_task(e);
    lv_draw_label_dsc_t *dsc = lv_draw_task_get_label_dsc(task);
    if (dsc == NULL || dsc->base.id1 == UINT32_MAX) return;

    uint32_t row = dsc->base.id1;
    uint32_t col = dsc->base.id2;
    if (row >= lv_table_get_row_cnt(obj) || col >= lv_table_get_col_cnt(obj)) return;

    ft8_cell_t *cell = lv_table_get_cell_user_data(obj, row, col);
    if (cell == NULL) {
        fill_style(obj, dsc, LV_PART_RX_INFO);
        return;
    }

    switch (cell->type) {
        case MSG_RX_INFO:  fill_style(obj, dsc, LV_PART_RX_INFO); break;
        case MSG_RX_MSG:   fill_style(obj, dsc, LV_PART_RX_MSG); break;
        case MSG_RX_CQ:    fill_style(obj, dsc, LV_PART_RX_CQ); break;
        case MSG_RX_TO_ME: fill_style(obj, dsc, LV_PART_RX_TO_ME); break;
        case MSG_TX_MSG:   fill_style(obj, dsc, LV_PART_TX_MSG); break;
    }

    if (cell->type == MSG_RX_MSG || cell->type == MSG_RX_CQ || cell->type == MSG_RX_TO_ME) {
        char buf[64];
        lv_area_t area;
        lv_draw_task_get_area(task, &area);

        lv_draw_label_dsc_t extra = *dsc;
        extra.base.id1 = UINT32_MAX;
        extra.base.id2 = UINT32_MAX;
        extra.align = LV_TEXT_ALIGN_RIGHT;
        extra.text_local = 1;
        extra.text_static = 0;
        extra.hint = NULL;

        area.x2 -= 15;
        area.x1 = area.x2 - 120;
        snprintf(buf, sizeof(buf), "%i dB", cell->snr);
        extra.text = buf;
        extra.text_length = strlen(buf);
        lv_draw_label(extra.base.layer, &extra, &area);

        if (cell->dist > 0) {
            area.x2 = area.x1 - 10;
            area.x1 = area.x2 - 200;
            snprintf(buf, sizeof(buf), "%i km", cell->dist);
            extra.text = buf;
            extra.text_length = strlen(buf);
            lv_draw_label(extra.base.layer, &extra, &area);
        }
    }
}

static void selected_msg_cb(lv_event_t * e) {
    uint32_t    row;
    uint32_t    col;

    lv_table_get_selected_cell(table, &row, &col);
}

static void destruct_cb() {
    done();

    if (timer != NULL) {
        lv_timer_delete(timer);
        timer = NULL;
    }
    lv_anim_delete(table, NULL);
    fade_run = false;
    queue_cancel(table, EVENT_FT8_MSG, free_pending_msg);

    free(audio_buf);
    audio_buf = NULL;

    free_table_cells();
    lv_obj_delete(table);
    table = NULL;

    lv_obj_delete(waterfall);
    waterfall = NULL;
    finder = NULL;
    op_work_restore();

    main_screen_lock_mode(false);
    main_screen_lock_freq(false);
    main_screen_lock_band(false);

    settings_ft8_save();
}

static void load_band() {
    switch (settings_ft8->protocol) {
        case PROTO_FT8:
            memory_label = memory_load(memory_ft8, settings_ft8->ft8_band);
            break;

        case PROTO_FT4:
            memory_label = memory_load(memory_ft4, settings_ft8->ft4_band);
            break;
    }
}

static void clean() {
    reset();

    free_table_cells();
    lv_table_set_row_cnt(table, 1);
    lv_table_set_cell_value(table, 0, 0, "Wait sync");

    lv_waterfall_clear_data(waterfall);

    table_rows = 0;

    int32_t *c = malloc(sizeof(int32_t));
    *c = LV_KEY_UP;

    lv_obj_send_event(table, LV_EVENT_KEY, c);
}

static void make_tx_msg(ft8_tx_msg_t msg, int16_t snr) {
    char qth[5] = "";

    if (strlen(options->op.qth) >= 4) {
        strncpy(qth, options->op.qth, sizeof(qth) - 1);
    }

    switch (msg) {
        case MSG_TX_CQ:
            snprintf(tx_msg, sizeof(tx_msg) - 1, "CQ %s %s", options->op.callsign, qth);
            break;

        case MSG_TX_CALLING:
            qso_item.local_snr = snr;
            snprintf(tx_msg, sizeof(tx_msg) - 1, "%s %s %s", qso_item.remote_callsign, options->op.callsign, qth);
            break;

        case MSG_TX_REPORT:
            qso_item.local_snr = snr;
            snprintf(tx_msg, sizeof(tx_msg) - 1, "%s %s %+02i", qso_item.remote_callsign, options->op.callsign, qso_item.local_snr);
            break;

        case MSG_TX_R_REPORT:
            snprintf(tx_msg, sizeof(tx_msg) - 1, "%s %s R%+02i", qso_item.remote_callsign, options->op.callsign, qso_item.local_snr);
            break;

        case MSG_TX_RR73:
            snprintf(tx_msg, sizeof(tx_msg) - 1, "%s %s RR73", qso_item.remote_callsign, options->op.callsign);
            break;

        default:
            return;
    }

    msg_set_text_fmt("Next TX: %s", tx_msg);
}

static ft8_tx_msg_t parse_rx_msg(const char * str) {
    char            *s = strdup(str);
    char            *call_to = NULL;
    char            *call_de = NULL;
    char            *extra = NULL;

    /* Splite */

    call_to = strtok(s, " ");

    if (call_to) {
        call_de = strtok(NULL, " ");

        if (call_de) {
            extra = strtok(NULL, " ");
        }
    }

    /* Analysis */

    if (call_to && strcmp(call_to, "CQ") == 0) {
        strcpy(qso_item.remote_callsign, call_de ? call_de : "");
        strcpy(qso_item.remote_qth, extra ? extra : "");

        free(s);
        return MSG_TX_CALLING;
    }

    if (call_to && to_me(call_to)) {
        if (extra && strcmp(extra, "RR73") == 0 || strcmp(extra, "73") == 0) {
            buttons_load(2, &button_tx_cq_en);

            free(s);
            return MSG_TX_DONE;
        }

        if (grid_check(extra)) {
            strcpy(qso_item.remote_callsign, call_de);
            strcpy(qso_item.remote_qth, extra);

            free(s);
            return MSG_TX_REPORT;
        }

        if (extra[0] == 'R' && (extra[1] == '-' || extra[1] == '+')) {
            qso_item.remote_snr = atoi(extra + 1);
            strcpy(qso_item.remote_callsign, call_de);

            free(s);
            return MSG_TX_RR73;
        }

        if (extra[0] == '-' || extra[0] == '+') {
            qso_item.remote_snr = atoi(extra);
            strcpy(qso_item.remote_callsign, call_de);

            free(s);
            return MSG_TX_R_REPORT;
        }
    }

    free(s);
    return MSG_TX_INVALID;
}

static bool do_rx_msg(ft8_cell_t *cell, const char * msg, bool pressed) {
    ft8_tx_msg_t next_msg = parse_rx_msg(msg);

    switch (next_msg) {
        case MSG_TX_CALLING:
            if (pressed) {
                send_info("Start QSO");
            }
            break;

        case MSG_TX_INVALID:
            return false;

        case MSG_TX_DONE:
            qso = QSO_IDLE;
            buttons_load(2, &button_tx_call_dis);
            return true;
    }

    qso = cell->odd ? QSO_EVEN : QSO_ODD;   /* Must be reversed */
    make_tx_msg(next_msg, cell->snr);

    buttons_load(2, &button_tx_call_en);
    return true;
}

static void band_cb(bool up) {
    int band = 0;
    int max = 0;

    switch (settings_ft8->protocol) {
        case PROTO_FT8:
            band = settings_ft8->ft8_band;
            max = memory_ft8->count - 1;
            break;

        case PROTO_FT4:
            band = settings_ft8->ft4_band;
            max = memory_ft4->count - 1;
            break;
    }

    if (up) {
        band++;

        if (band > max) {
            band = 0;
        }
    } else {
        band--;

        if (band < 0) {
            band = max;
        }
    }

    switch (settings_ft8->protocol) {
        case PROTO_FT8:
            settings_ft8->ft8_band = band;
            break;

        case PROTO_FT4:
            settings_ft8->ft4_band = band;
            break;
    }

    load_band();
    clean();
}

static void msg_timer(lv_timer_t *t) {
    lv_anim_set_values(&fade, lv_obj_get_style_opa_layered(table, 0), LV_OPA_COVER);
    lv_anim_start(&fade);
    timer = NULL;
}

static void fade_anim(void * obj, int32_t v) {
    lv_obj_set_style_opa_layered(obj, v, 0);
}

static void fade_ready(lv_anim_t * a) {
    fade_run = false;
}

static void rotary_cb(int32_t diff) {
    uint32_t f = settings_ft8->tx_freq + diff;

    if (f > op_mode->filter.high) {
        f = op_mode->filter.high;
    }

    if (f < op_mode->filter.low) {
        f = op_mode->filter.low;
    }

    settings_ft8->tx_freq = f;

    lv_finder_set_value(finder, f);

    if (!fade_run) {
        fade_run = true;
        lv_anim_set_values(&fade, lv_obj_get_style_opa_layered(table, 0), LV_OPA_TRANSP);
        lv_anim_start(&fade);
    }

    if (timer) {
        lv_timer_reset(timer);
    } else {
        timer = lv_timer_create(msg_timer, 1000, NULL);
        lv_timer_set_repeat_count(timer, 1);
    }
}

static void construct_cb(lv_obj_t *parent) {
    /* Settings */

    if (memory_ft8 == NULL) {
        memory_ft8 = settings_memory_load("/mnt/settings/memory/ft8.yaml");
    }

    if (memory_ft4 == NULL) {
        memory_ft4 = settings_memory_load("/mnt/settings/memory/ft4.yaml");
    }

    settings_ft8_load();

    /* * */

    dialog_init(parent, &dialog);

    audio_buf = cbuffercf_create(ADC_RATE * 3);

    /* Waterfall */

    waterfall = lv_waterfall_create(dialog.obj);

    lv_obj_remove_style_all(waterfall);
    lv_obj_add_style(waterfall, ft8_waterfall_style, 0);
    lv_obj_clear_flag(waterfall, LV_OBJ_FLAG_SCROLLABLE);

    lv_waterfall_set_grad(waterfall, waterfall_grad);

    lv_obj_update_layout(waterfall);
    lv_waterfall_set_data_size(waterfall, lv_obj_get_width(waterfall));

    /* Freq finder */

    finder = lv_finder_create(waterfall);

    lv_obj_remove_style_all(finder);
    lv_obj_add_style(finder, ft8_finder_style, LV_PART_MAIN);
    lv_obj_add_style(finder, ft8_finder_indicator_style, LV_PART_INDICATOR);

    lv_finder_set_width(finder, 50);
    lv_finder_set_value(finder, settings_ft8->tx_freq);

    /* Table */

    table = lv_table_create(dialog.obj);

    lv_obj_remove_style_all(table);
    lv_obj_add_style(table, ft8_table_style, LV_PART_MAIN);
    lv_obj_add_style(table, ft8_focused_style, LV_PART_ITEMS | LV_STATE_FOCUSED);

    lv_obj_add_style(table, ft8_rx_info_style, LV_PART_RX_INFO);
    lv_obj_add_style(table, ft8_rx_msg_style, LV_PART_RX_MSG);
    lv_obj_add_style(table, ft8_rx_cq_style, LV_PART_RX_CQ);
    lv_obj_add_style(table, ft8_rx_to_me_style, LV_PART_RX_TO_ME);
    lv_obj_add_style(table, ft8_tx_msg_style, LV_PART_TX_MSG);

    lv_obj_update_layout(table);

    lv_table_set_col_cnt(table, 1);
    lv_table_set_col_width(table, 0, lv_obj_get_width(table));

    lv_obj_add_event_cb(table, add_msg_cb, EVENT_FT8_MSG, NULL);
    lv_obj_add_event_cb(table, selected_msg_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(table, tx_call_dis_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(table, dialog_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_flag(table, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(table, table_draw_task_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);

    lv_group_add_obj(keyboard_group, table);
    lv_group_set_editing(keyboard_group, true);

    lv_table_set_cell_value(table, 0, 0, "Wait sync");

    /* Fade */

    lv_anim_init(&fade);
    lv_anim_set_var(&fade, table);
    lv_anim_set_time(&fade, 250);
    lv_anim_set_exec_cb(&fade, fade_anim);
    lv_anim_set_ready_cb(&fade, fade_ready);

    /* * */

    table_rows = 0;

    switch (settings_ft8->show) {
        case FT8_SHOW_ALL:
            buttons_load(0, &button_show_all);
            break;

        case FT8_SHOW_CQ:
            buttons_load(0, &button_show_cq);
            break;
    }

    switch (settings_ft8->protocol) {
        case PROTO_FT8:
            buttons_load(1, &button_mode_ft8);
            break;

        case PROTO_FT4:
            buttons_load(1, &button_mode_ft4);
            break;
    }

    buttons_load(2, &button_tx_cq_dis);
    buttons_load(3, settings_ft8->tx_auto ? &button_auto_en : &button_auto_dis);

    op_work_keep();
    load_band();

    lv_finder_set_range(finder, op_mode->filter.low, op_mode->filter.high);

    main_screen_lock_mode(true);
    main_screen_lock_freq(true);
    main_screen_lock_band(true);

    init();
}

static void show_all_cb(lv_event_t * e) {
    settings_ft8->show = FT8_SHOW_CQ;
    buttons_load(0, &button_show_cq);
}

static void show_cq_cb(lv_event_t * e) {
    settings_ft8->show = FT8_SHOW_ALL;
    buttons_load(0, &button_show_all);
}

static void mode_ft8_cb(lv_event_t * e) {
    settings_ft8->protocol = PROTO_FT4;

    buttons_load(1, &button_mode_ft4);

    done();
    init();
    clean();
    load_band();
}

static void mode_ft4_cb(lv_event_t * e) {
    settings_ft8->protocol = PROTO_FT8;

    buttons_load(1, &button_mode_ft8);

    done();
    init();
    clean();
    load_band();
}

static void mode_auto_cb(lv_event_t * e) {
    settings_ft8->tx_auto = !settings_ft8->tx_auto;

    buttons_load(3, settings_ft8->tx_auto ? &button_auto_en : &button_auto_dis);
}

static void tx_cq_dis_cb(lv_event_t * e) {
    if (strlen(options->op.callsign) == 0) {
        msg_set_text_fmt("Call sign required");

        return;
    }

    buttons_load(2, &button_tx_cq_en);
    make_tx_msg(MSG_TX_CQ, 0);
    qso = QSO_NEXT;
}

static void tx_cq_en_cb(lv_event_t * e) {
    buttons_load(2, &button_tx_cq_dis);

    if (state == TX_PROCESS) {
        state = TX_STOP;
    }
    qso = QSO_IDLE;
}

static void tx_cq_hold_cb(void * data) {
    LV_UNUSED(data);
    tx_cq_en_cb(NULL);
}

static void tx_call_off() {
    buttons_load(2, &button_tx_call_dis);
    state = TX_STOP;
    qso = QSO_IDLE;
}

static void tx_call_dis_cb(lv_event_t * e) {
    if (strlen(options->op.callsign) == 0) {
        msg_set_text_fmt("Call sign required");

        return;
    }

    if (state == TX_PROCESS) {
        tx_call_off();
    } else {
        uint32_t    row;
        uint32_t    col;

        lv_table_get_selected_cell(table, &row, &col);

        ft8_cell_t  *cell = lv_table_get_cell_user_data(table, row, col);

        if (cell == NULL || cell->type == MSG_TX_MSG || cell->type == MSG_RX_INFO) {
            msg_set_text_fmt("What should I do about it?");
        } else {
            if (!do_rx_msg(cell, lv_table_get_cell_value(table, row, col), true)) {
                msg_set_text_fmt("Invalid message");
                tx_call_off();
            }
        }
    }
}

static void tx_call_en_cb(lv_event_t * e) {
    buttons_load(2, &button_tx_call_dis);

    if (state == TX_PROCESS) {
        state = TX_STOP;
    }
    qso = QSO_IDLE;
}

static void audio_cb(float complex *samples, size_t n) {
    static uint64_t last_overflow_log;

    if (state == NOT_READY) {
        return;
    }

    if (state == IDLE || state == RX_PROCESS) {
        bool overflow = false;

        pthread_mutex_lock(&audio_mutex);
        if (cbuffercf_space_available(audio_buf) >= n) {
            cbuffercf_write(audio_buf, samples, n);
            pthread_cond_signal(&audio_cond);
        } else {
            overflow = true;
        }
        pthread_mutex_unlock(&audio_mutex);

        if (overflow) {
            uint64_t now = get_time();
            if (now - last_overflow_log >= 1000) {
                LV_LOG_WARN("Audio buffer overflow");
                last_overflow_log = now;
            }
        }
    }
}
