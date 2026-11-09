#include <dmr/phy.h>

#include "phy/discriminator.h"
#include "phy/matched_filter.h"
#include "phy/sync_detector.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DMR_DEFAULT_SYNC_THRESHOLD 0.97f
#define DMR_SYNC_FIRST_DIBIT 54u
#define DMR_RX_SAMPLE_RING_CAPACITY 640u
#define DMR_RX_PENDING_CAPACITY 4u
#define DMR_TIMING_REFINEMENT_STEPS 9u
#define DMR_TIMING_DRIFT_STEPS 11u
#define DMR_TIMING_DRIFT_MAX_PPM 2500.0f
#define DMR_TIMING_DRIFT_MIN_RELATIVE_IMPROVEMENT 0.10f
#define DMR_TIMESLOT_SAMPLES 576u
#define DMR_TRACK_MAX_EYE_ERROR 0.20f
#define DMR_TRACK_MAX_FAILURES 3u

typedef struct {
    bool active;
    dmr_sync_event_t sync;
    uint64_t burst_first_sample;
    uint64_t burst_last_sample;
    float timing_offset_samples;
    float timing_drift_per_symbol;
    unsigned int timeslot;
} dmr_pending_burst_t;

typedef struct {
    bool active;
    uint64_t anchor_first_sample;
    unsigned int anchor_timeslot;
    double next_first_sample;
    double slot_samples;
    float anchor_timing_offset;
    dmr_sync_kind_t anchor_sync_kind;
    bool clock_acquired;
    unsigned int next_timeslot;
    unsigned int failures;
    dmr_sync_fit_t fit;
    float timing_offset_samples;
    float timing_drift_per_symbol;
} dmr_timeslot_tracker_t;

struct dmr_phy_rx {
    dmr_phy_rx_config_t config;
    dmr_discriminator_t discriminator;
    dmr_matched_filter_t matched_filter;
    dmr_sync_detector_t sync_detector;

    float sample_ring[DMR_RX_SAMPLE_RING_CAPACITY];
    uint64_t next_filtered_sample;
    dmr_pending_burst_t pending[DMR_RX_PENDING_CAPACITY];
    dmr_timeslot_tracker_t tracker;
};

static uint8_t slice_dibit(float soft_level)
{
    if (soft_level > 2.0f)
        return 1u; /* 01 -> +3 */
    if (soft_level > 0.0f)
        return 0u; /* 00 -> +1 */
    if (soft_level > -2.0f)
        return 2u; /* 10 -> -1 */
    return 3u;     /* 11 -> -3 */
}

static float ring_sample(const dmr_phy_rx_t *rx, uint64_t sample_index)
{
    return rx->sample_ring[sample_index % DMR_RX_SAMPLE_RING_CAPACITY];
}

static float ring_sample_fractional(const dmr_phy_rx_t *rx,
                                    uint64_t sample_index,
                                    float offset)
{
    if (offset == 0.0f)
        return ring_sample(rx, sample_index);
    if (offset >= 0.0f) {
        const float first = ring_sample(rx, sample_index);
        const float second = ring_sample(rx, sample_index + 1u);
        return first + offset * (second - first);
    }

    const float first = ring_sample(rx, sample_index - 1u);
    const float second = ring_sample(rx, sample_index);
    return first + (offset + 1.0f) * (second - first);
}

static float ring_sample_at_offset(const dmr_phy_rx_t *rx,
                                   uint64_t sample_index,
                                   float offset)
{
    const int whole = (int)floorf(offset);
    const float fraction = offset - (float)whole;
    const uint64_t first_index = whole < 0
        ? sample_index - (uint64_t)(-whole)
        : sample_index + (uint64_t)whole;
    const float first = ring_sample(rx, first_index);
    const float second = ring_sample(rx, first_index + 1u);
    return first + fraction * (second - first);
}

