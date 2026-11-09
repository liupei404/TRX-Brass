#include "portable_fixed.h"

#include <limits.h>

static uint32_t saturated_magnitude32(int32_t value)
{
    if (value == INT32_MIN) {
        return INT32_MAX;
    }
    return value < 0 ? (uint32_t)(-value) : (uint32_t)value;
}

static uint32_t saturated_magnitude16(int16_t value)
{
    if (value == INT16_MIN) {
        return INT16_MAX;
    }
    return value < 0 ? (uint32_t)(-value) : (uint32_t)value;
}

static int32_t fixed_divide_common(
    int32_t numerator,
    int16_t denominator,
    int halve)
{
    uint32_t numerator_magnitude = saturated_magnitude32(numerator);
    uint32_t denominator_magnitude = saturated_magnitude16(denominator);
    uint32_t quotient = 0;

    if (numerator_magnitude >= denominator_magnitude) {
        quotient = numerator_magnitude / denominator_magnitude;
        if (halve) {
            quotient >>= 1;
        }
    }

    if ((numerator < 0) != (denominator < 0)) {
        return -(int32_t)quotient;
    }
    return (int32_t)quotient;
}

int32_t vocoder_fixed_divide(int32_t numerator, int16_t denominator)
{
    return fixed_divide_common(numerator, denominator, 0);
}

int32_t vocoder_fixed_divide_half(int32_t numerator, int16_t denominator)
{
    return fixed_divide_common(numerator, denominator, 1);
}

int16_t vocoder_classify_code(int16_t value)
{
    uint16_t bits = (uint16_t)value;

    if ((uint16_t)(bits - 5U) < 118U) {
        return 0;
    }
    if ((uint16_t)(bits - 128U) < 16U) {
        return 1;
    }
    if ((uint16_t)(bits - 144U) < 16U) {
        return 2;
    }
    if ((uint16_t)(bits - 160U) < 4U) {
        return 3;
    }
    return 4;
}

static unsigned int count_leading_zeros32(uint32_t value)
{
#if defined(__GNUC__) || defined(__clang__)
    return value == 0 ? 32U : (unsigned int)__builtin_clz(value);
#else
    unsigned int count = 0;
    if (value == 0) {
        return 32U;
    }
    while ((value & 0x80000000U) == 0) {
        value <<= 1;
        count++;
    }
    return count;
#endif
}

int16_t vocoder_normalized_divide(
    int32_t numerator,
    int32_t numerator_exponent,
    int16_t denominator,
    int32_t denominator_exponent,
    int32_t *result_exponent)
{
    uint32_t numerator_magnitude = saturated_magnitude32(numerator);
    uint32_t denominator_magnitude = saturated_magnitude16(denominator);

    if (numerator_magnitude >= (denominator_magnitude << 16)) {
        numerator_magnitude >>= 1;
        numerator_exponent++;
    }

    uint16_t quotient = (uint16_t)(
        (numerator_magnitude / denominator_magnitude) >> 1);
    if ((numerator < 0) != (denominator < 0)) {
        quotient = (uint16_t)(0U - quotient);
    }

    uint32_t shifted = (uint32_t)quotient << 16;
    uint32_t magnitude = (shifted & 0x80000000U) != 0
        ? ~shifted
        : shifted;
    unsigned int normalization_shift = quotient == 0
        ? 0U
        : count_leading_zeros32(magnitude) - 1U;
    shifted <<= normalization_shift;

    int16_t result = (int16_t)(shifted >> 16);
    if (result == 0) {
        *result_exponent = 0;
    }
    else {
        *result_exponent = numerator_exponent -
            denominator_exponent - (int32_t)normalization_shift;
    }
    return result;
}

static uint32_t arithmetic_shift_right(uint32_t value, unsigned int shift)
{
    if (shift == 0) {
        return value;
    }
    if (shift >= 32U) {
        return (value & 0x80000000U) != 0 ? UINT32_MAX : 0U;
    }
    uint32_t result = value >> shift;
    if ((value & 0x80000000U) != 0) {
        result |= UINT32_MAX << (32U - shift);
    }
    return result;
}

static uint32_t align_q15(int16_t mantissa, unsigned int shift)
{
    return arithmetic_shift_right((uint32_t)(uint16_t)mantissa << 16, shift);
}

static uint16_t normalize_q31(
    uint32_t value,
    int32_t exponent,
    int32_t *result_exponent)
{
    uint32_t magnitude = (value & 0x80000000U) != 0 ? ~value : value;
    unsigned int shift = value == 0
        ? 0U
        : count_leading_zeros32(magnitude) - 1U;
    uint16_t result = (uint16_t)((value << shift) >> 16);
    *result_exponent = result == 0 ? 0 : exponent - (int32_t)shift;
    return result;
}

