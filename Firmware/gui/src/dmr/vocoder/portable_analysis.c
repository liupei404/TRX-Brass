#include <stdbool.h>
#include <stdint.h>

#include "portable_fixed.h"
#include "portable_frame.h"

static unsigned int analysis_lzcount(uint32_t value)
{
    return value == 0U ? 32U : (unsigned int)__builtin_clz(value);
}

static uint32_t analysis_carry4(uint32_t left, uint32_t right)
{
    return left + right < left ? 1U : 0U;
}

static int32_t analysis_saturate_i32(int64_t value)
{
    if (value > INT32_MAX) {
        return INT32_MAX;
    }
    if (value < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static int32_t analysis_saturating_add_i32(
    int32_t left, int32_t right)
{
    return analysis_saturate_i32((int64_t)left + right);
}

static uint32_t analysis_lsl32(uint32_t value, unsigned int shift)
{
    shift &= 0xffU;
    return shift < 32U ? value << shift : 0U;
}

static uint32_t analysis_asr32(uint32_t value, unsigned int shift)
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

#define COUNT_LEADING_ZEROS(value) analysis_lzcount((uint32_t)(value))
#define ADD_HAS_CARRY(left, right) analysis_carry4((uint32_t)(left), (uint32_t)(right))
#define DISCARD_SATURATION_STATUS(value, bits) ((void)0)
#define ASR32(value, shift) analysis_asr32((uint32_t)(value), (unsigned int)(shift))
#define LSL32(value, shift) analysis_lsl32((uint32_t)(value), (unsigned int)(shift))
#define vocoder_copy_i16(destination, source, count) \
    vocoder_copy_i16((int16_t *)(uintptr_t)(destination), \
        (const int16_t *)(uintptr_t)(source), (int32_t)(count))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wpointer-sign"
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "portable_analysis_impl.inc"
#pragma GCC diagnostic pop

#undef vocoder_copy_i16

int32_t vocoder_update_spectral_energy_state(
    void *state,
    const int32_t values[16],
    int32_t value_exponent,
    int32_t divisor)
{
    return vocoder_update_spectral_energy_state_raw(
        (intptr_t)state, (int32_t *)(uintptr_t)values,
        value_exponent, divisor);
}