static void refine_sync_timing(dmr_phy_rx_t *rx,
                               dmr_sync_event_t *event,
                               float *best_offset)
{
    dmr_sync_fit_t best_fit = event->fit;
    *best_offset = 0.0f;

    for (size_t step = 0; step < DMR_TIMING_REFINEMENT_STEPS; ++step) {
        const float offset = -0.5f +
            (float)step / (float)(DMR_TIMING_REFINEMENT_STEPS - 1u);
        float observed[DMR_SYNC_SYMBOL_COUNT];
        for (size_t symbol = 0; symbol < DMR_SYNC_SYMBOL_COUNT; ++symbol) {
            const uint64_t index = event->first_symbol_sample +
                symbol * DMR_SYNC_SAMPLES_PER_SYMBOL;
            observed[symbol] = ring_sample_fractional(rx, index, offset);
        }

        dmr_sync_fit_t fit;
        if (dmr_sync_correlate(observed, event->kind, &fit) &&
            fit.score > best_fit.score) {
            best_fit = fit;
            *best_offset = offset;
        }
    }
    event->fit = best_fit;
}

static void accept_sync(void *context, const dmr_sync_event_t *event)
{
    dmr_phy_rx_t *rx = context;
    const uint64_t history =
        DMR_SYNC_FIRST_DIBIT * DMR_SYNC_SAMPLES_PER_SYMBOL;
    if (event->first_symbol_sample < history)
        return;

    for (size_t i = 0; i < DMR_RX_PENDING_CAPACITY; ++i) {
        if (rx->pending[i].active)
            continue;
        rx->pending[i].active = true;
        rx->pending[i].sync = *event;
        refine_sync_timing(rx, &rx->pending[i].sync,
                           &rx->pending[i].timing_offset_samples);
        rx->pending[i].burst_first_sample =
            event->first_symbol_sample - history;
        rx->pending[i].burst_last_sample =
            rx->pending[i].burst_first_sample +
            (DMR_BURST_DIBIT_COUNT - 1u) * DMR_SYNC_SAMPLES_PER_SYMBOL + 2u;
        if (rx->config.track_timeslots) {
            if (!rx->tracker.active) {
                rx->tracker.active = true;
                rx->tracker.anchor_first_sample =
                    rx->pending[i].burst_first_sample;
                rx->tracker.next_first_sample =
                    rx->pending[i].burst_first_sample + DMR_TIMESLOT_SAMPLES;
                rx->tracker.next_timeslot = 2u;
                rx->tracker.anchor_timeslot = 1u;
                rx->tracker.slot_samples = DMR_TIMESLOT_SAMPLES;
                rx->tracker.anchor_timing_offset = rx->pending[i].timing_offset_samples;
                rx->tracker.anchor_sync_kind = rx->pending[i].sync.kind;
                rx->tracker.clock_acquired = false;
            }
            const int64_t delta = (int64_t)rx->pending[i].burst_first_sample -
                                  (int64_t)rx->tracker.anchor_first_sample;
            /* Sync timing moves by a few samples with oscillator mismatch.
             * Floor division mislabeled an early sync as the opposite slot. */
            const int64_t slots = delta >= 0
                ? (delta + DMR_TIMESLOT_SAMPLES / 2) / DMR_TIMESLOT_SAMPLES
                : -((-delta + DMR_TIMESLOT_SAMPLES / 2) / DMR_TIMESLOT_SAMPLES);
            rx->pending[i].timeslot = (slots % 2 != 0)
                ? 3u - rx->tracker.anchor_timeslot : rx->tracker.anchor_timeslot;
        }
        return;
    }
}

static float nearest_level(float value)
{
    if (value > 2.0f)
        return 3.0f;
    if (value > 0.0f)
        return 1.0f;
    if (value > -2.0f)
        return -1.0f;
    return -3.0f;
}