uint16_t vocoder_normalized_add(
    int16_t left,
    int32_t left_exponent,
    int16_t right,
    int32_t right_exponent,
    int32_t *result_exponent)
{
    if (left == 0 || right_exponent - left_exponent > 31) {
        *result_exponent = right == 0 ? 0 : right_exponent;
        return (uint16_t)right;
    }
    if (right == 0 || left_exponent - right_exponent > 31) {
        *result_exponent = left_exponent;
        return (uint16_t)left;
    }

    int32_t exponent =
        (left_exponent > right_exponent ? left_exponent : right_exponent) + 1;
    uint32_t left_aligned = align_q15(
        left, (unsigned int)(exponent - left_exponent));
    uint32_t right_aligned = align_q15(
        right, (unsigned int)(exponent - right_exponent));
    return normalize_q31(
        left_aligned + right_aligned, exponent, result_exponent);
}

uint16_t vocoder_normalized_subtract(
    int16_t left,
    int32_t left_exponent,
    int16_t right,
    int32_t right_exponent,
    int32_t *result_exponent)
{
    if (left == 0 || right_exponent - left_exponent > 31) {
        *result_exponent = right == 0 ? 0 : right_exponent;
        return (uint16_t)right;
    }
    if (right == 0 || left_exponent - right_exponent > 31) {
        *result_exponent = left_exponent;
        return left == INT16_MIN
            ? (uint16_t)INT16_MAX
            : (uint16_t)(int16_t)-left;
    }

    int32_t exponent =
        (left_exponent > right_exponent ? left_exponent : right_exponent) + 1;
    uint32_t left_aligned = align_q15(
        left, (unsigned int)(exponent - left_exponent));
    uint32_t right_aligned = align_q15(
        right, (unsigned int)(exponent - right_exponent));
    return normalize_q31(
        right_aligned - left_aligned, exponent, result_exponent);
}

static uint32_t q15_polynomial_step(
    int16_t coefficient,
    int16_t accumulator,
    int16_t argument)
{
    int32_t product = (int32_t)accumulator * (int32_t)argument;
    return ((uint32_t)(int32_t)coefficient << 16) +
        ((uint32_t)product << 1);
}

uint32_t vocoder_normalized_sqrt(uint32_t mantissa, int32_t *exponent)
{
    static const int16_t coefficients[4] = {
        (int16_t)0xe676, (int16_t)0x714b,
        (int16_t)0x283f, (int16_t)0x5a82
    };

    if (mantissa == 0) {
        return 0;
    }

    uint32_t magnitude = (mantissa & 0x80000000U) != 0
        ? ~mantissa
        : mantissa;
    unsigned int normalization_shift =
        count_leading_zeros32(magnitude) - 1U;
    int16_t argument =
        (int16_t)((mantissa << normalization_shift) >> 16);
    int32_t adjusted_exponent =
        *exponent - (int32_t)normalization_shift;

    int32_t first_product =
        (int32_t)coefficients[0] * (int32_t)argument;
    uint32_t accumulator =
        ((uint32_t)(int32_t)coefficients[1] << 16) +
        ((uint32_t)first_product << 1);
    accumulator += 0x8000U;

    accumulator = q15_polynomial_step(
        coefficients[2], (int16_t)(accumulator >> 16), argument);
    accumulator = (accumulator + 0x8000U) & 0xffff0000U;

    if ((adjusted_exponent & 1) != 0) {
        int32_t product =
            (int32_t)(int16_t)(accumulator >> 16) *
            (int32_t)coefficients[3];
        accumulator = (uint32_t)product << 1;
    }
    accumulator = (accumulator + 0x8000U) & 0xffff0000U;

    *exponent = (int32_t)arithmetic_shift_right(
        (uint32_t)(adjusted_exponent + 1), 1);
    return accumulator;
}

static int32_t signed_high_half(uint32_t value)
{
    return (int32_t)(int16_t)(value >> 16);
}

static int32_t signed_low_half(uint32_t value)
{
    return (int32_t)(int16_t)value;
}

static int32_t smulbt(uint32_t left, uint32_t right)
{
    return signed_low_half(left) * signed_high_half(right);
}

static int32_t smultb(uint32_t left, uint32_t right)
{
    return signed_high_half(left) * signed_low_half(right);
}

static int32_t smultt(uint32_t left, uint32_t right)
{
    return signed_high_half(left) * signed_high_half(right);
}

