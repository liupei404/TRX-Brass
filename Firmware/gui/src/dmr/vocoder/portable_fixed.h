#ifndef PORTABLE_FIXED_H
#define PORTABLE_FIXED_H

#include <stdint.h>

/*
 * Fixed-point signed division primitives recovered from MD-380 firmware.
 * A denominator of zero is outside the firmware routine's valid domain.
 */
int32_t vocoder_fixed_divide(int32_t numerator, int16_t denominator);
int32_t vocoder_fixed_divide_half(int32_t numerator, int16_t denominator);
int16_t vocoder_classify_code(int16_t value);
int16_t vocoder_normalized_divide(
    int32_t numerator,
    int32_t numerator_exponent,
    int16_t denominator,
    int32_t denominator_exponent,
    int32_t *result_exponent);
uint16_t vocoder_normalized_add(
    int16_t left,
    int32_t left_exponent,
    int16_t right,
    int32_t right_exponent,
    int32_t *result_exponent);
uint16_t vocoder_normalized_subtract(
    int16_t left,
    int32_t left_exponent,
    int16_t right,
    int32_t right_exponent,
    int32_t *result_exponent);
uint32_t vocoder_normalized_sqrt(uint32_t mantissa, int32_t *exponent);
int32_t vocoder_log2_q15(uint16_t mantissa, int32_t exponent);
uint32_t vocoder_exp2_q16(int32_t value, int32_t *exponent);
int16_t vocoder_log2_scaled(
    uint16_t mantissa, int32_t exponent, int32_t output_q);
int16_t vocoder_exp2_scaled(
    int16_t value, int32_t input_q, int32_t output_exponent);
int16_t vocoder_sqrt_scaled(
    uint32_t mantissa, int32_t exponent, int32_t output_exponent);
int32_t vocoder_average_i16(const int16_t *values, int32_t count);
int16_t vocoder_harmonic_count(int32_t pitch);
uint32_t vocoder_resample_step_q16(int32_t output_pitch, int32_t input_pitch);
void vocoder_resample_spectrum(
    int16_t *output,
    int32_t output_pitch,
    int32_t input_pitch,
    const int16_t input[56],
    int32_t output_count);
void vocoder_decode_pitch(
    int16_t *pitch,
    int16_t *harmonic_count,
    int32_t encoded_pitch,
    int32_t allow_high_pitch);
int16_t vocoder_log_amplitude_clamped(
    uint16_t mantissa,
    int32_t exponent,
    int16_t bias);
int32_t vocoder_reciprocal_vector(
    int16_t *output,
    const int16_t *input,
    int32_t input_q,
    int32_t scale_shift,
    int32_t count);
void vocoder_decode_pitch_parameters(
    int32_t mode,
    int16_t *pitch,
    int16_t *harmonic_count,
    int32_t coefficient_index,
    int32_t divisor_shift,
    int16_t allow_high_pitch);
int16_t vocoder_one_minus_normalized_ratio(
    int16_t numerator,
    int32_t numerator_exponent,
    int16_t denominator,
    int32_t denominator_exponent);
void vocoder_quantize_log_amplitudes(
    int16_t output[8],
    const int16_t input[8],
    int32_t exponent,
    int32_t lower_last_four);
int32_t vocoder_update_gain_if_above_threshold(
    uint16_t *gain,
    int32_t offset,
    int32_t threshold_mantissa,
    int32_t threshold_exponent,
    int16_t limit,
    int16_t threshold,
    int16_t multiplier,
    int32_t subtraction);
int16_t vocoder_sum_normalized_bands(
    int32_t *result_exponent,
    const int32_t values[128],
    int32_t numerator,
    int32_t spacing,
    int32_t *initial_phase);
int16_t vocoder_quantize_pitch_index(
    int16_t *pitch,
    int16_t *harmonic_count,
    int32_t mode,
    int32_t table_bits,
    int16_t allow_high_pitch);
int32_t vocoder_adjust_harmonic_index(
    uint16_t *index,
    int32_t harmonic_count,
    int32_t adjustment,
    uint32_t maximum_index,
    int32_t update_index);

#endif