static void refine_burst_drift(dmr_phy_rx_t *rx,
                               dmr_pending_burst_t *candidate)
{
    const float sync_center = DMR_SYNC_FIRST_DIBIT +
        0.5f * (DMR_SYNC_SYMBOL_COUNT - 1u);
    float best_error = INFINITY;
    float zero_error = INFINITY;
    float best_drift = 0.0f;
    dmr_sync_fit_t best_fit = candidate->sync.fit;
    dmr_sync_fit_t zero_fit = candidate->sync.fit;

    for (size_t step = 0; step < DMR_TIMING_DRIFT_STEPS; ++step) {
        const float ppm = -DMR_TIMING_DRIFT_MAX_PPM +
            2.0f * DMR_TIMING_DRIFT_MAX_PPM * (float)step /
                (float)(DMR_TIMING_DRIFT_STEPS - 1u);
        const float drift = DMR_SYNC_SAMPLES_PER_SYMBOL * ppm * 1e-6f;
        float observed[DMR_SYNC_SYMBOL_COUNT];
        for (size_t symbol = 0; symbol < DMR_SYNC_SYMBOL_COUNT; ++symbol) {
            const size_t burst_symbol = DMR_SYNC_FIRST_DIBIT + symbol;
            const float sample_offset = candidate->timing_offset_samples +
                ((float)burst_symbol - sync_center) * drift;
            observed[symbol] = ring_sample_at_offset(
                rx, candidate->burst_first_sample +
                    burst_symbol * DMR_SYNC_SAMPLES_PER_SYMBOL,
                sample_offset);
        }

        dmr_sync_fit_t fit;
        if (!dmr_sync_correlate(observed, candidate->sync.kind, &fit) ||
            fit.score < rx->config.sync_threshold)
            continue;

        float squared_error = 0.0f;
        size_t held_out_symbols = 0;
        for (size_t symbol = 0; symbol < DMR_BURST_DIBIT_COUNT; ++symbol) {
            if (symbol >= DMR_SYNC_FIRST_DIBIT &&
                symbol < DMR_SYNC_FIRST_DIBIT + DMR_SYNC_SYMBOL_COUNT)
                continue;
            const float sample_offset = candidate->timing_offset_samples +
                ((float)symbol - sync_center) * drift;
            const float hz = ring_sample_at_offset(
                rx, candidate->burst_first_sample +
                    symbol * DMR_SYNC_SAMPLES_PER_SYMBOL,
                sample_offset);
            const float level = (hz - fit.offset_hz) /
                                fit.gain_hz_per_level;
            const float error = level - nearest_level(level);
            squared_error += fminf(error * error, 4.0f);
            ++held_out_symbols;
        }
        const float mean_error = squared_error / (float)held_out_symbols;
        if (ppm == 0.0f) {
            zero_error = mean_error;
            zero_fit = fit;
        }
        if (mean_error < best_error) {
            best_error = mean_error;
            best_drift = drift;
            best_fit = fit;
        }
    }
    if (best_drift != 0.0f && isfinite(zero_error) &&
        best_error > zero_error *
            (1.0f - DMR_TIMING_DRIFT_MIN_RELATIVE_IMPROVEMENT)) {
        best_drift = 0.0f;
        best_fit = zero_fit;
    }
    candidate->timing_drift_per_symbol = best_drift;
    candidate->sync.fit = best_fit;
}

