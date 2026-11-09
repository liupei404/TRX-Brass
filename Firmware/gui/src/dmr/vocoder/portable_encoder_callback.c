#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "portable_fixed.h"
#include "portable_frame.h"

static uint32_t callback_asr(uint32_t value, unsigned int shift)
{
    shift &= 0xffU;
    if (shift == 0U) return value;
    if (shift >= 32U) return (value & 0x80000000U) ? UINT32_MAX : 0U;
    uint32_t result = value >> shift;
    if ((value & 0x80000000U) != 0U) result |= UINT32_MAX << (32U - shift);
    return result;
}

static uint32_t callback_lsl(uint32_t value, unsigned int shift)
{
    shift &= 0xffU;
    return shift < 32U ? value << shift : 0U;
}

static int32_t callback_clz_shift(uint32_t value)
{
    if (value == 0U) return 0;
    uint32_t magnitude = (value & 0x80000000U) ? ~value : value;
#if defined(__GNUC__) || defined(__clang__)
    return (int32_t)__builtin_clz(magnitude) - 1;
#else
    int32_t shift = -1;
    do { magnitude <<= 1; ++shift; } while ((magnitude & 0x80000000U) == 0U);
    return shift;
#endif
}

static int32_t callback_saturating_add(int32_t left, int32_t right)
{
    int64_t sum = (int64_t)left + right;
    if (sum > INT32_MAX) return INT32_MAX;
    if (sum < INT32_MIN) return INT32_MIN;
    return (int32_t)sum;
}

static int16_t callback_load_i16(const void *base, size_t offset)
{
    int16_t value;
    memcpy(&value, (const uint8_t *)base + offset, sizeof(value));
    return value;
}

static uint32_t callback_load_u32(const void *base, size_t offset)
{
    uint32_t value;
    memcpy(&value, (const uint8_t *)base + offset, sizeof(value));
    return value;
}

static void callback_store_i16(void *base, size_t offset, int16_t value)
{
    memcpy((uint8_t *)base + offset, &value, sizeof(value));
}

void vocoder_filter_encoder_band_history(
    int16_t history[41], int16_t first[40], int16_t second[40],
    int16_t input)
{
    static const int16_t coefficients[9] = {
        2949, 3932, 2949, 3932, 5242, 3932, 2949, 3932, 2949
    };
    history[40] = history[39];
    for (int index = 0; index < 38; ++index) {
        int16_t first0 = first[index];
        int16_t second0 = second[index];
        int64_t sum =
            (int64_t)history[index + 2] * coefficients[8] +
            (int64_t)first[index + 2] * coefficients[5] +
            (int64_t)second[index + 2] * coefficients[2] +
            (int64_t)history[index + 1] * coefficients[7] +
            (int64_t)first[index + 1] * coefficients[4] +
            (int64_t)second[index + 1] * coefficients[1] +
            (int64_t)history[index] * coefficients[6] +
            (int64_t)second0 * coefficients[0] +
            (int64_t)first0 * coefficients[3];
        second[index] = first0;
        first[index] = history[index];
        history[index] = input;
        input = (int16_t)callback_asr((uint32_t)(sum * 2), 16);
    }
    history[38] = input;
    history[0] = history[1];
    history[39] = input;
}