int32_t vocoder_log2_q15(uint16_t mantissa, int32_t exponent)
{
    static const int16_t coefficients[6] = {
        (int16_t)0x5a82, (int16_t)0xa3ac, (int16_t)0x570d,
        (int16_t)0xa3ac, (int16_t)0x414a, (int16_t)0xc000
    };

    uint32_t normalized = (uint32_t)mantissa << 16;
    unsigned int shift = normalized == 0
        ? 0U
        : count_leading_zeros32(normalized) - 1U;
    normalized <<= shift;
    uint32_t argument =
        normalized - ((uint32_t)(int32_t)coefficients[0] << 16);

    uint32_t accumulator =
        0x8000U + ((uint32_t)smulbt(
            (uint32_t)(uint16_t)coefficients[1], argument) << 1);
    accumulator = (uint32_t)(int32_t)coefficients[2] +
        (accumulator >> 16);

    accumulator = 0x8000U +
        ((uint32_t)smulbt(accumulator, argument) << 1);
    accumulator = (uint32_t)(int32_t)coefficients[3] +
        (accumulator >> 16);

    accumulator = 0x8000U +
        ((uint32_t)smulbt(accumulator, argument) << 1);
    accumulator &= 0xffff0000U;
    accumulator =
        ((uint32_t)(int32_t)coefficients[4] << 16) +
        arithmetic_shift_right(accumulator, 1);

    uint32_t term = 0x8000U +
        ((uint32_t)smultt(accumulator, argument) << 1);
    term >>= 16;
    uint32_t polynomial =
        ((uint32_t)(int32_t)coefficients[5] << 16) +
        (term << 18);
    uint32_t integral =
        (uint32_t)(exponent - (int32_t)shift) << 16;
    return (int32_t)(integral +
        arithmetic_shift_right(polynomial, 15));
}

uint32_t vocoder_exp2_q16(int32_t value, int32_t *exponent)
{
    static const int16_t coefficients[6] = {
        (int16_t)0x5762, (int16_t)0x4ecb, (int16_t)0x71ac,
        (int16_t)0x7aff, (int16_t)0x58b9, (int16_t)0x4000
    };

    uint32_t bits = (uint32_t)value;
    uint32_t argument = (bits & 0xffffU) >> 1;

    int32_t product = (int32_t)coefficients[0] * (int32_t)argument;
    uint32_t temporary = ((uint32_t)product << 1) + 0x8000U;
    int32_t rounded = (int32_t)arithmetic_shift_right(temporary, 16);

    uint32_t accumulator =
        ((uint32_t)(int32_t)coefficients[1] << 16) +
        ((uint32_t)rounded << 13);
    product = smultb(accumulator, argument);
    temporary = ((uint32_t)product << 1) + 0x8000U;
    rounded = (int32_t)arithmetic_shift_right(temporary, 16);

    accumulator =
        ((uint32_t)(int32_t)coefficients[2] << 15) +
        ((uint32_t)rounded << 13);
    product = smultb(accumulator, argument);
    temporary = ((uint32_t)product << 1) + 0x8000U;
    rounded = (int32_t)arithmetic_shift_right(temporary, 16);

    accumulator =
        ((uint32_t)(int32_t)coefficients[3] << 15) +
        ((uint32_t)rounded << 14);
    product = smultb(accumulator, argument);
    temporary = ((uint32_t)product << 1) + 0x8000U;
    rounded = (int32_t)arithmetic_shift_right(temporary, 16);

    accumulator =
        ((uint32_t)(int32_t)coefficients[4] << 16) +
        ((uint32_t)rounded << 15);
    product = smultb(accumulator, argument);
    temporary = ((uint32_t)product << 1) + 0x8000U;
    rounded = (int32_t)arithmetic_shift_right(temporary, 16);

    *exponent = (int32_t)(int16_t)(bits >> 16) + 1;
    return ((uint32_t)(int32_t)coefficients[5] << 16) +
        ((uint32_t)rounded << 15);
}

static uint32_t logical_shift_left32(uint32_t value, unsigned int shift)
{
    return shift >= 32U ? 0U : value << shift;
}

int16_t vocoder_log2_scaled(
    uint16_t mantissa,
    int32_t exponent,
    int32_t output_q)
{
    uint32_t value = (uint32_t)vocoder_log2_q15(mantissa, exponent);
    int32_t shift = 15 - output_q;
    if (shift >= 0) {
        value = logical_shift_left32(
            value, (unsigned int)(uint8_t)shift);
    }
    else {
        value = arithmetic_shift_right(
            value, (unsigned int)(uint8_t)-shift);
    }
    value += 0x8000U;
    return (int16_t)(arithmetic_shift_right(value, 16) & 0xffffU);
}