static void emit_ready_bursts(dmr_phy_rx_t *rx, uint64_t current_sample)
{
    for (size_t pending = 0; pending < DMR_RX_PENDING_CAPACITY; ++pending) {
        dmr_pending_burst_t *candidate = &rx->pending[pending];
        if (!candidate->active || current_sample < candidate->burst_last_sample)
            continue;

        dmr_burst_t burst;
        memset(&burst, 0, sizeof(burst));
        refine_burst_drift(rx, candidate);
        const float offset = candidate->sync.fit.offset_hz;
        const float gain = candidate->sync.fit.gain_hz_per_level;
        for (size_t symbol = 0; symbol < DMR_BURST_DIBIT_COUNT; ++symbol) {
            const uint64_t sample_index = candidate->burst_first_sample +
                symbol * DMR_SYNC_SAMPLES_PER_SYMBOL;
            const float sync_center = DMR_SYNC_FIRST_DIBIT +
                0.5f * (DMR_SYNC_SYMBOL_COUNT - 1u);
            const float sample_offset = candidate->timing_offset_samples +
                ((float)symbol - sync_center) *
                    candidate->timing_drift_per_symbol;
            const float hz = ring_sample_at_offset(rx, sample_index,
                                                   sample_offset);
            dmr_burst_set_dibit(burst.data, symbol,
                                slice_dibit((hz - offset) / gain));
        }
        burst.first_sample_index = candidate->burst_first_sample;
        burst.timeslot = candidate->timeslot;
        burst.has_sync = true;
        burst.sync_kind = candidate->sync.kind;
        burst.cfo_hz = offset;
        burst.deviation_scale = gain;
        burst.sync_score = candidate->sync.fit.score;
        burst.sample_rate_error_ppm = candidate->timing_drift_per_symbol == 0.0f
            ? 0.0f
            : -candidate->timing_drift_per_symbol /
                DMR_SYNC_SAMPLES_PER_SYMBOL * 1e6f;

        if (rx->config.track_timeslots) {
            rx->tracker.fit = candidate->sync.fit;
            rx->tracker.timing_offset_samples =
                candidate->timing_offset_samples;
            rx->tracker.timing_drift_per_symbol =
                candidate->timing_drift_per_symbol;
            rx->tracker.failures = 0u;
            const int64_t distance = (int64_t)candidate->burst_first_sample -
                                     (int64_t)rx->tracker.anchor_first_sample;
            const int64_t slots = (distance + DMR_TIMESLOT_SAMPLES / 2) / DMR_TIMESLOT_SAMPLES;
            /* Data and voice training words have different timing-fit bias.
             * Estimate clock rate between like syncs, not header -> voice. */
            if (slots >= 2 && candidate->sync.kind == rx->tracker.anchor_sync_kind) {
                double interval = ((double)distance + candidate->timing_offset_samples -
                                   rx->tracker.anchor_timing_offset) / slots;
                if (fabs(interval - DMR_TIMESLOT_SAMPLES) < 2.0) {
                    rx->tracker.slot_samples = interval;
                    rx->tracker.clock_acquired = true;
                }
            }
            if (rx->tracker.clock_acquired)
                rx->tracker.timing_drift_per_symbol = DMR_SYNC_SAMPLES_PER_SYMBOL *
                    (rx->tracker.slot_samples / DMR_TIMESLOT_SAMPLES - 1.0);
            /* Re-anchor on each explicit sync instead of retaining the
             * original sample grid for the entire call. */
            rx->tracker.anchor_first_sample = candidate->burst_first_sample;
            rx->tracker.anchor_timeslot = candidate->timeslot;
            rx->tracker.anchor_timing_offset = candidate->timing_offset_samples;
            rx->tracker.anchor_sync_kind = candidate->sync.kind;
            rx->tracker.next_first_sample = candidate->burst_first_sample + rx->tracker.slot_samples;
            rx->tracker.next_timeslot = 3u - candidate->timeslot;
        }

        candidate->active = false;
        if (rx->config.burst_callback != NULL)
            rx->config.burst_callback(rx->config.callback_context, &burst);
    }
}

static bool pending_at(const dmr_phy_rx_t *rx, uint64_t first_sample)
{
    for (size_t i = 0; i < DMR_RX_PENDING_CAPACITY; ++i) {
        if (rx->pending[i].active) {
            int64_t distance = (int64_t)rx->pending[i].burst_first_sample - (int64_t)first_sample;
            if (distance >= -16 && distance <= 16)
                return true;
        }
    }
    return false;
}

static void emit_tracked_bursts(dmr_phy_rx_t *rx, uint64_t current_sample)
{
    dmr_timeslot_tracker_t *tracker = &rx->tracker;
    if (!tracker->active)
        return;
    const uint64_t first = (uint64_t)llround(tracker->next_first_sample);
    const float fractional = tracker->next_first_sample - first;
    const uint64_t last = first +
        (DMR_BURST_DIBIT_COUNT - 1u) * DMR_SYNC_SAMPLES_PER_SYMBOL + 2u;
    if (current_sample < last)
        return;

    if (!pending_at(rx, first)) {
        dmr_burst_t burst;
        memset(&burst, 0, sizeof(burst));
        float squared_error = 0.0f;
        const float sync_center = DMR_SYNC_FIRST_DIBIT +
            0.5f * (DMR_SYNC_SYMBOL_COUNT - 1u);
        for (size_t symbol = 0; symbol < DMR_BURST_DIBIT_COUNT; ++symbol) {
            const uint64_t index = first +
                symbol * DMR_SYNC_SAMPLES_PER_SYMBOL;
            const float hz = ring_sample_at_offset(
                rx, index, tracker->timing_offset_samples + fractional +
                    ((float)symbol - sync_center) *
                        tracker->timing_drift_per_symbol);
            const float level = (hz - tracker->fit.offset_hz) /
                                tracker->fit.gain_hz_per_level;
            const float error = level - nearest_level(level);
            squared_error += fminf(error * error, 4.0f);
            dmr_burst_set_dibit(burst.data, symbol, slice_dibit(level));
        }
        const float eye_error = squared_error / DMR_BURST_DIBIT_COUNT;
        if (eye_error <= DMR_TRACK_MAX_EYE_ERROR) {
            burst.first_sample_index = first;
            burst.timeslot = tracker->next_timeslot;
            burst.has_sync = false;
            burst.cfo_hz = tracker->fit.offset_hz;
            burst.deviation_scale = tracker->fit.gain_hz_per_level;
            burst.sample_rate_error_ppm =
                -tracker->timing_drift_per_symbol /
                    DMR_SYNC_SAMPLES_PER_SYMBOL * 1e6f;
            tracker->failures = 0u;
            if (rx->config.burst_callback != NULL)
                rx->config.burst_callback(rx->config.callback_context,
                                          &burst);
        } else if (++tracker->failures >= DMR_TRACK_MAX_FAILURES) {
            tracker->active = false;
        }
    }
    tracker->next_first_sample += tracker->slot_samples;
    tracker->next_timeslot = tracker->next_timeslot == 1u ? 2u : 1u;
}