void vocoder_update_encoder_spectral_callback(
    int32_t selector, uintptr_t context_value,
    vocoder_frame_state *frame, uintptr_t analysis_value,
    uintptr_t spectral_state_value, uintptr_t exponent_table_value,
    uintptr_t sample_table_value, int32_t slice_index,
    int32_t spectrum_index, int32_t mode)
{
    static const int16_t weights[40] = {
        4096,4096,4096,4096,4096,4915,6553,8192,
        9830,11468,13107,13926,14745,15564,16384,16384,
        16384,16384,16384,16384,16384,16384,16384,16384,
        16384,16384,16384,16384,16384,16384,16384,16384,
        16384,16384,16384,14745,13107,11468,9830,8192
    };
    int16_t *context = (int16_t *)context_value;
    uint8_t *analysis = (uint8_t *)analysis_value;
    const int16_t *pitch_table = (const int16_t *)spectral_state_value;
    const int16_t *band_samples_table =
        (const int16_t *)exponent_table_value;
    const int16_t *exponent_table =
        (const int16_t *)sample_table_value;
    int16_t local_values[48] = {0};
    int16_t local_exponents[42] = {0};

    int32_t pitch_sum = (int32_t)pitch_table[spectrum_index + 2] * 0x8000;
    uint32_t pitch_average = (uint32_t)(pitch_sum +
        (int32_t)pitch_table[spectrum_index + 3] * 0x8000) >> 16;
    int32_t mask = (int32_t)callback_lsl(
        0xfU, (unsigned int)((spectrum_index - slice_index) + 2));
    uint32_t voicing = callback_load_u32(frame, 8);
    int32_t limited_mode = mode & ~(mode >> 31);
    if (limited_mode > 0x4000) limited_mode = 0x4000;
    uint32_t selected = ((uint32_t)mask >> 2) & (uint32_t)selector;
    int32_t neighboring_pitch = (int32_t)callback_asr(
        (uint32_t)(int32_t)pitch_table[spectrum_index + 1] << 16, 1);
    int special = selected == 0U &&
        pitch_sum + neighboring_pitch < -0x08000000;

    int16_t first_exponent = exponent_table[slice_index * 2];
    int16_t second_exponent = exponent_table[slice_index * 2 + 1];
    int32_t difference = first_exponent - second_exponent;
    uint32_t first_shift;
    uint32_t second_shift;
    if (difference < 1) {
        first_shift = (uint32_t)(difference + 15);
        second_shift = 15U;
    } else {
        second_shift = (uint32_t)(-difference + 15);
        first_shift = 15U;
        second_exponent = first_exponent;
    }
    const int16_t *band_samples = band_samples_table + slice_index * 80;
    for (int index = 0; index < 40; ++index) {
        int32_t first = (int32_t)callback_lsl(
            (uint32_t)(int32_t)band_samples[index], first_shift);
        int32_t second = (int32_t)callback_lsl(
            (uint32_t)(int32_t)band_samples[40 + index], second_shift);
        uint32_t first_magnitude = first < 0
            ? 0U - (uint32_t)first : (uint32_t)first;
        uint32_t second_magnitude = second < 0
            ? 0U - (uint32_t)second : (uint32_t)second;
        uint32_t sum = first_magnitude + second_magnitude;
        int32_t shift0 = callback_clz_shift(sum);
        int32_t normalized0 = (int32_t)callback_asr(
            callback_lsl(sum, (unsigned int)shift0), 16);
        uint32_t scaled = (uint32_t)(normalized0 * 0x4000);
        int32_t shift1 = callback_clz_shift(scaled);
        local_exponents[index] = (int16_t)(
            (second_exponent + 2 - shift0) - shift1);
        local_values[index + 6] = (int16_t)callback_asr(
            callback_lsl(scaled, (unsigned int)shift1), 16);
    }

    for (int index = 0; index < 40; ++index) {
        int16_t *old_value = context + 1 + index;
        int16_t *old_exponent = context + 41 + index;
        int32_t output_exponent;
        if (special) {
            *old_value = (int16_t)vocoder_normalized_add(
                local_values[index + 6], local_exponents[index] - 2,
                (int16_t)((int32_t)*old_value * 0xc000 >> 16),
                *old_exponent, &output_exponent);
            *old_exponent = (int16_t)output_exponent;
        } else {
            int32_t above = vocoder_compare_scaled(
                *old_value, *old_exponent,
                (int16_t)((int32_t)local_values[index + 6] * 0xaaaa >> 16),
                local_exponents[index] - 1);
            if (above != 0 && (voicing & 0x55555555U) == 0U) {
                *old_value = (int16_t)vocoder_normalized_add(
                    (int16_t)((int32_t)local_values[index + 6] * 0x6666 >> 15),
                    local_exponents[index] - 3,
                    (int16_t)((int32_t)*old_value * 0x7333 >> 15),
                    *old_exponent, &output_exponent);
                *old_exponent = (int16_t)output_exponent;
            }
        }
    }

    voicing = callback_load_u32(frame, 8);
    int32_t field_position = 0;
    for (int index = 0; index < 40; ++index) {
        int32_t mantissa = local_values[index + 6];
        int32_t exponent = local_exponents[index];
        if (field_position > 39) {
            field_position -= 40;
            voicing = (uint32_t)((int32_t)voicing >> 2);
        }
        field_position += 16;
        if ((voicing & 1U) == 0U) {
            uint32_t base = callback_asr((uint32_t)(int32_t)weights[index] << 16, 1)
                + 0x20000000U;
            int32_t base_shift = callback_clz_shift(base);
            int16_t base_mantissa = (int16_t)callback_asr(
                callback_lsl(base, (unsigned int)base_shift), 16);
            uint32_t product = (uint32_t)(
                (int32_t)base_mantissa * mantissa * 2);
            int32_t product_shift = callback_clz_shift(product);
            exponent = exponent + 1 - base_shift - product_shift;
            mantissa = (int16_t)callback_asr(
                callback_lsl(product, (unsigned int)product_shift), 16);
        }
        uint32_t current_bits = (uint32_t)mantissa << 16;
        uint32_t old_bits = (uint32_t)(int32_t)context[index + 1] << 16;
        int32_t current_shift = callback_clz_shift(current_bits);
        int32_t old_shift = callback_clz_shift(old_bits);
        uint32_t current_normalized = callback_lsl(
            current_bits, (unsigned int)current_shift);
        if (current_normalized == 0U) {
            local_values[index + 6] = 0;
            continue;
        }
        int32_t old_exponent = context[index + 41] - old_shift;
        int32_t current_exponent = exponent - current_shift;
        uint32_t old_normalized = callback_lsl(old_bits, (unsigned int)old_shift);
        if ((old_exponent <= current_exponent || old_normalized == 0U) &&
                (old_exponent != current_exponent ||
                 (int32_t)old_normalized < (int32_t)current_normalized)) {
            int32_t ratio_exponent;
            int32_t ratio = vocoder_normalized_divide(
                (int32_t)old_normalized, old_exponent,
                (int16_t)(current_normalized >> 16), current_exponent,
                &ratio_exponent);
            uint32_t ratio_bits = (uint32_t)ratio << 16;
            ratio_bits = ratio_exponent < 0
                ? callback_asr(ratio_bits, (unsigned int)(-ratio_exponent))
                : callback_lsl(ratio_bits, (unsigned int)ratio_exponent);
            int32_t difference_bits = callback_saturating_add(
                (int32_t)ratio_bits, INT32_MIN);
            local_values[index + 6] = (int16_t)(
                ((int32_t)(int16_t)(difference_bits >> 16) * -0xaccc) >> 16);
        } else {
            local_values[index + 6] = 0;
        }
    }

    vocoder_filter_encoder_band_history(
        local_values + 6, context + 81, context + 121,
        (int16_t)limited_mode);

    int16_t pitch_factor;
    if ((int16_t)pitch_average == INT16_MIN) {
        pitch_factor = INT16_MAX;
    } else {
        int32_t product = (int32_t)(
            (uint32_t)(int32_t)(int16_t)pitch_average *
            (uint32_t)-0x40000);
        pitch_factor = (int16_t)callback_asr((uint32_t)product, 16);
        if ((int16_t)pitch_average * -0x10000 -
                (int32_t)callback_asr((uint32_t)product, 2) != 0) {
            pitch_factor = (int16_t)(
                ((int16_t)pitch_average * -0x10000 >> 31) ^ 0x7fff);
        }
    }
    int32_t positive_pitch = pitch_factor;
    if (positive_pitch <= 0x4000) {
        positive_pitch &= ~(positive_pitch >> 31);
    } else {
        positive_pitch = 0x4000;
    }
    int16_t scaled_context;
    if (selected == 0U) {
        scaled_context = (int16_t)((uint32_t)((int32_t)*context * 0xf58c) >> 16);
    } else if (((uint32_t)selector & ((uint32_t)mask >> 3)) == 0U) {
        scaled_context = *context;
    } else {
        scaled_context = (int16_t)((uint32_t)((int32_t)*context * 0x10b5c) >> 16);
    }
    int32_t bounded = scaled_context;
    if (bounded > limited_mode) bounded = limited_mode;
    if (bounded < limited_mode / 2) bounded = limited_mode / 2;
    *context = (int16_t)bounded;
    if (slice_index != 0) return;

    uint32_t current_voicing = callback_load_u32(frame, 8);
    int32_t pitch_complement =
        (int32_t)((uint32_t)(positive_pitch * -0x10000 + 0x40000000) >> 16);
    int32_t table_index = 0;
    int32_t bit_position = 0;
    uint32_t voicing_word = current_voicing;
    int32_t previous_weight = weights[0];
    uint32_t previous_value = (uint16_t)local_values[6];
    for (;;) {
        uint32_t active = voicing_word & 1U;
        int32_t current_weight = previous_weight;
        uint32_t current_value = previous_value;
        if (active == 0U) {
            current_weight = (previous_weight * bounded * 4) >> 16;
            if ((int32_t)current_value > 0x4000) current_value = 0x4000;
            if ((int32_t)current_value < current_weight) current_value = (uint32_t)current_weight;
            local_values[6 + table_index] = (int16_t)(
                (pitch_complement * (int32_t)current_value +
                 current_weight * positive_pitch) * 4 >> 16);
        } else {
            /* The voiced path joins the same lower-bound clamp as the
             * unvoiced path, using bounded instead of the band weight. */
            if ((int32_t)current_value > 0x4000) current_value = 0x4000;
            if ((int32_t)current_value < bounded) current_value = (uint32_t)bounded;
            local_values[6 + table_index] = (int16_t)(
                (pitch_complement * (int32_t)current_value +
                 bounded * positive_pitch) * 4 >> 16);
        }
        if (table_index == 39) break;
        ++table_index;
        /* Forty bands span sixteen two-bit voicing fields. Advance by
         * 16 units per band, carrying at 40 as in the Thumb loop. */
        bit_position += 16;
        previous_weight = weights[table_index];
        previous_value = (uint16_t)local_values[6 + table_index];
        if (bit_position >= 40) {
            voicing_word = (uint32_t)((int32_t)voicing_word >> 2);
            bit_position -= 40;
        }
    }

    int16_t harmonic_count = callback_load_i16(frame, 4);
    int16_t spacing = callback_load_i16(frame, 12);
    int16_t accumulated = callback_load_i16(analysis, 0x13c);
    uint32_t accumulated_exponent =
        (uint16_t)callback_load_i16(analysis, 0x13e);
    for (int index = 8; index < 16; ++index) {
        accumulated = (int16_t)vocoder_normalized_subtract(
            callback_load_i16(analysis, 0x150 + (size_t)(index - 8) * 2U),
            callback_load_i16(analysis, 0x170 + (size_t)(index - 8) * 2U),
            accumulated, (int32_t)accumulated_exponent,
            (int32_t *)&accumulated_exponent);
    }
    int16_t reference_log;
    int32_t reference_exponent;
    if (accumulated < 1) {
        reference_log = -0x2800;
        reference_exponent = 5;
    } else {
        uint32_t logarithm = (uint32_t)vocoder_log2_q15(
            (uint16_t)accumulated, (int32_t)accumulated_exponent + 0x1e);
        int32_t normalization = callback_clz_shift(logarithm);
        reference_log = (int16_t)callback_asr(
            callback_lsl(logarithm, (unsigned int)normalization), 16);
        reference_exponent = 15 - normalization;
    }
    if (harmonic_count < 1) return;

    int32_t frequency_step = spacing * 10;
    uint32_t phase_step = (uint32_t)(spacing * 4);
    int32_t frequency = frequency_step - 0x8000;
    uint32_t phase = phase_step;
    int32_t interpolation = frequency_step;
    for (int index = 0; index < harmonic_count; ++index) {
        int32_t bin = frequency >> 16;
        int32_t interpolated =
            ((int32_t)local_values[bin + 7] - local_values[bin + 6]) *
                (int32_t)(uint16_t)frequency +
            (int32_t)local_values[bin + 6] * 0x10000;
        uint32_t interpolation_bits = (uint32_t)interpolated;
        int32_t normalization = callback_clz_shift(interpolation_bits);
        int32_t logarithm = interpolation_bits == 0U ? -0x8000 :
            vocoder_log2_scaled(
                (uint16_t)(callback_lsl(interpolation_bits,
                    (unsigned int)normalization) >> 16),
                1 - normalization, 4);
        if (((int32_t)current_voicing >>
                (unsigned int)(((int32_t)(phase + 0x8000U) >> 15) & 0xfc) & 1) != 0) {
            if (frequency < 0x38000) {
                logarithm = (int32_t)callback_asr((uint32_t)logarithm << 16, 17);
            } else if (frequency < 0x78000) {
                logarithm = logarithm *
                    (int32_t)callback_asr((uint32_t)interpolation << 12, 16) * 2 >> 16;
            }
        }
        if (callback_load_i16(analysis, 0x180) != 0 &&
                frequency > 0x138000) {
            int32_t adaptive_mantissa;
            int32_t adaptive_exponent;
            int32_t phase_index = (int32_t)phase >> 16;
            if (phase_index < 15) {
                int32_t interpolated_exponent;
                int16_t delta = (int16_t)vocoder_normalized_subtract(
                    callback_load_i16(analysis,
                        0x140U + (size_t)phase_index * 2U),
                    callback_load_i16(analysis,
                        0x160U + (size_t)phase_index * 2U),
                    callback_load_i16(analysis,
                        0x142U + (size_t)phase_index * 2U),
                    callback_load_i16(analysis,
                        0x162U + (size_t)phase_index * 2U),
                    &interpolated_exponent);
                adaptive_mantissa = (int16_t)vocoder_normalized_add(
                    (int16_t)(((int32_t)delta *
                        (int32_t)(uint16_t)phase) >> 16),
                    interpolated_exponent,
                    callback_load_i16(analysis,
                        0x140U + (size_t)phase_index * 2U),
                    callback_load_i16(analysis,
                        0x160U + (size_t)phase_index * 2U),
                    &adaptive_exponent);
            } else {
                adaptive_mantissa = callback_load_i16(analysis, 0x70);
                adaptive_exponent = callback_load_i16(analysis, 0x90);
            }
            int32_t logarithm_exponent;
            if (adaptive_mantissa > 0) {
                logarithm_exponent = adaptive_exponent + 0x1e;
            } else {
                adaptive_exponent = -0x2b;
                logarithm_exponent = -0xd;
                adaptive_mantissa = 0x6894;
            }
            uint32_t adaptive_logarithm = (uint32_t)vocoder_log2_q15(
                (uint16_t)(int16_t)adaptive_mantissa, logarithm_exponent);
            int32_t adaptive_shift = callback_clz_shift(adaptive_logarithm);
            int32_t adaptive_log_exponent = 15 - adaptive_shift;
            int32_t difference_exponent;
            int16_t log_difference = (int16_t)vocoder_normalized_subtract(
                (int16_t)(callback_lsl(adaptive_logarithm,
                    (unsigned int)adaptive_shift) >> 16),
                adaptive_log_exponent, reference_log, reference_exponent,
                &difference_exponent);
            int32_t correction_exponent;
            int32_t correction = (int16_t)vocoder_normalized_subtract(
                0x2000, 2, log_difference, difference_exponent,
                &correction_exponent);
            uint32_t correction_bits = (uint32_t)correction << 16;
            correction_bits = correction_exponent < 15
                ? callback_asr(correction_bits,
                    (unsigned int)(15 - correction_exponent))
                : callback_lsl(correction_bits,
                    (unsigned int)(correction_exponent - 15));
            int32_t correction_value =
                (int32_t)correction_bits - ((int32_t)phase >> 3);
            int32_t negative_half =
                (correction_value >> 1) & (correction_value >> 31);
            if (negative_half < -0x30000) negative_half = -0x30000;
            logarithm = (int32_t)((uint32_t)(
                negative_half + logarithm * 0x20) * 0x800U) >> 16;
        }
        int32_t output = callback_saturating_add(
            (int32_t)((uint32_t)logarithm << 16),
            (int32_t)((uint32_t)(int32_t)callback_load_i16(
                frame, 16 + (size_t)index * 2U) << 16));
        int32_t value = output >> 16;
        if (value < -0x7800) value = -0x7800;
        callback_store_i16(frame, 16 + (size_t)index * 2U, (int16_t)value);
        phase += phase_step;
        frequency += frequency_step;
        interpolation += frequency_step;
    }
    (void)reference_log;
    (void)reference_exponent;
}