int16_t vocoder_exp2_scaled(
    int16_t value,
    int32_t input_q,
    int32_t output_exponent)
{
    uint32_t converted = (uint32_t)(uint16_t)value << 16;
    if (input_q > 14) {
        converted = logical_shift_left32(
            converted, (unsigned int)(uint8_t)(input_q - 15));
    }
    else {
        converted = arithmetic_shift_right(
            converted, (unsigned int)(uint8_t)(15 - input_q));
    }

    int32_t generated_exponent;
    uint32_t mantissa =
        vocoder_exp2_q16((int32_t)converted, &generated_exponent);
    int32_t shift = generated_exponent - output_exponent;
    if (shift >= 0) {
        mantissa = logical_shift_left32(
            mantissa, (unsigned int)(uint8_t)shift);
    }
    else {
        mantissa = arithmetic_shift_right(
            mantissa, (unsigned int)(uint8_t)-shift);
    }
    mantissa += 0x8000U;
    return (int16_t)(arithmetic_shift_right(mantissa, 16) & 0xffffU);
}

int16_t vocoder_sqrt_scaled(
    uint32_t mantissa,
    int32_t exponent,
    int32_t output_exponent)
{
    int32_t generated_exponent = exponent;
    uint32_t result =
        vocoder_normalized_sqrt(mantissa, &generated_exponent);
    int32_t shift = generated_exponent - output_exponent;
    if (shift >= 0) {
        result = logical_shift_left32(
            result, (unsigned int)(uint8_t)shift);
    }
    else {
        result = arithmetic_shift_right(
            result, (unsigned int)(uint8_t)-shift);
    }
    result += 0x8000U;
    return (int16_t)(arithmetic_shift_right(result, 16) & 0xffffU);
}

int32_t vocoder_average_i16(const int16_t *values, int32_t count)
{
    uint32_t sum = 0;
    for (int32_t index = 0; index < count; ++index) {
        int16_t value = values[index];
        if (value >= 0) {
            sum += (uint32_t)(uint16_t)value;
        }
        else {
            sum -= (uint32_t)(-(int32_t)value);
        }
    }
    return vocoder_fixed_divide((int32_t)sum, (int16_t)count);
}

int16_t vocoder_harmonic_count(int32_t pitch)
{
    int32_t count =
        (int16_t)vocoder_fixed_divide(0x3b39c, (int16_t)pitch);
    uint32_t product =
        (uint32_t)count * 2U * (uint32_t)pitch + (uint32_t)pitch;

    /* The firmware uses a signed comparison after the wrapping MLA. */
    if ((product & 0x80000000U) == 0U && product >= 0x80000U) {
        uint32_t quotient =
            (uint32_t)vocoder_fixed_divide(0x80000, (int16_t)pitch);
        uint32_t adjusted = (quotient << 16) - 0x10000U;
        count = (int32_t)arithmetic_shift_right(adjusted, 17);
    }

    if (count > 56) {
        count = 56;
    }
    if (count < 9) {
        count = 9;
    }
    return (int16_t)count;
}

uint32_t vocoder_resample_step_q16(
    int32_t output_pitch,
    int32_t input_pitch)
{
    if (output_pitch == input_pitch) {
        return 0x10000U;
    }

    uint32_t scaled_input = (uint32_t)input_pitch << 16;
    uint32_t magnitude = (scaled_input & 0x80000000U) != 0U
        ? ~scaled_input
        : scaled_input;
    int32_t normalization_shift = magnitude == 0U
        ? 0
        : (int32_t)count_leading_zeros32(magnitude) - 1;
    uint32_t normalized_bits = logical_shift_left32(
        scaled_input, (unsigned int)(uint8_t)normalization_shift);
    int32_t normalized_input =
        (int32_t)arithmetic_shift_right(normalized_bits, 16);
    uint32_t scaled_output = (uint32_t)output_pitch << 16;

    int32_t quotient;
    int32_t result_shift;
    if (output_pitch < normalized_input) {
        quotient = vocoder_fixed_divide_half(
            (int32_t)scaled_output, (int16_t)normalized_input);
        result_shift = normalization_shift;
    }
    else {
        quotient = vocoder_fixed_divide_half(
            (int32_t)arithmetic_shift_right(scaled_output, 1),
            (int16_t)normalized_input);
        result_shift = normalization_shift + 1;
    }

    uint32_t result = (uint32_t)quotient << 16;
    if (result_shift > 14) {
        return logical_shift_left32(
            result, (unsigned int)(uint8_t)(result_shift - 15));
    }
    return arithmetic_shift_right(
        result, (unsigned int)(uint8_t)(15 - result_shift));
}

