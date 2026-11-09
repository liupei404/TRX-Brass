#include <stdbool.h>
#include <stdint.h>

#include "portable_fixed.h"
#include "portable_frame.h"


static const int16_t encoder_window_58[58] = {
    2644, 2820, 3171, 3692, 4377, 5219, 6206, 7329,
    8573, 9924, 11366, 12882, 14454, 16065, 17695, 19324,
    20935, 22508, 24024, 25466, 26817, 28061, 29183, 30171,
    31012, 31697, 32219, 32569, 32746, 0, -4456, 3034,
    19608, 29164, 19608, 3034, 1137, 2214, 3621, 5360,
    7417, 9756, 12321, 15041, 17827, 20581, 23199, 25576,
    27614, 29226, 30342, 30913, -5686, -566, -2614, -2870,
    -6966, -2102
};

static const int16_t encoder_window_128[128] = {
    381, 789, 1223, 1685, 2174, 2693, 2859, 3030,
    3204, 3382, 3564, 3749, 3939, 4132, 4329, 4530,
    4734, 4942, 5154, 5369, 5588, 5810, 6035, 6264,
    6495, 6730, 6968, 7210, 7454, 7701, 7950, 8203,
    8458, 8715, 8976, 9238, 9503, 9770, 10039, 10310,
    10583, 10857, 11134, 11412, 11691, 11972, 12254, 12537,
    12821, 13106, 13392, 13679, 13966, 14253, 14541, 14829,
    15117, 15405, 15693, 15980, 16267, 16554, 16839, 17124,
    17408, 17691, 17972, 18252, 18531, 18808, 19084, 19357,
    19628, 19898, 20165, 20429, 20691, 20951, 21208, 21461,
    21712, 21960, 22204, 22445, 22682, 22916, 23146, 23372,
    23594, 23812, 24026, 24236, 24441, 24641, 24837, 25029,
    25215, 25397, 25573, 25745, 25911, 26072, 26228, 26378,
    26523, 26662, 26795, 26923, 27045, 27161, 27271, 27375,
    27473, 27565, 27651, 27731, 27804, 27871, 27932, 27987,
    28035, 28077, 28112, 28141, 28164, 28180, 28190, 28193
};

static unsigned int encoder_popcount8(uint8_t value)
{
    unsigned int count = 0;
    while (value != 0U) {
        count += value & 1U;
        value = (uint8_t)(value >> 1);
    }
    return count;
}

static unsigned int encoder_lzcount(uint32_t value)
{
    return value == 0U ? 32U : (unsigned int)__builtin_clz(value);
}

static int32_t encoder_saturating_add_i32(
    int32_t left, int32_t right)
{
    int64_t result = (int64_t)left + right;
    if (result > INT32_MAX) {
        return INT32_MAX;
    }
    if (result < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)result;
}

static uint32_t encoder_lsl32(uint32_t value, unsigned int shift)
{
    shift &= 0xffU;
    return shift < 32U ? value << shift : 0U;
}

static uint32_t encoder_asr32(uint32_t value, unsigned int shift)
{
    shift &= 0xffU;
    if (shift == 0U) return value;
    if (shift >= 32U) {
        return (value & 0x80000000U) != 0U ? UINT32_MAX : 0U;
    }
    uint32_t result = value >> shift;
    if ((value & 0x80000000U) != 0U) {
        result |= UINT32_MAX << (32U - shift);
    }
    return result;
}

static int32_t encoder_track_pitch_adapter(
    intptr_t state, intptr_t history,
    intptr_t correlation, intptr_t energy,
    intptr_t encoder_state, int32_t enabled,
    int32_t auxiliary, int16_t *flag,
    intptr_t unused_state, int32_t unused_analysis_flag)
{
    (void)unused_state;
    (void)unused_analysis_flag;
    return vocoder_track_pitch_analysis(
        (int16_t *)(uintptr_t)state,
        (int16_t *)(uintptr_t)history,
        (const int16_t *)(uintptr_t)correlation,
        (const int32_t *)(uintptr_t)energy,
        (const void *)(uintptr_t)encoder_state,
        enabled, auxiliary, flag);
}

#define COUNT_LEADING_ZEROS(value) encoder_lzcount((uint32_t)(value))
#define SATURATE_SIGNED(value, bits) \
    encoder_saturating_add_i32(0, (int32_t)(value))
#define DISCARD_SATURATION_STATUS(value, bits) ((void)0)
#define ENCODER_ASR32(value, shift) \
    encoder_asr32((uint32_t)(value), (unsigned int)(shift))
#define ENCODER_LSL32(value, shift) \
    encoder_lsl32((uint32_t)(value), (unsigned int)(shift))
#define vocoder_track_pitch_analysis encoder_track_pitch_adapter

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wpointer-sign"
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#pragma GCC diagnostic ignored "-Woverflow"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "portable_encoder_analysis_impl.inc"
#pragma GCC diagnostic pop

#undef vocoder_track_pitch_analysis

void vocoder_analyze_encoder_frame(
    void *subframe_state,
    void *encoder_state,
    int32_t enabled,
    const int16_t *samples,
    int32_t new_sample_count,
    void *analysis_state,
    int16_t option_flag,
    int16_t *output_flag,
    void *auxiliary_state)
{
    vocoder_analyze_encoder_frame_raw(
        (intptr_t)subframe_state, (intptr_t)encoder_state,
        enabled, (intptr_t)samples, new_sample_count,
        (intptr_t)analysis_state, option_flag, output_flag,
        (intptr_t)auxiliary_state);
}