dmr_phy_rx_t *dmr_phy_rx_create(const dmr_phy_rx_config_t *config)
{
    if (config == NULL)
        return NULL;

    dmr_phy_rx_t *rx = calloc(1, sizeof(*rx));
    if (rx == NULL)
        return NULL;
    rx->config = *config;
    if (!(rx->config.sync_threshold > 0.0f &&
          rx->config.sync_threshold <= 1.0f)) {
        rx->config.sync_threshold = DMR_DEFAULT_SYNC_THRESHOLD;
    }
    if (rx->config.matched_filter_delay_symbols == 0u)
        rx->config.matched_filter_delay_symbols =
            DMR_MATCHED_FILTER_DELAY_SYMBOLS;
    if (rx->config.matched_filter_delay_symbols < 3u ||
        rx->config.matched_filter_delay_symbols > 5u) {
        free(rx);
        return NULL;
    }
    if (!dmr_matched_filter_init_with_delay(
            &rx->matched_filter,
            rx->config.matched_filter_delay_symbols)) {
        free(rx);
        return NULL;
    }
    dmr_phy_rx_reset(rx);
    return rx;
}

void dmr_phy_rx_destroy(dmr_phy_rx_t *rx)
{
    if (rx == NULL)
        return;
    dmr_matched_filter_deinit(&rx->matched_filter);
    free(rx);
}

void dmr_phy_rx_reset(dmr_phy_rx_t *rx)
{
    dmr_discriminator_reset(&rx->discriminator);
    dmr_matched_filter_reset(&rx->matched_filter);
    dmr_sync_detector_init(&rx->sync_detector, rx->config.sync_threshold);
    memset(rx->sample_ring, 0, sizeof(rx->sample_ring));
    memset(rx->pending, 0, sizeof(rx->pending));
    memset(&rx->tracker, 0, sizeof(rx->tracker));
    rx->next_filtered_sample = 0;
}

void dmr_phy_rx_process(dmr_phy_rx_t *rx,
                        const float complex *samples,
                        size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        const float in_phase = crealf(samples[i]);
        const float quadrature = cimagf(samples[i]);
        if (!isfinite(in_phase) || !isfinite(quadrature) ||
            (in_phase == 0.0f && quadrature == 0.0f)) {
            /* A missing IQ sample is a stream discontinuity, not merely a
             * discriminator input that produced no output. Flush FIR, sync,
             * and pending-burst history so no burst can bridge the gap. */
            dmr_phy_rx_reset(rx);
            continue;
        }

        float frequency_hz;
        const size_t produced = dmr_discriminator_process(
            &rx->discriminator, &samples[i], 1, &frequency_hz);
        if (produced == 0)
            continue;

        float filtered;
        dmr_matched_filter_process(&rx->matched_filter, &frequency_hz, 1,
                                   &filtered);
        const uint64_t sample_index = rx->next_filtered_sample++;
        rx->sample_ring[sample_index % DMR_RX_SAMPLE_RING_CAPACITY] = filtered;
        dmr_sync_detector_process(&rx->sync_detector, &filtered, 1,
                                  accept_sync, rx);
        emit_ready_bursts(rx, sample_index);
        emit_tracked_bursts(rx, sample_index);
    }
}