static int32_t saturate_i32(int64_t value)
{
    if (value > INT32_MAX) {
        return INT32_MAX;
    }
    if (value < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static int32_t arm_qdadd(int32_t left, int32_t doubled_right)
{
    int32_t doubled =
        saturate_i32((int64_t)doubled_right * 2);
    return saturate_i32((int64_t)left + doubled);
}

void vocoder_resample_spectrum(
    int16_t *output,
    int32_t output_pitch,
    int32_t input_pitch,
    const int16_t input[56],
    int32_t output_count)
{
    int16_t extended[60];
    extended[0] = input[0];
    for (int32_t index = 1; index < 57; ++index) {
        extended[index] = input[index - 1];
    }
    for (int32_t index = 57; index < 60; ++index) {
        extended[index] = input[55];
    }

    uint32_t step =
        vocoder_resample_step_q16(output_pitch, input_pitch);
    uint32_t position = step;
    for (int32_t output_index = 0;
         output_index < output_count;
         ++output_index) {
        uint32_t fraction = (position >> 1) & 0x7fffU;
        int32_t inverse_fraction = 0x7fff - (int32_t)fraction;
        int32_t input_index =
            (int32_t)arithmetic_shift_right(position, 16);
        if (input_index > 59) {
            input_index = 59;
        }
        int32_t next_index = input_index + 1;
        if (next_index == 60) {
            next_index = 59;
        }

        int32_t left_product =
            (int32_t)extended[input_index] * inverse_fraction;
        uint32_t doubled_left_bits =
            (uint32_t)left_product << 1;
        int32_t right_product =
            (int32_t)extended[next_index] * (int32_t)fraction;
        int32_t interpolated = arm_qdadd(
            (int32_t)doubled_left_bits, right_product);
        output[output_index] =
            (int16_t)(arithmetic_shift_right(
                (uint32_t)interpolated, 16) & 0xffffU);
        position += step;
    }
}

void vocoder_decode_pitch(
    int16_t *pitch,
    int16_t *harmonic_count,
    int32_t encoded_pitch,
    int32_t allow_high_pitch)
{
    int32_t shift = -4;
    while (encoded_pitch < -0x1000) {
        encoded_pitch =
            (int32_t)(int16_t)(encoded_pitch + 0x1000);
        shift++;
    }

    int32_t coefficient = (encoded_pitch * 315) / 0x1000;
    if (coefficient < 0 && (coefficient * 0x1000) !=
            encoded_pitch * 315) {
        coefficient--;
    }
    coefficient += 0x71b;

    int32_t product =
        (int16_t)encoded_pitch * (int16_t)coefficient;
    coefficient = (int32_t)arithmetic_shift_right(
        (uint32_t)product, 12);
    coefficient = (int16_t)(coefficient + 0x1ec0);

    product = (int16_t)encoded_pitch * (int16_t)coefficient;
    coefficient = (int32_t)arithmetic_shift_right(
        (uint32_t)product, 12);
    coefficient = (int16_t)(coefficient + 0x58b9);

    product = (int16_t)encoded_pitch * (int16_t)coefficient;
    uint16_t generated_bits =
        (uint16_t)(((uint32_t)product >> 12) & 0xffffU);
    int16_t generated = (int16_t)(generated_bits ^ 0x8000U);
    uint32_t shifted = arithmetic_shift_right(
        (uint32_t)(int32_t)generated,
        (unsigned int)(uint8_t)shift);
    int16_t result = (int16_t)(shifted & 0xffffU);

    if (result <= 0x1078) {
        result = 0x1079;
    }
    else if (result > 0x6bca && allow_high_pitch == 0) {
        result = 0x6bca;
    }

    *pitch = result;
    *harmonic_count = vocoder_harmonic_count(result);
}

int16_t vocoder_log_amplitude_clamped(
    uint16_t mantissa,
    int32_t exponent,
    int16_t bias)
{
    if (exponent < -59 || mantissa == 0U) {
        return -0x1400;
    }

    int16_t logarithm =
        vocoder_log2_scaled(mantissa, exponent + 30, 5);
    int16_t result = (int16_t)(
        (uint16_t)logarithm + (uint16_t)bias);
    if (result > 0x7800) {
        return 0x7800;
    }
    if (result < -0x1400) {
        return -0x1400;
    }
    return result;
}

int32_t vocoder_reciprocal_vector(
    int16_t *output,
    const int16_t *input,
    int32_t input_q,
    int32_t scale_shift,
    int32_t count)
{
    uint32_t numerator = 0x8000U;
    if (scale_shift >= 0) {
        numerator = logical_shift_left32(
            numerator, (unsigned int)(uint8_t)scale_shift);
    }
    else {
        numerator = arithmetic_shift_right(
            numerator,
            (unsigned int)(uint8_t)(0U - (uint32_t)scale_shift));
    }

    for (int32_t index = 0; index < count; ++index) {
        int16_t value = input[index];
        output[index] = value == 0
            ? INT16_MAX
            : (int16_t)vocoder_fixed_divide_half(
                (int32_t)numerator, value);
    }
    return 16 - scale_shift - input_q;
}

void vocoder_decode_pitch_parameters(
    int32_t mode,
    int16_t *pitch,
    int16_t *harmonic_count,
    int32_t coefficient_index,
    int32_t divisor_shift,
    int16_t allow_high_pitch)
{
    if (mode != 1) {
        *pitch = 0x4027;
        *harmonic_count = 15;
        return;
    }

    uint32_t numerator =
        (uint32_t)coefficient_index * 0x54a4U + 0x2a52U;
    uint32_t denominator_bits = logical_shift_left32(
        0x78U,
        (unsigned int)(uint8_t)(divisor_shift - 6));
    int32_t quotient = vocoder_fixed_divide(
        (int32_t)numerator, (int16_t)denominator_bits);
    uint32_t encoded_bits =
        0xbb030000U - ((uint32_t)quotient << 16);
    int32_t encoded_pitch =
        (int32_t)arithmetic_shift_right(encoded_bits, 16);
    vocoder_decode_pitch(
        pitch, harmonic_count, encoded_pitch, allow_high_pitch);
}

int16_t vocoder_one_minus_normalized_ratio(
    int16_t numerator,
    int32_t numerator_exponent,
    int16_t denominator,
    int32_t denominator_exponent)
{
    if (denominator <= 0) {
        return INT16_MAX;
    }

    int32_t result_exponent;
    int16_t ratio = vocoder_normalized_divide(
        (int32_t)((uint32_t)(uint16_t)numerator << 16),
        numerator_exponent,
        denominator,
        denominator_exponent,
        &result_exponent);
    if (result_exponent > 0) {
        return 0;
    }

    uint32_t ratio_bits = (uint32_t)(uint16_t)ratio << 16;
    if (result_exponent != 0) {
        ratio_bits = arithmetic_shift_right(
            ratio_bits,
            (unsigned int)(uint8_t)(
                0U - (uint32_t)result_exponent));
    }
    if (ratio_bits == 0x80000000U) {
        return -2;
    }
    return (int16_t)((0x7fff0000U - ratio_bits) >> 16);
}

void vocoder_quantize_log_amplitudes(
    int16_t output[8],
    const int16_t input[8],
    int32_t exponent,
    int32_t lower_last_four)
{
    for (int32_t index = 0; index < 8; ++index) {
        int16_t value = input[index];
        if (value == 0) {
            output[index] = INT16_MAX;
            continue;
        }

        int16_t logarithm =
            vocoder_log2_scaled((uint16_t)value, exponent, 6);
        uint32_t base =
            lower_last_four != 0 && index >= 4
            ? 0xf95d0000U
            : 0xfb5d0000U;
        uint32_t adjusted =
            base - ((uint32_t)(uint16_t)logarithm << 16);
        if ((adjusted & 0x80000000U) != 0U) {
            output[index] = 0;
        }
        else if (adjusted <= 0x03ffffffU) {
            output[index] = (int16_t)(
                (adjusted & 0x07ffffffU) >> 11);
        }
        else {
            output[index] = INT16_MAX;
        }
    }
}

int32_t vocoder_update_gain_if_above_threshold(
    uint16_t *gain,
    int32_t offset,
    int32_t threshold_mantissa,
    int32_t threshold_exponent,
    int16_t limit,
    int16_t threshold,
    int16_t multiplier,
    int32_t subtraction)
{
    uint32_t threshold_bits =
        (uint32_t)threshold_mantissa << 16;
    if (threshold_exponent < 5) {
        threshold_bits = arithmetic_shift_right(
            threshold_bits,
            (unsigned int)(uint8_t)(5 - threshold_exponent));
    }
    else {
        threshold_bits = logical_shift_left32(
            threshold_bits,
            (unsigned int)(uint8_t)(threshold_exponent - 5));
    }
    int16_t scaled_threshold = (int16_t)(
        arithmetic_shift_right(threshold_bits, 16) & 0xffffU);
    if (scaled_threshold <= threshold) {
        return 1;
    }

    uint32_t difference_bits =
        ((uint32_t)(uint16_t)limit << 16) -
        ((uint32_t)offset << 16) -
        ((uint32_t)subtraction << 16);
    if (difference_bits == 0U ||
            (difference_bits & 0x80000000U) != 0U) {
        return 1;
    }
    int32_t difference = (int32_t)(
        arithmetic_shift_right(difference_bits, 16));
    if (difference >= 512) {
        difference = 512;
    }

    uint32_t exponential_input =
        (uint32_t)(int32_t)multiplier *
        (uint32_t)difference * 2U;
    uint32_t magnitude =
        (exponential_input & 0x80000000U) != 0U
        ? ~exponential_input
        : exponential_input;
    int32_t normalization_shift = magnitude == 0U
        ? 0
        : (int32_t)count_leading_zeros32(magnitude) - 1;
    uint32_t normalized_bits = logical_shift_left32(
        exponential_input,
        (unsigned int)(uint8_t)normalization_shift);
    int16_t normalized = (int16_t)(
        arithmetic_shift_right(normalized_bits, 16) & 0xffffU);
    int16_t scale = vocoder_exp2_scaled(
        normalized, 8 - normalization_shift, 3);

    uint32_t product =
        (uint32_t)(
            (int32_t)scale * (int32_t)(int16_t)*gain);
    uint32_t doubled = product << 1;
    uint32_t times_sixteen = product << 4;
    uint32_t divided = arithmetic_shift_right(times_sixteen, 3);
    if (doubled != divided) {
        *gain = (doubled & 0x80000000U) != 0U
            ? 0x8000U
            : 0x7fffU;
    }
    else {
        *gain = (uint16_t)(times_sixteen >> 16);
    }
    return 0;
}

int16_t vocoder_sum_normalized_bands(
    int32_t *result_exponent,
    const int32_t values[128],
    int32_t numerator,
    int32_t spacing,
    int32_t *initial_phase)
{
    int16_t quotient = (int16_t)vocoder_fixed_divide(
        numerator, (int16_t)spacing);
    uint32_t scaled_bits = (uint32_t)spacing << 9;
    int32_t scaled_eighth = (int32_t)arithmetic_shift_right(
        scaled_bits, 3);
    int32_t scaled_thirty_second =
        (int32_t)arithmetic_shift_right(scaled_bits, 5);
    uint32_t phase_bits =
        (uint32_t)scaled_eighth *
            (uint32_t)(int32_t)(quotient - 1) +
        (uint32_t)scaled_eighth +
        (0x10000U - (uint32_t)scaled_thirty_second);
    int32_t phase = (int32_t)phase_bits;
    int32_t step = (int32_t)arithmetic_shift_right(
        scaled_bits, 4);
    int32_t start = (int32_t)arithmetic_shift_right(
        phase_bits, 16);
    uint32_t end_bits = phase_bits + (uint32_t)step;
    *initial_phase = phase;

    int64_t sum = 0;
    int32_t end = (int32_t)arithmetic_shift_right(
        end_bits, 16);
    int32_t width = end - start;
    do {
        for (int32_t index = 0; index < width; ++index) {
            sum += values[start + index];
        }
        uint32_t next_start_bits = end_bits + (uint32_t)step;
        end_bits += (uint32_t)step * 2U;
        start = (int32_t)arithmetic_shift_right(
            next_start_bits, 16);
        end = (int32_t)arithmetic_shift_right(end_bits, 16);
        width = end - start;
    } while (end < 129);

    if (start < 128) {
        for (int32_t index = start; index < 128; ++index) {
            sum += values[index];
        }
    }

    if (sum == 0) {
        *result_exponent = 0;
        return 0;
    }

    uint64_t sum_bits = (uint64_t)sum;
    uint64_t magnitude = sum < 0 ? ~sum_bits : sum_bits;
    uint32_t high = (uint32_t)(magnitude >> 32);
    unsigned int leading = high == 0U
        ? count_leading_zeros32((uint32_t)magnitude) + 32U
        : count_leading_zeros32(high);
    int32_t shift = (int32_t)leading - 33;
    *result_exponent = -shift;
    uint64_t normalized = shift >= 0
        ? sum_bits << (unsigned int)shift
        : sum_bits >> (unsigned int)(-shift);
    return (int16_t)((normalized >> 16) & 0xffffU);
}

int16_t vocoder_quantize_pitch_index(
    int16_t *pitch,
    int16_t *harmonic_count,
    int32_t mode,
    int32_t table_bits,
    int16_t allow_high_pitch)
{
    if (mode == 2) {
        *pitch = 0x4027;
        *harmonic_count = 15;
        uint32_t fallback = logical_shift_left32(
            62U, (unsigned int)(uint8_t)(table_bits - 6));
        return (int16_t)(fallback & 0xffffU);
    }
    if (mode != 1) {
        return 0;
    }

    uint32_t level_count_bits = logical_shift_left32(
        60U, (unsigned int)(uint8_t)(table_bits - 6));
    int32_t level_count = (int32_t)level_count_bits;
    uint32_t doubled_levels = level_count_bits << 1;

    int32_t logarithm =
        vocoder_log2_q15((uint16_t)*pitch, -4);
    int16_t transformed;
    if (logarithm == INT32_MIN) {
        transformed = -0x44fe;
    }
    else {
        uint32_t negated = 0U - (uint32_t)logarithm;
        int16_t extracted =
            (int16_t)((negated >> 4) & 0xffffU);
        transformed = (int16_t)(
            (uint16_t)extracted - 0x44fdU);
    }

    uint32_t first_numerator =
        (uint32_t)(int32_t)transformed * doubled_levels;
    int16_t index = (int16_t)vocoder_fixed_divide(
        (int32_t)first_numerator, 0x54a4);
    uint32_t second_numerator;
    if (index < 0) {
        index = 0;
        second_numerator = 0x2a52U;
    }
    else {
        if ((int32_t)index >= level_count) {
            index = (int16_t)(level_count - 1);
        }
        second_numerator =
            (uint32_t)(int32_t)index * 0x54a4U + 0x2a52U;
    }

    int32_t quotient = vocoder_fixed_divide(
        (int32_t)second_numerator, (int16_t)doubled_levels);
    uint32_t encoded_bits =
        0xbb030000U - ((uint32_t)quotient << 16);
    int32_t encoded_pitch = (int32_t)arithmetic_shift_right(
        encoded_bits, 16);
    vocoder_decode_pitch(
        pitch, harmonic_count, encoded_pitch, allow_high_pitch);
    return index;
}

int32_t vocoder_adjust_harmonic_index(
    uint16_t *index,
    int32_t harmonic_count,
    int32_t adjustment,
    uint32_t maximum_index,
    int32_t update_index)
{
    uint32_t count_bits = (uint32_t)harmonic_count << 16;
    uint32_t count_magnitude =
        (count_bits & 0x80000000U) != 0U
        ? ~count_bits
        : count_bits;
    int32_t count_shift = count_magnitude == 0U
        ? 0
        : (int32_t)count_leading_zeros32(count_magnitude) - 1;
    uint32_t normalized_count_bits = logical_shift_left32(
        count_bits, (unsigned int)(uint8_t)count_shift);
    int16_t normalized_count = (int16_t)(
        arithmetic_shift_right(normalized_count_bits, 16) &
        0xffffU);

    int32_t ratio_exponent;
    int16_t ratio = vocoder_normalized_divide(
        0x41000000, 8, normalized_count,
        15 - count_shift, &ratio_exponent);
    uint32_t selected_index = maximum_index;

    if (update_index != 0) {
        uint32_t adjusted_bits =
            arithmetic_shift_right(
                (uint32_t)adjustment << 16, 1) +
            0x20800000U;
        uint32_t adjusted_magnitude =
            (adjusted_bits & 0x80000000U) != 0U
            ? ~adjusted_bits
            : adjusted_bits;
        int32_t adjusted_shift = adjusted_magnitude == 0U
            ? 0
            : (int32_t)count_leading_zeros32(
                adjusted_magnitude) - 1;
        uint32_t normalized_adjusted = logical_shift_left32(
            adjusted_bits,
            (unsigned int)(uint8_t)adjusted_shift);

        int32_t bound_exponent;
        int16_t bound_mantissa = vocoder_normalized_divide(
            (int32_t)normalized_adjusted,
            8 - adjusted_shift,
            ratio, ratio_exponent,
            &bound_exponent);
        uint32_t bound_bits =
            (uint32_t)(uint16_t)bound_mantissa << 16;
        int32_t bound;
        if (bound_exponent < 15) {
            uint32_t shifted = arithmetic_shift_right(
                bound_bits,
                (unsigned int)(uint8_t)(15 - bound_exponent));
            bound = (int32_t)arithmetic_shift_right(shifted, 16);
        }
        else {
            uint32_t shifted = logical_shift_left32(
                bound_bits,
                (unsigned int)(uint8_t)(bound_exponent - 15));
            bound = (int32_t)arithmetic_shift_right(shifted, 16);
        }

        int32_t candidate = harmonic_count <= bound
            ? harmonic_count - 1
            : (bound < 0 ? 0 : bound);
        if (candidate <= (int32_t)maximum_index) {
            selected_index =
                (uint32_t)(candidate < 0 ? 0 : candidate);
        }
        *index = (uint16_t)selected_index;
    }
    else {
        selected_index = *index;
    }

    int32_t exponent_shift = -7 - ratio_exponent;
    uint32_t base;
    if (exponent_shift >= 0) {
        base = logical_shift_left32(
            0xbf000000U,
            (unsigned int)(uint8_t)exponent_shift);
    }
    else {
        base = arithmetic_shift_right(
            0xbf000000U,
            (unsigned int)(uint8_t)(ratio_exponent + 7));
    }
    uint32_t odd_index =
        (selected_index << 1) + 1U;
    uint32_t combined = base +
        (uint32_t)(int32_t)ratio * odd_index * 2U;
    if (ratio_exponent < -7) {
        combined = arithmetic_shift_right(
            combined,
            (unsigned int)(uint8_t)(-7 - ratio_exponent));
    }
    else {
        combined = logical_shift_left32(
            combined,
            (unsigned int)(uint8_t)(ratio_exponent + 7));
    }
    combined += 0x8000U;
    return (int32_t)arithmetic_shift_right(combined, 16);
}
