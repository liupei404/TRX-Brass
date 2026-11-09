#include <stdbool.h>
#include <stdint.h>

#include "portable_fixed.h"
#include "portable_frame.h"

static unsigned int pitch_lzcount(uint32_t value)
{
    return value == 0U ? 32U : (unsigned int)__builtin_clz(value);
}

static int32_t pitch_saturate_i32(int64_t value)
{
    if (value > INT32_MAX) {
        return INT32_MAX;
    }
    if (value < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

#define COUNT_LEADING_ZEROS(value) pitch_lzcount((uint32_t)(value))
#define SATURATE_SIGNED(value, bits) (value)
#define DISCARD_SATURATION_STATUS(value, bits) ((void)0)
#define PITCH_ARGMAX_I16(address, count) \
    vocoder_argmax_i16((const int16_t *)(uintptr_t)(address), \
                    (int32_t)(count))
#define PITCH_REFINE_INDEX(index, address) \
    vocoder_refine_table_index_q16((int32_t)(index), \
        (const int16_t *)(uintptr_t)(address))
#define PITCH_SUM_I16(address, count) \
    vocoder_sum_i16((const int16_t *)(uintptr_t)(address), \
                 (int32_t)(count))
#define PITCH_SUM_INTERVALS(address, step, exponent, count) \
    vocoder_sum_adaptive_intervals( \
        (const int16_t *)(uintptr_t)(address), \
        (int16_t)(step), (int32_t)(exponent), (int32_t)(count))
#define PITCH_TAIL_COMPLEMENT(value, exponent, address, step, step_exponent) \
    vocoder_tail_sum_complement((int16_t)(value), (int32_t)(exponent), \
        (const int16_t *)(uintptr_t)(address), \
        (int16_t)(step), (int32_t)(step_exponent))
#define PITCH_SUM_BANDS(exponent, address, numerator, spacing, phase) \
    vocoder_sum_normalized_bands((exponent), \
        (const int32_t *)(uintptr_t)(address), \
        (int32_t)(numerator), (int32_t)(spacing), (phase))
#define PITCH_SUM_I32(address, count, exponent) \
    vocoder_sum_i32_normalized( \
        (const int32_t *)(uintptr_t)(address), \
        (int32_t)(count), (exponent))
#define PITCH_NORMALIZED_COMPLEMENT vocoder_one_minus_normalized_ratio
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wpointer-sign"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "portable_pitch_impl.inc"
#pragma GCC diagnostic pop

int32_t vocoder_track_pitch_analysis(
    int16_t state[7],
    int16_t history[10],
    const int16_t *correlation,
    const int32_t energy[128],
    const void *encoder_state,
    int32_t enabled,
    int32_t auxiliary_value,
    int16_t *output_flag)
{
    return vocoder_track_pitch_analysis_raw(
        state, history, (intptr_t)correlation,
        (intptr_t)energy,
        (uint32_t)(uintptr_t)encoder_state, enabled,
        (uint32_t)auxiliary_value,
        (uint16_t *)(void *)output_flag);
}
