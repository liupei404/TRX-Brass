#include "portable_frame.h"

#include <string.h>

void *vocoder_memset_bytes(
    void *destination,
    size_t count,
    int32_t value)
{
    uint8_t *bytes = (uint8_t *)destination;
    uint8_t byte = (uint8_t)value;
    while (count != 0U) {
        --count;
        bytes[count] = byte;
    }
    return destination;
}

void *vocoder_fill_bytes(
    void *destination,
    int32_t value,
    size_t count)
{
    (void)vocoder_memset_bytes(destination, count, value);
    return destination;
}

void vocoder_copy_i16(
    int16_t *destination,
    const int16_t *source,
    int32_t count)
{
    for (int32_t index = 0; index < count; ++index) {
        destination[index] = source[index];
    }
}

int16_t *vocoder_fill_i16(
    int16_t *destination,
    int32_t value,
    int32_t count)
{
    for (int32_t index = 0; index < count; ++index) {
        destination[index] = (int16_t)value;
    }
    return destination + (count > 0 ? count : 0);
}

#include "portable_fixed.h"
#include "portable_codebooks.inc"

static unsigned int frame_count_leading_zeros(uint32_t value);
static int32_t frame_fixed_normalization_shift(uint32_t value);

static uint32_t frame_arithmetic_shift_right(
    uint32_t value,
    unsigned int shift)
{
    shift &= 0xffU;
    if (shift == 0U) {
        return value;
    }
    if (shift >= 32U) {
        return (value & 0x80000000U) != 0U
            ? UINT32_MAX
            : 0U;
    }
    uint32_t shifted = value >> shift;
    if ((value & 0x80000000U) != 0U) {
        shifted |= UINT32_MAX << (32U - shift);
    }
    return shifted;
}

static uint32_t frame_shift_left(
    uint32_t value,
    unsigned int shift)
{
    shift &= 0xffU;
    return shift < 32U ? value << shift : 0U;
}

void vocoder_expand_log_spectrum(
    int16_t spectrum[56],
    int32_t *exponent,
    int32_t harmonic_count)
{
    int32_t maximum = INT16_MIN;
    for (int32_t index = 0; index < harmonic_count; ++index) {
        int32_t value = spectrum[index];
        if (value < -0x77ff) {
            value = -0x77ff;
        }
        if (value > 0x77ff) {
            value = 0x77ff;
        }
        spectrum[index] = (int16_t)value;
        if (maximum < value) {
            maximum = value;
        }
    }

    uint32_t scaled = (uint32_t)maximum << 16;
    scaled = frame_arithmetic_shift_right(scaled, 11);
    scaled += 0x10000U;
    int32_t output_exponent = (int32_t)
        frame_arithmetic_shift_right(scaled, 16);

    for (int32_t index = 0; index < harmonic_count; ++index) {
        spectrum[index] = vocoder_exp2_scaled(
            spectrum[index], 4, output_exponent);
    }
    *exponent = output_exponent - 15;
    vocoder_fill_i16(
        spectrum + harmonic_count, 0, 56 - harmonic_count);
}

void vocoder_multiply_symmetric(
    int16_t *output,
    const int16_t *right,
    int32_t output_count,
    int32_t center_output,
    const int16_t *left,
    int32_t active_count)
{
    int32_t half = active_count >> 1;
    int16_t *forward;
    int16_t *backward;
    if (center_output == 0) {
        forward = output;
        backward = output + output_count - 1;
    }
    else {
        forward = output + center_output;
        backward = forward - 1;
    }

    const int16_t *right_forward = right + half;
    if ((active_count & 1) != 0) {
        uint32_t product =
            (uint32_t)((int32_t)left[half] *
                       (int32_t)right[half]);
        product = product * 2U + 0x8000U;
        *forward++ = (int16_t)(product >> 16);
        right_forward++;
    }

    const int16_t *right_backward =
        right + half - 1;
    const int16_t *left_backward =
        left + half - 1;
    for (int32_t index = 0; index < half; ++index) {
        uint32_t forward_product =
            (uint32_t)((int32_t)*left_backward *
                       (int32_t)*right_forward) +
            0x4000U;
        uint32_t backward_product =
            (uint32_t)((int32_t)*left_backward *
                       (int32_t)*right_backward) +
            0x4000U;
        *forward++ = (int16_t)frame_arithmetic_shift_right(
            forward_product, 15);
        *backward-- = (int16_t)frame_arithmetic_shift_right(
            backward_product, 15);
        right_forward++;
        right_backward--;
        left_backward--;
    }
    vocoder_fill_i16(
        forward, 0, (int16_t)(output_count - active_count));
}

void vocoder_shift_i16(
    int16_t *output,
    const int16_t *input,
    int32_t count,
    int32_t shift)
{
    if (shift == 0) {
        if (output != input) {
            vocoder_copy_i16(output, input, (int16_t)count);
        }
        return;
    }
    if (shift < 0) {
        unsigned int amount =
            (unsigned int)(0U - (uint32_t)shift) & 0xffU;
        for (int32_t index = 0; index < count; ++index) {
            output[index] = (int16_t)frame_arithmetic_shift_right(
                (uint32_t)(int32_t)input[index], amount);
        }
    }
    else {
        unsigned int amount = (unsigned int)shift & 0xffU;
        for (int32_t index = 0; index < count; ++index) {
            output[index] = (int16_t)frame_shift_left(
                (uint32_t)(int32_t)input[index], amount);
        }
    }
}

void vocoder_copy_frame_record(
    vocoder_frame_record *destination,
    const vocoder_frame_record *source)
{
    destination->state.header[0] = source->state.header[0];
    destination->state.header[1] = source->state.header[1];
    destination->state.header[2] = source->state.header[2];
    destination->state.header[3] = source->state.header[3];
    destination->state.header[4] = source->state.header[4];
    destination->state.header[5] = source->state.header[5];
    destination->state.header[6] = source->state.header[6];
    destination->state.header[7] = source->state.header[7];
    vocoder_copy_i16(
        destination->state.spectrum,
        source->state.spectrum,
        VOCODER_SPECTRUM_SIZE);
    destination->metadata[0] = 0;
    destination->metadata[1] = 0;
    destination->metadata[2] = source->metadata[2];
    destination->metadata[3] = source->metadata[3];
}

void vocoder_prepare_extended_frame(
    vocoder_extended_frame *destination,
    const vocoder_frame_state *source,
    int32_t tag)
{
    destination->state.header[3] = source->header[3];
    destination->state.header[0] = source->header[0];
    destination->state.header[1] = source->header[1];
    if (source->header[0] != 1U) {
        destination->state.header[4] = 0;
        destination->state.header[5] = 0;
        destination->working[5] = 0;
        destination->state.header[7] = 0;
        destination->working[3] = 0;
        return;
    }

    uint16_t count = source->header[2];
    destination->state.header[2] = count;
    destination->state.header[6] = source->header[6];
    destination->working[5] = (uint16_t)tag;
    destination->state.header[4] = source->header[4];
    destination->state.header[5] = source->header[5];
    destination->state.header[7] = source->header[7];
    vocoder_copy_i16(
        destination->state.spectrum,
        source->spectrum,
        (int16_t)count);
    vocoder_fill_i16(
        destination->state.spectrum + count,
        source->spectrum[count - 1U],
        (int16_t)(VOCODER_SPECTRUM_SIZE - count));
    destination->working[3] = 0;
}

void vocoder_expand_voicing_bands(
    int16_t output[128],
    const int16_t thresholds[8],
    const int16_t values[8],
    int32_t exponent)
{
    static const uint8_t boundaries[9] = {
        0, 13, 29, 45, 61, 77, 93, 109, 128
    };
    for (int32_t band = 0; band < 8; ++band) {
        uint32_t raw = (uint32_t)(int32_t)values[band] << 16;
        uint32_t magnitude =
            (raw & 0x80000000U) != 0U ? ~raw : raw;
        unsigned int normalization = 0;
        if (raw != 0U) {
            normalization =
                frame_count_leading_zeros(magnitude) - 1U;
        }

        int16_t fill = 0;
        if (frame_shift_left(raw, normalization) != 0U &&
                exponent - (int32_t)normalization >= -42 &&
                thresholds[band] >= 0x1999) {
            fill = 0x4000;
        }
        int32_t begin = boundaries[band];
        int32_t end = boundaries[band + 1];
        vocoder_fill_i16(output + begin, fill, end - begin);
    }
}

static void generate_voicing_flags_to(
    const vocoder_frame_state *state,
    int16_t active[VOCODER_SPECTRUM_SIZE])
{
    int16_t mode = (int16_t)state->header[0];
    if (mode == 2) {
        vocoder_fill_i16(active, 0, VOCODER_SPECTRUM_SIZE);
        return;
    }
    if (mode == 3) {
        vocoder_fill_i16(active, 0, VOCODER_SPECTRUM_SIZE);
        int16_t classification =
            vocoder_classify_code((int16_t)state->header[1]);
        if (classification == 0) {
            active[(int16_t)state->header[3]] = 1;
            return;
        }
        if (classification >= 0 && classification < 4) {
            uint16_t indices = state->header[3];
            active[indices >> 8] = 1;
            active[indices & 0xffU] = 1;
        }
        return;
    }
    if (mode != 1) {
        return;
    }

    uint16_t groups[16];
    uint32_t packed =
        (uint32_t)state->header[4] |
        (uint32_t)state->header[5] << 16;
    for (int index = 0; index < 16; ++index) {
        groups[index] = (uint16_t)(packed & 3U);
        packed >>= 2;
    }

    int32_t count = (int16_t)state->header[2];
    int32_t step = (int16_t)state->header[6] * 4;
    int32_t phase = step;
    for (int32_t index = 0; index < count; ++index) {
        int32_t group = phase >> 16;
        uint16_t combined = groups[15];
        uint16_t current = groups[15];
        if (group < 16) {
            current = groups[group];
            if (group != 15) {
                combined = groups[group + 1];
            }
        }
        combined = (uint16_t)(combined | current);
        if ((int16_t)combined > 2) {
            combined = 1;
        }
        active[index] = (int16_t)combined;
        phase += step;
    }
    vocoder_fill_i16(
        active + count, 0,
        VOCODER_SPECTRUM_SIZE - count);
}

void vocoder_generate_voicing_flags(vocoder_harmonic_state *state)
{
    generate_voicing_flags_to(
        (const vocoder_frame_state *)(const void *)state, state->active);
}

int32_t vocoder_track_sync_word(
    uint16_t *symbol,
    uint16_t history[18],
    int32_t sync_mode)
{
    static const uint8_t sync_patterns[2][17] = {
        {
            0x15, 0x14, 0x14, 0x15, 0x15, 0x14,
            0x14, 0x14, 0x15, 0x15, 0x16, 0x16,
            0x16, 0x16, 0x16, 0x15, 0x15
        },
        {
            0x15, 0x14, 0x14, 0x14, 0x14, 0x14,
            0x15, 0x15, 0x16, 0x16, 0x15, 0x15,
            0x16, 0x16, 0x16, 0x15, 0x15
        }
    };
    static const uint32_t correction_mask =
        0x3e83ea00U ^ 0x3c670ce3U;

    vocoder_copy_i16(
        (int16_t *)(history + 1),
        (const int16_t *)(history + 2), 16);
    history[17] = *symbol;

    if (*symbol == 0xffU) {
        history[0] = 0;
        return 0;
    }
    if ((int16_t)history[0] > 0 &&
            *symbol > 0x12U && *symbol < 0x17U) {
        unsigned int shift =
            (32U - (uint16_t)history[0]) & 0xffU;
        *symbol = (uint16_t)(
            0x15U -
            ((correction_mask >> shift) & 1U));
        history[0]--;
        return 1;
    }

    history[0] = 0;
    const uint8_t *pattern =
        sync_patterns[sync_mode == 1 ? 0 : 1];
    int mismatches = 0;
    for (int index = 0; index < 17; ++index) {
        if (history[index + 1] != pattern[index]) {
            mismatches++;
        }
    }
    if (mismatches < 2) {
        history[0] = 0x20;
    }
    return 0;
}

static int16_t frame_cosine_q15(unsigned int index)
{
    static const int16_t quarter[129] = {
        32767, 32766, 32758, 32746, 32729, 32706, 32679, 32647,
        32610, 32568, 32522, 32470, 32413, 32352, 32286, 32214,
        32138, 32058, 31972, 31881, 31786, 31686, 31581, 31471,
        31357, 31238, 31114, 30986, 30853, 30715, 30572, 30425,
        30274, 30118, 29957, 29792, 29622, 29448, 29269, 29086,
        28899, 28707, 28511, 28311, 28106, 27897, 27684, 27467,
        27246, 27020, 26791, 26557, 26320, 26078, 25833, 25583,
        25330, 25073, 24812, 24548, 24279, 24008, 23732, 23453,
        23170, 22884, 22595, 22302, 22006, 21706, 21403, 21097,
        20788, 20475, 20160, 19841, 19520, 19195, 18868, 18538,
        18205, 17869, 17531, 17190, 16846, 16500, 16151, 15800,
        15447, 15091, 14733, 14373, 14010, 13646, 13279, 12910,
        12540, 12167, 11793, 11417, 11039, 10660, 10279, 9896,
        9512, 9127, 8740, 8351, 7962, 7571, 7180, 6787,
        6393, 5998, 5602, 5205, 4808, 4410, 4011, 3612,
        3212, 2811, 2411, 2009, 1608, 1206, 804, 402,
        0
    };
    index &= 0x1ffU;
    if (index <= 128U) {
        return quarter[index];
    }
    if (index <= 256U) {
        return (int16_t)-quarter[256U - index];
    }
    if (index <= 384U) {
        return (int16_t)-quarter[index - 256U];
    }
    return quarter[512U - index];
}

void vocoder_generate_complex_phasors(
    int16_t *output,
    const int16_t *phase_words,
    uint32_t phase_bits,
    int32_t count)
{
    uint32_t nonnegative_bits = phase_bits &
        ~((uint32_t)((int32_t)phase_bits >> 31));
    uint32_t right_shift = nonnegative_bits - phase_bits;
    uint32_t mask = nonnegative_bits == 0U
        ? INT32_MAX
        : frame_shift_left(1U, 31U - nonnegative_bits) - 1U;
    mask = frame_arithmetic_shift_right(mask, 16U);

    for (int32_t item = 0; item < count; ++item) {
        uint32_t shifted = frame_arithmetic_shift_right(
            (uint32_t)(int32_t)phase_words[item * 2],
            right_shift);
        uint32_t phase = frame_shift_left(
            shifted & mask, nonnegative_bits);
        unsigned int table_index = (unsigned int)(
            frame_arithmetic_shift_right(phase, 6U) & 0x1ffU);
        int16_t fraction = (int16_t)(
            frame_shift_left(phase, 26U) >> 17);

        int16_t cosine0 = frame_cosine_q15(table_index);
        int16_t cosine1 = frame_cosine_q15(table_index + 1U);
        uint32_t packed_cosine =
            (uint32_t)(uint16_t)cosine0 |
            ((uint32_t)(uint16_t)cosine1 << 16);
        uint32_t cosine_accumulator = packed_cosine << 16;
        cosine_accumulator += (uint32_t)(
            (int32_t)cosine0 * (int32_t)fraction * -2);
        cosine_accumulator += (uint32_t)(
            (int32_t)cosine1 * (int32_t)fraction * 2);
        output[item * 2] = (int16_t)(
            frame_arithmetic_shift_right(
                cosine_accumulator, 16U));

        unsigned int sine_index =
            (table_index + 384U) & 0x1ffU;
        int16_t sine0 = frame_cosine_q15(sine_index);
        int16_t sine1 = frame_cosine_q15(sine_index + 1U);
        uint32_t packed_sine =
            (uint32_t)(uint16_t)sine0 |
            ((uint32_t)(uint16_t)sine1 << 16);
        uint32_t sine_accumulator = packed_sine << 16;
        sine_accumulator += (uint32_t)(
            (int32_t)sine0 * (int32_t)fraction * -2);
        sine_accumulator += (uint32_t)(
            (int32_t)sine1 * (int32_t)fraction * 2);
        output[item * 2 + 1] = (int16_t)(
            frame_arithmetic_shift_right(
                sine_accumulator, 16U));
    }
}

void vocoder_polyphase_filter_16(
    int16_t *output,
    const int16_t *input,
    int32_t output_count)
{
    static const int16_t coefficients[4] = {
        -4456, 3034, 19608, 29164
    };

    for (int32_t phase = 0; phase < 16; ++phase) {
        int16_t *phase_output = output + phase * 11;
        const int16_t *cursor = input + phase;
        for (int32_t item = 0; item < output_count; item += 2) {
            uint32_t even = 0x8000U;
            even += (uint32_t)(
                (int32_t)cursor[0] * coefficients[0]);
            even += (uint32_t)(
                (int32_t)cursor[16] * coefficients[1]);
            even += (uint32_t)(
                (int32_t)cursor[32] * coefficients[2]);
            even += (uint32_t)(
                (int32_t)cursor[48] * coefficients[3]);
            even += (uint32_t)(
                (int32_t)cursor[64] * coefficients[2]);
            even += (uint32_t)(
                (int32_t)cursor[80] * coefficients[1]);
            even += (uint32_t)(
                (int32_t)cursor[96] * coefficients[0]);
            phase_output[item] = (int16_t)
                frame_arithmetic_shift_right(even, 16U);

            uint32_t odd = 0x8000U;
            odd += (uint32_t)(
                (int32_t)cursor[32] * coefficients[0]);
            odd += (uint32_t)(
                (int32_t)cursor[48] * coefficients[1]);
            odd += (uint32_t)(
                (int32_t)cursor[64] * coefficients[2]);
            odd += (uint32_t)(
                (int32_t)cursor[80] * coefficients[3]);
            odd += (uint32_t)(
                (int32_t)cursor[96] * coefficients[2]);
            odd += (uint32_t)(
                (int32_t)cursor[112] * coefficients[1]);
            odd += (uint32_t)(
                (int32_t)cursor[128] * coefficients[0]);
            phase_output[item + 1] = (int16_t)
                frame_arithmetic_shift_right(odd, 16U);
            cursor += 64;
        }
    }
}

void vocoder_cosine_transform(
    int16_t *values,
    int32_t output_count,
    int32_t input_count)
{
    int16_t input[56];
    input[0] = values[0];
    if (input_count > output_count) {
        input_count = output_count;
    }
    if (input_count > 1) {
        vocoder_shift_i16(
            input + 1, values + 1, input_count - 1, 1);
    }

    int16_t step = (int16_t)(output_count == 1
        ? INT16_MAX
        : 0x8000 / output_count);
    int16_t phase_step = (int16_t)(step >> 1);
    for (int32_t output_index = 0;
            output_index < output_count; ++output_index) {
        uint32_t phase = 0x40U;
        uint32_t accumulator = 0;
        for (int32_t input_index = 0;
                input_index < input_count; ++input_index) {
            unsigned int table_index =
                (unsigned int)((phase & 0xffffU) >> 7);
            int32_t product =
                (int32_t)frame_cosine_q15(table_index) *
                (int32_t)input[input_index];
            accumulator += (uint32_t)product << 1;
            phase = phase + (uint32_t)(uint16_t)phase_step;
        }
        values[output_index] = (int16_t)(
            (accumulator + 0x8000U) >> 16);
        phase_step = (int16_t)(
            (int16_t)phase_step + step);
    }
}

static uint32_t frame_logical_shift_left(
    uint32_t value,
    unsigned int shift)
{
    shift &= 0xffU;
    return shift >= 32U ? 0U : value << shift;
}

static unsigned int frame_count_leading_zeros(uint32_t value)
{
#if defined(__GNUC__) || defined(__clang__)
    return value == 0U
        ? 32U
        : (unsigned int)__builtin_clz(value);
#else
    unsigned int count = 0;
    if (value == 0U) {
        return 32U;
    }
    while ((value & 0x80000000U) == 0U) {
        value <<= 1;
        count++;
    }
    return count;
#endif
}

void vocoder_copy_frame_state(
    vocoder_frame_state *destination,
    const vocoder_frame_state *source)
{
    for (int index = 0; index < 8; ++index) {
        destination->header[index] = source->header[index];
    }
    for (int index = 0; index < VOCODER_SPECTRUM_SIZE; ++index) {
        destination->spectrum[index] = source->spectrum[index];
    }
}

static uint32_t frame_voicing_mask(const vocoder_frame_state *frame)
{
    return (uint32_t)frame->header[4] |
        ((uint32_t)frame->header[5] << 16);
}

static void set_frame_voicing_mask(
    vocoder_frame_state *frame,
    uint32_t mask)
{
    frame->header[4] = (uint16_t)mask;
    frame->header[5] = (uint16_t)(mask >> 16);
}

static int16_t average_spectrum_value(int16_t left, int16_t right)
{
    int32_t sum = (int32_t)left + (int32_t)right;
    int32_t average = sum >= 0
        ? sum / 2
        : -((-sum + 1) / 2);
    if (average >= 0x77ff) {
        average = 0x77ff;
    }
    if (average < -0x77ff) {
        average = -0x77ff;
    }
    return (int16_t)average;
}

static int16_t halve_toward_negative_infinity(int16_t value)
{
    int32_t promoted = value;
    return (int16_t)(promoted >= 0
        ? promoted / 2
        : -((-promoted + 1) / 2));
}

void vocoder_interpolate_frame_state(
    vocoder_frame_state *destination,
    const vocoder_frame_record *left,
    const vocoder_frame_record *right)
{
    const vocoder_frame_state *left_state = &left->state;
    const vocoder_frame_state *right_state = &right->state;

    if ((int16_t)left_state->header[1] != 0xff ||
            (int16_t)right_state->header[1] != 0xff) {
        vocoder_copy_frame_state(destination, left_state);
        return;
    }

    if ((int16_t)right_state->header[0] == 2) {
        uint32_t saved_mask = frame_voicing_mask(destination);
        uint16_t saved_decay = destination->header[7];
        vocoder_copy_frame_state(destination, left_state);
        set_frame_voicing_mask(destination, saved_mask);
        destination->header[7] = saved_decay;
        if ((int16_t)left_state->header[0] != 2) {
            for (int index = 0; index < VOCODER_SPECTRUM_SIZE; ++index) {
                destination->spectrum[index] =
                    halve_toward_negative_infinity(
                        destination->spectrum[index]);
            }
        }
        return;
    }

    if ((int16_t)left_state->header[0] == 2) {
        if ((int16_t)right->metadata[3] < 2 &&
                (int16_t)right_state->header[0] != 3) {
            vocoder_copy_frame_state(destination, right_state);
        }
        else {
            vocoder_copy_frame_state(destination, left_state);
        }

        int16_t decay = (int16_t)destination->header[7];
        if (decay > 0) {
            destination->header[7] =
                (uint16_t)((uint32_t)(uint16_t)decay - 0x5000U);
        }
        else {
            set_frame_voicing_mask(
                destination,
                frame_voicing_mask(destination) & 0x55555555U);
            destination->header[7] = 0;
        }
        return;
    }

    uint32_t right_mask = frame_voicing_mask(right_state);
    uint32_t destination_mask = frame_voicing_mask(destination);
    int16_t pitch;
    if ((right_mask & 0x55555555U) == 0U ||
            (destination_mask & 0xaaaaaaaaU) != 0U) {
        pitch = (int16_t)left_state->header[6];
    }
    else if ((frame_voicing_mask(left_state) & 0x55555555U) == 0U) {
        pitch = (int16_t)right_state->header[6];
    }
    else {
        int32_t product =
            (int16_t)left_state->header[6] *
            (int16_t)right_state->header[6] * 2;
        pitch = vocoder_sqrt_scaled((uint32_t)product, -8, -4);
    }

    destination->header[6] = (uint16_t)pitch;
    int16_t count = vocoder_harmonic_count(pitch);
    destination->header[1] = left_state->header[1];
    destination->header[2] = (uint16_t)count;

    int16_t left_spectrum[VOCODER_SPECTRUM_SIZE];
    int16_t right_spectrum[VOCODER_SPECTRUM_SIZE];
    vocoder_resample_spectrum(
        left_spectrum, pitch, (int16_t)left_state->header[6],
        left_state->spectrum, count);
    vocoder_resample_spectrum(
        right_spectrum, pitch, (int16_t)right_state->header[6],
        right_state->spectrum, count);

    int16_t last = 0;
    for (int index = 0; index < count; ++index) {
        last = average_spectrum_value(
            left_spectrum[index], right_spectrum[index]);
        destination->spectrum[index] = last;
    }
    for (int index = count; index < VOCODER_SPECTRUM_SIZE; ++index) {
        destination->spectrum[index] = last;
    }
}

static int32_t frame_saturating_add(int32_t left, int32_t right)
{
    int64_t sum = (int64_t)left + right;
    if (sum > INT32_MAX) {
        return INT32_MAX;
    }
    if (sum < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)sum;
}

int32_t vocoder_analyze_energy_trend(
    int16_t output[16],
    vocoder_energy_state *state)
{
    uint32_t baseline = 0xfc00U;
    int32_t sum = 0;

    for (int32_t index = 0; index < 16; ++index) {
        uint32_t scaled =
            (uint32_t)(int32_t)state->values[index] << 16;
        int32_t exponent = state->exponents[index];
        if (exponent < 5) {
            scaled = frame_arithmetic_shift_right(
                scaled, (unsigned int)(uint8_t)(5 - exponent));
        }
        else {
            scaled = frame_logical_shift_left(
                scaled, (unsigned int)(uint8_t)(exponent - 5));
        }

        uint32_t base_bits = scaled + 0x78000000U;
        uint32_t difference_bits =
            base_bits - (baseline << 16);
        int32_t difference = (int32_t)frame_arithmetic_shift_right(
            difference_bits, 16);
        if (difference < 0x66) {
            difference = 0x66;
        }

        int16_t logarithm =
            vocoder_log2_scaled((uint16_t)difference, 5, 5);
        int16_t adjustment =
            (int16_t)((uint16_t)logarithm - 0x021dU);
        int32_t result;
        if (adjustment < 0) {
            result = frame_saturating_add(
                (int32_t)((uint32_t)(uint16_t)adjustment << 16),
                (int32_t)(base_bits & 0xffff0000U));
        }
        else {
            result = (int32_t)(base_bits & 0xffff0000U);
        }
        output[index] = (int16_t)(
            ((uint32_t)result >> 16) & 0xffffU);
        sum += output[index];
        baseline = (baseline - 0x40U) & 0xffffU;
    }

    uint32_t margin = sum <= 0x10000
        ? 0x78000U
        : 0x3c000U;
    uint32_t decision = margin +
        ((uint32_t)(sum - state->previous_sum) << 6);
    if ((decision & 0x80000000U) != 0U) {
        state->previous_sum = sum;
        return 2;
    }
    return 0;
}

static int16_t frame_normalize_i32_to_i16(
    uint32_t value,
    int32_t *exponent)
{
    if (value == 0U) {
        return 0;
    }
    uint32_t magnitude =
        (value & 0x80000000U) != 0U ? ~value : value;
    unsigned int shift = frame_count_leading_zeros(magnitude) - 1U;
    *exponent -= (int32_t)shift;
    return (int16_t)frame_arithmetic_shift_right(
        frame_shift_left(value, shift), 16);
}

static int32_t frame_multiply_q15_truncated(
    int32_t left,
    int32_t right)
{
    uint32_t product =
        (uint32_t)(left * right);
    return (int32_t)frame_arithmetic_shift_right(
        product << 1, 16);
}

static int16_t frame_normalize_i64_to_i16(
    uint64_t value,
    int32_t *exponent)
{
    if (value == 0U) {
        return 0;
    }
    int negative = (value & UINT64_C(0x8000000000000000)) != 0U;
    uint64_t magnitude = negative ? ~value : value;
    unsigned int shift;
#if defined(__GNUC__) || defined(__clang__)
    shift = (unsigned int)__builtin_clzll(magnitude);
#else
    shift = 0U;
    uint64_t probe = magnitude;
    while ((probe & UINT64_C(0x8000000000000000)) == 0U) {
        probe <<= 1;
        ++shift;
    }
#endif
    int32_t adjustment = (int32_t)shift - 33;
    uint32_t normalized;
    if (adjustment < 0) {
        unsigned int right = (unsigned int)(-adjustment);
        uint64_t shifted = value >> right;
        if (negative) {
            shifted |= UINT64_MAX << (64U - right);
        }
        normalized = (uint32_t)shifted;
    }
    else {
        normalized = (uint32_t)(value << (unsigned int)adjustment);
    }
    *exponent -= adjustment;
    int16_t result = (int16_t)(normalized >> 16);
    return result == INT16_MIN ? (int16_t)-32767 : result;
}

void vocoder_apply_adaptive_spectral_attenuation(
    int16_t *samples,
    int32_t *exponent,
    int16_t *smoothed_mantissa,
    int16_t *smoothed_exponent,
    int32_t phase_step,
    int32_t count)
{
    int32_t energy_exponent;
    int32_t energy_q31 = vocoder_vector_energy(
        &energy_exponent, samples, (uint32_t)count);
    energy_exponent += *exponent * 2;
    int16_t energy = (int16_t)(
        frame_arithmetic_shift_right((uint32_t)energy_q31, 16));

    int32_t weighted_exponent = *exponent * 2;
    int16_t weighted = 0;
    int16_t windows[56];
    if (energy != 0) {
        uint64_t weighted_sum = 0;
        uint32_t phase = (uint32_t)phase_step << 6;
        uint32_t increment = phase;
        for (int32_t index = 0; index < count; ++index) {
            int16_t window = frame_cosine_q15(phase >> 16);
            int32_t square = (int32_t)samples[index] * samples[index];
            int16_t square_q15 =
                (int16_t)((uint32_t)square >> 15);
            weighted_sum +=
                (uint64_t)(int64_t)(
                    (int32_t)window * square_q15 * 2);
            windows[index] = window;
            phase += increment;
        }
        weighted = frame_normalize_i64_to_i16(
            weighted_sum, &weighted_exponent);
    }

    int32_t common_exponent =
        *smoothed_exponent > energy_exponent
            ? *smoothed_exponent : energy_exponent;
    uint32_t old_term =
        (uint32_t)((int32_t)*smoothed_mantissa * 0x799a * 2);
    uint32_t new_term =
        (uint32_t)((int32_t)energy * 0x6666 * 2);
    int32_t old_shift = common_exponent - *smoothed_exponent;
    int32_t new_shift =
        common_exponent - (energy_exponent - 4);
    old_term = frame_arithmetic_shift_right(
        old_term, (unsigned int)old_shift);
    new_term = frame_arithmetic_shift_right(
        new_term, (unsigned int)new_shift);
    uint32_t smoothed = old_term + new_term;
    int16_t normalized_smoothed =
        frame_normalize_i32_to_i16(
            smoothed, &common_exponent);
    if (normalized_smoothed == 0 ||
            common_exponent < -16) {
        *smoothed_mantissa = 0x7fff;
        *smoothed_exponent = -17;
    }
    else {
        *smoothed_mantissa = normalized_smoothed;
        *smoothed_exponent = (int16_t)common_exponent;
    }
    if (energy <= 0) {
        return;
    }

    int32_t energy_square =
        (int32_t)energy * energy;
    int32_t pair_exponent = energy_exponent * 2 + 1;
    int32_t weighted_square =
        (int32_t)weighted * weighted * 2;
    int32_t exponent_difference =
        weighted_exponent * 2 - pair_exponent;
    if (exponent_difference < 0) {
        weighted_square = (int32_t)frame_arithmetic_shift_right(
            (uint32_t)weighted_square,
            (unsigned int)(-exponent_difference));
    }
    else {
        weighted_square = (int32_t)frame_shift_left(
            (uint32_t)weighted_square,
            (unsigned int)exponent_difference);
    }
    int32_t residual =
        (energy_square - weighted_square) >> 16;
    int32_t combined =
        (energy_square + weighted_square) >> 16;

    int32_t first_count = count >> 3;
    if (weighted != 0 && residual == 0) {
        vocoder_shift_i16_between_exponents(
            samples, samples, first_count,
            *exponent, *exponent + 1);
        for (int32_t index = first_count;
                index < count; ++index) {
            int32_t gain = 0x7fff;
            samples[index] = (int16_t)(
                ((int32_t)samples[index] * gain * 2) >> 16);
        }
        goto renormalize;
    }

    int32_t divisor =
        frame_multiply_q15_truncated(residual, energy) *
        phase_step * 2;
    int32_t divisor_exponent = 0;
    int16_t divisor_mantissa =
        frame_normalize_i32_to_i16(
            (uint32_t)divisor, &divisor_exponent);
    int16_t scale = (int16_t)vocoder_fixed_divide_half(
        0x3cd013a9, divisor_mantissa);

    int32_t base_value = combined * scale * 2;
    int32_t base_exponent =
        4 - energy_exponent - divisor_exponent;
    int16_t base = frame_normalize_i32_to_i16(
        (uint32_t)base_value, &base_exponent);

    int16_t cross = (int16_t)
        frame_multiply_q15_truncated(weighted, energy);
    int32_t window_value = (int32_t)((uint32_t)
        frame_multiply_q15_truncated(scale, cross) << 15);
    int32_t window_exponent =
        weighted_exponent - divisor_exponent +
        6 - pair_exponent;
    int16_t window_scale = frame_normalize_i32_to_i16(
        (uint32_t)window_value, &window_exponent);

    vocoder_shift_i16_between_exponents(
        samples, samples, first_count,
        *exponent, *exponent + 1);
    int32_t target_exponent =
        (base_exponent > window_exponent
            ? base_exponent : window_exponent) + 1;
    int32_t aligned_base = (int32_t)frame_arithmetic_shift_right(
        (uint32_t)(int32_t)base << 16,
        (unsigned int)(target_exponent - base_exponent));
    for (int32_t index = first_count;
            index < count; ++index) {
        int32_t weighted_window =
            (int32_t)((uint32_t)(
                (int32_t)window_scale * windows[index]) << 1);
        weighted_window = (int32_t)frame_arithmetic_shift_right(
            (uint32_t)weighted_window,
            (unsigned int)(target_exponent - window_exponent));
        int32_t difference = aligned_base - weighted_window;
        int32_t root_exponent = target_exponent;
        uint32_t root = vocoder_normalized_sqrt(
            (uint32_t)difference, &root_exponent);
        root_exponent += *exponent;
        root = vocoder_normalized_sqrt(
            (uint32_t)((int32_t)(int16_t)(root >> 16) *
                       samples[index] * 2),
            &root_exponent);
        int32_t gain;
        if (root_exponent < 2) {
            gain = root_exponent == 1
                ? (int32_t)root >> 16
                : (int32_t)frame_arithmetic_shift_right(
                    root, (unsigned int)(1 - root_exponent)) >> 16;
            if (gain < 0x2000) {
                gain = 0x2000;
            }
            if (gain > 0x4ccd) {
                gain = 0x4ccd;
            }
        }
        else {
            gain = 0x4ccd;
        }
        samples[index] = (int16_t)
            frame_multiply_q15_truncated(
                gain, samples[index]);
    }

renormalize:
    {
        int32_t original_energy_exponent = energy_exponent;
        ++*exponent;
        vocoder_normalize_i16_vector(
            samples, samples, count, exponent);
        int32_t output_exponent;
        int32_t output_energy = vocoder_vector_energy(
            &output_exponent, samples, (uint32_t)count);
        output_exponent += *exponent * 2;
        int16_t output_mantissa = (int16_t)(
            frame_arithmetic_shift_right(
                (uint32_t)output_energy, 16));
        if (output_mantissa != 0) {
            int32_t ratio_exponent;
            int16_t ratio = vocoder_normalized_divide(
                (int32_t)((uint32_t)(uint16_t)energy << 16),
                original_energy_exponent,
                output_mantissa, output_exponent,
                &ratio_exponent);
            uint32_t root = vocoder_normalized_sqrt(
                (uint32_t)(uint16_t)ratio << 16,
                &ratio_exponent);
            vocoder_scale_i16_vector(
                samples, samples, count,
                (int16_t)(root >> 16));
            *exponent += ratio_exponent;
            if (*exponent > 0) {
                vocoder_saturating_shift_i16(
                    samples, samples, count,
                    *exponent, 0);
                *exponent = 0;
            }
        }
    }
}

static void move_active_harmonic_to(
    vocoder_frame_state *state,
    int16_t new_index,
    int16_t active[VOCODER_SPECTRUM_SIZE])
{
    int16_t old_index = (int16_t)state->header[3];
    int16_t saved = state->spectrum[old_index];
    state->spectrum[old_index] = 0;
    active[old_index] = 0;
    state->header[3] = (uint16_t)new_index;
    state->spectrum[new_index] = saved;
    active[new_index] = 1;

    uint32_t denominator_bits =
        (uint32_t)(int32_t)(new_index + 1) << 16;
    uint32_t denominator_magnitude =
        (denominator_bits & 0x80000000U) != 0U
        ? ~denominator_bits
        : denominator_bits;
    int32_t denominator_shift =
        denominator_magnitude == 0U
        ? 0
        : (int32_t)frame_count_leading_zeros(
            denominator_magnitude) - 1;

    int16_t numerator_value = (int16_t)state->header[1];
    uint32_t numerator_bits =
        (uint32_t)(int32_t)numerator_value << 16;
    uint32_t numerator_magnitude =
        (numerator_bits & 0x80000000U) != 0U
        ? ~numerator_bits
        : numerator_bits;
    int32_t numerator_shift = numerator_magnitude == 0U
        ? 0
        : (int32_t)frame_count_leading_zeros(
            numerator_magnitude) - 1;

    uint32_t normalized_denominator = frame_logical_shift_left(
        denominator_bits,
        (unsigned int)(uint8_t)denominator_shift);
    uint32_t normalized_numerator = frame_logical_shift_left(
        numerator_bits,
        (unsigned int)(uint8_t)numerator_shift);
    int32_t result_exponent;
    int16_t ratio = vocoder_normalized_divide(
        (int32_t)normalized_numerator,
        15 - numerator_shift,
        (int16_t)(frame_arithmetic_shift_right(
            normalized_denominator, 16) & 0xffffU),
        23 - denominator_shift,
        &result_exponent);

    uint32_t ratio_bits = (uint32_t)(uint16_t)ratio << 16;
    if (result_exponent < -4) {
        ratio_bits = frame_arithmetic_shift_right(
            ratio_bits,
            (unsigned int)(uint8_t)(-4 - result_exponent));
    }
    else {
        ratio_bits = frame_logical_shift_left(
            ratio_bits,
            (unsigned int)(uint8_t)(result_exponent + 4));
    }
    state->header[6] = (uint16_t)(
        frame_arithmetic_shift_right(ratio_bits, 16) &
        0xffffU);

    int32_t count = (int16_t)state->header[2];
    if (count <= new_index) {
        count = new_index + 1;
    }
    if (count < 9) {
        count = 9;
    }
    if (count >= 56) {
        count = 56;
    }
    state->header[2] = (uint16_t)count;
}

void vocoder_move_active_harmonic(
    vocoder_harmonic_state *state,
    int16_t new_index)
{
    move_active_harmonic_to(
        (vocoder_frame_state *)(void *)state, new_index, state->active);
}

int16_t vocoder_combine_harmonic_indices(
    uint16_t *combined_index,
    vocoder_frame_state *left,
    vocoder_frame_state *right,
    int32_t left_adjustment,
    int32_t right_adjustment,
    uint32_t table_bits,
    int32_t update_indices)
{
    int eligible =
        update_indices == 0 || (int16_t)right->header[0] == 1;
    uint32_t left_mask = frame_voicing_mask(left);
    uint32_t right_mask = frame_voicing_mask(right);

    if (((left_mask | right_mask) & 0x55555555U) == 0U &&
            eligible != 0) {
        int left_active =
            (left_mask & 0xaaaaaaaaU) != 0U;
        int right_active =
            (right_mask & 0xaaaaaaaaU) != 0U;
        if (left_active != 0 || right_active != 0) {
            uint32_t maximum_index = update_indices == 0
                ? 0U
                : frame_logical_shift_left(
                    60U,
                    (unsigned int)(uint8_t)(table_bits - 6U)) - 1U;
            uint32_t harmonic_count_bits = frame_logical_shift_left(
                1U, (unsigned int)(uint8_t)table_bits);
            int32_t harmonic_count =
                (int32_t)harmonic_count_bits;

            if (left_active == 0) {
                left->header[7] = 0;
                right->header[7] = (uint16_t)
                    vocoder_adjust_harmonic_index(
                        combined_index, harmonic_count,
                        right_adjustment, maximum_index,
                        update_indices);
                return 1;
            }
            if (right_active == 0) {
                right->header[7] = 0;
                left->header[7] = (uint16_t)
                    vocoder_adjust_harmonic_index(
                        combined_index, harmonic_count,
                        left_adjustment, maximum_index,
                        update_indices);
                return 1;
            }

            uint32_t group_bits = frame_logical_shift_left(
                1U,
                (unsigned int)(uint8_t)(table_bits >> 1));
            int32_t group_size = (int32_t)group_bits;
            if ((table_bits & 1U) != 0U) {
                uint32_t product =
                    group_bits * 0x16a08U;
                group_size = (int32_t)
                    frame_arithmetic_shift_right(product, 16);
            }

            int16_t left_index;
            int16_t right_index;
            if (update_indices != 0) {
                uint16_t left_index_bits = 0;
                uint16_t right_index_bits = 0;
                left->header[7] = (uint16_t)
                    vocoder_adjust_harmonic_index(
                        &left_index_bits, group_size,
                        left_adjustment, maximum_index,
                        update_indices);
                right->header[7] = (uint16_t)
                    vocoder_adjust_harmonic_index(
                        &right_index_bits, group_size,
                        right_adjustment, maximum_index,
                        update_indices);
                left_index = (int16_t)left_index_bits;
                right_index = (int16_t)right_index_bits;
                int32_t combined =
                    group_size * right_index + left_index;
                if (combined <= (int32_t)maximum_index) {
                    maximum_index =
                        (uint32_t)(combined < 0 ? 0 : combined);
                }
                *combined_index = (uint16_t)maximum_index;
                return 1;
            }

            right_index = (int16_t)vocoder_fixed_divide(
                *combined_index, (int16_t)group_size);
            left_index = (int16_t)(
                (int32_t)*combined_index -
                group_size * right_index);
            if (right_index >= group_size) {
                right_index = (int16_t)(group_size - 1);
            }
            if (left_index >= group_size) {
                left_index = (int16_t)(group_size - 1);
            }
            uint16_t left_index_bits = (uint16_t)left_index;
            uint16_t right_index_bits = (uint16_t)right_index;
            left->header[7] = (uint16_t)
                vocoder_adjust_harmonic_index(
                    &left_index_bits, group_size,
                    left_adjustment, maximum_index, 0);
            right->header[7] = (uint16_t)
                vocoder_adjust_harmonic_index(
                    &right_index_bits, group_size,
                    right_adjustment, maximum_index, 0);
            return 1;
        }
    }

    right->header[7] = 0;
    left->header[7] = 0;
    return 0;
}

int16_t vocoder_update_encoder_metric(
    const vocoder_encoder_metric_state *state,
    const vocoder_encoder_aux_state *auxiliary,
    vocoder_encoder_metric_parameters *parameters)
{
    int16_t current = parameters->value[0];
    int16_t delta = (int16_t)(
        (uint16_t)state->reference - (uint16_t)current);
    int16_t delta_limit = parameters->value[9];
    if (delta < -delta_limit) {
        delta = (int16_t)-delta_limit;
    }

    int16_t coefficient;
    int16_t division_threshold = parameters->value[10];
    int update = 1;
    if (delta > delta_limit) {
        coefficient = parameters->value[5];
    }
    else if (delta > 0) {
        coefficient = parameters->value[6];
    }
    else if (state->activity <= 0x4000) {
        coefficient = parameters->value[8];
        if (state->activity <= 0x2000) {
            update = 0;
        }
    }
    else if (auxiliary->indicator <= 0x0ccb &&
            division_threshold <
                (int16_t)(
                    (uint16_t)state->reference -
                    (uint16_t)state->slope)) {
        coefficient = parameters->value[7];
    }
    else {
        coefficient = parameters->value[8];
    }

    if (update != 0) {
        int32_t product =
            (int32_t)delta * coefficient;
        uint32_t updated_bits =
            ((uint32_t)(uint16_t)current << 16) +
            ((uint32_t)product << 1);
        current = (int16_t)(updated_bits >> 16);
        parameters->value[0] = current;
    }

    int16_t minimum = parameters->value[4];
    int16_t limited = current < minimum ? minimum : current;
    if (current < minimum) {
        parameters->value[0] = minimum;
    }

    uint32_t difference_bits =
        ((uint32_t)(uint16_t)parameters->value[1] << 16) -
        ((uint32_t)(uint16_t)limited << 16);
    int16_t difference = (int16_t)(
        frame_arithmetic_shift_right(difference_bits, 16) &
        0xffffU);
    int16_t output = parameters->value[2];
    uint32_t upper_bits =
        (uint32_t)(uint16_t)parameters->value[2] << 16;
    if ((int32_t)difference_bits <= (int32_t)upper_bits) {
        output = parameters->value[3];
        if (difference >= output) {
            output = difference;
        }
    }

    int16_t distance = (int16_t)(
        (uint16_t)limited - (uint16_t)state->slope);
    if (distance < division_threshold) {
        int32_t quotient = vocoder_fixed_divide_half(
            (int32_t)((uint32_t)(int32_t)distance << 12),
            division_threshold);
        int16_t scaled_quotient = (int16_t)(
            ((uint32_t)quotient << 20) >> 16);
        int32_t product =
            (int32_t)scaled_quotient * output;
        output = (int16_t)(((uint32_t)product >> 15) & 0xffffU);
    }
    return output;
}

static int64_t frame_arithmetic_shift_right64(
    int64_t value,
    unsigned int shift)
{
    if (shift == 0U) {
        return value;
    }
    if (shift >= 64U) {
        return value < 0 ? -1 : 0;
    }
    if (value >= 0) {
        return value / ((int64_t)1 << shift);
    }
    uint64_t magnitude = 0U - (uint64_t)value;
    uint64_t divisor = (uint64_t)1 << shift;
    return -(int64_t)((magnitude + divisor - 1U) / divisor);
}

static unsigned int frame_count_leading_zeros64(uint64_t value)
{
    uint32_t high = (uint32_t)(value >> 32);
    if (high != 0U) {
        return frame_count_leading_zeros(high);
    }
    return frame_count_leading_zeros((uint32_t)value) + 32U;
}

static int32_t frame_saturate_i32(int64_t value)
{
    if (value > INT32_MAX) {
        return INT32_MAX;
    }
    if (value < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

void vocoder_filter_i32_state6(
    int32_t *output,
    const int32_t *input,
    int32_t state[6],
    int32_t count)
{
    int32_t previous_input = state[0];
    int32_t first = state[1];
    int32_t second = state[2];
    int32_t third = state[3];
    int32_t fourth = state[4];
    int32_t fifth = state[5];
    int32_t current_input = 0;
    int32_t current_output = 0;

    for (int32_t index = 0; index < count; ++index) {
        current_input = input[index];
        uint32_t next_first = (uint32_t)
            frame_arithmetic_shift_right64(
                (int64_t)first * 0x78ab, 15U);
        next_first += (uint32_t)current_input -
            (uint32_t)previous_input;

        uint32_t next_output = (uint32_t)
            frame_arithmetic_shift_right64(
                (int64_t)third * 0x7c20, 15U) * 2U;
        next_output += (uint32_t)fourth;
        next_output -= (uint32_t)second * 2U;
        next_output += next_first;
        next_output -= (uint32_t)
            frame_arithmetic_shift_right64(
                (int64_t)fifth * 0x78af, 15U);
        current_output = (int32_t)next_output;
        output[index] = current_output;

        previous_input = current_input;
        fifth = third;
        third = current_output;
        fourth = second;
        second = (int32_t)next_first;
        first = (int32_t)next_first;
    }

    state[0] = current_input;
    state[1] = first;
    state[2] = first;
    state[3] = current_output;
    state[4] = fourth;
    state[5] = fifth;
}

void vocoder_filter_i16_state6(
    int32_t state[6],
    int16_t *output,
    const int16_t *input,
    int32_t count)
{
    int32_t workspace[85];
    for (int32_t index = 0; index < count; ++index) {
        workspace[index] = (int32_t)(
            (uint32_t)(int32_t)input[index] << 14);
    }
    vocoder_filter_i32_state6(
        workspace, workspace, state, count);

    for (int32_t index = 0; index < count; ++index) {
        int32_t scaled = (int32_t)
            frame_arithmetic_shift_right64(
                (int64_t)workspace[index] * 0x78aa, 15U);
        uint32_t shifted = (uint32_t)scaled << 2;
        if (scaled != (int32_t)
                frame_arithmetic_shift_right(shifted, 2U)) {
            shifted = ((uint32_t)(scaled >> 31)) ^
                (uint32_t)INT32_MAX;
        }
        int32_t rounded = frame_saturating_add(
            (int32_t)shifted, 0x8000);
        output[index] = (int16_t)
            frame_arithmetic_shift_right(
                (uint32_t)rounded, 16U);
    }
}

int32_t vocoder_test_weighted_curvature(
    int16_t threshold,
    int32_t value_exponent,
    const int16_t values[5],
    int16_t input_exponent)
{
    uint32_t weighted_sum = 0U;
    uint32_t weighted_square_sum = 0U;
    for (int32_t index = 4; index > 0; --index) {
        weighted_sum += (uint32_t)(
            index * (int32_t)values[index] * 2);
        weighted_square_sum += (uint32_t)(
            index * index * (int32_t)values[index] * 2);
    }

    uint32_t magnitude = (int32_t)weighted_sum < 0
        ? ~weighted_sum
        : weighted_sum;
    int16_t sum_shift = weighted_sum == 0U
        ? 0
        : (int16_t)(frame_count_leading_zeros(magnitude) - 1U);
    int16_t normalized_sum = (int16_t)(
        frame_shift_left(weighted_sum, (uint16_t)sum_shift) >> 16);
    uint32_t squared_sum = (uint32_t)(
        (int32_t)normalized_sum * normalized_sum * 2);

    magnitude = (int32_t)weighted_square_sum < 0
        ? ~weighted_square_sum
        : weighted_square_sum;
    int16_t square_shift = weighted_square_sum == 0U
        ? 0
        : (int16_t)(frame_count_leading_zeros(magnitude) - 1U);
    int16_t normalized_square = (int16_t)(
        frame_shift_left(
            weighted_square_sum, (uint16_t)square_shift) >> 16);

    int32_t sum_exponent =
        (int32_t)input_exponent + 15 - sum_shift;
    int32_t square_exponent =
        (int32_t)input_exponent + 15 - square_shift +
        value_exponent;
    int32_t comparison_shift =
        sum_exponent * 2 - square_exponent;
    if (comparison_shift >= 0) {
        squared_sum = frame_shift_left(
            squared_sum, (uint32_t)comparison_shift);
    }
    else {
        squared_sum = frame_arithmetic_shift_right(
            squared_sum, (uint32_t)-comparison_shift);
    }

    int16_t threshold_factor = (int16_t)(
        ((uint32_t)((int32_t)threshold * 0x999a)) >> 16);
    uint32_t scaled_threshold = (uint32_t)(
        (int32_t)threshold * threshold_factor * 2);
    int32_t threshold_shift =
        value_exponent * 2 - square_exponent;
    if (threshold_shift >= 0) {
        scaled_threshold = frame_shift_left(
            scaled_threshold, (uint32_t)threshold_shift);
    }
    else {
        scaled_threshold = frame_arithmetic_shift_right(
            scaled_threshold, (uint32_t)-threshold_shift);
    }

    uint32_t left = (uint32_t)(
        (int32_t)normalized_square * threshold * 2);
    left -= squared_sum;
    return (int32_t)left < (int32_t)scaled_threshold;
}

void vocoder_sqrt_i32_vector(
    int16_t *output,
    int16_t output_exponent,
    const int32_t *input,
    uint16_t input_exponent,
    int32_t count)
{
    for (int32_t index = 0; index < count; ++index) {
        int32_t value = input[index];
        uint32_t approximation = 0U;
        uint16_t root_exponent = input_exponent;
        if (value != 0) {
            uint16_t normalization = (uint16_t)(
                frame_count_leading_zeros((uint32_t)value) - 1U);
            int16_t mantissa = (int16_t)(
                frame_shift_left(
                    (uint32_t)value, (uint16_t)normalization) >> 16);
            uint16_t exponent_difference = (uint16_t)(
                input_exponent - normalization);

            uint32_t first = (uint32_t)(
                (int32_t)29003 * 0x10000 + 0x8000);
            first += (uint32_t)(
                (int32_t)-6538 * mantissa * 2);
            int16_t first_stage = (int16_t)(first >> 16);

            approximation = (uint32_t)(
                (int32_t)10303 * 0x10000 + 0x8000);
            approximation += (uint32_t)(
                (int32_t)first_stage * mantissa * 2);
            approximation &= 0xffff0000U;
            if ((exponent_difference & 1U) != 0U) {
                approximation = (uint32_t)(
                    (int32_t)(int16_t)(approximation >> 16) *
                    23170 * 2);
            }
            approximation =
                (approximation + 0x8000U) & 0xffff0000U;
            root_exponent = (uint16_t)(
                ((uint32_t)(
                    (int32_t)(int16_t)(
                        exponent_difference + 1U) * 0x8000)) >> 16);
        }

        int32_t shift =
            (int32_t)(int16_t)root_exponent - output_exponent;
        uint32_t aligned;
        if (shift < 0) {
            aligned = frame_arithmetic_shift_right(
                approximation, (uint32_t)-shift);
        }
        else {
            aligned = frame_shift_left(
                approximation, (uint32_t)shift);
        }
        output[index] = (int16_t)
            frame_arithmetic_shift_right(
                aligned + 0x8000U, 16U);
    }
}

int32_t vocoder_vector_quantize_i16(
    int16_t *values,
    int32_t dimension,
    uint32_t stride_exponent,
    const int16_t *table,
    uint32_t code_bits,
    int32_t stride,
    int16_t **cursor,
    int32_t encode)
{
    if ((int32_t)stride_exponent < (int32_t)code_bits) {
        stride = (int32_t)frame_shift_left(
            (uint32_t)stride, code_bits - stride_exponent);
        code_bits = stride_exponent;
    }

    int32_t selected;
    if (encode == 0) {
        const int16_t *read_cursor = *cursor;
        selected = (int32_t)vocoder_read_bits_cursor(
            &read_cursor, (int32_t)code_bits);
        *cursor = (int16_t *)read_cursor;
    }
    else {
        int32_t candidate_count = (int32_t)frame_shift_left(
            1U, code_bits);
        int32_t best_metric = INT32_MAX;
        selected = candidate_count;
        for (int32_t candidate = 0;
                candidate < candidate_count; ++candidate) {
            int32_t metric = 0;
            const int16_t *candidate_values =
                table + candidate * stride;
            for (int32_t index = 0; index < dimension; ++index) {
                int32_t difference = frame_saturate_i32(
                    ((int64_t)values[index] -
                     candidate_values[index]) * 0x10000);
                int16_t difference_q0 = (int16_t)(
                    (uint32_t)difference >> 16);
                int32_t square = frame_saturate_i32(
                    (int64_t)difference_q0 *
                    difference_q0 * 2);
                metric = frame_saturate_i32(
                    (int64_t)metric + square);
            }
            if (metric < best_metric) {
                best_metric = metric;
                selected = candidate;
            }
        }
        vocoder_write_bits_cursor(
            cursor, (int32_t)code_bits, selected);
    }

    const int16_t *selected_values = table + selected * stride;
    vocoder_copy_i16(values, selected_values, dimension);
    return selected;
}

void vocoder_quantize_log_average(
    int16_t *output,
    int32_t dimension,
    const int16_t *values,
    int32_t count,
    int16_t *previous,
    uint32_t code_bits,
    int16_t **cursor,
    int32_t encode)
{
    static const int16_t table[32] = {
        -4096, -1372, 610, 1359, 2123, 2945, 3870, 4562,
        5075, 5463, 5721, 5925, 6186, 6427, 6630, 6804,
        7029, 7315, 7570, 7812, 8030, 8238, 8445, 8659,
        8950, 9305, 9641, 9930, 10356, 10908, 11832, 14078
    };

    if (encode != 0) {
        *output = (int16_t)vocoder_average_i16(values, count);
        int32_t logarithm = vocoder_log2_scaled(
            (uint16_t)count, 15, 5);
        int32_t combined = frame_saturate_i32(
            (int64_t)logarithm * 0x10000 +
            (int64_t)*output * 0x10000);
        combined = frame_saturate_i32(
            (int64_t)combined -
            (int64_t)*previous * 0x8000);
        combined = frame_saturate_i32(
            (int64_t)combined + 0x8000);
        *output = (int16_t)((uint32_t)combined >> 16);
        if (dimension == 2) {
            int32_t biased = frame_saturate_i32(
                (int64_t)*output * 0x10000 +
                0x10000000);
            *output = (int16_t)(
                (uint32_t)biased >> 16);
        }
    }

    (void)vocoder_vector_quantize_i16(
        output, 1, code_bits, table,
        code_bits, 1, cursor, encode);

    if (dimension == 2) {
        *output = (int16_t)(*output - 0x1000);
    }
    uint32_t smoothed = (uint32_t)(
        (int32_t)*previous * 0x8000);
    smoothed += (uint32_t)(
        (int32_t)*output * 0x10000);
    int32_t result = (int32_t)
        frame_arithmetic_shift_right(smoothed, 16U);
    if (result >= 0x6800) {
        result = 0x6800;
    }
    *output = (int16_t)result;
    if (dimension != 2) {
        *previous = (int16_t)result;
    }
}

int16_t vocoder_select_spectral_candidate(
    const int16_t candidates[5],
    const int16_t scores[5],
    int32_t target,
    int32_t local_index,
    int32_t fallback_index)
{
    int32_t selected_value = scores[local_index];
    if (local_index > 0 && scores[local_index - 1] < 0x199a) {
        int32_t previous_score_q16 = (int32_t)(
            (uint32_t)(int32_t)scores[local_index - 1] << 16);
        int32_t current_half_q16 = (int32_t)(
            (uint32_t)(int32_t)scores[local_index] * 0x8000U);
        uint32_t next_q16 =
            (uint32_t)(uint16_t)scores[local_index + 1] << 16;
        int32_t next_half_q16 = (int32_t)
            frame_arithmetic_shift_right(next_q16, 1U);
        if (previous_score_q16 < current_half_q16 &&
                previous_score_q16 < next_half_q16) {
            uint32_t current_q16 =
                (uint32_t)(uint16_t)candidates[local_index] << 16;
            uint32_t previous_q16 =
                (uint32_t)(uint16_t)candidates[local_index - 1] << 16;
            int32_t difference = (int32_t)(
                current_q16 -
                frame_arithmetic_shift_right(previous_q16, 1U));
            uint32_t magnitude = difference == INT32_MIN
                ? 0x80000000U
                : (uint32_t)(difference < 0
                    ? -difference
                    : difference);
            int32_t previous_eighth = (int32_t)
                frame_arithmetic_shift_right(previous_q16, 3U);
            if ((int32_t)magnitude < previous_eighth) {
                selected_value = (int16_t)(
                    (uint16_t)candidates[local_index] * 2U);
                goto scale_result;
            }
        }
    }

    if (selected_value < 0x3334) {
        selected_value = candidates[local_index];
    }
    else {
        if (scores[fallback_index] > 0x3332) {
            int32_t minimum_index = 0;
            int16_t minimum_score = scores[0];
            for (int32_t index = 1; index < 5; ++index) {
                if (scores[index] < minimum_score) {
                    minimum_score = scores[index];
                    minimum_index = index;
                }
            }
            if (minimum_score > 0x3332) {
                int32_t closest_index = 0;
                int32_t closest_distance =
                    (int32_t)candidates[0] - target;
                if (closest_distance < 0) {
                    closest_distance = closest_distance == INT16_MIN
                        ? INT16_MAX
                        : -closest_distance;
                }
                for (int32_t index = 1; index < 5; ++index) {
                    int32_t distance =
                        (int32_t)candidates[index] - target;
                    if (distance < 0) {
                        distance = distance == INT16_MIN
                            ? INT16_MAX
                            : -distance;
                    }
                    if (distance < closest_distance) {
                        closest_distance = distance;
                        closest_index = index;
                    }
                }
                selected_value = candidates[closest_index];
                goto scale_result;
            }
            fallback_index = minimum_index;
        }
        selected_value = candidates[fallback_index];
    }

scale_result:
    selected_value = frame_saturate_i32(
        (int64_t)selected_value * 2);
    if (selected_value > INT16_MAX) {
        selected_value = INT16_MAX;
    }
    if (selected_value < INT16_MIN) {
        selected_value = INT16_MIN;
    }
    return (int16_t)selected_value;
}

int32_t vocoder_initialize_spectral_frame(
    int16_t frame[64],
    uint32_t code,
    int32_t first_level,
    int32_t second_level,
    int16_t spread)
{
    static const int16_t spacing_table[36] = {
        5145, 11369, 4588, 5702, 7206, 12562, 4599, 4657,
        7968, 13894, 7629, 6301, 4653, 15352, 8807, 8805,
        4478, 9883, 4445, 5667, 6278, 10939, 4437, 4898,
        6940, 6080, 6655, 5506, 4445, 6704, 7668, 7700,
        5753, 4642, 7995, 4588
    };
    static const int16_t difference_table[36] = {
        2252, 1740, 409, 5734, -1228, 2560, 3072, -1740,
        -2457, 1024, 2768, 2355, 921, 1024, -614, 1024,
        -1536, 921, 1126, 5120, 921, 2048, 1638, -819,
        0, 2457, 3481, 7168, 1024, 2252, 0, -1024,
        614, 1024, 409, -204
    };

    frame[0] = 3;
    frame[4] = 1;
    frame[5] = 0;
    frame[7] = 0;
    vocoder_fill_i16(frame + 8, -0x4fff, 56);

    uint32_t short_code = code & 0xffffU;
    if (((short_code - 5U) & 0xffffU) < 0x76U) {
        int32_t partition =
            vocoder_lookup_partition_value(0U, (int32_t)code, 0);
        frame[1] = (int16_t)code;
        frame[3] = (int16_t)partition;
        int32_t denominator =
            (partition * 0x1000000 + 0x1000000) >> 16;
        frame[6] = (int16_t)vocoder_fixed_divide_half(
            (int32_t)(code << 20), (int16_t)denominator);
        int32_t harmonic_count = partition + 1;
        if (harmonic_count < 9) {
            harmonic_count = 9;
        }
        if (harmonic_count > 56) {
            harmonic_count = 56;
        }
        frame[2] = (int16_t)harmonic_count;
        int32_t level = second_level;
        if (level < -0x5000) {
            level = -0x5000;
        }
        if (level > 0x77ff) {
            level = 0x77ff;
        }
        frame[partition + 8] = (int16_t)level;
        if (spread != 0) {
            int32_t delta = frame_saturate_i32(
                (int64_t)level * 0x10000 -
                0x50000000LL);
            vocoder_spread_spectral_peak(
                frame + 8, (int16_t)partition, delta >> 16);
        }
        return 0;
    }

    int32_t group;
    if (((short_code - 0x80U) & 0xffffU) < 0x10U) {
        group = 1;
    }
    else if (((short_code - 0x90U) & 0xffffU) < 0x10U) {
        group = 2;
    }
    else if (((short_code - 0xa0U) & 0xffffU) < 4U) {
        group = 3;
    }
    else {
        frame[0] = 0;
        return 4;
    }

    int32_t first_partition =
        vocoder_lookup_partition_value(
            (uint32_t)group, (int32_t)code, 0);
    int32_t second_partition =
        vocoder_lookup_partition_value(
            (uint32_t)group, (int32_t)code, 1);
    int32_t table_index = (int16_t)(short_code - 0x80U);
    frame[1] = (int16_t)code;
    frame[3] = (int16_t)(
        second_partition + (first_partition << 8));
    frame[6] = spacing_table[table_index];
    int32_t harmonic_count = second_partition + 1;
    if (harmonic_count < 9) {
        harmonic_count = 9;
    }
    if (harmonic_count > 56) {
        harmonic_count = 56;
    }
    frame[2] = (int16_t)harmonic_count;

    if (spread == 0) {
        if (first_level < -0x5000) {
            first_level = -0x5000;
        }
        if (first_level > 0x6fff) {
            first_level = 0x6fff;
        }
        if (second_level < -0x5000) {
            second_level = -0x5000;
        }
        if (second_level > 0x6fff) {
            second_level = 0x6fff;
        }
        frame[first_partition + 8] = (int16_t)first_level;
        frame[second_partition + 8] = (int16_t)second_level;
        return 0;
    }

    int32_t midpoint = frame_saturate_i32(
        (int64_t)second_level * 0x8000 +
        (int64_t)first_level * 0x8000);
    int32_t midpoint_q0 = midpoint >> 16;
    int32_t difference = difference_table[table_index];
    int32_t first = frame_saturate_i32(
        (int64_t)midpoint_q0 * 0x10000 -
        (int64_t)difference * 0x10000) >> 16;
    if (first < -0x5000) {
        first = -0x5000;
    }
    if (first > 0x6fff) {
        first = 0x6fff;
    }
    frame[first_partition + 8] = (int16_t)first;
    int32_t first_delta = frame_saturate_i32(
        (int64_t)first * 0x10000 -
        0x50000000LL);
    vocoder_spread_spectral_peak(
        frame + 8, (int16_t)first_partition, first_delta >> 16);

    int32_t second = frame_saturate_i32(
        (int64_t)difference * 0x10000 +
        (int64_t)midpoint_q0 * 0x10000) >> 16;
    if (second < -0x5000) {
        second = -0x5000;
    }
    if (second > 0x6fff) {
        second = 0x6fff;
    }
    frame[second_partition + 8] = (int16_t)second;
    int32_t second_delta = frame_saturate_i32(
        (int64_t)second * 0x10000 -
        0x50000000LL);
    vocoder_spread_spectral_peak(
        frame + 8, (int16_t)second_partition, second_delta >> 16);
    return 0;
}

int32_t vocoder_decode_spectral_frame_block(
    int16_t frame[64],
    vocoder_frame_state *destination,
    const int16_t **cursor,
    const int16_t *config)
{
    (void)vocoder_read_bits_cursor(cursor, 6);
    int32_t pitch_code = (int32_t)
        vocoder_read_bits_cursor(cursor, 6);
    uint16_t bytes[8] = {0};
    int32_t byte_count =
        ((int32_t)config[2] - 13) >> 3;
    if (byte_count > 0) {
        for (int32_t index = 0;
                index < byte_count; ++index) {
            bytes[index] = (uint16_t)
                vocoder_read_bits_cursor(cursor, 8);
        }
    }

    int32_t errors = 0;
    for (int32_t index = 1; index < byte_count; ++index) {
        uint32_t difference =
            (uint8_t)bytes[index] ^ (uint8_t)bytes[0];
        uint32_t temporary = difference;
        while (temporary != 0U) {
            errors += (int32_t)(temporary & 1U);
            temporary >>= 1;
        }
    }
    uint16_t frame_code = bytes[0];
    if ((((bytes[0] ^ bytes[1]) & 0xf0U) != 0U) ||
            errors > 5) {
        frame_code = UINT16_MAX;
    }

    uint32_t flag = vocoder_read_bits_cursor(cursor, 1);
    uint32_t packed = flag | ((uint32_t)pitch_code << 1);
    uint32_t biased = packed * 0x20000U - 0x110000U;
    int16_t coarse = (int16_t)(
        frame_arithmetic_shift_right(biased, 16U));
    uint32_t level_product =
        (uint32_t)(int32_t)coarse * 0x78f200U;
    int32_t level = (int32_t)
        frame_arithmetic_shift_right(level_product, 16U);
    int32_t result = vocoder_initialize_spectral_frame(
        frame, frame_code, level, level, 0);
    if (result == 0) {
        vocoder_copy_frame_state(
            destination, (const vocoder_frame_state *)frame);
    }
    return result;
}

int32_t vocoder_log_i32_vector(
    int16_t *output,
    const int32_t *input,
    int32_t input_exponent,
    int32_t count)
{
    for (int32_t index = 0; index < count; ++index) {
        uint32_t value = (uint32_t)input[index];
        uint32_t magnitude = (int32_t)value < 0
            ? ~value
            : value;
        output[index * 2] = INT16_MIN;
        output[index * 2 + 1] = 0;
        if (value == 0U) {
            continue;
        }

        uint32_t normalization =
            frame_count_leading_zeros(magnitude) - 1U;
        int16_t mantissa = (int16_t)
            frame_arithmetic_shift_right(
                frame_shift_left(value, normalization), 16U);
        if (mantissa == 0) {
            continue;
        }
        int16_t delta = (int16_t)(mantissa - 23170);

        uint32_t polynomial = 0x8000U;
        polynomial += (uint32_t)(
            (int32_t)-23636 * delta * 2);
        polynomial += (uint32_t)(
            (int32_t)22285 * 0x10000);
        polynomial = (uint32_t)(
            (int32_t)(int16_t)(polynomial >> 16) *
            delta);
        polynomial = 0x8000U + polynomial * 2U;
        polynomial += (uint32_t)(
            (int32_t)-23636 * 0x10000);
        polynomial = (uint32_t)(
            (int32_t)(int16_t)(polynomial >> 16) *
            delta);
        polynomial = 0x8000U + polynomial * 2U;
        polynomial = (uint32_t)(
            (int32_t)16714 * 0x10000) +
            frame_arithmetic_shift_right(polynomial, 1U);
        polynomial = (uint32_t)(
            (int32_t)(int16_t)(polynomial >> 16) *
            delta);
        polynomial = 0x8000U + polynomial * 2U;
        uint32_t polynomial_high = polynomial >> 16;

        uint32_t result = (uint32_t)(
            (int32_t)-16384 * 0x10000);
        result += polynomial_high << 18;
        uint32_t exponent_term = (uint32_t)(
            input_exponent - (int32_t)normalization) << 16;
        result = exponent_term +
            frame_arithmetic_shift_right(result, 15U);

        uint32_t shifted = result << 10;
        if ((int32_t)result != (int32_t)
                frame_arithmetic_shift_right(shifted, 10U)) {
            shifted = ((uint32_t)((int32_t)result >> 31)) ^
                (uint32_t)INT32_MAX;
        }
        int32_t rounded = frame_saturating_add(
            (int32_t)shifted, 0x8000);
        output[index * 2] = (int16_t)
            frame_arithmetic_shift_right(
                (uint32_t)rounded, 16U);
    }
    return 4;
}

void vocoder_transform_32_symmetric(int16_t values[32])
{
    static const int16_t coefficients[480] = {
        32138, 30274, 27246, 23170, 18205, 12540, 6393, 0,
        -6393, -12540, -18205, -23170, -27246, -30274, -32138, -32767,
        -6393, -12540, -18205, -23170, -27246, -30274, -32138, -32767,
        -32138, -30274, -27246, -23170, -18205, -12540, -6393, 0,
        30274, 23170, 12540, 0, -12540, -23170, -30274, -32767,
        -30274, -23170, -12540, 0, 12540, 23170, 30274, 32767,
        -12540, -23170, -30274, -32767, -30274, -23170, -12540, 0,
        12540, 23170, 30274, 32767, 30274, 23170, 12540, 0,
        27246, 12540, -6393, -23170, -32138, -30274, -18205, 0,
        18205, 30274, 32138, 23170, 6393, -12540, -27246, -32767,
        -18205, -30274, -32138, -23170, -6393, 12540, 27246, 32767,
        27246, 12540, -6393, -23170, -32138, -30274, -18205, 0,
        23170, 0, -23170, -32767, -23170, 0, 23170, 32767,
        23170, 0, -23170, -32767, -23170, 0, 23170, 32767,
        -23170, -32767, -23170, 0, 23170, 32767, 23170, 0,
        -23170, -32767, -23170, 0, 23170, 32767, 23170, 0,
        18205, -12540, -32138, -23170, 6393, 30274, 27246, 0,
        -27246, -30274, -6393, 23170, 32138, 12540, -18205, -32767,
        -27246, -30274, -6393, 23170, 32138, 12540, -18205, -32767,
        -18205, 12540, 32138, 23170, -6393, -30274, -27246, 0,
        12540, -23170, -30274, 0, 30274, 23170, -12540, -32767,
        -12540, 23170, 30274, 0, -30274, -23170, 12540, 32767,
        -30274, -23170, 12540, 32767, 12540, -23170, -30274, 0,
        30274, 23170, -12540, -32767, -12540, 23170, 30274, 0,
        6393, -30274, -18205, 23170, 27246, -12540, -32138, 0,
        32138, 12540, -27246, -23170, 18205, 30274, -6393, -32767,
        -32138, -12540, 27246, 23170, -18205, -30274, 6393, 32767,
        6393, -30274, -18205, 23170, 27246, -12540, -32138, 0,
        0, -32767, 0, 32767, 0, -32767, 0, 32767,
        0, -32767, 0, 32767, 0, -32767, 0, 32767,
        -32767, 0, 32767, 0, -32767, 0, 32767, 0,
        -32767, 0, 32767, 0, -32767, 0, 32767, 0,
        -6393, -30274, 18205, 23170, -27246, -12540, 32138, 0,
        -32138, 12540, 27246, -23170, -18205, 30274, 6393, -32767,
        -32138, 12540, 27246, -23170, -18205, 30274, 6393, -32767,
        6393, 30274, -18205, -23170, 27246, 12540, -32138, 0,
        -12540, -23170, 30274, 0, -30274, 23170, 12540, -32767,
        12540, 23170, -30274, 0, 30274, -23170, -12540, 32767,
        -30274, 23170, 12540, -32767, 12540, 23170, -30274, 0,
        30274, -23170, -12540, 32767, -12540, -23170, 30274, 0,
        -18205, -12540, 32138, -23170, -6393, 30274, -27246, 0,
        27246, -30274, 6393, 23170, -32138, 12540, 18205, -32767,
        -27246, 30274, -6393, -23170, 32138, -12540, -18205, 32767,
        -18205, -12540, 32138, -23170, -6393, 30274, -27246, 0,
        -23170, 0, 23170, -32767, 23170, 0, -23170, 32767,
        -23170, 0, 23170, -32767, 23170, 0, -23170, 32767,
        -23170, 32767, -23170, 0, 23170, -32767, 23170, 0,
        -23170, 32767, -23170, 0, 23170, -32767, 23170, 0,
        -27246, 12540, 6393, -23170, 32138, -30274, 18205, 0,
        -18205, 30274, -32138, 23170, -6393, -12540, 27246, -32767,
        -18205, 30274, -32138, 23170, -6393, -12540, 27246, -32767,
        27246, -12540, -6393, 23170, -32138, 30274, -18205, 0,
        -30274, 23170, -12540, 0, 12540, -23170, 30274, -32767,
        30274, -23170, 12540, 0, -12540, 23170, -30274, 32767,
        -12540, 23170, -30274, 32767, -30274, 23170, -12540, 0,
        12540, -23170, 30274, -32767, 30274, -23170, 12540, 0,
        -32138, 30274, -27246, 23170, -18205, 12540, -6393, 0,
        6393, -12540, 18205, -23170, 27246, -30274, 32138, -32767,
        -6393, 12540, -18205, 23170, -27246, 30274, -32138, 32767,
        -32138, 30274, -27246, 23170, -18205, 12540, -6393, 0
    };
    int16_t sums[15];
    int16_t differences[15];
    int16_t first = (int16_t)frame_arithmetic_shift_right(
        (uint32_t)(int32_t)values[0], 1U);
    int32_t total = first;
    for (int32_t index = 0; index < 15; ++index) {
        int32_t left = values[index + 1];
        int32_t right = values[31 - index];
        sums[index] = (int16_t)frame_arithmetic_shift_right(
            (uint32_t)(left + right), 1U);
        differences[index] = (int16_t)
            frame_arithmetic_shift_right(
                (uint32_t)(left - right), 1U);
        total += sums[index];
    }
    int16_t center = (int16_t)frame_arithmetic_shift_right(
        (uint32_t)(int32_t)values[16], 1U);
    values[0] = (int16_t)frame_arithmetic_shift_right(
        (uint32_t)(total + center), 3U);
    values[1] = 0;

    for (int32_t output_pair = 0;
            output_pair < 15; ++output_pair) {
        const int16_t *block =
            coefficients + output_pair * 32;
        int64_t even = (int64_t)first * 0x8000;
        int64_t odd = 0;
        for (int32_t index = 0; index < 15; ++index) {
            even += (int64_t)sums[index] * block[index];
            odd += (int64_t)differences[index] *
                block[16 + index];
        }
        even += (int64_t)center * block[15];
        values[2 + output_pair * 2] = (int16_t)
            frame_arithmetic_shift_right64(even, 18U);
        values[3 + output_pair * 2] = (int16_t)
            frame_arithmetic_shift_right64(odd, 18U);
    }
}

int32_t vocoder_analyze_subframes(
    void *state_pointer,
    int16_t *filtered_output,
    int32_t energy[16],
    int32_t *consumed_samples,
    const int16_t *samples,
    int32_t available_samples,
    int32_t new_samples)
{
    static const int16_t analysis_window[32] = {
        1137, 2214, 3621, 5360, 7417, 9756, 12321, 15041,
        17827, 20581, 23199, 25576, 27614, 29226, 30342, 30913,
        -5686, -566, -2614, -2870, -6966, -2102, -2358, -4918,
        -1846, -3894, -7478, -6454, -6710, -3126, -5942, -6198
    };
    uint8_t *state = (uint8_t *)state_pointer;
    int16_t work[433];
    int16_t *history = work;
    int16_t *spectra = work + 81;
    int16_t input[116];
    uint16_t transformed[64];
    int32_t magnitudes[18];

    int32_t total = new_samples +
        (int32_t)*(uint16_t *)(void *)(state + 0x6dc);
    int32_t subframes = (int32_t)
        frame_arithmetic_shift_right((uint32_t)total << 16, 19U);
    int16_t remainder = (int16_t)(
        total - subframes * 8);
    int32_t input_count = subframes * 8 + 28;
    *(int16_t *)(void *)(state + 0x6dc) = remainder;

    vocoder_copy_i16(
        input,
        samples + available_samples - input_count - remainder,
        input_count);
    vocoder_copy_i16(
        history, (const int16_t *)(const void *)(state + 0x63c),
        80);

    uint16_t *positions =
        (uint16_t *)(void *)(state + 0x62e);
    for (int32_t index = 0; index < 6; ++index) {
        int32_t value =
            (int16_t)positions[index + 1] - subframes;
        positions[index] = (uint16_t)(value > 0 ? value : 0);
    }
    *(int16_t *)(void *)(state + 0x63a) =
        (int16_t)(58 - (total >> 3));
    for (int32_t index = 0; index < 6; ++index) {
        uint16_t *flags =
            (uint16_t *)(void *)(state + 0x620);
        flags[index] =
            (int16_t)positions[index + 1] -
                    (int16_t)positions[index] < 1
                ? 0x8000U
                : flags[index + 1];
    }

    *consumed_samples = subframes * 2;
    vocoder_fill_i16((int16_t *)energy, 0, 32);
    magnitudes[16] = 0;
    vocoder_normalize_i16_vector(
        input, input, input_count, &magnitudes[16]);
    int32_t old_exponent =
        *(int16_t *)(void *)(state + 0x62c);
    if (old_exponent != magnitudes[16]) {
        for (int32_t offset = 0; offset < 80; offset += 16) {
            vocoder_saturating_shift_i16(
                history + offset, history + offset, 16,
                old_exponent, magnitudes[16]);
        }
    }
    int32_t exponent = magnitudes[16];
    *(int16_t *)(void *)(state + 0x62c) = (int16_t)exponent;
    int32_t magnitude_exponent = exponent * 2;

    for (int32_t subframe = 0; subframe < subframes; ++subframe) {
        for (int32_t phase = 0; phase < 2; ++phase) {
            int16_t *window = input + subframe * 8 + phase * 4;
            int16_t *spectrum =
                spectra + subframe * 32 + phase * 16;
            vocoder_multiply_symmetric(
                (int16_t *)transformed, window,
                32, 16, analysis_window, 32);
            vocoder_transform_32_symmetric(
                (int16_t *)transformed);
            spectrum[-1] = (int16_t)(
                transformed[0] &
                (uint16_t)~((int16_t)transformed[0] >> 15));
            vocoder_complex_magnitude_squared(
                magnitudes, (const uint32_t *)transformed, 16);
            for (int32_t index = 0; index < 16; ++index) {
                energy[index] = (int32_t)(
                    (uint32_t)energy[index] +
                    (uint32_t)frame_arithmetic_shift_right(
                        (uint32_t)magnitudes[index], 7U));
            }
            vocoder_sqrt_i32_vector(
                spectrum, (int16_t)exponent,
                magnitudes + 1,
                (uint16_t)magnitude_exponent, 15);
        }
    }

    vocoder_polyphase_filter_16(
        filtered_output, history, subframes);
    vocoder_copy_i16(
        (int16_t *)(void *)(state + 0x63c),
        history + subframes * 32, 80);
    return subframes;
}

void vocoder_accumulate_interpolated_samples(
    uint32_t *phase,
    const int16_t *samples,
    int32_t phase_step,
    int32_t window_phase,
    int32_t **output_cursor,
    int16_t sample_shift,
    int16_t window_step,
    int16_t count)
{
    int32_t *output = *output_cursor;
    for (int32_t item = 0; item < count; ++item) {
        uint32_t current_phase = *phase;
        uint32_t sample_index =
            (current_phase & 0x00ffffffU) >> 16;
        int16_t left = samples[sample_index];
        int16_t right = samples[sample_index + 1U];
        *phase = current_phase +
            (uint32_t)phase_step * 32U;
        int16_t fraction = (int16_t)(
            (uint16_t)current_phase >> 1);
        int16_t difference_half = (int16_t)(
            ((uint32_t)(
                (int32_t)right * 0x8000 -
                (int32_t)left * 0x8000)) >> 16);
        uint32_t interpolated = (uint32_t)(
            (int32_t)left * 0x8000);
        interpolated += (uint32_t)(
            (int32_t)fraction * difference_half * 2);

        if (window_step != 0) {
            uint32_t shifted_phase =
                (uint32_t)window_phase << 10;
            shifted_phase = frame_arithmetic_shift_right(
                shifted_phase, 16U) << 16;
            uint32_t absolute_phase =
                frame_arithmetic_shift_right(
                    shifted_phase, 6U);
            if ((int32_t)absolute_phase < 0) {
                absolute_phase =
                    0U - absolute_phase;
            }
            uint32_t cosine_index = absolute_phase >> 16;
            uint32_t cosine_fraction =
                absolute_phase & 0xffffU;
            int32_t cosine_left =
                frame_cosine_q15(cosine_index);
            int32_t cosine_right =
                frame_cosine_q15(cosine_index + 1U);
            uint32_t cosine_accumulator = (uint32_t)(
                cosine_left * 0x10000);
            cosine_accumulator += (uint32_t)(
                (cosine_right - cosine_left) *
                (int32_t)cosine_fraction);
            int32_t cosine = (int32_t)
                frame_arithmetic_shift_right(
                    cosine_accumulator, 16U);
            int32_t window = (int32_t)
                frame_arithmetic_shift_right(
                    (uint32_t)(
                        (0x8000 - cosine) * 0x8000),
                    16U);
            interpolated = (uint32_t)
                frame_arithmetic_shift_right64(
                    (int64_t)window *
                    (int32_t)interpolated, 15U);
            window_phase = (int32_t)(
                (uint32_t)window_phase +
                (uint32_t)(int32_t)window_step * 0x10000U);
        }

        uint32_t aligned = sample_shift < 0
            ? frame_arithmetic_shift_right(
                interpolated, (uint16_t)-sample_shift)
            : frame_shift_left(
                interpolated, (uint16_t)sample_shift);
        *output = frame_saturate_i32(
            (int64_t)*output + (int32_t)aligned);
        ++output;
    }
    *output_cursor = output;
}

static uint32_t frame_normalize_i48(
    int64_t value,
    int16_t *exponent)
{
    const uint64_t mask = (UINT64_C(1) << 48) - 1U;
    uint64_t bits = (uint64_t)value & mask;
    if ((bits & (UINT64_C(1) << 47)) != 0U) {
        bits |= ~mask;
    }
    if ((int64_t)bits == 0) {
        *exponent = 0;
        return 0U;
    }
    uint32_t low = (uint32_t)bits;
    uint32_t high = (uint32_t)(int32_t)(int16_t)(bits >> 32);
    uint32_t magnitude_high =
        (int32_t)high < 0 ? ~high : high;
    uint32_t magnitude_low =
        (int32_t)high < 0 ? ~low : low;
    int32_t leading = (int32_t)
        frame_count_leading_zeros(magnitude_high);
    if (leading == 32) {
        leading += (int32_t)
            frame_count_leading_zeros(magnitude_low);
    }
    int32_t shift = leading - 33;
    *exponent = (int16_t)-shift;
    if (shift >= 0) {
        return frame_shift_left(low, (uint32_t)shift);
    }
    return (uint32_t)frame_arithmetic_shift_right64(
        (int64_t)bits, (uint32_t)-shift);
}

void vocoder_integrate_resampled_intervals(
    uint32_t *mantissas,
    int16_t *exponents,
    int32_t spacing,
    int32_t count,
    const int32_t table[128])
{
    uint32_t scaled = (uint32_t)spacing << 9;
    int32_t step = (int32_t)
        frame_arithmetic_shift_right(scaled, 4U);
    uint32_t half_scaled = (uint32_t)spacing << 9;
    int32_t position = step + (int32_t)
        frame_arithmetic_shift_right(half_scaled, 5U) +
        0x8000;

    for (int32_t item = 0; item < count; ++item) {
        int32_t start_index = position >> 16;
        uint16_t start_fraction = (uint16_t)position;
        uint32_t next_position =
            (uint32_t)step + (uint32_t)position;
        int32_t end_index = (int32_t)next_position >> 16;
        if (end_index > 127) {
            end_index = 127;
        }
        int64_t integral = -frame_arithmetic_shift_right64(
            (int64_t)(start_fraction >> 1) *
            table[start_index], 15U);
        for (int32_t index = start_index;
                index < end_index; ++index) {
            integral += table[index];
        }
        uint16_t end_fraction =
            (uint16_t)next_position;
        integral += frame_arithmetic_shift_right64(
            (int64_t)(end_fraction >> 1) *
            table[end_index], 15U);
        mantissas[item] =
            frame_normalize_i48(integral, &exponents[item]);
        position = step +
            (int32_t)((uint32_t)end_index << 16 |
                      end_fraction);
    }
}

int32_t vocoder_integrate_resampled_to_end(
    uint32_t *mantissas,
    int16_t *exponents,
    int32_t spacing,
    const int32_t table[128])
{
    uint32_t scaled = (uint32_t)spacing << 9;
    int32_t half_step = (int32_t)
        frame_arithmetic_shift_right(scaled, 4U);
    int32_t step = (int32_t)
        frame_arithmetic_shift_right(scaled, 3U);
    uint32_t position = (uint32_t)half_step + 0x8000U;
    int32_t current_index = (int32_t)position >> 16;
    const int32_t *current = table + current_index;
    int32_t produced = 0;
    int32_t start_value = *current;
    uint32_t start_partial = (uint32_t)
        frame_arithmetic_shift_right64(
            (int64_t)(((uint16_t)position) >> 1) *
            start_value, 15U);
    position = (uint32_t)step + position;
    int32_t interval =
        ((int32_t)position >> 16) - current_index;

    for (;;) {
        int64_t integral = -(int64_t)(int32_t)start_partial;
        for (int32_t offset = 0;
                offset < interval; ++offset) {
            integral += current[offset];
        }
        uint16_t fraction = (uint16_t)position;
        int32_t end_value = current[interval];
        int64_t end_partial = frame_arithmetic_shift_right64(
            (int64_t)(fraction >> 1) *
            end_value, 15U);
        integral += end_partial;
        mantissas[produced] =
            frame_normalize_i48(
                integral, &exponents[produced]);
        ++produced;

        position = (uint32_t)step +
            ((uint32_t)(current_index + interval) << 16 |
             fraction);
        int32_t next_index = (int32_t)position >> 16;
        if (next_index > 127) {
            break;
        }
        int32_t previous_end = current_index + interval;
        start_value = end_value;
        start_partial = (uint32_t)end_partial;
        current += interval;
        interval = next_index - previous_end;
        current_index = previous_end;
    }
    return produced;
}

static int16_t frame_sum_i32_range_normalized(
    const uint32_t *values,
    int32_t first,
    int32_t last,
    int32_t base_exponent,
    int32_t *result_exponent)
{
    if (last < first) {
        *result_exponent = base_exponent;
        return 0;
    }
    int64_t sum = 0;
    for (int32_t index = first; index <= last; ++index) {
        sum += (int32_t)values[index];
    }
    int16_t adjustment;
    uint32_t normalized =
        frame_normalize_i48(sum, &adjustment);
    *result_exponent =
        base_exponent + adjustment;
    return (int16_t)(normalized >> 16);
}

void vocoder_compare_resampled_bands(
    int16_t values[8],
    int16_t spacing,
    const int32_t table[128])
{
    uint32_t first[56] = {0};
    uint32_t second[56] = {0};
    int16_t exponents[56];
    exponents[0] = -30;
    int32_t interval_count =
        vocoder_integrate_resampled_to_end(
            first + 1, exponents + 1, spacing, table);
    int32_t first_exponent =
        vocoder_max_i16(exponents + 1, interval_count);
    for (int32_t index = 1;
            index <= interval_count; ++index) {
        first[index] = frame_arithmetic_shift_right(
            first[index],
            (uint32_t)(first_exponent - exponents[index]));
    }

    exponents[0] = -30;
    vocoder_integrate_resampled_intervals(
        second + 1, exponents + 1,
        spacing, interval_count, table);
    int32_t second_exponent =
        vocoder_max_i16(exponents + 1, interval_count);
    for (int32_t index = 1;
            index <= interval_count; ++index) {
        second[index] = frame_arithmetic_shift_right(
            second[index],
            (uint32_t)(second_exponent - exponents[index]));
    }

    int32_t step_exponent;
    int16_t step_mantissa = vocoder_normalized_divide(
        0x20000000, -3, spacing, -3, &step_exponent);
    uint32_t step_q16 = (uint32_t)(int32_t)step_mantissa << 16;
    if (step_exponent < 15) {
        step_q16 = frame_arithmetic_shift_right(
            step_q16, (uint32_t)(15 - step_exponent));
    }
    else {
        step_q16 = frame_shift_left(
            step_q16, (uint32_t)(step_exponent - 15));
    }

    uint32_t boundary = step_q16 * 2U + 0x8000U;
    int32_t start = 0;
    for (int32_t band = 0; band < 8; ++band) {
        int32_t end = (int32_t)boundary >> 16;
        if (end > interval_count) {
            end = interval_count;
        }
        int32_t second_sum_exponent;
        int32_t first_sum_exponent;
        int16_t second_sum = frame_sum_i32_range_normalized(
            second, start, end,
            second_exponent, &second_sum_exponent);
        int16_t first_sum = frame_sum_i32_range_normalized(
            first, start, end,
            first_exponent, &first_sum_exponent);
        int32_t ratio = vocoder_one_minus_normalized_ratio(
            second_sum, second_sum_exponent,
            first_sum, first_sum_exponent);
        if (ratio < values[band]) {
            values[band] = (int16_t)ratio;
        }
        start = (int32_t)(boundary - step_q16) >> 16;
        boundary += step_q16 * 2U;
    }
}

void vocoder_synthesize_interpolated_segment(
    int32_t *output,
    int16_t amplitude,
    int32_t amplitude_exponent,
    uint16_t spacing,
    const int16_t *samples,
    int16_t output_shift)
{
    uint32_t aligned_amplitude =
        (uint32_t)(int32_t)amplitude << 16;
    if (amplitude_exponent < 15) {
        aligned_amplitude = frame_arithmetic_shift_right(
            aligned_amplitude,
            (uint32_t)(15 - amplitude_exponent));
    }
    else {
        aligned_amplitude = frame_shift_left(
            aligned_amplitude,
            (uint32_t)(amplitude_exponent - 15));
    }
    int32_t spacing_exponent;
    uint32_t spacing_value =
        (uint32_t)spacing << 16;
    uint32_t magnitude = (int32_t)spacing_value < 0
        ? ~spacing_value
        : spacing_value;
    int16_t normalization = spacing == 0
        ? 0
        : (int16_t)(
            frame_count_leading_zeros(magnitude) - 1U);
    int16_t normalized_spacing = (int16_t)(
        frame_shift_left(
            spacing_value, (uint16_t)normalization) >> 16);
    int16_t reciprocal = vocoder_normalized_divide(
        0x40000000, 1, normalized_spacing,
        -4 - normalization, &spacing_exponent);
    int32_t sum_exponent;
    int16_t sum = (int16_t)vocoder_normalized_add(
        amplitude, amplitude_exponent,
        reciprocal, spacing_exponent, &sum_exponent);

    int32_t rounded_amplitude = frame_saturate_i32(
        (int64_t)(int32_t)aligned_amplitude + 0xffff);
    int32_t left_index = rounded_amplitude >> 16;
    uint32_t aligned_sum = (uint32_t)(int32_t)sum << 16;
    if (sum_exponent < 15) {
        aligned_sum = frame_arithmetic_shift_right(
            aligned_sum, (uint32_t)(15 - sum_exponent));
    }
    else {
        aligned_sum = frame_shift_left(
            aligned_sum, (uint32_t)(sum_exponent - 15));
    }
    int32_t right_index = (int32_t)aligned_sum >> 16;
    int32_t limited_right = right_index > 0xa5
        ? 0xa6
        : right_index;
    int32_t final_index;
    if (limited_right < 0x97) {
        final_index = limited_right + 0x10;
        if (final_index < 1) {
            return;
        }
    }
    else {
        final_index = 0xa6;
    }

    uint32_t phase_product = (uint32_t)(
        left_index * 0x10000 -
        (int32_t)aligned_amplitude);
    phase_product *= 0x8000U;
    uint32_t phase = (uint32_t)(
        ((int32_t)(int16_t)spacing *
         (int32_t)(int16_t)(phase_product >> 16) * 2) >> 11);
    int32_t window_phase = (int32_t)
        frame_arithmetic_shift_right(
            phase_product & 0xffff0000U, 15U);
    uint32_t tail_phase = (uint32_t)(
        (int32_t)aligned_sum -
        right_index * 0x10000);
    tail_phase = (uint32_t)(
        frame_arithmetic_shift_right(
            tail_phase * 0x8000U, 15U) &
        0xfffffffeU);
    tail_phase += 0x000f0000U;

    int32_t *cursor;
    int32_t current_index;
    if (left_index < 0) {
        phase += (uint32_t)(
            -(int32_t)(int16_t)(
                (uint32_t)rounded_amplitude >> 16) *
            (int32_t)(int16_t)spacing * 0x20);
        window_phase += left_index < -0x10
            ? 0x100000
            : -left_index * 0x10000;
        cursor = output;
        current_index = 0;
        if (limited_right < 0) {
            tail_phase += (uint32_t)(
                (limited_right + 1) * 0x10000);
        }
    }
    else {
        cursor = output + left_index;
        current_index = left_index;
    }

    int32_t first_count =
        (left_index + 0x10) - current_index;
    int16_t sample_shift = (int16_t)(output_shift - 0x10);
    if (first_count > 0) {
        vocoder_accumulate_interpolated_samples(
            &phase, samples, (int16_t)spacing,
            window_phase, &cursor, sample_shift,
            1, (int16_t)first_count);
        current_index = left_index + 0x10;
    }
    int32_t middle_count =
        (limited_right - current_index) + 1;
    if (middle_count > 0) {
        vocoder_accumulate_interpolated_samples(
            &phase, samples, (int16_t)spacing,
            0, &cursor, sample_shift,
            0, (int16_t)middle_count);
        current_index += middle_count;
    }
    int32_t last_count =
        final_index - current_index + 1;
    vocoder_accumulate_interpolated_samples(
        &phase, samples, (int16_t)spacing,
        (int32_t)tail_phase, &cursor, sample_shift,
        -1, (int16_t)last_count);
}

void vocoder_select_frame_band_candidate(
    vocoder_frame_state *frame,
    int16_t *weights,
    int16_t *lower_bounds,
    int16_t quantized[8],
    int32_t slice_index,
    int32_t spectrum_index,
    const vocoder_band_candidate_config *configuration)
{
    int32_t band = slice_index + spectrum_index;
    int32_t neighbor_distance = band < 2 ? 1 : 2;
    uint32_t shape_flag = 0;
    vocoder_compute_band_weights(
        weights, lower_bounds,
        configuration->band_values,
        configuration->band_basis,
        configuration->band_mask,
        band, neighbor_distance, &shape_flag);
    vocoder_quantize_log_amplitudes(
        quantized,
        configuration->quantized_inputs + band * 8,
        configuration->quantized_exponents[band], 0);
    frame->header[6] = (uint16_t)
        vocoder_select_spectral_candidate(
            configuration->candidates,
            configuration->scores,
            configuration->target, band,
            1 - slice_index);
}

int32_t vocoder_build_inverse_spectrum(
    int16_t output[258],
    int16_t *output_exponent,
    int32_t spacing,
    const int16_t *phase_offsets,
    const int16_t *labels,
    const int16_t *amplitudes,
    int16_t amplitude_exponent,
    int32_t end,
    int32_t start,
    int32_t selected_label)
{
    if (start >= end) {
        vocoder_fill_i16(output, 0, 256);
        *output_exponent = -0x20;
        output[256] = output[0];
        return 0;
    }

    uint32_t maximum = 0;
    for (int32_t index = start; index < end; ++index) {
        if (labels[index] == selected_label) {
            uint32_t candidate =
                (uint32_t)(int32_t)amplitudes[index] << 16;
            if ((int32_t)maximum < (int32_t)candidate) {
                maximum = candidate;
            }
        }
    }
    vocoder_fill_i16(output, 0, 256);
    uint32_t magnitude = (int32_t)maximum < 0
        ? ~maximum
        : maximum;
    int32_t normalization = maximum == 0U
        ? 0
        : (int32_t)frame_count_leading_zeros(magnitude) - 1;
    int32_t inverse_normalization = -normalization;
    int32_t matches = 0;

    for (int32_t index = start; index < end; ++index) {
        if (labels[index] != selected_label) {
            continue;
        }
        int32_t output_index = index + 1;
        uint32_t phase = (uint32_t)(
            output_index * spacing + phase_offsets[index]);
        phase = phase * 0x400U + 0x8000U;
        uint32_t folded =
            (phase & 0x80000000U) +
            (phase & 0x01ffffffU) * 0x40U;
        int32_t quantized_phase = (int32_t)
            frame_arithmetic_shift_right(folded, 6U);
        if (quantized_phase < 0) {
            quantized_phase = -(int32_t)(
                (uint32_t)-quantized_phase &
                0xffff0000U);
        }
        else {
            quantized_phase = (int32_t)(
                (uint32_t)quantized_phase &
                0xffff0000U);
        }
        uint32_t cosine_index =
            ((uint32_t)quantized_phase &
             0x01ffffffU) >> 16;
        int32_t cosine_product =
            (int32_t)amplitudes[index] *
            frame_cosine_q15(cosine_index) * 2;
        int32_t sine_product =
            (int32_t)amplitudes[index] *
            frame_cosine_q15(
                (cosine_index + 384U) & 0x1ffU) * 2;
        uint32_t aligned_cosine = normalization < 0
            ? frame_arithmetic_shift_right(
                (uint32_t)cosine_product,
                (uint32_t)inverse_normalization)
            : frame_shift_left(
                (uint32_t)cosine_product,
                (uint32_t)normalization);
        uint32_t aligned_sine = normalization < 0
            ? frame_arithmetic_shift_right(
                (uint32_t)sine_product,
                (uint32_t)inverse_normalization)
            : frame_shift_left(
                (uint32_t)sine_product,
                (uint32_t)normalization);
        output[output_index * 2] =
            (int16_t)(aligned_cosine >> 16);
        output[output_index * 2 + 1] =
            (int16_t)(aligned_sine >> 16);
        ++matches;
    }

    if (matches == 0) {
        *output_exponent = -0x20;
    }
    else {
        *output_exponent = (int16_t)(
            amplitude_exponent + 0x17 +
            inverse_normalization);
        *output_exponent = (int16_t)
            vocoder_real_fft_inverse_64(
                output, *output_exponent, 8, 0);
    }
    output[256] = output[0];
    return matches;
}

void vocoder_synthesize_harmonic_series(
    int32_t *output,
    uint16_t *last_mantissa,
    int16_t *last_exponent,
    uint16_t spacing,
    const int16_t *samples,
    int16_t output_shift,
    int32_t phase_offset,
    int16_t slope,
    int16_t base,
    int32_t scale,
    int32_t count)
{
    uint16_t mantissas[20] = {0};
    int16_t exponents[20] = {0};
    if (count > 0) {
        uint32_t base_square = (uint32_t)(
            (int32_t)base * base * 2);
        uint32_t scale_square =
            (uint32_t)scale * (uint32_t)scale * 2U;
        int32_t base_shift =
            frame_fixed_normalization_shift(base_square);
        int32_t scale_shift =
            frame_fixed_normalization_shift(scale_square);
        int16_t normalized_base_square = (int16_t)(
            frame_shift_left(
                base_square, (uint32_t)base_shift) >> 16);
        int16_t normalized_scale_square = (int16_t)(
            frame_shift_left(
                scale_square, (uint32_t)scale_shift) >> 16);
        uint32_t square_product = (uint32_t)(
            (int32_t)normalized_base_square *
            normalized_scale_square * 2);
        int32_t product_shift =
            frame_fixed_normalization_shift(square_product);
        uint32_t cross_product = (uint32_t)(
            (int32_t)base * scale * 2);
        int32_t cross_shift =
            frame_fixed_normalization_shift(cross_product);
        int32_t square_exponent =
            0x16 - base_shift - scale_shift - product_shift;
        uint32_t scale_q16 = (uint32_t)scale << 16;

        for (int32_t item = 0; item < count; ++item) {
            uint32_t delta = (uint32_t)(
                (item + 1) * 0x10000 - phase_offset);
            int32_t delta_shift =
                frame_fixed_normalization_shift(delta);
            int16_t normalized_delta = (int16_t)(
                frame_shift_left(
                    delta, (uint32_t)delta_shift) >> 16);
            uint32_t delta_scale = (uint32_t)(
                (int32_t)normalized_delta * scale * 2);
            int32_t delta_scale_shift =
                frame_fixed_normalization_shift(delta_scale);
            uint32_t normalized_delta_scale =
                frame_shift_left(
                    delta_scale,
                    (uint32_t)delta_scale_shift);
            uint32_t slope_product = (uint32_t)(
                (int32_t)(int16_t)(
                    normalized_delta_scale >> 16) *
                slope * 2);
            int32_t delta_exponent =
                0x1f - delta_shift -
                delta_scale_shift;
            int32_t slope_shift =
                frame_fixed_normalization_shift(slope_product);

            int32_t work_exponent = square_exponent;
            int16_t combined = (int16_t)vocoder_normalized_add(
                (int16_t)(
                    frame_shift_left(
                        square_product,
                        (uint32_t)product_shift) >> 16),
                square_exponent,
                (int16_t)(
                    frame_shift_left(
                        slope_product,
                        (uint32_t)slope_shift) >> 16),
                delta_exponent - 4 - slope_shift,
                &work_exponent);
            uint32_t root = vocoder_normalized_sqrt(
                (uint32_t)(uint16_t)combined << 16,
                &work_exponent);
            int32_t root_shift =
                frame_fixed_normalization_shift(root);
            work_exponent -= root_shift;
            int16_t denominator = (int16_t)
                vocoder_normalized_add(
                    (int16_t)(
                        frame_shift_left(
                            cross_product,
                            (uint32_t)cross_shift) >> 16),
                    0x0b - cross_shift,
                    (int16_t)(
                        frame_shift_left(
                            root,
                            (uint32_t)root_shift) >> 16),
                    work_exponent, &work_exponent);
            uint16_t quotient = (uint16_t)
                vocoder_normalized_divide(
                    (int32_t)(
                        normalized_delta_scale &
                        0xffff0000U),
                    delta_exponent,
                    denominator, work_exponent,
                    &work_exponent);
            uint32_t aligned =
                (uint32_t)quotient << 16;
            if (work_exponent < 15) {
                aligned = frame_arithmetic_shift_right(
                    aligned,
                    (uint32_t)(15 - work_exponent));
            }
            else {
                aligned = frame_shift_left(
                    aligned,
                    (uint32_t)(work_exponent - 15));
            }
            if ((int32_t)(aligned - scale_q16) >= 0) {
                int32_t scale_normalization =
                    frame_fixed_normalization_shift(
                        scale_q16);
                work_exponent =
                    15 - scale_normalization;
                quotient = (uint16_t)(
                    frame_shift_left(
                        scale_q16,
                        (uint32_t)scale_normalization) >> 16);
            }
            mantissas[item] = quotient;
            exponents[item] = (int16_t)work_exponent;
        }

        *last_mantissa = mantissas[count - 1];
        *last_exponent = exponents[count - 1];
        for (int32_t item = 0; item < count; ++item) {
            int16_t call_mantissa = (int16_t)mantissas[item];
            int32_t call_exponent = exponents[item];
            vocoder_synthesize_interpolated_segment(
                output, call_mantissa,
                call_exponent, spacing, samples,
                output_shift);
        }
    }
}

static void *frame_load_packed_pointer(
    const uint8_t *bytes, size_t offset)
{
    uint32_t address;
    memcpy(&address, bytes + offset, sizeof(address));
    return (void *)(uintptr_t)address;
}

static int32_t frame_transition_step(int32_t count)
{
    uint32_t bits = (uint32_t)count << 16;
    int32_t normalization = bits == 0U ? 0 :
        (int32_t)frame_count_leading_zeros(
            (int32_t)bits < 0 ? ~bits : bits) - 1;
    int16_t normalized = (int16_t)(
        frame_shift_left(bits, (unsigned int)normalization) >> 16);
    int32_t exponent;
    int16_t reciprocal = vocoder_normalized_divide(
        0x40000000, 1, normalized,
        15 - normalization, &exponent);
    uint32_t step = (uint32_t)(uint16_t)reciprocal << 16;
    return (int32_t)(exponent < 0
        ? frame_arithmetic_shift_right(
            step, (unsigned int)(-exponent))
        : frame_shift_left(step, (unsigned int)exponent));
}

static void frame_crossfade_i32(
    int32_t *output,
    const int32_t *samples,
    int32_t count,
    int32_t step,
    int32_t fade_in)
{
    int32_t phase = 0;
    for (int32_t index = 0; index < count; ++index) {
        int16_t fraction = (int16_t)
            frame_arithmetic_shift_right((uint32_t)phase, 16);
        int64_t product =
            (int64_t)fraction * samples[index];
        int32_t scaled = (int32_t)
            frame_arithmetic_shift_right64(
                product, 15);
        output[index] = frame_saturating_add(
            output[index], fade_in != 0
                ? scaled : samples[index] - scaled);
        phase = frame_saturating_add(phase, step);
    }
}

static void synthesize_frame_transition_native(
    int32_t *output,
    int32_t count,
    void *synthesis_state_pointer,
    const vocoder_frame_record *current,
    const vocoder_frame_record *previous,
    int16_t pitch,
    const int16_t *labels_override)
{
    uint8_t *state = (uint8_t *)synthesis_state_pointer;
    const uint8_t *frame =
        (const uint8_t *)(const void *)current;
    int32_t history[168];
    int32_t generated[168];
    memcpy(history, state, 84U * sizeof(history[0]));
    memset(history + 84, 0,
           84U * sizeof(history[0]));
    memset(generated, 0, sizeof(generated));
    uint32_t current_mask;
    uint32_t previous_mask;
    memcpy(&current_mask, frame + 8, sizeof(current_mask));
    memcpy(&previous_mask,
           (const uint8_t *)(const void *)previous + 8,
           sizeof(previous_mask));
    int32_t current_voiced =
        (current_mask & 0x55555555U) != 0U;
    int32_t previous_voiced =
        (previous_mask & 0x55555555U) != 0U;
    int16_t residual_mantissa;
    int32_t residual_exponent_word;
    memcpy(&residual_mantissa, state + 0x154,
           sizeof(residual_mantissa));
    memcpy(&residual_exponent_word, state + 0x158,
           sizeof(residual_exponent_word));
    int16_t residual_exponent =
        (int16_t)residual_exponent_word;
    int32_t initial_phase;
    memcpy(&initial_phase, state + 0x150,
           sizeof(initial_phase));
    int32_t phase_residual = 0;
    int32_t transition_harmonics = 0;
    int16_t transition_slope = 0;
    int16_t transition_base = 0;
    int32_t transition_phase_total = 0;
    int32_t octave_transition = 0;
    if (previous_voiced != 0) {
        int16_t previous_spacing;
        memcpy(&previous_spacing,
               (const uint8_t *)(const void *)previous + 0x0c,
               sizeof(previous_spacing));
        int16_t next_spacing = previous_spacing;
        if (current_voiced != 0) {
            memcpy(&next_spacing, frame + 0x0c,
                   sizeof(next_spacing));
            int32_t previous_value = previous_spacing;
            int32_t next_value = next_spacing;
            if (previous_value <
                    (next_value * 0x4ccc >> 15) &&
                    (next_value * 0x6666 >> 16) <
                        previous_value &&
                    previous_value < 0x4000) {
                octave_transition = 1;
                next_spacing =
                    (int16_t)(next_value >> 1);
            }
            else if ((previous_value * 0x3555 >> 15) <
                    next_value &&
                    next_value <
                        (previous_value * 5 >> 3) &&
                    next_value < 0x4000) {
                octave_transition = -1;
                next_spacing =
                    (int16_t)(next_value * 2);
            }
        }
        int32_t phase_increment;
        if (current_voiced != 0) {
            int32_t averaged = (int32_t)
                frame_arithmetic_shift_right(
                    (uint32_t)(
                        (int32_t)previous_spacing +
                        (int32_t)next_spacing),
                    1);
            phase_increment = (int32_t)
                frame_arithmetic_shift_right(
                    (uint32_t)(averaged * count * 2), 4);
            transition_slope = (int16_t)(
                (uint16_t)next_spacing -
                (uint16_t)previous_spacing);
        }
        else {
            phase_increment = (int32_t)
                frame_arithmetic_shift_right(
                    (uint32_t)(
                        (int32_t)previous_spacing * count * 2),
                    4);
        }
        transition_base = previous_spacing;
        transition_phase_total = frame_saturating_add(
            initial_phase,
            phase_increment);
        transition_harmonics = (int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)transition_phase_total, 16);
        if (transition_harmonics > 0 &&
                transition_harmonics <= 20) {
            uint16_t last_mantissa =
                (uint16_t)residual_mantissa;
            int16_t last_exponent = residual_exponent;
            int16_t sample_exponent;
            memcpy(&sample_exponent, state + 0x1ce,
                   sizeof(sample_exponent));
            vocoder_synthesize_harmonic_series(
                history, &last_mantissa, &last_exponent,
                (uint16_t)previous_spacing,
                (const int16_t *)(const void *)(state + 0x1d0),
                sample_exponent, initial_phase,
                transition_slope,
                previous_spacing, count,
                transition_harmonics);
            residual_mantissa =
                (int16_t)last_mantissa;
            residual_exponent = last_exponent;
        }
        phase_residual = transition_phase_total -
            transition_harmonics * 0x10000;
    }
    int32_t step = frame_transition_step(count);
    frame_crossfade_i32(output, history, count, step, 0);

    int16_t spacing;
    int16_t shift;
    int16_t harmonic_count;
    memcpy(&spacing, frame + 0x0c, sizeof(spacing));
    memcpy(&shift, frame + 0x0e, sizeof(shift));
    memcpy(&harmonic_count, frame + 0x04,
           sizeof(harmonic_count));
    int16_t *phase_offsets =
        (int16_t *)(void *)(state + 0x15e);
    int16_t *samples =
        (int16_t *)(void *)(state + 0x1d0);
    int16_t *sample_exponent =
        (int16_t *)(void *)(state + 0x1ce);
    const int16_t *labels = labels_override;
    if (labels == NULL) {
        labels = (const int16_t *)
            frame_load_packed_pointer(frame, 0x80);
    }
    const int16_t *amplitudes =
        (const int16_t *)(const void *)(frame + 0x10);
    int32_t amplitude_exponent_word;
    memcpy(&amplitude_exponent_word, frame + 0x84,
           sizeof(amplitude_exponent_word));
    int16_t amplitude_exponent =
        (int16_t)amplitude_exponent_word;

    uint32_t pitch_bits =
        (uint32_t)(int32_t)pitch << 16;
    uint32_t spacing_bits =
        (uint32_t)(int32_t)spacing << 16;
    int32_t pitch_norm = pitch_bits == 0U ? 0 :
        (int32_t)frame_count_leading_zeros(
            (int32_t)pitch_bits < 0
                ? ~pitch_bits : pitch_bits) - 1;
    int32_t spacing_norm = spacing_bits == 0U ? 0 :
        (int32_t)frame_count_leading_zeros(
            (int32_t)spacing_bits < 0
                ? ~spacing_bits : spacing_bits) - 1;
    int16_t normalized_pitch = (int16_t)(
        frame_shift_left(
            pitch_bits, (unsigned int)pitch_norm) >> 16);
    int16_t normalized_spacing = (int16_t)(
        frame_shift_left(
            spacing_bits, (unsigned int)spacing_norm) >> 16);
    int32_t ratio_exponent;
    int16_t ratio = vocoder_normalized_divide(
        (int32_t)((uint32_t)(uint16_t)normalized_pitch << 16),
        -4 - pitch_norm, normalized_spacing,
        -4 - spacing_norm, &ratio_exponent);
    uint32_t aligned_ratio =
        (uint32_t)(uint16_t)ratio << 16;
    aligned_ratio = ratio_exponent < 15
        ? frame_arithmetic_shift_right(
            aligned_ratio,
            (unsigned int)(15 - ratio_exponent))
        : frame_shift_left(
            aligned_ratio,
            (unsigned int)(ratio_exponent - 15));
    int32_t selected_label =
        (int16_t)((aligned_ratio + 0x8000U) >> 16);

    int32_t matches = vocoder_build_inverse_spectrum(
        samples, sample_exponent, -(int32_t)spacing,
        phase_offsets, labels, amplitudes,
        amplitude_exponent, harmonic_count,
        selected_label, 2);
    if (matches != 0) {
        uint32_t position = (uint32_t)count * 0x10000U +
            (uint32_t)(int32_t)shift * 0x100U -
            0x100000U;
        int32_t norm = position == 0U ? 0 :
            (int32_t)frame_count_leading_zeros(
                (int32_t)position < 0
                    ? ~position : position) - 1;
        int16_t mantissa = (int16_t)(
            frame_shift_left(
                position, (unsigned int)norm) >> 16);
        vocoder_synthesize_interpolated_segment(
            generated, mantissa, 15 - norm,
            (uint16_t)spacing, samples, *sample_exponent);
    }

    if (octave_transition < 0) {
        int32_t adjusted_initial = (int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)initial_phase, 1);
        transition_phase_total = adjusted_initial +
            (int32_t)frame_arithmetic_shift_right(
                (uint32_t)(
                    transition_phase_total - initial_phase), 1);
        initial_phase = adjusted_initial;
        transition_slope =
            (int16_t)(transition_slope >> 1);
        transition_base =
            (int16_t)(transition_base >> 1);
        transition_harmonics = (int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)transition_phase_total, 16);
    }
    else if (octave_transition > 0) {
        int32_t adjusted_initial = initial_phase * 2;
        if (adjusted_initial > 0xffff) {
            adjusted_initial -= 0x10000;
            int32_t numerator = adjusted_initial == INT32_MIN
                ? 0x3fffffff
                : (int32_t)frame_arithmetic_shift_right(
                    (uint32_t)(-adjusted_initial), 1);
            uint32_t numerator_bits =
                (uint32_t)numerator;
            int32_t numerator_norm = numerator_bits == 0U
                ? 0
                : (int32_t)frame_count_leading_zeros(
                    (int32_t)numerator_bits < 0
                        ? ~numerator_bits
                        : numerator_bits) - 1;
            uint32_t denominator_bits =
                (uint32_t)(int32_t)transition_base << 16;
            int32_t denominator_norm =
                denominator_bits == 0U ? 0 :
                (int32_t)frame_count_leading_zeros(
                    (int32_t)denominator_bits < 0
                        ? ~denominator_bits
                        : denominator_bits) - 1;
            int32_t adjusted_exponent;
            int16_t adjusted_mantissa =
                vocoder_normalized_divide(
                    (int32_t)frame_shift_left(
                        numerator_bits,
                        (unsigned int)numerator_norm),
                    15 - numerator_norm,
                    (int16_t)(
                        frame_shift_left(
                            denominator_bits,
                            (unsigned int)denominator_norm) >>
                        16),
                    -4 - denominator_norm,
                    &adjusted_exponent);
            memcpy(state + 0x154,
                   &adjusted_mantissa,
                   sizeof(adjusted_mantissa));
            memcpy(state + 0x158,
                   &adjusted_exponent,
                   sizeof(adjusted_exponent));
        }
        transition_phase_total = adjusted_initial +
            (transition_phase_total - initial_phase) * 2;
        initial_phase = adjusted_initial;
        transition_slope = (int16_t)(
            ((uint16_t)transition_slope & 0x7fffU) << 1);
        transition_base =
            (int16_t)((uint16_t)transition_base << 1);
        transition_harmonics = (int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)transition_phase_total, 16);
    }

    if (current_voiced != 0) {
        int32_t phase_total;
        int32_t harmonic_series_count;
        if (previous_voiced != 0) {
            phase_total = transition_phase_total;
            harmonic_series_count =
                transition_harmonics;
        }
        else {
            phase_total = frame_saturating_add(
                initial_phase,
                (int32_t)frame_arithmetic_shift_right(
                    (uint32_t)(
                        (int32_t)spacing * count * 2),
                    4));
            harmonic_series_count =
                (int32_t)frame_arithmetic_shift_right(
                    (uint32_t)phase_total, 16);
            transition_base = spacing;
        }
        (void)vocoder_build_inverse_spectrum(
            samples, sample_exponent, -(int32_t)spacing,
            phase_offsets, labels, amplitudes,
            amplitude_exponent, harmonic_count, 0, 1);
        int16_t synthesis_mantissa;
        int32_t synthesis_exponent_word;
        memcpy(&synthesis_mantissa, state + 0x154,
               sizeof(synthesis_mantissa));
        memcpy(&synthesis_exponent_word, state + 0x158,
               sizeof(synthesis_exponent_word));
        vocoder_synthesize_interpolated_segment(
            generated, synthesis_mantissa,
            (int16_t)synthesis_exponent_word,
            (uint16_t)spacing,
            samples, *sample_exponent);
        if (harmonic_series_count > 0 &&
                harmonic_series_count <= 20) {
            uint16_t last_mantissa =
                (uint16_t)residual_mantissa;
            int16_t last_exponent = residual_exponent;
            vocoder_synthesize_harmonic_series(
                generated, &last_mantissa,
                &last_exponent, (uint16_t)spacing,
                samples, *sample_exponent,
                initial_phase, transition_slope,
                transition_base,
                count, harmonic_series_count);
            residual_mantissa =
                (int16_t)last_mantissa;
            residual_exponent = last_exponent;
        }
        phase_residual = phase_total -
            harmonic_series_count * 0x10000;
    }
    memcpy(state + 0x150, &phase_residual,
           sizeof(phase_residual));
    frame_crossfade_i32(output, generated, count, step, 1);
    memcpy(state, generated + count,
           84U * sizeof(generated[0]));

    if (current_voiced != 0 || previous_voiced != 0) {
        uint32_t count_bits = (uint32_t)count << 16;
        int32_t count_norm = count_bits == 0U ? 0 :
            (int32_t)frame_count_leading_zeros(
                (int32_t)count_bits < 0
                    ? ~count_bits : count_bits) - 1;
        int16_t normalized_count = (int16_t)(
            frame_shift_left(
                count_bits,
                (unsigned int)count_norm) >> 16);
        int32_t new_exponent;
        int16_t new_mantissa =
            (int16_t)vocoder_normalized_subtract(
                normalized_count, 15 - count_norm,
                residual_mantissa, residual_exponent,
                &new_exponent);
        memcpy(state + 0x154, &new_mantissa,
               sizeof(new_mantissa));
        memcpy(state + 0x158, &new_exponent,
               sizeof(new_exponent));
    }
    else {
        int16_t zero_mantissa = 0;
        int32_t zero_exponent = 0;
        memcpy(state + 0x154, &zero_mantissa,
               sizeof(zero_mantissa));
        memcpy(state + 0x158, &zero_exponent,
               sizeof(zero_exponent));
    }
}

void vocoder_synthesize_frame_transition(
    int32_t *output,
    int32_t count,
    void *synthesis_state_pointer,
    const vocoder_frame_record *current,
    const vocoder_frame_record *previous,
    int16_t pitch)
{
    synthesize_frame_transition_native(
        output, count, synthesis_state_pointer,
        current, previous, pitch, NULL);
}

static const int16_t frame_overlap_window[81] = {
    0, 3664, 5181, 6345, 7327, 8192, 8974, 9693,
    10362, 10991, 11585, 12151, 12691, 13209, 13708, 14189,
    14654, 15105, 15543, 15969, 16384, 16789, 17184, 17570,
    17948, 18318, 18681, 19037, 19386, 19729, 20066, 20398,
    20724, 21046, 21362, 21674, 21981, 22285, 22584, 22879,
    23170, 23458, 23743, 24024, 24301, 24576, 24848, 25116,
    25382, 25645, 25905, 26163, 26418, 26671, 26922, 27170,
    27416, 27659, 27901, 28140, 28378, 28613, 28847, 29079,
    29309, 29537, 29763, 29988, 30211, 30432, 30652, 30870,
    31086, 31302, 31515, 31727, 31938, 32148, 32356, 32563,
    32767
};

static int32_t frame_shift_product_saturating(
    int32_t value,
    int32_t exponent)
{
    if (exponent < 3) {
        return (int32_t)frame_arithmetic_shift_right(
            (uint32_t)value, (unsigned int)(2 - exponent));
    }
    unsigned int shift = (unsigned int)(exponent - 2);
    if (shift >= 31U) {
        return value < 0 ? INT32_MIN : INT32_MAX;
    }
    return frame_saturate_i32(
        (int64_t)value * ((int64_t)1 << shift));
}

static void synthesize_excitation_frame_native(
    int32_t *output,
    int32_t count,
    int16_t *overlap_state,
    const vocoder_frame_record *frame_record,
    int16_t cutoff,
    const int16_t *active_override)
{
    const vocoder_frame_state *frame = &frame_record->state;
    int16_t window[86];
    uint32_t count_bits = (uint32_t)count << 16;
    uint32_t count_magnitude =
        (int32_t)count_bits < 0
            ? ~count_bits : count_bits;
    int32_t count_normalization = count_bits == 0U
        ? 0
        : (int32_t)frame_count_leading_zeros(
            count_magnitude) - 1;
    int16_t normalized_count = (int16_t)(
        frame_shift_left(
            count_bits,
            (unsigned int)count_normalization) >> 16);
    int32_t step_exponent;
    int16_t step_mantissa = vocoder_normalized_divide(
        0x50000000, -8, normalized_count,
        -count_normalization, &step_exponent);
    uint32_t step =
        (uint32_t)(uint16_t)step_mantissa << 16;
    step = step_exponent < 15
        ? frame_arithmetic_shift_right(
            step, (unsigned int)(15 - step_exponent))
        : frame_shift_left(
            step, (unsigned int)(step_exponent - 15));
    uint32_t phase = 0;
    window[0] = frame_overlap_window[0];
    for (int32_t index = 1; index < count; ++index) {
        phase += step;
        unsigned int table_index = phase >> 16;
        uint32_t fraction = (phase & 0xffffU) >> 1;
        int32_t interpolated =
            (int32_t)frame_overlap_window[table_index] * 0x10000 +
            (int32_t)frame_overlap_window[table_index] *
                (int32_t)fraction * -2 +
            (int32_t)fraction *
                frame_overlap_window[table_index + 1U] * 2;
        window[index] = (int16_t)
            frame_arithmetic_shift_right(
                (uint32_t)interpolated, 16);
    }
    window[count] = frame_overlap_window[80];

    int16_t previous_exponent = overlap_state[169];
    int16_t generated[256] = {0};
    int32_t generated_exponent = 0;
    if ((int16_t)frame->header[0] == 3) {
        vocoder_fill_i16(overlap_state + 1, 0, 84);
    }
    else {
        for (int32_t index = 0; index < count; ++index) {
            generated[index] = (int16_t)
                frame_multiply_q15_truncated(
                    overlap_state[index + 1],
                    window[index]);
        }
        int32_t retained = 84 - count;
        for (int32_t index = 0; index < retained; ++index) {
            overlap_state[index + 1] =
                overlap_state[index + count + 1];
        }
        for (int32_t index = 0; index < count; ++index) {
            int16_t noise = (int16_t)(
                (uint16_t)overlap_state[0] * 0x00adU +
                0x3619U);
            overlap_state[0] = noise;
            overlap_state[retained + index + 1] = noise;
        }
        for (int32_t index = 0; index < count; ++index) {
            generated[count + index] = (int16_t)
                frame_multiply_q15_truncated(
                    overlap_state[index + 1],
                    window[count - index]);
        }

        int32_t fft_size = 256;
        int32_t fft_log2 = 8;
        generated_exponent =
            vocoder_real_fft_forward_64(
                generated, 0, fft_log2, 0);
        int32_t half_size = fft_size >> 1;
        int16_t harmonic_count =
            (int16_t)frame->header[2];
        int16_t spacing = (int16_t)frame->header[6];
        const int16_t *active = active_override;
        if (active == NULL) {
            active = (const int16_t *)frame_load_packed_pointer(
                    (const uint8_t *)(const void *)frame_record,
                    0x80);
        }
        int32_t frame_exponent;
        memcpy(&frame_exponent,
               (const uint8_t *)(const void *)frame_record +
                   0x84,
               sizeof(frame_exponent));

        uint32_t maximum_q16 = 0;
        for (int32_t index = 0;
                index < harmonic_count; ++index) {
            if (active[index] == 0) {
                uint32_t candidate =
                    (uint32_t)frame->spectrum[index] << 16;
                if ((int32_t)candidate >
                        (int32_t)maximum_q16) {
                    maximum_q16 = candidate;
                }
            }
        }
        int32_t maximum_shift = maximum_q16 == 0U
            ? 0
            : (int32_t)frame_count_leading_zeros(
                (int32_t)maximum_q16 < 0
                    ? ~maximum_q16 : maximum_q16) - 1;

        int32_t scale_exponent = 17;
        uint32_t root_input;
        if ((int16_t)frame->header[0] == 2) {
            root_input = (uint32_t)(
                harmonic_count * 0x86b6);
        }
        else {
            root_input = (uint32_t)(int16_t)(
                harmonic_count * count) * 0x86b6U;
        }
        uint32_t root = vocoder_normalized_sqrt(
            root_input, &scale_exponent);
        int16_t base_scale = (int16_t)(root >> 16);
        if ((int16_t)frame->header[0] == 2) {
            scale_exponent += generated_exponent + 1;
            base_scale = (int16_t)(
                ((int32_t)base_scale * 0x57fa) >> 15);
        }

        int32_t bin_step =
            fft_size * spacing * 2 >> 4;
        int32_t boundary = bin_step + 0x10000;
        int32_t previous_bin = boundary >> 17;
        boundary >>= 1;
        int32_t common_exponent =
            (scale_exponent - maximum_shift) + 2;
        vocoder_fill_i16(generated, 0, previous_bin * 2);
        for (int32_t harmonic = 0;
                harmonic < harmonic_count; ++harmonic) {
            boundary += bin_step;
            int32_t end_bin = boundary >> 16;
            if (end_bin > half_size) {
                end_bin = half_size;
            }
            int32_t bin_count = end_bin - previous_bin;
            int16_t *band = generated + previous_bin * 2;
            if (active[harmonic] == 0) {
                int32_t multiplier;
                int32_t multiplier_exponent;
                if ((int16_t)frame->header[0] == 2) {
                    multiplier = (int32_t)
                        base_scale *
                        frame->spectrum[harmonic] * 2;
                    multiplier_exponent =
                        scale_exponent;
                }
                else {
                    int32_t energy_exponent;
                    int32_t energy = vocoder_vector_energy(
                        &energy_exponent, band,
                        (uint32_t)(bin_count * 2));
                    int16_t energy_mantissa = (int16_t)
                        frame_arithmetic_shift_right(
                            (uint32_t)energy, 16);
                    int16_t factor = 0;
                    int32_t factor_exponent = 0;
                    if (energy_mantissa > 0) {
                        uint32_t bins_q16 =
                            (uint32_t)bin_count << 16;
                        int32_t bins_shift =
                            bins_q16 == 0U ? 0 :
                            (int32_t)
                                frame_count_leading_zeros(
                                    bins_q16) - 1;
                        int32_t bins_exponent =
                            15 - bins_shift;
                        uint32_t normalized_bins =
                            frame_shift_left(
                                bins_q16,
                                (unsigned int)bins_shift);
                        int16_t bins_mantissa =
                            (int16_t)(normalized_bins >> 16);
                        if (energy_mantissa <=
                                bins_mantissa) {
                            ++bins_exponent;
                            normalized_bins =
                                frame_arithmetic_shift_right(
                                    normalized_bins, 1);
                            bins_mantissa =
                                (int16_t)(normalized_bins >> 16);
                        }
                        int16_t quotient =
                            (int16_t)vocoder_fixed_divide_half(
                                (int32_t)normalized_bins,
                                energy_mantissa);
                        factor_exponent =
                            bins_exponent -
                            energy_exponent;
                        uint32_t factor_root =
                            vocoder_normalized_sqrt(
                                (uint32_t)(uint16_t)quotient <<
                                    16,
                                &factor_exponent);
                        factor =
                            (int16_t)(factor_root >> 16);
                    }
                    int16_t combined_scale = (int16_t)
                        frame_multiply_q15_truncated(
                            factor, base_scale);
                    multiplier = (int32_t)
                        combined_scale *
                        frame->spectrum[harmonic] * 2;
                    multiplier_exponent =
                        factor_exponent +
                        scale_exponent;
                }
                multiplier = maximum_shift < 0
                    ? (int32_t)
                        frame_arithmetic_shift_right(
                            (uint32_t)multiplier,
                            (unsigned int)(-maximum_shift))
                    : (int32_t)frame_shift_left(
                        (uint32_t)multiplier,
                        (unsigned int)maximum_shift);
                vocoder_multiply_i16_between_exponents(
                    band, band, bin_count * 2,
                    (int16_t)frame_arithmetic_shift_right(
                        (uint32_t)multiplier, 16),
                    multiplier_exponent - maximum_shift,
                    common_exponent);
            }
            else {
                vocoder_fill_i16(band, 0, bin_count * 2);
            }
            previous_bin = end_bin;
        }
        int32_t high_clear = half_size -
            (half_size == 256 ? 242 : 121);
        int32_t unused_bins = half_size - previous_bin;
        if (high_clear < unused_bins) {
            high_clear = unused_bins;
        }
        vocoder_fill_i16(
            generated + fft_size - high_clear * 2,
            0, high_clear * 2);
        int32_t low_clear =
            (fft_size * cutoff * 2 >> 20) * 2;
        vocoder_fill_i16(generated, 0, low_clear);
        generated_exponent = vocoder_real_fft_inverse_64(
            generated,
            frame_exponent + common_exponent,
            fft_log2, 0);
    }

    for (int32_t index = 0; index < count; ++index) {
        int64_t old_product =
            (int64_t)overlap_state[85 + index] *
            window[count - index] * 2;
        int32_t old_scaled = frame_shift_product_saturating(
            frame_saturate_i32(old_product),
            previous_exponent);
        int64_t new_product =
            (int64_t)generated[index] *
            window[index] * 2;
        int32_t new_scaled = frame_shift_product_saturating(
            frame_saturate_i32(new_product),
            generated_exponent);
        int32_t combined = frame_saturating_add(
            old_scaled, new_scaled);
        output[index] = frame_saturating_add(
            output[index], combined);
    }
    vocoder_copy_i16(
        overlap_state + 85, generated + count, count);
    vocoder_fill_i16(
        overlap_state + 85 + count, 0, 84 - count);
    overlap_state[169] = (int16_t)generated_exponent;
}

void vocoder_synthesize_excitation_frame(
    int32_t *output,
    int32_t count,
    int16_t *overlap_state,
    const vocoder_frame_record *frame_record,
    int16_t cutoff)
{
    synthesize_excitation_frame_native(
        output, count, overlap_state, frame_record, cutoff, NULL);
}

void vocoder_synthesize_audio_frame(
    vocoder_frame_record *frame_record,
    int16_t *output,
    int32_t count,
    int32_t preserve_pitch,
    void *decoder_state)
{
    uint8_t *state = (uint8_t *)decoder_state;
    vocoder_frame_state *frame = &frame_record->state;
    vocoder_frame_record *previous =
        (vocoder_frame_record *)(void *)(state + 0x474);
    uint16_t history;
    memcpy(&history, state + 0x7be, sizeof(history));
    if ((history & 1U) == 0U) {
        if ((int16_t)frame->header[0] == 1) {
            vocoder_filter_log_spectrum(
                (int16_t *)(void *)(state + 0x176),
                frame->spectrum,
                (int16_t)frame->header[2]);
        }
        else {
            vocoder_fill_i16(
                (int16_t *)(void *)(state + 0x176),
                (int32_t)(history & 1U), 56);
        }
        int32_t *frame_exponent =
            (int32_t *)(void *)((uint8_t *)(void *)frame_record +
                                0x84);
        vocoder_expand_log_spectrum(
            frame->spectrum, frame_exponent,
            (int16_t)frame->header[2]);
        if ((int16_t)frame->header[0] == 1) {
            int16_t *attenuation_mantissa =
                (int16_t *)(void *)(state + 0x7c4);
            int16_t *attenuation_exponent =
                (int16_t *)(void *)(state + 0x7c6);
            vocoder_apply_adaptive_spectral_attenuation(
                frame->spectrum, frame_exponent,
                attenuation_mantissa, attenuation_exponent,
                (int16_t)frame->header[6],
                (int16_t)frame->header[2]);
        }
        if ((int16_t)frame->header[0] == 2) {
            vocoder_copy_frame_record(
                (vocoder_frame_record *)(void *)(state + 0x4fc),
                frame_record);
        }
    }

    int16_t current_active[56];
    int16_t previous_active[56];
    generate_voicing_flags_to(
        &frame_record->state,
        current_active);
    generate_voicing_flags_to(
        &previous->state,
        previous_active);

    if ((int16_t)frame->header[1] != 0xff) {
        int16_t special_mode;
        memcpy(&special_mode, state + 0x7a4,
               sizeof(special_mode));
        if (special_mode != 1) {
            uint32_t kind = (uint16_t)vocoder_classify_code(
                (int16_t)frame->header[1]);
            int32_t first = vocoder_lookup_partition_value(
                kind, (int16_t)frame->header[1], 0);
            int32_t second = vocoder_lookup_partition_value(
                kind, (int16_t)frame->header[1], 1);
            int32_t harmonics = (int16_t)frame->header[2];
            for (int32_t index = 0;
                    index < harmonics; ++index) {
                if (index != first && index != second) {
                    frame->spectrum[index] = (int16_t)(
                        (int32_t)frame->spectrum[index] *
                        0x51e >> 15);
                }
            }
            uint16_t code = frame->header[1];
            if ((uint16_t)(code - 0x80U) < 0x20U) {
                int32_t sum =
                    frame->spectrum[first] *
                        frame->spectrum[first] +
                    frame->spectrum[second] *
                        frame->spectrum[second];
                int16_t value = vocoder_sqrt_scaled(
                    (uint32_t)sum, 8, 4);
                frame->spectrum[first] = value;
                frame->spectrum[second] = value;
            }
            if ((uint16_t)(code - 0x14U) < 3U) {
                frame->header[6] = (uint16_t)
                    vocoder_fixed_divide_half(
                        (int32_t)((uint32_t)code << 20),
                        (int16_t)(
                            ((int16_t)frame->header[3] + 1) *
                            0x100));
            }
        }
        else {
            int16_t current_kind = vocoder_classify_code(
                (int16_t)frame->header[1]);
            int16_t previous_mode =
                (int16_t)previous->state.header[0];
            int16_t previous_kind = vocoder_classify_code(
                (int16_t)previous->state.header[1]);
            if (current_kind == 0 && previous_mode == 3 &&
                    previous_kind == 0) {
                int32_t previous_index =
                    (int16_t)previous->state.header[3];
                int32_t difference =
                    (int16_t)frame->header[3] - previous_index;
                if (difference < 0) {
                    difference = -difference;
                }
                int32_t destination;
                if (difference < 2 &&
                        ((previous_index + 1) * 16 >
                         (int16_t)frame->header[1])) {
                    destination = previous_index;
                }
                else {
                    int16_t quotient = (int16_t)vocoder_fixed_divide(
                        (int16_t)frame->header[1] *
                            (previous_index + 1) +
                        (int16_t)previous->state.header[1] / 2,
                        (int16_t)previous->state.header[1]);
                    int32_t boundary = quotient * 16;
                    while (boundary <=
                            (int16_t)frame->header[1]) {
                        boundary += 16;
                        ++quotient;
                    }
                    destination = (int16_t)(quotient - 1);
                }
                move_active_harmonic_to(
                    &frame_record->state,
                    (int16_t)destination, current_active);
            }
        }
    }
    else {
        int16_t pitch;
        memcpy(&pitch, state + 0x7c2, sizeof(pitch));
        if (preserve_pitch == 0) {
            uint32_t packed;
            memcpy(&packed, frame->header + 4, sizeof(packed));
            pitch = vocoder_smooth_if_mask_majority(
                pitch, (int16_t)frame->header[6], packed);
            memcpy(state + 0x7c2, &pitch, sizeof(pitch));
        }
        int32_t limit = pitch;
        if (limit > 0x3333) {
            limit = 0x3333;
        }
        int32_t current = (int16_t)frame->header[6];
        if (current < limit && limit != 0) {
            uint32_t limit_q16 = (uint32_t)limit << 16;
            int32_t normalization =
                (int32_t)frame_count_leading_zeros(
                    (int32_t)limit_q16 < 0
                        ? ~limit_q16 : limit_q16) - 1;
            int32_t ratio_exponent;
            int16_t ratio = vocoder_normalized_divide(
                0x40000000, 1,
                (int16_t)(frame_shift_left(
                    limit_q16,
                    (unsigned int)normalization) >> 16),
                -4 - normalization, &ratio_exponent);
            int32_t accumulator = current;
            int32_t spectrum_index = 0;
            while (accumulator < limit) {
                int32_t product = accumulator * ratio * 2;
                int32_t product_shift =
                    (int32_t)frame_count_leading_zeros(
                        product < 0 ? ~((uint32_t)product) :
                        (uint32_t)product) - 1;
                int16_t normalized = (int16_t)(
                    frame_shift_left(
                        (uint32_t)product,
                        (unsigned int)product_shift) >> 16);
                int32_t square =
                    normalized * normalized * 2;
                int32_t exponent =
                    ratio_exponent - 4 - product_shift;
                uint32_t scaled = exponent < 0
                    ? frame_arithmetic_shift_right(
                        (uint32_t)square,
                        (unsigned int)(-2 * exponent))
                    : frame_shift_left(
                        (uint32_t)square,
                        (unsigned int)(2 * exponent));
                frame->spectrum[spectrum_index] = (int16_t)
                    frame_multiply_q15_truncated(
                        (int16_t)(scaled >> 16),
                        frame->spectrum[spectrum_index]);
                accumulator += current;
                ++spectrum_index;
            }
        }

        int32_t harmonics = (int16_t)frame->header[2];
        int16_t mantissas[56];
        int16_t exponents[56];
        int32_t frame_exponent;
        memcpy(&frame_exponent,
               (uint8_t *)(void *)frame_record + 0x84,
               sizeof(frame_exponent));
        for (int32_t index = 0; index < harmonics; ++index) {
            mantissas[index] = frame->spectrum[index];
            exponents[index] = (int16_t)frame_exponent;
        }
        int32_t threshold =
            (int16_t)frame->header[6] * harmonics * 0x2000;
        int32_t threshold_q16 =
            (int16_t)((uint32_t)threshold >> 16);
        while (threshold_q16 > 0x4ccd &&
                harmonics > 0) {
            int32_t index = harmonics - 1;
            int32_t product =
                (int16_t)(((uint32_t)(
                    (int16_t)((uint32_t)threshold >> 16) *
                    0xd556) >> 16)) *
                mantissas[index] * 2;
            int32_t shift =
                (int32_t)frame_count_leading_zeros(
                    product < 0 ? ~((uint32_t)product) :
                    (uint32_t)product) - 1;
            exponents[index] = (int16_t)(
                (int16_t)frame_exponent + 1 - shift);
            mantissas[index] = (int16_t)(
                frame_shift_left(
                    (uint32_t)product,
                    (unsigned int)shift) >> 16);
            threshold = threshold_q16 * 0x10000 -
                (int16_t)frame->header[6] * 0x2000;
            threshold_q16 =
                (int16_t)((uint32_t)threshold >> 16);
            --harmonics;
        }
        int32_t maximum_exponent =
            (int16_t)frame_exponent;
        for (int32_t index = 1;
                index < (int16_t)frame->header[2]; ++index) {
            if (maximum_exponent < exponents[index]) {
                maximum_exponent = exponents[index];
            }
        }
        for (int32_t index = 0;
                index < (int16_t)frame->header[2]; ++index) {
            int32_t shift =
                maximum_exponent - exponents[index];
            uint32_t value =
                (uint32_t)(int32_t)mantissas[index] << 16;
            frame->spectrum[index] = (int16_t)(
                frame_arithmetic_shift_right(
                    value, (unsigned int)shift) >> 16);
        }
        memcpy((uint8_t *)(void *)frame_record + 0x84,
               &maximum_exponent, sizeof(maximum_exponent));
    }

    int32_t synthesized[84] = {0};
    int16_t pitch;
    memcpy(&pitch, state + 0x7c2, sizeof(pitch));
    synthesize_excitation_frame_native(
        synthesized, count,
        (int16_t *)(void *)(state + 0x64c),
        frame_record, pitch, current_active);
    synthesize_frame_transition_native(
        synthesized, count, state + 0x18,
        frame_record, previous, pitch, current_active);
    vocoder_filter_i32_state6(
        synthesized, synthesized,
        (int32_t *)(void *)state, count);
    vocoder_convert_i32_to_i16_rounded(
        output, 0xf, synthesized, 0x11, count);
    vocoder_copy_frame_record(previous, frame_record);
}

static int32_t frame_align_i32_word(
    int32_t value,
    int32_t source_exponent,
    int32_t target_exponent)
{
    int32_t difference = source_exponent - target_exponent;
    return difference < 0
        ? (int32_t)frame_arithmetic_shift_right(
            (uint32_t)value, (unsigned int)(-difference))
        : (int32_t)frame_shift_left(
            (uint32_t)value, (unsigned int)difference);
}

static uint32_t frame_absolute_saturating_i32(int32_t value)
{
    if (value == INT32_MIN) {
        return (uint32_t)INT32_MAX;
    }
    return value < 0
        ? (uint32_t)(-value) : (uint32_t)value;
}

static int16_t frame_log_absolute_difference(
    int32_t total_mantissa,
    int32_t total_exponent,
    int32_t part_mantissa,
    int32_t part_exponent,
    int32_t bias)
{
    int32_t aligned = frame_align_i32_word(
        part_mantissa << 16,
        part_exponent, total_exponent);
    uint32_t difference = frame_absolute_saturating_i32(
        total_mantissa * 0x10000 - aligned);
    int32_t normalization = difference == 0U ? 0 :
        (int32_t)frame_count_leading_zeros(difference) - 1;
    return vocoder_log_amplitude_clamped(
        (uint16_t)(frame_shift_left(
            difference, (unsigned int)normalization) >> 16),
        total_exponent - normalization, (int16_t)bias);
}

int32_t vocoder_classify_spectral_frame(
    int16_t output[2],
    const int32_t values[128],
    int32_t value_exponent,
    int32_t log_bias,
    void *classifier_state,
    int16_t relaxed_threshold)
{
    static const uint8_t first_left_offsets[4] = {
        0x14, 0x17, 0x19, 0x1c
    };
    static const uint8_t first_right_offsets[4] = {
        0x25, 0x29, 0x2d, 0x32
    };
    static const int16_t first_left_multipliers[4] = {
        0x2c9c, 0x3148, 0x3687, 0x3c39
    };
    static const int16_t first_right_multipliers[4] = {
        0x4d60, 0x5581, 0x5e87, 0x6883
    };
    static const uint8_t second_left_offsets[4] = {
        0x11, 0x13, 0x16, 0x18
    };
    static const uint8_t second_right_offsets[4] = {
        0x20, 0x23, 0x27, 0x2c
    };
    static const int16_t second_left_multipliers[4] = {
        0x1311, 0x1816, 0x26c9, 0x2afd
    };
    static const int16_t second_right_multipliers[4] = {
        0x4352, 0x4a56, 0x52fa, 0x5b7d
    };
    static const uint8_t identifier_map[16] = {
        1, 4, 7, 14, 2, 5, 8, 0,
        3, 6, 9, 15, 10, 11, 12, 13
    };
    static const uint8_t fallback_ranges[8] = {
        9, 12, 12, 13, 13, 18, 9, 15
    };
    uint8_t *state = (uint8_t *)classifier_state;
    int16_t previous_identifier;
    memcpy(&previous_identifier, state + 8,
           sizeof(previous_identifier));
    int32_t total_exponent = value_exponent;
    uint32_t total_sum = (uint32_t)vocoder_sum_i32_normalized(
        values, 128, &total_exponent);
    int16_t total_mantissa = (int16_t)(total_sum >> 16);
    if (total_mantissa == 0 || total_exponent < -26) {
        return 0xff;
    }

    int32_t unused_maximum;
    int32_t first_left_index;
    int32_t first_right_index;
    vocoder_select_maximum_five_sum(
        &unused_maximum, &first_left_index,
        values, first_left_offsets);
    vocoder_select_maximum_five_sum(
        &unused_maximum, &first_right_index,
        values, first_right_offsets);
    int16_t first_left;
    int16_t first_right;
    int32_t first_exponent = vocoder_align_two_i32_sums(
        &first_left, &first_right, values, value_exponent,
        first_left_offsets[first_left_index],
        first_right_offsets[first_right_index]);
    int32_t first_identifier = 0x80 +
        identifier_map[(first_left_index +
            first_right_index * 4) & 15];
    int32_t first_valid = previous_identifier == first_identifier ||
        vocoder_test_two_spectral_neighborhoods(
            first_left, first_right, first_exponent,
            values, value_exponent,
            first_left_offsets[first_left_index],
            first_left_multipliers[first_left_index],
            first_right_offsets[first_right_index],
            first_right_multipliers[first_right_index],
            0x51ec) == 1;

    int32_t second_left_index;
    int32_t second_right_index;
    vocoder_select_maximum_five_sum(
        &unused_maximum, &second_left_index,
        values, second_left_offsets);
    vocoder_select_maximum_five_sum(
        &unused_maximum, &second_right_index,
        values, second_right_offsets);
    int16_t second_left;
    int16_t second_right;
    int32_t second_exponent = vocoder_align_two_i32_sums(
        &second_left, &second_right, values, value_exponent,
        second_left_offsets[second_left_index],
        second_right_offsets[second_right_index]);
    int32_t second_identifier = 0x90 +
        identifier_map[(second_left_index +
            second_right_index * 4) & 15];
    int32_t second_valid = previous_identifier == second_identifier ||
        vocoder_test_two_spectral_neighborhoods(
            second_left, second_right, second_exponent,
            values, value_exponent,
            second_left_offsets[second_left_index],
            second_left_multipliers[second_left_index],
            second_right_offsets[second_right_index],
            second_right_multipliers[second_right_index],
            0x4189) == 1;

    if (first_valid != 0 || second_valid != 0) {
        if (first_valid != 0 && second_valid != 0) {
            int32_t first_combined =
                (first_left + first_right) * 0x8000;
            int32_t second_combined =
                (second_left + second_right) * 0x8000;
            int32_t first_shift = first_combined == 0 ? 0 :
                (int32_t)frame_count_leading_zeros(
                    first_combined < 0
                        ? ~((uint32_t)first_combined)
                        : (uint32_t)first_combined) - 1;
            int32_t second_shift = second_combined == 0 ? 0 :
                (int32_t)frame_count_leading_zeros(
                    second_combined < 0
                        ? ~((uint32_t)second_combined)
                        : (uint32_t)second_combined) - 1;
            int32_t comparison = vocoder_compare_scaled(
                (int16_t)(frame_shift_left(
                    (uint32_t)second_combined,
                    (unsigned int)second_shift) >> 16),
                second_exponent - second_shift,
                (int16_t)(frame_shift_left(
                    (uint32_t)first_combined,
                    (unsigned int)first_shift) >> 16),
                first_exponent - first_shift);
            if (comparison != 0) {
                first_left = second_left;
                first_right = second_right;
                first_exponent = second_exponent;
                first_identifier = second_identifier;
            }
        }
        else if (first_valid == 0) {
            first_left = second_left;
            first_right = second_right;
            first_exponent = second_exponent;
            first_identifier = second_identifier;
        }
        int32_t left_q12 = (int32_t)first_left << 12;
        int32_t right_weighted = first_right * 0xa000;
        if (left_q12 <= right_weighted &&
                ((int32_t)first_right << 12) <=
                    first_left * 0xa000) {
            int32_t pair_sum =
                ((int32_t)first_left << 15) +
                ((int32_t)first_right << 15);
            pair_sum = frame_align_i32_word(
                pair_sum, first_exponent + 1,
                total_exponent);
            int32_t threshold = total_mantissa * 0xf810;
            if (threshold <= pair_sum) {
                output[0] = vocoder_log_amplitude_clamped(
                    (uint16_t)first_left, first_exponent,
                    (int16_t)log_bias);
                output[1] = vocoder_log_amplitude_clamped(
                    (uint16_t)first_right, first_exponent,
                    (int16_t)log_bias);
                return first_identifier;
            }
        }
    }

    uint16_t previous = (uint16_t)previous_identifier;
    if (previous == 0xffU ||
            ((previous - 5U) & 0xffffU) < 0x76U) {
        int32_t peak = vocoder_argmax_i32(values + 5, 0x76);
        int32_t peak_identifier = peak + 5;
        const int32_t *peak_values = values + peak + 3;
        int32_t peak_exponent = value_exponent;
        int32_t peak_sum = vocoder_sum_i32_normalized(
            peak_values, 5, &peak_exponent);
        int16_t peak_mantissa = (int16_t)(
            (uint32_t)peak_sum >> 16);
        int16_t peak_log = vocoder_log_amplitude_clamped(
            (uint16_t)peak_mantissa, peak_exponent,
            (int16_t)log_bias);
        int16_t residual_log = frame_log_absolute_difference(
            total_mantissa, total_exponent,
            peak_mantissa, peak_exponent, log_bias);
        if ((int16_t)previous > 4) {
            int16_t distance = (int16_t)(
                peak_identifier - (int16_t)previous);
            if (distance >= -1 && distance < 2) {
                int16_t old_peak;
                int16_t old_residual;
                memcpy(&old_peak, state + 0x16,
                       sizeof(old_peak));
                memcpy(&old_residual, state + 0x18,
                       sizeof(old_residual));
                peak_log = (int16_t)(
                    (old_peak * 0xdffe +
                     peak_log * 0x2000 + 0x8000) >> 16);
                residual_log = (int16_t)(
                    (old_residual * 0xdffe +
                     residual_log * 0x2000 + 0x8000) >> 16);
            }
        }
        int16_t difference = (int16_t)(peak_log - residual_log);
        int32_t use_peak = 0;
        if (relaxed_threshold != 0 && previous == 0xffU &&
                peak_identifier >= 0x58) {
            use_peak = difference > 0x2a84;
        }
        else if (difference > 0x1a92) {
            use_peak = peak_identifier >= 0x13 ||
                difference >= 0x2138 ||
                (peak_identifier >= 0x0b ||
                 difference >= 0x2689);
        }
        if (use_peak != 0) {
            int16_t normalized[6];
            int32_t normalized_exponent = value_exponent;
            vocoder_normalize_i32_to_i16(
                normalized, peak_values, 5,
                &normalized_exponent);
            if (vocoder_test_weighted_curvature(
                    peak_mantissa, peak_exponent,
                    normalized,
                    (int16_t)normalized_exponent) != 0) {
                output[0] = (int16_t)0xb000;
                output[1] = vocoder_log_amplitude_clamped(
                    (uint16_t)peak_mantissa, peak_exponent,
                    (int16_t)log_bias);
                memcpy(state + 0x16, &peak_log,
                       sizeof(peak_log));
                memcpy(state + 0x18, &residual_log,
                       sizeof(residual_log));
                return peak_identifier;
            }
        }
        previous = (uint16_t)previous_identifier;
    }

    int32_t range_index;
    if (((previous - 0xa0U) & 0xffffU) < 4U) {
        range_index = (int16_t)previous - 0xa0;
    }
    else {
        int32_t best = -1;
        range_index = 0;
        for (int32_t index = 0; index < 4; ++index) {
            int32_t width =
                fallback_ranges[index * 2 + 1] -
                fallback_ranges[index * 2];
            int32_t score = vocoder_sum_i32_shifted(
                values + fallback_ranges[index * 2],
                (width * 0x10000 + 0x50000) >> 16, 3);
            if (width > 5) {
                score -= vocoder_sum_i32_shifted(
                    values + fallback_ranges[index * 2] + 5,
                    (width * 0x10000 - 0x50000) >> 16, 3);
            }
            if (best <= score) {
                best = score;
                range_index = index;
            }
        }
    }
    int32_t lower = fallback_ranges[range_index * 2];
    int32_t upper = fallback_ranges[range_index * 2 + 1];
    int32_t lower_exponent = value_exponent;
    int32_t lower_sum = vocoder_sum_i32_normalized(
        values + lower, 5, &lower_exponent);
    int32_t upper_exponent = value_exponent;
    int32_t upper_sum = vocoder_sum_i32_normalized(
        values + upper, 5, &upper_exponent);
    int32_t common_exponent = lower_exponent > upper_exponent
        ? lower_exponent : upper_exponent;
    int32_t aligned_lower = frame_align_i32_word(
        lower_sum, lower_exponent, common_exponent);
    int32_t aligned_upper = frame_align_i32_word(
        upper_sum, upper_exponent, common_exponent);
    if (((aligned_lower >> 16) * 0x2000 <=
            (aligned_upper >> 16) * 0x8000) &&
            ((aligned_upper >> 16) * 0x2000 <=
             (aligned_lower >> 16) * 0x8000)) {
        int32_t middle_exponent = value_exponent;
        int32_t middle_count = (int16_t)(upper - lower) + 7;
        int32_t middle_sum_word = vocoder_sum_i32_normalized(
            values + lower - 1, middle_count,
            &middle_exponent);
        int16_t middle_sum = (int16_t)(
            (uint32_t)middle_sum_word >> 16);
        int16_t middle_log = vocoder_log_amplitude_clamped(
            (uint16_t)middle_sum, middle_exponent,
            (int16_t)log_bias);
        int16_t residual_log = frame_log_absolute_difference(
            total_mantissa, total_exponent,
            middle_sum, middle_exponent, log_bias);
        if (range_index + 0xa0 == (int16_t)previous) {
            int16_t old_middle;
            int16_t old_residual;
            memcpy(&old_middle, state + 0x16,
                   sizeof(old_middle));
            memcpy(&old_residual, state + 0x18,
                   sizeof(old_residual));
            middle_log = (int16_t)(
                (old_middle * 0xdffe +
                 middle_log * 0x2000 + 0x8000) >> 16);
            residual_log = (int16_t)(
                (old_residual * 0xdffe +
                 residual_log * 0x2000 + 0x8000) >> 16);
        }
        if ((int32_t)(middle_log - residual_log) * 0x10000 >
                0x23dfffff) {
            if (upper - lower < 4) {
                int16_t normalized[6];
                int32_t normalized_exponent = value_exponent;
                int32_t center =
                    ((upper + lower + 4) >> 1) - 2;
                vocoder_normalize_i32_to_i16(
                    normalized, values + center, 5,
                    &normalized_exponent);
                if (vocoder_test_weighted_curvature(
                        middle_sum, middle_exponent,
                        normalized,
                        (int16_t)normalized_exponent) == 1) {
                    return 0xff;
                }
            }
            int16_t result_log = (int16_t)(
                ((int32_t)middle_log * 0x10000 -
                 0x04000000) >> 16);
            output[0] = result_log;
            output[1] = result_log;
            memcpy(state + 0x16, &middle_log,
                   sizeof(middle_log));
            memcpy(state + 0x18, &residual_log,
                   sizeof(residual_log));
            return range_index + 0xa0;
        }
    }
    return 0xff;
}

void vocoder_update_spectral_classification(
    void *classifier_state,
    const int32_t values[128],
    int32_t value_exponent,
    uint32_t flags,
    int32_t slot,
    int16_t relaxed_threshold)
{
    uint8_t *state = (uint8_t *)classifier_state;
    size_t wide_offset = (size_t)slot * 4U;
    size_t narrow_offset = (size_t)slot * 2U;
    int16_t zero = 0;
    int16_t invalid = 0xff;
    if ((flags & 0x1000U) == 0U) {
        memcpy(state + wide_offset + 0x0e,
               &zero, sizeof(zero));
        memcpy(state + narrow_offset + 0x0a,
               &invalid, sizeof(invalid));
        memcpy(state + wide_offset + 0x10,
               &zero, sizeof(zero));
        return;
    }
    int16_t classified[2];
    memcpy(&classified[0], state + wide_offset + 0x0e,
           sizeof(classified[0]));
    memcpy(&classified[1], state + wide_offset + 0x10,
           sizeof(classified[1]));
    int32_t identifier = vocoder_classify_spectral_frame(
        classified, values, value_exponent, 0,
        state, relaxed_threshold);
    if (identifier == 0xff) {
        int16_t previous_identifier;
        int16_t hold_count;
        memcpy(&previous_identifier, state + 8,
               sizeof(previous_identifier));
        memcpy(&hold_count, state + 6,
               sizeof(hold_count));
        if (hold_count > 0 &&
                (previous_identifier >= 0x80 ||
                 previous_identifier == 0x13 ||
                 previous_identifier == 0x3a)) {
            if (previous_identifier >= 0x80) {
                int16_t held = (int16_t)0xb001;
                memcpy(state + wide_offset + 0x0e,
                       &held, sizeof(held));
                memcpy(state + wide_offset + 0x10,
                       &held, sizeof(held));
            }
            memcpy(state + narrow_offset + 0x0a,
                   &previous_identifier,
                   sizeof(previous_identifier));
            --hold_count;
            memcpy(state + 6, &hold_count,
                   sizeof(hold_count));
            memcpy(state + 8, &previous_identifier,
                   sizeof(previous_identifier));
            return;
        }
        memcpy(state + wide_offset + 0x0e,
               &zero, sizeof(zero));
        memcpy(state + narrow_offset + 0x0a,
               &invalid, sizeof(invalid));
        memcpy(state + wide_offset + 0x10,
               &zero, sizeof(zero));
        memcpy(state + 8, &invalid, sizeof(invalid));
        return;
    }
    memcpy(state + wide_offset + 0x0e,
           &classified[0], sizeof(classified[0]));
    int16_t short_identifier = (int16_t)identifier;
    memcpy(state + narrow_offset + 0x0a,
           &short_identifier, sizeof(short_identifier));
    memcpy(state + wide_offset + 0x10,
           &classified[1], sizeof(classified[1]));
    int16_t hold_count = 2;
    memcpy(state + 6, &hold_count, sizeof(hold_count));
    memcpy(state + 8, &short_identifier,
           sizeof(short_identifier));
}

void vocoder_analyze_complex_spectral_ratios(
    int16_t *state,
    uint32_t complex_values[129],
    int32_t complex_exponent,
    const int32_t spectrum[129],
    int32_t spectrum_exponent,
    const int16_t thresholds[8],
    const int16_t voicing_values[8],
    int32_t voicing_exponent)
{
    state[2] = state[1];
    state[5] = state[4];
    state[8] = state[7];
    vocoder_copy_i16(state + 28, state + 20, 8);
    state[38] = state[37];
    state[1] = state[0];
    state[4] = state[3];
    state[7] = state[6];
    vocoder_copy_i16(state + 20, state + 12, 8);
    state[37] = state[36];

    int16_t phase[258];
    int32_t adjusted_spectrum_exponent =
        spectrum_exponent + 0x1e;
    int32_t phase_exponent = vocoder_log_i32_vector(
        phase, spectrum, adjusted_spectrum_exponent, 129);
    phase_exponent = vocoder_real_fft_inverse_64(
        phase, phase_exponent, 8, 1);
    phase[0] = (int16_t)(phase[0] >> 1);
    phase[128] = (int16_t)(phase[128] >> 1);
    vocoder_scale_i16_vector(phase, phase, 129, 0x70f7);
    phase_exponent -= 2;
    vocoder_normalize_i16_vector(
        phase, phase, 129, &phase_exponent);
    vocoder_fill_i16(phase + 129, 0, 127);
    phase_exponent = vocoder_real_fft_forward_64(
        phase, phase_exponent, 8, 1);
    vocoder_generate_complex_phasors(
        phase, phase + 1, (uint32_t)phase_exponent, 128);

    int16_t roots[257];
    int32_t root_exponent = vocoder_fourth_root_vector(
        roots, (const uint32_t *)(const void *)spectrum,
        adjusted_spectrum_exponent, 129);
    int32_t phase_product_exponent =
        vocoder_multiply_complex_by_real(
            phase, (const uint32_t *)(const void *)phase, 0,
            roots, root_exponent, 129);
    int16_t minimum = 0x7fff;
    for (int32_t index = 0; index < 129; ++index) {
        int16_t value = roots[index];
        if (value != 0 && value <= minimum) {
            minimum = value;
        }
    }
    int32_t reciprocal_shift = 0;
    if (minimum != 1) {
        int32_t value = (int32_t)minimum - 1;
        uint32_t bits = (uint32_t)value << 16;
        int32_t normalization = bits == 0U ? 0 :
            (int32_t)frame_count_leading_zeros(
                (int32_t)bits < 0 ? ~bits : bits) - 1;
        reciprocal_shift = 15 - normalization;
    }
    root_exponent = vocoder_reciprocal_vector(
        roots, roots, root_exponent,
        reciprocal_shift, 129);
    phase_product_exponent = vocoder_adjacent_complex_products(
        (uint32_t *)(void *)phase, phase_product_exponent);
    int32_t input_product_exponent =
        vocoder_multiply_complex_by_real(
            (int16_t *)(void *)complex_values,
            complex_values, complex_exponent + 15,
            roots, root_exponent, 129);
    input_product_exponent = vocoder_adjacent_complex_products(
        complex_values, input_product_exponent);

    vocoder_expand_voicing_bands(
        roots, thresholds, voicing_values,
        voicing_exponent);
    vocoder_fill_i16(roots, 0, 12);
    int16_t correlation[2];
    int32_t correlation_exponent =
        vocoder_complex_weighted_correlation(
            correlation, roots,
            (const uint32_t *)(const void *)phase,
            phase_product_exponent,
            complex_values, input_product_exponent, 127);
    int32_t angle_exponent;
    int16_t angle = vocoder_atan2_scaled(
        correlation[1], correlation_exponent,
        correlation[0], correlation_exponent,
        &angle_exponent);
    uint32_t angle_product = (uint32_t)(
        (int32_t)angle * -0xa2f8);
    uint32_t shifted_angle = angle_exponent < 1
        ? frame_arithmetic_shift_right(
            angle_product,
            (unsigned int)(1 - angle_exponent))
        : frame_shift_left(
            angle_product,
            (unsigned int)(angle_exponent - 1));
    state[0] = (int16_t)(shifted_angle >> 16);

    int16_t normalized_real = correlation[0] == INT16_MIN
        ? (int16_t)-0x7fff : correlation[0];
    int16_t normalized_imaginary = correlation[1] == INT16_MIN
        ? (int16_t)-0x7fff : correlation[1];
    uint32_t packed_correlation =
        (uint16_t)normalized_real |
        ((uint32_t)(uint16_t)normalized_imaginary << 16);
    int16_t magnitude_exponent;
    int16_t magnitude = vocoder_complex_vector_energy(
        &magnitude_exponent, &packed_correlation,
        (int16_t)correlation_exponent, 1);
    if (magnitude > 0) {
        int32_t square_root_exponent = magnitude_exponent;
        uint32_t square_root = vocoder_normalized_sqrt(
            (uint32_t)(uint16_t)magnitude << 16,
            &square_root_exponent);
        int32_t reciprocal_exponent;
        int16_t reciprocal = vocoder_normalized_divide(
            0x40000000, 1,
            (int16_t)(square_root >> 16),
            square_root_exponent, &reciprocal_exponent);
        normalized_real = (int16_t)(
            ((uint32_t)((int32_t)normalized_real *
                        reciprocal * 2)) >> 16);
        normalized_imaginary = (int16_t)(
            ((uint32_t)((int32_t)normalized_imaginary *
                        reciprocal * 2)) >> 16);
        correlation_exponent += reciprocal_exponent;
    }
    else {
        correlation_exponent = 0;
    }
    uint32_t normalized_coefficient =
        (uint16_t)normalized_real |
        ((uint32_t)(uint16_t)normalized_imaginary << 16);
    int32_t corrected_exponent = vocoder_multiply_complex_vector(
        complex_values, &normalized_coefficient,
        correlation_exponent,
        complex_values, input_product_exponent, 127);

    int16_t difference_values[8];
    int16_t difference_exponents[8];
    int16_t energy_values[8];
    int16_t energy_exponents[8];
    int32_t start = 0;
    int32_t boundary = 13;
    for (int32_t band = 0; band < 8; ++band) {
        int32_t end = band == 7 ? 127 : boundary;
        int16_t count = (int16_t)(end - start);
        difference_values[band] = vocoder_squared_vector_difference(
            &difference_exponents[band],
            (const int16_t *)(const void *)(complex_values + start),
            corrected_exponent,
            phase + start * 2,
            (int16_t)phase_product_exponent, count);
        energy_values[band] = vocoder_complex_vector_energy(
            &energy_exponents[band],
            (const uint32_t *)(const void *)(phase + start * 2),
            (int16_t)phase_product_exponent, count);
        start = end;
        boundary += 16;
    }
    int16_t difference_common = vocoder_align_by_exponents(
        difference_values, difference_values,
        difference_exponents, 8);
    int16_t energy_common = vocoder_align_by_exponents(
        energy_values, energy_values, energy_exponents, 8);
    int16_t ratio_exponents[8];
    for (int32_t band = 0; band < 8; ++band) {
        int32_t difference_sum = difference_values[band];
        int32_t energy_sum = energy_values[band];
        if (band > 0) {
            difference_sum += difference_values[band - 1];
            energy_sum += energy_values[band - 1];
        }
        if (band < 7) {
            difference_sum += difference_values[band + 1];
            energy_sum += energy_values[band + 1];
        }
        uint32_t energy_bits = (uint32_t)energy_sum;
        int32_t energy_shift = energy_bits == 0U ? 0 :
            (int32_t)frame_count_leading_zeros(
                (int32_t)energy_bits < 0
                    ? ~energy_bits : energy_bits) - 1;
        uint32_t difference_bits = (uint32_t)difference_sum;
        int32_t difference_shift = difference_bits == 0U ? 0 :
            (int32_t)frame_count_leading_zeros(
                (int32_t)difference_bits < 0
                    ? ~difference_bits : difference_bits) - 1;
        int32_t normalized_energy = (int32_t)frame_shift_left(
            energy_bits, (unsigned int)energy_shift);
        if (normalized_energy == 0) {
            state[12 + band] = 0x4000;
            ratio_exponents[band] = 1;
        }
        else {
            int32_t result_exponent =
                difference_common - difference_shift;
            state[12 + band] = vocoder_normalized_divide(
                (int32_t)frame_shift_left(
                    difference_bits,
                    (unsigned int)difference_shift),
                difference_common - difference_shift,
                (int16_t)(normalized_energy >> 16),
                energy_common - energy_shift,
                &result_exponent);
            ratio_exponents[band] =
                (int16_t)result_exponent;
        }
    }
    int16_t ratio_common = vocoder_align_by_exponents(
        state + 12, state + 12, ratio_exponents, 8);
    state[9] = energy_common;
    state[36] = ratio_common;

    if (state[1] > 0 && state[0] < 0 &&
            state[10] <= energy_common &&
            (state[10] != energy_common ||
             state[7] > state[6])) {
        int32_t old_exponent = state[37];
        int32_t new_exponent = ratio_common;
        int32_t common = old_exponent > new_exponent
            ? old_exponent : new_exponent;
        ++common;
        for (int32_t band = 0; band < 8; ++band) {
            int32_t current = frame_align_i32_word(
                (int32_t)state[12 + band] << 16,
                new_exponent, common);
            int32_t previous = frame_align_i32_word(
                (int32_t)state[20 + band] << 16,
                old_exponent, common);
            if (current - previous < 0) {
                state[20 + band] = (int16_t)(
                    frame_align_i32_word(
                        (int32_t)state[12 + band] << 16,
                        new_exponent, old_exponent) >> 16);
            }
        }
    }
}

static int16_t frame_load_i16(const uint8_t *base, size_t offset)
{
    int16_t value;
    memcpy(&value, base + offset, sizeof(value));
    return value;
}

static void frame_store_i16(
    uint8_t *base, size_t offset, int16_t value)
{
    memcpy(base + offset, &value, sizeof(value));
}

static int32_t frame_signed_normalization(uint32_t value)
{
    uint32_t magnitude = (int32_t)value < 0 ? ~value : value;
    return value == 0U ? 0 :
        (int32_t)frame_count_leading_zeros(magnitude) - 1;
}

static int16_t frame_threshold_at_exponent(int16_t exponent)
{
    int32_t left_shift = -(int32_t)exponent - 3;
    if (left_shift <= 0) {
        uint32_t value = frame_arithmetic_shift_right(
            0x66660000U, (unsigned int)((int32_t)exponent + 3));
        return (int16_t)(value >> 16);
    }
    uint32_t shifted = frame_shift_left(
        0x66660000U, (unsigned int)left_shift);
    if (frame_arithmetic_shift_right(
            shifted, (unsigned int)left_shift) != 0x66660000U) {
        return INT16_MAX;
    }
    return (int16_t)(shifted >> 16);
}

void vocoder_update_complex_spectral_analysis(
    void *opaque_state,
    int32_t enabled,
    const int16_t *sample_history,
    uint32_t scratch_words[129],
    int32_t sample_count,
    const void *opaque_auxiliary_state)
{
    uint8_t *state = (uint8_t *)opaque_state;
    int16_t *scratch = (int16_t *)(void *)scratch_words;
    const uint8_t *auxiliary_state =
        (const uint8_t *)opaque_auxiliary_state;
    static const int16_t correlation_window[80] = {
        0x0020, 0x0116, 0x02f1, 0x059a, 0x08fd, 0x0d03,
        0x1198, 0x16a4, 0x1c12, 0x21cb, 0x27b9, 0x2dc5,
        0x33da, 0x39e3, 0x3fca, 0x457b, 0x4ae3, 0x4fee,
        0x548b, 0x58a8, 0x5c52, 0x601a, 0x63f8, 0x67cd,
        0x6b7f, 0x6ef8, 0x7229, 0x7506, 0x7788, 0x79ad,
        0x7b74, 0x7ce1, 0x7dfa, 0x7ec8, 0x7f55, 0x7fae,
        0x7fe0, 0x7ff6, 0x7ffe, 0x7fff, 0x7ffa, 0x0283,
        0x7fc7, 0x0789, 0x7f62, 0x0c8c, 0x7ecb, 0x118a,
        0x7e02, 0x1680, 0x7d07, 0x1b6e, 0x7bda, 0x2052,
        0x7a7d, 0x2528, 0x78ef, 0x29f0, 0x7732, 0x2ea7,
        0x7546, 0x334c, 0x732b, 0x37dc, 0x70e3, 0x3c57,
        0x6e6e, 0x40b9, 0x6bce, 0x4502, 0x6903, 0x4930,
        0x660f, 0x4d41, 0x62f2, 0x5134, 0x5fae, 0x5506,
        0x5c45, 0x58b7
    };

    int16_t spectrum_exponent = frame_load_i16(state, 0x60c);
    vocoder_analyze_complex_spectral_ratios(
        (int16_t *)(void *)(state + 0x6e0),
        scratch_words,
        spectrum_exponent >> 1,
        (const int32_t *)(const void *)state,
        spectrum_exponent,
        (const int16_t *)(const void *)(state + 0x636),
        (const int16_t *)(const void *)(state + 0x686),
        frame_load_i16(state, 0x6d6));

    if (enabled > 0) {
        int16_t auxiliary_flag =
            frame_load_i16(auxiliary_state, 0x180);
        int16_t first_weight = frame_load_i16(state, 0x62c);
        int16_t second_weight = frame_load_i16(state, 0x630);
        int32_t copy_first = 0;
        int32_t copy_second = 0;
        if (first_weight < 0x3334) {
            if (first_weight < 0x199a) {
                if (second_weight >= 0x3334) {
                    copy_first = 1;
                }
                else if (second_weight > 0x1999 &&
                         first_weight < (second_weight >> 1)) {
                    copy_first = 1;
                }
            }
            else if (second_weight < (first_weight >> 1)) {
                copy_second = 1;
            }
            else if (second_weight > 0x3333) {
                copy_first = 1;
            }
            else if (second_weight > 0x1999 &&
                     first_weight < (second_weight >> 1)) {
                copy_first = 1;
            }
        }
        else {
            if (second_weight <= 0x3332 ||
                    second_weight < (first_weight >> 1)) {
                copy_second = 1;
            }
            else if (first_weight < (second_weight >> 1)) {
                copy_first = 1;
            }
        }
        if (copy_first) {
            frame_store_i16(state, 0x62e, first_weight);
            frame_store_i16(state, 0x624,
                            frame_load_i16(state, 0x622));
        }
        else if (copy_second) {
            frame_store_i16(state, 0x62e, second_weight);
            frame_store_i16(state, 0x624,
                            frame_load_i16(state, 0x626));
        }
        else {
            uint16_t first = (uint16_t)frame_load_i16(state, 0x622);
            int32_t second =
                (int32_t)frame_load_i16(state, 0x626) * 0x10000;
            int32_t half_first = (int32_t)
                frame_arithmetic_shift_right((uint32_t)first << 16, 1);
            int16_t blended = (int16_t)
                (((uint32_t)half_first +
                  (uint32_t)frame_arithmetic_shift_right(
                      (uint32_t)second, 1)) >> 16);
            frame_store_i16(state, 0x624, blended);
            frame_store_i16(state, 0x62e,
                (int16_t)(((int32_t)second_weight + first_weight) >> 1));
            if (first_weight < 0x0ccc) {
                int32_t difference = (int32_t)((uint32_t)first << 16) -
                    (int32_t)frame_arithmetic_shift_right(
                        (uint32_t)second, 1);
                int32_t magnitude = difference == INT32_MIN
                    ? INT32_MAX : (difference < 0 ? -difference : difference);
                if (magnitude - (int32_t)(int16_t)first * 0x3332 < 0) {
                    frame_store_i16(state, 0x624, (int16_t)
                        (((uint32_t)half_first +
                          frame_arithmetic_shift_right(
                              (uint32_t)second, 2)) >> 16));
                }
                else {
                    difference = second - half_first;
                    magnitude = difference == INT32_MIN
                        ? INT32_MAX :
                          (difference < 0 ? -difference : difference);
                    if (magnitude - (int32_t)(int16_t)first * 0x1998 < 0) {
                        frame_store_i16(state, 0x624, (int16_t)
                            (((uint32_t)second +
                              (uint32_t)half_first) >> 16));
                    }
                }
            }
        }

        int32_t ratio_exponent;
        int16_t ratio = vocoder_ratio_of_i32_sums(
            (const int32_t *)(const void *)(state + 0x204),
            frame_load_i16(state, 0x60e),
            (const int32_t *)(const void *)state,
            spectrum_exponent, &ratio_exponent);
        int16_t old_exponent = frame_load_i16(state, 0x6da);
        int16_t new_exponent = frame_load_i16(state, 0x6d6);
        frame_store_i16(state, 0x6d8,
                        (int16_t)(ratio_exponent + new_exponent));
        for (int32_t index = 0; index < 3; ++index) {
            int16_t threshold = frame_load_i16(
                state, 0x636U + (size_t)index * 2U);
            int16_t alternate = frame_load_i16(
                state, 0x656U + (size_t)index * 2U);
            frame_store_i16(state, 0x646U + (size_t)index * 2U,
                            alternate < threshold ? alternate : threshold);
            int16_t voicing = frame_load_i16(
                state, 0x686U + (size_t)index * 2U);
            frame_store_i16(state, 0x696U + (size_t)index * 2U,
                (int16_t)(((uint32_t)((int32_t)ratio * voicing * 2)) >> 16));
        }
        for (int32_t index = 0; index < 5; ++index) {
            size_t offset = (size_t)index * 2U;
            int16_t current = frame_load_i16(state, 0x68cU + offset);
            int16_t previous = frame_load_i16(state, 0x6acU + offset);
            int32_t current_shift = frame_signed_normalization(
                (uint32_t)(uint16_t)current << 16);
            int32_t previous_shift = frame_signed_normalization(
                (uint32_t)(uint16_t)previous << 16);
            int32_t current_scaled_exponent = new_exponent - current_shift;
            int32_t previous_scaled_exponent = old_exponent - previous_shift;
            int16_t current_factor = frame_load_i16(state, 0x63cU + offset);
            int16_t previous_factor = frame_load_i16(state, 0x65cU + offset);
            uint32_t current_product = (uint32_t)
                ((int32_t)current * current_factor * 2);
            uint32_t previous_product = (uint32_t)
                ((int32_t)previous * previous_factor * 2);
            int32_t current_product_shift =
                frame_signed_normalization(current_product);
            int32_t previous_product_shift =
                frame_signed_normalization(previous_product);
            int32_t current_product_exponent =
                new_exponent - current_product_shift;
            int32_t previous_product_exponent =
                old_exponent - previous_product_shift;
            int32_t sum_exponent = current_scaled_exponent >
                previous_scaled_exponent ? current_scaled_exponent :
                previous_scaled_exponent;
            ++sum_exponent;
            int32_t product_sum_exponent = current_product_exponent >
                previous_product_exponent ? current_product_exponent :
                previous_product_exponent;
            ++product_sum_exponent;
            uint32_t current_normalized = frame_shift_left(
                (uint32_t)(uint16_t)current << 16,
                (unsigned int)current_shift);
            uint32_t previous_normalized = frame_shift_left(
                (uint32_t)(uint16_t)previous << 16,
                (unsigned int)previous_shift);
            uint32_t sum = frame_arithmetic_shift_right(
                    current_normalized,
                    (unsigned int)(sum_exponent-current_scaled_exponent)) +
                frame_arithmetic_shift_right(
                    previous_normalized,
                    (unsigned int)(sum_exponent-previous_scaled_exponent));
            int32_t sum_shift = frame_signed_normalization(sum);
            uint32_t normalized_sum = frame_shift_left(
                sum, (unsigned int)sum_shift);
            uint32_t current_product_normalized = frame_shift_left(
                current_product, (unsigned int)current_product_shift);
            uint32_t previous_product_normalized = frame_shift_left(
                previous_product, (unsigned int)previous_product_shift);
            uint32_t product_sum = frame_arithmetic_shift_right(
                    current_product_normalized,
                    (unsigned int)(product_sum_exponent-current_product_exponent)) +
                frame_arithmetic_shift_right(
                    previous_product_normalized,
                    (unsigned int)(product_sum_exponent-previous_product_exponent));
            int32_t product_sum_shift =
                frame_signed_normalization(product_sum);
            int16_t output;
            if ((normalized_sum >> 16) == 0U) {
                output = INT16_MAX;
            }
            else {
                int32_t quotient_exponent;
                int16_t quotient = vocoder_normalized_divide(
                    (int32_t)frame_shift_left(
                        product_sum,
                        (unsigned int)product_sum_shift),
                    product_sum_exponent - product_sum_shift,
                    (int16_t)(normalized_sum >> 16),
                    sum_exponent - sum_shift, &quotient_exponent);
                uint32_t quotient_value = (uint32_t)(int32_t)quotient << 16;
                if (quotient_exponent < 1) {
                    quotient_value = frame_arithmetic_shift_right(
                        quotient_value, (unsigned int)-quotient_exponent);
                }
                else {
                    uint32_t shifted = frame_shift_left(
                        quotient_value, (unsigned int)quotient_exponent);
                    if ((int32_t)quotient_value !=
                        (int32_t)frame_arithmetic_shift_right(
                            shifted, (unsigned int)quotient_exponent)) {
                        shifted = ((uint32_t)INT32_MAX) ^
                            (uint32_t)((int32_t)quotient_value >> 31);
                    }
                    quotient_value = shifted;
                }
                output = (int16_t)(quotient_value >> 16);
            }
            frame_store_i16(state, 0x64cU + offset, output);
            frame_store_i16(state, 0x69cU + offset,
                (int16_t)(((uint32_t)((int32_t)ratio * current * 2)) >> 16));
        }
        uint16_t combined = 0;
        for (int32_t index = 0; index < 8; ++index) {
            combined |= (uint16_t)frame_load_i16(
                state, 0x696U + (size_t)index * 2U);
        }
        if (combined == 0U) {
            for (int32_t index = 0; index < 8; ++index) {
                frame_store_i16(state,
                    0x696U + (size_t)index * 2U, 1);
            }
        }
        for (int32_t index = 0; index < 8; ++index) {
            size_t offset = (size_t)index * 2U;
            if (auxiliary_flag == 0 || index < 5) {
                int16_t first_threshold = frame_threshold_at_exponent(
                    frame_load_i16(state, 0x728));
                if (frame_load_i16(state, 0x6f8U + offset) >
                    first_threshold) {
                    first_threshold = frame_threshold_at_exponent(
                        frame_load_i16(state, 0x72a));
                    if (second_weight < 0x0ccc &&
                        frame_load_i16(state, 0x708U + offset) <
                        first_threshold) {
                        frame_store_i16(state, 0x708U + offset,
                                        first_threshold);
                    }
                }
                else {
                    int16_t second_threshold = frame_threshold_at_exponent(
                        frame_load_i16(state, 0x72a));
                    if (second_weight < 0x0ccc &&
                        frame_load_i16(state, 0x708U + offset) <
                        second_threshold) {
                        frame_store_i16(state, 0x708U + offset,
                                        second_threshold);
                    }
                }
            }
            else {
                int16_t third_threshold = frame_threshold_at_exponent(
                    frame_load_i16(state, 0x72c));
                if (frame_load_i16(state, 0x718U + offset) <
                    third_threshold) {
                    frame_store_i16(state, 0x718U + offset,
                                    third_threshold);
                }
                int16_t second_threshold = frame_threshold_at_exponent(
                    frame_load_i16(state, 0x72a));
                if (frame_load_i16(state, 0x708U + offset) <
                    second_threshold) {
                    frame_store_i16(state, 0x708U + offset,
                                    second_threshold);
                }
            }
        }
    }

    vocoder_copy_i16((int16_t *)(void *)(state + 0x86e),
                  (const int16_t *)(const void *)(state + 0x7ce), 80);
    frame_store_i16(state, 0x916, frame_load_i16(state, 0x912));
    frame_store_i16(state, 0x918, frame_load_i16(state, 0x914));
    vocoder_copy_i16((int16_t *)(void *)(state + 0x7ce),
                  (const int16_t *)(const void *)(state + 0x72e), 80);
    frame_store_i16(state, 0x912, frame_load_i16(state, 0x90e));
    frame_store_i16(state, 0x914, frame_load_i16(state, 0x910));
    vocoder_copy_i16(scratch,
                  sample_history + 2 * (114 - sample_count), 80);
    frame_store_i16(state, 0x90e, (int16_t)vocoder_correlate_window_40(
        (int16_t *)(void *)(state + 0x72e), scratch, 15,
        correlation_window));
    vocoder_copy_i16(scratch,
                  sample_history + 2 * (134 - sample_count), 80);
    frame_store_i16(state, 0x910, (int16_t)vocoder_correlate_window_40(
        (int16_t *)(void *)(state + 0x77e), scratch, 15,
        correlation_window));
}


void vocoder_extract_log_spectral_bands(
    int16_t *output,
    int32_t spacing,
    int32_t output_count,
    int32_t flags,
    vocoder_spectral_band_state *state,
    int32_t bias,
    const int32_t *spectra,
    const int16_t *exponents,
    int32_t spectrum_index)
{
    const int32_t *spectrum =
        spectra + spectrum_index * 129;
    int32_t base_exponent = exponents[spectrum_index];
    uint32_t scaled_spacing = (uint32_t)spacing << 9;
    uint32_t phase =
        frame_arithmetic_shift_right(scaled_spacing, 5) + 0x8000U;
    uint32_t step =
        frame_arithmetic_shift_right(scaled_spacing, 4);
    int32_t previous_index = (int32_t)
        frame_arithmetic_shift_right(phase, 16);
    int32_t previous_value = spectrum[previous_index];

    if (output_count > 0) {
        int16_t fraction =
            (int16_t)((uint16_t)phase >> 1);
        int64_t previous_partial = frame_arithmetic_shift_right64(
            (int64_t)fraction * previous_value, 15);

        for (int32_t output_index = 0;
             output_index < output_count;
             ++output_index) {
            phase += step;
            int32_t next_index = (int32_t)
                frame_arithmetic_shift_right(phase, 16);
            if (next_index > 127) {
                next_index = 127;
            }

            int64_t accumulated =
                (int64_t)previous_value - previous_partial;
            for (int32_t index = previous_index + 1;
                 index < next_index;
                 ++index) {
                accumulated += spectrum[index];
            }

            fraction = (int16_t)((uint16_t)phase >> 1);
            int32_t next_value = spectrum[next_index];
            int64_t next_partial = frame_arithmetic_shift_right64(
                (int64_t)fraction * next_value, 15);
            int64_t total = accumulated + next_partial;

            uint32_t mantissa;
            int32_t exponent;
            if (total == 0) {
                mantissa = 0;
                exponent = base_exponent;
            }
            else {
                uint64_t total_bits = (uint64_t)total;
                uint64_t magnitude =
                    total < 0 ? ~total_bits : total_bits;
                int32_t normalization_shift =
                    (int32_t)frame_count_leading_zeros64(
                        magnitude) - 33;
                uint64_t normalized =
                    normalization_shift >= 0
                    ? total_bits << (unsigned int)normalization_shift
                    : total_bits >> (unsigned int)(
                        -normalization_shift);
                mantissa = (uint32_t)normalized;
                exponent = base_exponent - normalization_shift;
            }

            if (exponent < -59 || mantissa == 0U) {
                output[output_index] = (int16_t)0x88aa;
            }
            else {
                int16_t logarithm = vocoder_log2_scaled(
                    (uint16_t)(mantissa >> 16),
                    exponent + 30, 5);
                output[output_index] = (int16_t)(
                    (uint16_t)(bias + 0xaa) +
                    (uint16_t)logarithm);
            }

            previous_index = next_index;
            previous_value = next_value;
            previous_partial = next_partial;
        }
    }

    if ((flags & 1) != 0) {
        int16_t old_pitch = state->smoothed_pitch;
        if (spacing < 0x3334) {
            int32_t multiplier;
            int32_t index;
            if (spacing < 0x199b) {
                if (spacing < 0x1112) {
                    multiplier = 3;
                    index = 2;
                }
                else {
                    multiplier = 2;
                    index = 1;
                }
            }
            else {
                multiplier = 1;
                index = 0;
            }

            uint32_t old_product =
                (uint32_t)(
                    (int32_t)old_pitch * 0x6666);
            uint32_t remaining_bits =
                ((uint32_t)(multiplier * spacing) << 16) -
                (old_product << 1);
            int32_t remaining = (int32_t)
                frame_arithmetic_shift_right(remaining_bits, 16);
            while (remaining > 0 && index >= 0 &&
                    index + 1 < output_count) {
                remaining -= spacing;
                int32_t decrease = frame_saturate_i32(
                    -(int64_t)spacing * 0x3c00);
                int32_t candidate = frame_saturate_i32(
                    (int64_t)decrease +
                    (int64_t)output[index + 1] * 0x10000);
                int16_t candidate_value = (int16_t)(
                    ((uint32_t)candidate >> 16) & 0xffffU);
                if (output[index] < candidate_value) {
                    output[index] = candidate_value;
                }
                index--;
            }
        }

        int64_t smoothed =
            (int64_t)old_pitch * 0xe666 +
            frame_arithmetic_shift_right64(
                (int64_t)(int16_t)spacing * 0x6666, 2);
        state->smoothed_pitch = (int16_t)(
            ((uint64_t)smoothed >> 16) & 0xffffU);
    }
}

void vocoder_inverse_cosine_transform(
    int16_t *values,
    int32_t transform_count,
    int32_t output_count)
{
    int16_t input[58];
    vocoder_copy_i16(input, values, (int16_t)transform_count);
    int16_t step = (int16_t)(transform_count == 1
        ? INT16_MAX
        : 0x8000 / transform_count);
    if (output_count > transform_count) {
        output_count = transform_count;
    }

    for (int32_t output_index = 0;
            output_index < output_count; ++output_index) {
        int32_t phase_increment = output_index * (int32_t)step;
        uint32_t phase =
            (uint32_t)(phase_increment >> 1) + 0x40U;
        int64_t sum = 0;
        for (int32_t input_index = 0;
                input_index < transform_count; ++input_index) {
            unsigned int table_index =
                (unsigned int)((phase & 0xffffU) >> 7);
            sum +=
                (int32_t)frame_cosine_q15(table_index) *
                (int32_t)input[input_index];
            phase += (uint32_t)(uint16_t)phase_increment;
        }

        if (sum == 0) {
            values[output_index] = 0;
            continue;
        }
        uint64_t sum_bits = (uint64_t)sum;
        uint64_t magnitude = sum < 0 ? ~sum_bits : sum_bits;
        int32_t leading = (int32_t)
            frame_count_leading_zeros64(magnitude);
        int32_t normalization = leading - 33;
        uint64_t normalized = normalization >= 0
            ? sum_bits << (unsigned int)normalization
            : sum_bits >> (unsigned int)(-normalization);
        int16_t mantissa =
            (int16_t)((normalized >> 16) & 0xffffU);
        int32_t exponent = 34 - leading;
        uint32_t product =
            (uint32_t)((int32_t)mantissa * (int32_t)step) << 1;
        uint32_t scaled = exponent < 0
            ? frame_arithmetic_shift_right(
                product, (unsigned int)(-exponent))
            : frame_shift_left(
                product, (unsigned int)exponent);
        values[output_index] = (int16_t)(
            (scaled + 0x8000U) >> 16);
    }
}

void vocoder_compute_band_weights(
    int16_t output[8],
    int16_t lower_bounds[8],
    const int16_t *band_values,
    const int16_t *band_basis,
    uint32_t band_mask,
    int32_t band,
    int32_t neighbor_distance,
    uint32_t *shape_flag)
{
    const int16_t *basis = band_basis + band * 8;
    for (int index = 0; index < 8; ++index) {
        lower_bounds[index] =
            basis[index] < 1 ? 1 : basis[index];
    }
    unsigned int center_bit =
        (unsigned int)(band + 2) & 0xffU;
    if (((band_mask >> center_bit) & 1U) == 0U) {
        vocoder_fill_i16(output, 0, 8);
        *shape_flag = 0;
        return;
    }

    vocoder_copy_i16(output, basis, 8);
    vocoder_inverse_cosine_transform(output, 8, 2);
    uint32_t relation =
        (uint32_t)((int32_t)output[0] * -0x8000) +
        (uint32_t)((int32_t)output[1] * -0x10000);
    int shape =
        relation != 0U &&
        (int32_t)((uint32_t)(int32_t)output[1] << 16) <=
        (int32_t)((uint32_t)(
            (int32_t)output[0] * -0x8000));
    *shape_flag = (uint32_t)shape;

    int16_t shifts[8] = {
        0, 0, 0, 0,
        (int16_t)shape, (int16_t)shape,
        (int16_t)shape, (int16_t)shape
    };
    int32_t selected[8];
    const int16_t *current = band_values + band * 8;
    unsigned int previous_bit =
        (unsigned int)(band + 2 - neighbor_distance) & 0xffU;
    unsigned int next_bit =
        (unsigned int)(band + 2 + neighbor_distance) & 0xffU;
    if (band == 0 ||
            ((band_mask >> previous_bit) & 1U) == 0U) {
        const int16_t *next =
            band_values + (band + neighbor_distance) * 8;
        int next_present =
            ((band_mask >> next_bit) & 1U) != 0U;
        for (int index = 0; index < 8; ++index) {
            int32_t next_value = next[index];
            if (next_present == 0 || next_value > 0x1999) {
                selected[index] =
                    (int32_t)((uint32_t)(uint16_t)current[index] << 16);
            }
            else if (next_value < 0x1127) {
                selected[index] =
                    (int32_t)((uint32_t)(
                        (int32_t)current[index] * 0x55c3) << 1);
            }
            else {
                int16_t factor = (int16_t)(
                    (uint16_t)next[index] +
                    (uint16_t)((uint32_t)next_value << 2));
                selected[index] =
                    (int32_t)((uint32_t)(
                        (int32_t)factor *
                        (int32_t)current[index]) << 1);
            }
        }
    }
    else {
        const int16_t *previous =
            band_values + (band - neighbor_distance) * 8;
        const int16_t *next =
            ((band_mask >> next_bit) & 1U) != 0U
                ? band_values + (band + neighbor_distance) * 8
                : current;
        for (int index = 0; index < 8; ++index) {
            int32_t previous_value = previous[index];
            int32_t next_value = next[index];
            int32_t current_value = current[index];
            int32_t choice;
            if (previous_value == next_value ||
                    previous_value < next_value) {
                choice = current_value <= next_value
                    ? current_value : next_value;
            }
            else {
                choice = previous_value < current_value
                    ? previous_value : current_value;
            }
            selected[index] = (int32_t)(
                (uint32_t)(uint16_t)choice << 16);
        }
    }

    for (int index = 0; index < 8; ++index) {
        uint32_t scaled;
        if (shifts[index] > 0) {
            unsigned int amount = (unsigned int)shifts[index];
            scaled = frame_shift_left(
                (uint32_t)selected[index], amount);
            if (selected[index] !=
                    (int32_t)frame_arithmetic_shift_right(
                        scaled, amount)) {
                scaled = selected[index] < 0
                    ? 0x80000000U : 0x7fffffffU;
            }
        }
        else {
            scaled = frame_arithmetic_shift_right(
                (uint32_t)selected[index],
                (unsigned int)(-shifts[index]));
        }
        int16_t mantissa = (int16_t)(scaled >> 16);
        if ((scaled >> 16) == 0U) {
            output[index] = INT16_MAX;
            continue;
        }
        int32_t logarithm =
            vocoder_log2_q15((uint16_t)mantissa, 0);
        int32_t weight =
            -0xa934 - (int32_t)frame_arithmetic_shift_right(
                (uint32_t)logarithm, 1);
        if (weight < 0) {
            output[index] = 0;
        }
        else if (weight > 0xfffe) {
            output[index] = INT16_MAX;
        }
        else {
            output[index] = (int16_t)(
                ((uint32_t)weight * 0x8000U + 0x8000U) >>
                16);
        }
    }
}

void vocoder_filter_log_spectrum(
    int16_t output[56],
    const int16_t *input,
    int32_t count)
{
    static const int16_t coefficients[10] = {
        0x47eb, 0x17f9,
        0x0e62, 0x0a46, 0x07fe,
        0x068a, 0x0588, 0x04cb,
        0x043b, 0x03c9
    };
    int16_t extended[64];
    for (int32_t index = 0; index < count; ++index) {
        extended[index] = input[index];
    }
    int32_t tail = (int32_t)(
        (uint32_t)(uint16_t)input[count - 1] << 16);
    for (int32_t index = count; index < 64; ++index) {
        int64_t decreased = (int64_t)tail - 0x05c50000LL;
        if (decreased < INT32_MIN) {
            tail = INT32_MIN;
        }
        else if (decreased > INT32_MAX) {
            tail = INT32_MAX;
        }
        else {
            tail = (int32_t)decreased;
        }
        extended[index] = (int16_t)((uint32_t)tail >> 16);
    }

    int16_t samples[39 + 85];
    int16_t *center = samples + 19;
    center[0] = extended[0];
    center[1] = 0;
    for (int index = 0; index < 64; ++index) {
        center[index + 2] = extended[index];
    }
    for (int index = 0; index <= 18; ++index) {
        center[-index] = extended[index];
    }
    if (count >= 46) {
        for (int32_t index = 0; index <= count - 46; ++index) {
            center[66 + index] = center[64 - index];
        }
    }

    for (int32_t output_index = 0;
            output_index < count; ++output_index) {
        uint32_t accumulator = 0;
        for (int tap = 0; tap < 10; ++tap) {
            int32_t difference =
                (int32_t)center[output_index + 3 + tap * 2] -
                (int32_t)center[output_index + 1 - tap * 2];
            int32_t product =
                difference * (int32_t)coefficients[tap];
            accumulator += (uint32_t)product << 1;
        }
        uint32_t scaled =
            (accumulator & 0x80000000U) +
            (accumulator & 0x3fffffffU) * 2U;
        output[output_index] = (int16_t)(scaled >> 16);
    }
    vocoder_fill_i16(output + count, 0, 56 - count);
}

void vocoder_populate_spectral_frame(
    vocoder_frame_state *frame,
    vocoder_spectral_band_state *state,
    int32_t slice_index,
    int32_t spectrum_index,
    const vocoder_spectral_config *config,
    const int16_t *copy_source)
{
    int32_t effective_spectrum_index =
        spectrum_index + slice_index;
    if (slice_index == 0) {
        int32_t count = (int16_t)frame->header[2];
        if ((int16_t)frame->header[0] == 2) {
            vocoder_copy_i16(
                frame->spectrum, copy_source + 1, count);
        }
        else {
            uint32_t flags =
                (uint32_t)frame->header[4] |
                (uint32_t)frame->header[5] << 16;
            vocoder_extract_log_spectral_bands(
                frame->spectrum,
                (int16_t)frame->header[6],
                count,
                (int32_t)flags,
                state,
                config->logarithm_bias,
                config->spectra,
                config->exponents,
                effective_spectrum_index);
        }
    }
    if (config->callback_enabled != 0 &&
            (int16_t)frame->header[0] != 3) {
        config->callback(
            config->callback_selector,
            config->callback_context,
            frame,
            config->callback_argument_3,
            config->callback_argument_4,
            config->callback_argument_5,
            config->callback_argument_6,
            slice_index,
            effective_spectrum_index,
            config->callback_mode);
    }
}

int32_t vocoder_vector_normalization_shift(
    const int16_t *values,
    int32_t count)
{
    int32_t maximum = 0;
    for (int32_t index = 0; index < count; ++index) {
        int32_t magnitude = values[index];
        if (magnitude < 0) {
            magnitude = -magnitude;
        }
        if (maximum < magnitude) {
            maximum = magnitude;
        }
    }
    uint32_t scaled = maximum == 0x8000
        ? 0x7fff0000U
        : (uint32_t)maximum << 16;
    uint32_t magnitude =
        (scaled & 0x80000000U) != 0U ? ~scaled : scaled;
    unsigned int shift = scaled != 0U
        ? frame_count_leading_zeros(magnitude) - 1U
        : 0U;
    return -(int32_t)shift;
}

void vocoder_normalize_i16_vector(
    int16_t *output,
    const int16_t *input,
    int32_t count,
    int32_t *exponent)
{
    int32_t shift =
        vocoder_vector_normalization_shift(input, count);
    if (output != input || shift != 0) {
        vocoder_shift_i16(output, input, count, -shift);
    }
    *exponent += shift;
}

int32_t vocoder_multiply_complex_by_real(
    int16_t *output,
    const uint32_t *complex_input,
    int32_t input_exponent,
    const int16_t *real_input,
    int32_t real_exponent,
    int32_t count)
{
    for (int32_t index = 0; index < count; ++index) {
        uint32_t packed = complex_input[index];
        int16_t scale = real_input[index];
        int32_t low_product =
            (int32_t)(int16_t)packed * (int32_t)scale;
        int32_t high_product =
            (int32_t)(int16_t)(packed >> 16) *
            (int32_t)scale;
        output[index * 2] = (int16_t)(
            ((uint32_t)low_product << 1) >> 16);
        output[index * 2 + 1] = (int16_t)(
            ((uint32_t)high_product << 1) >> 16);
    }
    int32_t exponent = input_exponent + real_exponent;
    vocoder_normalize_i16_vector(
        output, output, count * 2, &exponent);
    return exponent;
}

int32_t vocoder_multiply_complex_vector(
    uint32_t *output,
    const uint32_t *coefficient,
    int32_t coefficient_exponent,
    const uint32_t *input,
    int32_t input_exponent,
    int32_t count)
{
    int16_t coefficient_real = (int16_t)*coefficient;
    int16_t coefficient_imaginary =
        (int16_t)(*coefficient >> 16);
    for (int32_t index = 0; index < count; ++index) {
        int16_t input_real = (int16_t)input[index];
        int16_t input_imaginary =
            (int16_t)(input[index] >> 16);
        int32_t real =
            (int32_t)coefficient_real * (int32_t)input_real -
            (int32_t)coefficient_imaginary *
                (int32_t)input_imaginary;
        int32_t imaginary =
            (int32_t)coefficient_real *
                (int32_t)input_imaginary +
            (int32_t)coefficient_imaginary *
                (int32_t)input_real;
        output[index] =
            ((uint32_t)imaginary & 0xffff0000U) |
            (frame_arithmetic_shift_right(
                (uint32_t)real, 16) & 0xffffU);
    }
    int32_t exponent =
        coefficient_exponent + 1 + input_exponent;
    vocoder_normalize_i16_vector(
        (int16_t *)output, (const int16_t *)output,
        count * 2, &exponent);
    return exponent;
}

static int16_t frame_fourth_root_step(
    int16_t accumulator,
    int16_t argument,
    int16_t coefficient)
{
    int32_t product =
        (int32_t)accumulator * (int32_t)argument;
    uint32_t rounded =
        ((uint32_t)product * 2U + 0x8000U) &
        0xffff0000U;
    uint32_t combined =
        rounded + ((uint32_t)(int32_t)coefficient << 16);
    return (int16_t)(combined >> 16);
}

int32_t vocoder_fourth_root_vector(
    int16_t *output,
    const uint32_t *input,
    int32_t input_exponent,
    int32_t count)
{
    static const int16_t coefficients[9] = {
        (int16_t)0xf073, 0x3f26,
        (int16_t)0x9292, 0x7b83, 0x4251,
        0x4c1c, 0x5a82, 0x6ba2,
        (int16_t)0xe676
    };
    int32_t common_exponent =
        (input_exponent + 3) >> 2;
    for (int32_t index = 0; index < count; ++index) {
        uint32_t value = input[index];
        uint32_t mantissa = 0;
        int32_t exponent = 0;
        if (value != 0U) {
            uint32_t magnitude =
                (value & 0x80000000U) != 0U
                    ? ~value : value;
            unsigned int input_shift =
                frame_count_leading_zeros(magnitude) - 1U;
            int16_t argument = (int16_t)(
                frame_shift_left(value, input_shift) >> 16);

            int32_t first_product =
                (int32_t)coefficients[0] *
                (int32_t)argument;
            uint32_t first =
                ((uint32_t)first_product * 2U + 0x8000U) &
                0xffff0000U;
            int16_t accumulator = (int16_t)(
                (first +
                 ((uint32_t)(int32_t)coefficients[1] << 16)) >>
                16);
            accumulator = frame_fourth_root_step(
                accumulator, argument, coefficients[2]);
            accumulator = frame_fourth_root_step(
                accumulator, argument, coefficients[3]);
            int32_t final_product =
                (int32_t)accumulator * (int32_t)argument;
            uint32_t final_term =
                ((uint32_t)final_product * 2U + 0x8000U) &
                0xffff0000U;
            mantissa =
                (uint32_t)((int32_t)coefficients[4] * 0x8000) +
                frame_arithmetic_shift_right(final_term, 1);

            uint32_t root_magnitude =
                (mantissa & 0x80000000U) != 0U
                    ? ~mantissa : mantissa;
            unsigned int root_shift =
                frame_count_leading_zeros(root_magnitude) - 1U;
            mantissa = frame_shift_left(mantissa, root_shift);
            exponent =
                ((input_exponent - (int32_t)input_shift) + 1) -
                (int32_t)root_shift;
            unsigned int remainder =
                (unsigned int)exponent & 3U;
            if (remainder != 0U) {
                exponent++;
                int32_t correction =
                    (int32_t)(int16_t)(mantissa >> 16) *
                    (int32_t)coefficients[4 + remainder];
                mantissa =
                    ((uint32_t)correction * 2U + 0x8000U) &
                    0xffff0000U;
            }
        }

        int32_t element_exponent =
            (exponent + 3) >> 2;
        int32_t difference =
            element_exponent - common_exponent;
        uint32_t scaled = difference < 0
            ? frame_arithmetic_shift_right(
                mantissa, (unsigned int)(-difference))
            : frame_shift_left(
                mantissa, (unsigned int)difference);
        output[index] = (int16_t)(
            (scaled + 0x8000U) >> 16);
    }
    vocoder_normalize_i16_vector(
        output, output, count, &common_exponent);
    return common_exponent;
}

static uint32_t frame_adjacent_complex_product(
    uint32_t current,
    uint32_t next)
{
    int16_t current_real = (int16_t)current;
    int16_t current_imaginary =
        (int16_t)(current >> 16);
    int16_t next_real = (int16_t)next;
    int16_t next_imaginary =
        (int16_t)(next >> 16);
    int32_t real =
        (int32_t)current_real * (int32_t)next_real +
        (int32_t)current_imaginary *
            (int32_t)next_imaginary;
    int32_t imaginary =
        (int32_t)next_real * (int32_t)current_imaginary -
        (int32_t)next_imaginary * (int32_t)current_real;
    return
        ((uint32_t)imaginary & 0xffff0000U) |
        (frame_arithmetic_shift_right(
            (uint32_t)real, 16) & 0xffffU);
}

int32_t vocoder_adjacent_complex_products(
    uint32_t values[129],
    int32_t input_exponent)
{
    uint32_t current = values[0];
    for (int index = 0; index < 128; index += 2) {
        uint32_t middle = values[index + 1];
        uint32_t next = values[index + 2];
        values[index] =
            frame_adjacent_complex_product(current, middle);
        values[index + 1] =
            frame_adjacent_complex_product(middle, next);
        current = next;
    }
    int32_t exponent = input_exponent * 2 + 1;
    vocoder_normalize_i16_vector(
        (int16_t *)values, (const int16_t *)values,
        256, &exponent);
    return exponent;
}

static void frame_twiddle_160(
    unsigned int index,
    int16_t *real,
    int16_t *imaginary)
{
    static const int16_t first_quadrant[80] = {
        32762, 643, 32711, 1929, 32610, 3212, 32459, 4490,
        32258, 5760, 32007, 7022, 31706, 8274, 31357, 9512,
        30959, 10736, 30514, 11943, 30022, 13132, 29483, 14300,
        28899, 15447, 28270, 16569, 27598, 17666, 26883, 18736,
        26127, 19777, 25330, 20788, 24494, 21766, 23621, 22711,
        22711, 23621, 21766, 24494, 20788, 25330, 19777, 26127,
        18736, 26883, 17666, 27598, 16569, 28270, 15447, 28899,
        14300, 29483, 13132, 30022, 11943, 30514, 10736, 30959,
        9512, 31357, 8274, 31706, 7022, 32007, 5760, 32258,
        4490, 32459, 3212, 32610, 1929, 32711, 643, 32762
    };
    index %= 160U;
    if (index < 40U) {
        *real = first_quadrant[index * 2U];
        *imaginary = first_quadrant[index * 2U + 1U];
    }
    else if (index < 80U) {
        unsigned int mirror = 79U - index;
        *real = (int16_t)-first_quadrant[mirror * 2U];
        *imaginary = first_quadrant[mirror * 2U + 1U];
    }
    else if (index < 120U) {
        unsigned int mirror = index - 80U;
        *real = (int16_t)-first_quadrant[mirror * 2U];
        *imaginary =
            (int16_t)-first_quadrant[mirror * 2U + 1U];
    }
    else {
        unsigned int mirror = 159U - index;
        *real = first_quadrant[mirror * 2U];
        *imaginary =
            (int16_t)-first_quadrant[mirror * 2U + 1U];
    }
}

static int16_t frame_normalize_correlation(
    int64_t sum,
    int8_t *exponent)
{
    int64_t doubled = sum * 2;
    if (doubled == 0) {
        *exponent = 0;
        return 0;
    }
    uint64_t magnitude = doubled < 0
        ? (uint64_t)(-doubled)
        : (uint64_t)doubled;
    int32_t leading = (int32_t)
        frame_count_leading_zeros64(magnitude);
    int32_t normalization = leading - 33;
    uint64_t bits = (uint64_t)doubled;
    uint64_t normalized = normalization >= 0
        ? bits << (unsigned int)normalization
        : bits >> (unsigned int)(-normalization);
    int16_t mantissa =
        (int16_t)((normalized >> 16) & 0xffffU);
    int32_t product =
        (int32_t)mantissa * 0x6666;
    *exponent = (int8_t)(33 - leading);
    return (int16_t)(
        ((uint32_t)product >> 15) & 0xffffU);
}

int32_t vocoder_correlate_window_40(
    int16_t output[40],
    int16_t values[80],
    int32_t input_exponent,
    const int16_t window[80])
{
    vocoder_normalize_i16_vector(
        values, values, 80, &input_exponent);
    int16_t complex_values[40];
    for (int index = 0; index < 20; ++index) {
        int32_t real =
            (int32_t)values[index] * (int32_t)window[index] -
            (int32_t)values[39 - index] *
                (int32_t)window[39 - index];
        int32_t imaginary =
            (int32_t)values[index + 40] *
                (int32_t)window[39 - index] +
            (int32_t)values[79 - index] *
                (int32_t)window[index];
        complex_values[index * 2] = (int16_t)
            frame_arithmetic_shift_right((uint32_t)real, 16);
        complex_values[index * 2 + 1] = (int16_t)
            frame_arithmetic_shift_right((uint32_t)imaginary, 16);
    }
    vocoder_normalize_i16_vector(
        complex_values, complex_values, 40, &input_exponent);

    int8_t exponents[40];
    int32_t maximum_exponent = -32;
    for (int output_index = 0;
            output_index < 40; ++output_index) {
        unsigned int table_index =
            (unsigned int)(139 - output_index);
        unsigned int table_step =
            (unsigned int)(output_index * 2 + 1);
        int64_t sum = 0;
        for (int input_index = 0;
                input_index < 20; ++input_index) {
            int16_t table_real;
            int16_t table_imaginary;
            frame_twiddle_160(
                table_index, &table_real, &table_imaginary);
            int16_t value_real =
                complex_values[input_index * 2];
            int16_t value_imaginary =
                complex_values[input_index * 2 + 1];
            if ((output_index & 1) == 0) {
                sum +=
                    (int32_t)value_real * (int32_t)table_real +
                    (int32_t)value_imaginary *
                        (int32_t)table_imaginary;
            }
            else {
                sum +=
                    (int32_t)value_real *
                        (int32_t)table_imaginary +
                    (int32_t)value_imaginary *
                        (int32_t)table_real;
            }
            table_index =
                (table_index + 160U - table_step) % 160U;
        }
        output[output_index] =
            frame_normalize_correlation(
                sum, &exponents[output_index]);
        if (output[output_index] != 0 &&
                maximum_exponent < exponents[output_index]) {
            maximum_exponent = exponents[output_index];
        }
    }

    if (maximum_exponent == -32) {
        return -32;
    }
    for (int index = 0; index < 40; ++index) {
        int32_t difference =
            (int32_t)exponents[index] - maximum_exponent;
        uint32_t value =
            (uint32_t)(int32_t)output[index] << 16;
        uint32_t aligned = difference < 0
            ? frame_arithmetic_shift_right(
                value, (unsigned int)(-difference))
            : frame_shift_left(
                value, (unsigned int)difference);
        output[index] = (int16_t)(aligned >> 16);
    }
    return input_exponent - 4 + maximum_exponent;
}

int32_t vocoder_sum_i16(
    const int16_t *values,
    int32_t count)
{
    int32_t sum = 0;
    for (int32_t index = 0; index < count; ++index) {
        sum += values[index];
    }
    return sum;
}

int16_t vocoder_max_i16(
    const int16_t *values,
    int32_t count)
{
    int16_t maximum = INT16_MIN;
    for (int32_t index = 0; index < count; ++index) {
        if (maximum < values[index]) {
            maximum = values[index];
        }
    }
    return maximum;
}

int32_t vocoder_compare_scaled(
    int16_t left,
    int32_t left_exponent,
    int16_t right,
    int32_t right_exponent)
{
    uint32_t left_value =
        (uint32_t)(int32_t)left << 16;
    uint32_t right_value =
        (uint32_t)(int32_t)right << 16;
    int32_t difference =
        right_exponent - left_exponent;
    if (difference <= 0) {
        unsigned int shift = difference < -32
            ? 32U : (unsigned int)(-difference);
        right_value = frame_arithmetic_shift_right(
            right_value, shift);
    }
    else {
        unsigned int shift = difference > 32
            ? 32U : (unsigned int)difference;
        left_value = frame_arithmetic_shift_right(
            left_value, shift);
    }
    return (int32_t)right_value < (int32_t)left_value;
}

static int32_t frame_normalize_i64_to_i32(
    int64_t value,
    int32_t *exponent)
{
    if (value == 0) {
        return 0;
    }
    uint64_t magnitude = value < 0
        ? (uint64_t)(-value)
        : (uint64_t)value;
    int32_t shift = (int32_t)
        frame_count_leading_zeros64(magnitude) - 33;
    uint64_t bits = (uint64_t)value;
    if (shift >= 0) {
        *exponent -= shift;
        return (int32_t)(uint32_t)(
            bits << (unsigned int)shift);
    }
    unsigned int right = (unsigned int)(-shift);
    *exponent += (int32_t)right;
    uint64_t shifted = bits >> right;
    if (value < 0) {
        shifted |= UINT64_MAX << (64U - right);
    }
    return (int32_t)(uint32_t)shifted;
}

int32_t vocoder_sum_i32_normalized(
    const int32_t *values,
    int32_t count,
    int32_t *exponent)
{
    int64_t sum = 0;
    for (int32_t index = 0; index < count; ++index) {
        sum += values[index];
    }
    return frame_normalize_i64_to_i32(sum, exponent);
}

int32_t vocoder_vector_energy(
    int32_t *exponent,
    const int16_t *values,
    uint32_t count)
{
    int64_t sum = 0;
    for (uint32_t index = 0; index < count; ++index) {
        int32_t value = values[index];
        sum += (int64_t)value * value;
    }
    *exponent = 0;
    return frame_normalize_i64_to_i32(sum * 2, exponent);
}

int16_t vocoder_align_by_exponents(
    int16_t *output,
    const int16_t *values,
    const int16_t *exponents,
    int32_t count)
{
    int16_t maximum = vocoder_max_i16(exponents, count);
    for (int32_t index = 0; index < count; ++index) {
        unsigned int shift = (unsigned int)(
            (int32_t)maximum - exponents[index]) & 0xffU;
        output[index] = (int16_t)frame_arithmetic_shift_right(
            (uint32_t)(int32_t)values[index], shift);
    }
    return maximum;
}

int16_t vocoder_complex_vector_energy(
    int16_t *exponent,
    const uint32_t *values,
    int16_t input_exponent,
    int32_t complex_count)
{
    int32_t energy_exponent;
    int32_t energy = vocoder_vector_energy(
        &energy_exponent,
        (const int16_t *)values,
        (uint32_t)complex_count * 2U);
    *exponent = (int16_t)(
        energy_exponent + (int32_t)input_exponent * 2);
    return (int16_t)frame_arithmetic_shift_right(
        (uint32_t)energy, 16);
}

int16_t vocoder_ratio_of_i32_sums(
    const int32_t numerator_values[128],
    int32_t numerator_exponent,
    const int32_t denominator_values[128],
    int32_t denominator_exponent,
    int32_t *result_exponent)
{
    int32_t numerator = vocoder_sum_i32_normalized(
        numerator_values, 128, &numerator_exponent);
    int32_t denominator = vocoder_sum_i32_normalized(
        denominator_values, 128, &denominator_exponent);
    int16_t denominator_mantissa =
        (int16_t)frame_arithmetic_shift_right(
            (uint32_t)denominator, 16);
    if (denominator_mantissa == 0) {
        denominator_mantissa = 0x68db;
        denominator_exponent = -13;
    }
    return vocoder_normalized_divide(
        numerator, numerator_exponent,
        denominator_mantissa, denominator_exponent,
        result_exponent);
}

static int16_t frame_i32_mantissa_at_exponent(
    int32_t value,
    int32_t value_exponent,
    int32_t target_exponent)
{
    int32_t difference = value_exponent - target_exponent;
    uint32_t aligned = difference < 0
        ? frame_arithmetic_shift_right(
            (uint32_t)value, (unsigned int)(-difference))
        : frame_shift_left(
            (uint32_t)value, (unsigned int)difference);
    return (int16_t)frame_arithmetic_shift_right(aligned, 16);
}

int32_t vocoder_align_two_i32_sums(
    int16_t *left,
    int16_t *right,
    const int32_t *values,
    int32_t input_exponent,
    int32_t left_index,
    int32_t right_index)
{
    int32_t left_exponent = input_exponent;
    int32_t left_sum = vocoder_sum_i32_normalized(
        values + left_index, 5, &left_exponent);
    int32_t right_exponent = input_exponent;
    int32_t right_sum = vocoder_sum_i32_normalized(
        values + right_index, 5, &right_exponent);
    int32_t common_exponent = left_exponent < right_exponent
        ? right_exponent : left_exponent;
    *left = frame_i32_mantissa_at_exponent(
        left_sum, left_exponent, common_exponent);
    *right = frame_i32_mantissa_at_exponent(
        right_sum, right_exponent, common_exponent);
    return common_exponent;
}

int32_t vocoder_sum_adaptive_intervals(
    const int16_t *values,
    int16_t step_mantissa,
    int32_t step_exponent,
    int32_t total_count)
{
    int32_t half_count = total_count >> 1;
    uint32_t step = (uint32_t)(int32_t)step_mantissa << 16;
    if (step_exponent < 15) {
        step = frame_arithmetic_shift_right(
            step, (unsigned int)(15 - step_exponent));
    }
    else {
        step = frame_shift_left(
            step, (unsigned int)(step_exponent - 15));
    }
    uint32_t cursor =
        step - frame_arithmetic_shift_right(step, 2) + 0x10000U;
    int32_t index = (int32_t)frame_arithmetic_shift_right(
        cursor, 16);
    int32_t sum = 0;
    while (index < half_count) {
        int32_t next_index = (int32_t)frame_arithmetic_shift_right(
            cursor + frame_arithmetic_shift_right(step, 1), 16);
        int32_t interval_count = next_index < half_count
            ? next_index - index : half_count - index;
        sum += vocoder_sum_i16(values + index, interval_count);
        cursor += frame_arithmetic_shift_right(step, 1) * 2U;
        index = (int32_t)frame_arithmetic_shift_right(cursor, 16);
    }
    return sum;
}

int16_t vocoder_tail_sum_complement(
    int16_t numerator,
    int32_t numerator_exponent,
    const int16_t values[32],
    int16_t start_mantissa,
    int32_t start_exponent)
{
    uint32_t start_value =
        (uint32_t)(int32_t)start_mantissa << 16;
    if (start_exponent < 16) {
        start_value = frame_arithmetic_shift_right(
            start_value, (unsigned int)(16 - start_exponent));
    }
    else {
        start_value = frame_shift_left(
            start_value, (unsigned int)(start_exponent - 16));
    }
    int32_t start = (int32_t)frame_arithmetic_shift_right(
        start_value + 0x10000U, 16);
    int32_t sum = vocoder_sum_i16(values + start, 32 - start);
    if (sum < 1) {
        return INT16_MAX;
    }

    unsigned int normalization =
        frame_count_leading_zeros((uint32_t)sum) - 1U;
    int16_t denominator = (int16_t)
        frame_arithmetic_shift_right(
            (uint32_t)sum << normalization, 16);
    int32_t result_exponent;
    int16_t quotient = vocoder_normalized_divide(
        (int32_t)((uint32_t)(int32_t)numerator << 16),
        numerator_exponent,
        denominator,
        -(int32_t)normalization,
        &result_exponent);
    if (result_exponent > 0) {
        return 0;
    }
    uint32_t aligned =
        (uint32_t)(int32_t)quotient << 16;
    if (result_exponent != 0) {
        aligned = frame_arithmetic_shift_right(
            aligned, (unsigned int)(-result_exponent));
    }
    if (aligned == 0x80000000U) {
        return -2;
    }
    return (int16_t)((0x7fff0000U - aligned) >> 16);
}

static int32_t frame_fixed_normalization_shift(uint32_t value)
{
    if (value == 0U) {
        return 0;
    }
    uint32_t magnitude = (value & 0x80000000U) != 0U
        ? ~value : value;
    return (int32_t)frame_count_leading_zeros(magnitude) - 1;
}

static uint32_t frame_scale_i32(uint32_t value, int32_t shift)
{
    return shift < 0
        ? frame_arithmetic_shift_right(
            value, (unsigned int)(-shift))
        : frame_shift_left(value, (unsigned int)shift);
}

int16_t vocoder_atan_scaled(
    int16_t input,
    int32_t input_exponent,
    int32_t *result_exponent)
{
    int32_t sign = 1;
    int16_t x_signed = input;
    if (x_signed < 0) {
        sign = -1;
        x_signed = x_signed == INT16_MIN
            ? INT16_MAX : (int16_t)-x_signed;
    }
    uint32_t x = (uint16_t)x_signed;
    int32_t x_exponent = input_exponent;
    uint32_t square;
    uint32_t linear;
    uint32_t offset;
    int reciprocal_range = 0;

    if (vocoder_compare_scaled(
            (int16_t)x, x_exponent, 0x4d41, 2)) {
        int16_t reciprocal = vocoder_normalized_divide(
            0x40000000, 1, (int16_t)x, x_exponent,
            &x_exponent);
        if (reciprocal == INT16_MIN) {
            x = 0x7fffU;
            square = 0x7ffe0002U;
            linear = 0x7fff0000U;
        }
        else {
            reciprocal = (int16_t)-reciprocal;
            x = (uint16_t)reciprocal;
            square = (uint32_t)(
                (int32_t)reciprocal * reciprocal) * 2U;
            linear = x << 16;
        }
        offset = 0x64880000U;
        reciprocal_range = 1;
    }
    else if (vocoder_compare_scaled(
                 (int16_t)x, x_exponent, 0x6a0a, -1)) {
        int32_t sum_exponent;
        uint16_t sum = vocoder_normalized_add(
            0x4000, 1, (int16_t)x, x_exponent,
            &sum_exponent);
        int32_t difference_exponent;
        uint16_t difference = vocoder_normalized_subtract(
            0x4000, 1, (int16_t)x, x_exponent,
            &difference_exponent);
        uint32_t difference_q31 = (uint32_t)difference << 16;
        int16_t ratio;
        if ((difference_q31 & 0x80000000U) != 0U) {
            uint32_t magnitude = difference_q31 == 0x80000000U
                ? 0x7fffffffU : 0U - difference_q31;
            ratio = vocoder_normalized_divide(
                (int32_t)magnitude, difference_exponent,
                (int16_t)sum, sum_exponent, &x_exponent);
            if (ratio == INT16_MIN) {
                x = 0x7fffU;
                square = 0x7ffe0002U;
                linear = 0x7fff0000U;
            }
            else {
                ratio = (int16_t)-ratio;
                x = (uint16_t)ratio;
                square = (uint32_t)((int32_t)ratio * ratio) * 2U;
                linear = x << 16;
            }
        }
        else {
            ratio = vocoder_normalized_divide(
                (int32_t)difference_q31, difference_exponent,
                (int16_t)sum, sum_exponent, &x_exponent);
            x = (uint16_t)ratio;
            square = (uint32_t)((int32_t)ratio * ratio) * 2U;
            linear = x << 16;
        }
        offset = 0x64880000U;
    }
    else {
        square = x * x * 2U;
        linear = x << 16;
        offset = 0;
    }

    int32_t square_shift =
        frame_fixed_normalization_shift(square);
    int16_t square_mantissa = (int16_t)(
        frame_shift_left(square, (unsigned int)square_shift) >> 16);
    int32_t square_exponent =
        x_exponent * 2 - square_shift;

    uint32_t polynomial = frame_scale_i32(
        (uint32_t)((int32_t)square_mantissa * 0xe38e),
        square_exponent - 1);
    polynomial += 0xb6db0000U;
    int32_t coefficient_shift =
        frame_fixed_normalization_shift(polynomial);
    int16_t coefficient = (int16_t)(
        frame_shift_left(
            polynomial, (unsigned int)coefficient_shift) >> 16);
    int32_t product_exponent =
        square_exponent - coefficient_shift - 2;
    uint32_t product = frame_scale_i32(
        (uint32_t)((int32_t)coefficient *
                   square_mantissa * 2),
        product_exponent + 2);

    polynomial = product + 0x66660000U;
    coefficient_shift =
        frame_fixed_normalization_shift(polynomial);
    coefficient = (int16_t)(
        frame_shift_left(
            polynomial, (unsigned int)coefficient_shift) >> 16);
    product_exponent =
        square_exponent - coefficient_shift - 2;
    product = frame_scale_i32(
        (uint32_t)((int32_t)coefficient *
                   square_mantissa * 2),
        product_exponent + 1);

    polynomial = product + 0xaaab0000U;
    coefficient_shift =
        frame_fixed_normalization_shift(polynomial);
    coefficient = (int16_t)(
        frame_shift_left(
            polynomial, (unsigned int)coefficient_shift) >> 16);
    uint32_t final_product =
        (uint32_t)((int32_t)coefficient *
                   square_mantissa * 2);
    int32_t final_shift =
        frame_fixed_normalization_shift(final_product);
    int16_t final_mantissa = (int16_t)(
        frame_shift_left(
            final_product, (unsigned int)final_shift) >> 16);
    uint32_t correction = (uint32_t)(
        (int32_t)(int16_t)x * final_mantissa * 2);
    correction = frame_scale_i32(
        correction,
        square_exponent - coefficient_shift - final_shift - 1);

    uint32_t angle = linear + correction;
    angle = frame_scale_i32(angle, x_exponent - 1);
    if (!reciprocal_range) {
        offset = frame_arithmetic_shift_right(offset, 1);
    }
    angle += offset;

    int32_t normalization = frame_fixed_normalization_shift(angle);
    int16_t result = (int16_t)(
        frame_shift_left(angle, (unsigned int)normalization) >> 16);
    *result_exponent = result == 0 ? 0 : 1 - normalization;
    if (result != 0 && sign < 0) {
        result = result == INT16_MIN
            ? INT16_MAX : (int16_t)-result;
    }
    return result;
}

int16_t vocoder_atan2_scaled(
    int16_t numerator,
    int32_t numerator_exponent,
    int16_t denominator,
    int32_t denominator_exponent,
    int32_t *result_exponent)
{
    unsigned int quadrant = denominator < 0 ? 2U : 0U;
    if (numerator < 0) {
        quadrant |= 1U;
    }
    if (denominator == 0) {
        if ((quadrant & 1U) != 0U) {
            *result_exponent = 1;
            return (int16_t)-0x6488;
        }
        if (numerator != 0) {
            *result_exponent = 1;
            return 0x6488;
        }
        *result_exponent = 0;
        return 0;
    }
    if (numerator == 0) {
        if ((quadrant & 2U) != 0U) {
            *result_exponent = 2;
            return 0x6488;
        }
        *result_exponent = 0;
        return 0;
    }

    int32_t output_base_exponent;
    int32_t atan_bias;
    uint32_t offset;
    if (quadrant >= 2U) {
        output_base_exponent = 3;
        atan_bias = -3;
        int16_t constant = quadrant == 2U
            ? 0x6488 : (int16_t)-0x6488;
        offset = (uint32_t)(int32_t)constant << 15;
    }
    else {
        output_base_exponent = 1;
        atan_bias = -1;
        offset = 0;
    }

    uint32_t denominator_q31 =
        (uint32_t)(int32_t)denominator << 16;
    uint32_t numerator_q31 =
        (uint32_t)(int32_t)numerator << 16;
    int32_t denominator_shift =
        frame_fixed_normalization_shift(denominator_q31);
    int32_t numerator_shift =
        frame_fixed_normalization_shift(numerator_q31);
    int32_t ratio_exponent;
    int16_t ratio = vocoder_normalized_divide(
        (int32_t)frame_shift_left(
            numerator_q31, (unsigned int)numerator_shift),
        numerator_exponent - numerator_shift,
        (int16_t)(frame_shift_left(
            denominator_q31,
            (unsigned int)denominator_shift) >> 16),
        denominator_exponent - denominator_shift,
        &ratio_exponent);
    int32_t atan_exponent;
    int16_t angle = vocoder_atan_scaled(
        ratio, ratio_exponent, &atan_exponent);
    uint32_t aligned = frame_scale_i32(
        (uint32_t)(int32_t)angle << 16,
        atan_bias + atan_exponent);
    uint32_t combined = aligned + offset;
    int32_t normalization =
        frame_fixed_normalization_shift(combined);
    *result_exponent = output_base_exponent - normalization;
    return (int16_t)(
        frame_shift_left(
            combined, (unsigned int)normalization) >> 16);
}

static int16_t frame_normalize_i32_mantissa(
    int32_t value,
    int32_t *exponent)
{
    if (value == 0) {
        *exponent = 0;
        return 0;
    }
    int32_t shift =
        frame_fixed_normalization_shift((uint32_t)value);
    *exponent = -shift;
    return (int16_t)(
        frame_shift_left(
            (uint32_t)value, (unsigned int)shift) >> 16);
}

int16_t vocoder_analyze_spectral_blocks(
    vocoder_spectral_history *state,
    int16_t pitch,
    const int16_t spectra[8][32],
    const int32_t exponents[8])
{
    for (int row = 4; row >= 2; --row) {
        vocoder_copy_i16(state->bands[row], state->bands[row - 2], 8);
        vocoder_copy_i16(state->bands[row + 5],
                      state->bands[row + 3], 8);
        state->exponents[row] = state->exponents[row - 2];
    }

    uint32_t pitch_q31 =
        (uint32_t)(int32_t)pitch << 16;
    int32_t first_start = (int32_t)frame_arithmetic_shift_right(
        frame_arithmetic_shift_right(pitch_q31, 10) + 0x10000U,
        16);
    int32_t second_start = (int32_t)frame_arithmetic_shift_right(
        frame_arithmetic_shift_right(pitch_q31, 11) + 0x10000U,
        16);
    int16_t low_mask = 0;
    int16_t local_exponents[8];

    for (int band = 0; band < 8; ++band) {
        low_mask = (int16_t)(low_mask * 2);
        int32_t tail_sum = vocoder_sum_i16(
            spectra[band] + first_start, 32 - first_start);
        int32_t tail_exponent;
        int16_t tail_mantissa = frame_normalize_i32_mantissa(
            tail_sum, &tail_exponent);
        int32_t interval_sum = vocoder_sum_adaptive_intervals(
            spectra[band], pitch, 6, 64);
        int32_t interval_exponent;
        int16_t interval_mantissa = frame_normalize_i32_mantissa(
            interval_sum, &interval_exponent);

        if (pitch > 0x0ccd) {
            int32_t alternate_tail_sum = vocoder_sum_i16(
                spectra[band] + second_start, 32 - second_start);
            int32_t alternate_tail_exponent;
            int16_t alternate_tail =
                frame_normalize_i32_mantissa(
                    alternate_tail_sum,
                    &alternate_tail_exponent);
            int32_t alternate_interval_sum =
                vocoder_sum_adaptive_intervals(
                    spectra[band], pitch, 5, 64);
            int32_t alternate_interval_exponent;
            int16_t alternate_interval =
                frame_normalize_i32_mantissa(
                    alternate_interval_sum,
                    &alternate_interval_exponent);

            uint32_t left_product = (uint32_t)(
                (int32_t)alternate_interval *
                tail_mantissa * 2);
            int32_t left_shift =
                frame_fixed_normalization_shift(left_product);
            int16_t left_mantissa = (int16_t)(
                frame_shift_left(
                    left_product, (unsigned int)left_shift) >> 16);
            int32_t left_exponent =
                alternate_interval_exponent -
                left_shift + tail_exponent;

            uint32_t right_product = (uint32_t)(
                (int32_t)interval_mantissa *
                alternate_tail * 2);
            int32_t right_shift =
                frame_fixed_normalization_shift(right_product);
            int16_t right_mantissa = (int16_t)(
                frame_shift_left(
                    right_product, (unsigned int)right_shift) >> 16);
            int32_t right_exponent =
                interval_exponent -
                right_shift + alternate_tail_exponent;
            if (vocoder_compare_scaled(
                    left_mantissa, left_exponent,
                    right_mantissa, right_exponent)) {
                tail_mantissa = alternate_tail;
                tail_exponent = alternate_tail_exponent;
                interval_mantissa = alternate_interval;
                interval_exponent = alternate_interval_exponent;
            }
        }

        state->bands[5][band] = tail_mantissa;
        local_exponents[band] =
            (int16_t)(exponents[band] + tail_exponent);
        int16_t ratio = vocoder_one_minus_normalized_ratio(
            interval_mantissa, interval_exponent,
            tail_mantissa, tail_exponent);
        state->bands[0][band] = ratio;
        if (ratio < 0x199a) {
            ++low_mask;
        }
    }

    int16_t maximum_exponent =
        vocoder_max_i16(local_exponents, 8);
    vocoder_align_by_exponents(
        state->bands[5], state->bands[5],
        local_exponents, 8);
    state->exponents[0] = maximum_exponent;
    uint16_t combined = 0;
    for (int band = 0; band < 8; ++band) {
        combined |= (uint16_t)state->bands[5][band];
    }
    if (combined == 0U) {
        vocoder_fill_i16(state->bands[5], 1, 8);
        state->exponents[0] = -30;
    }
    return low_mask;
}

void vocoder_adjust_harmonic_samples(
    vocoder_harmonic_buffer *buffer,
    vocoder_harmonic_reference *reference,
    int32_t frame_class,
    int32_t mode)
{
    int32_t energy_exponent;
    int32_t energy = vocoder_vector_energy(
        &energy_exponent, buffer->samples,
        (uint32_t)(uint16_t)buffer->count);
    uint32_t count_q16 =
        (uint32_t)(int32_t)buffer->count << 16;
    int32_t count_shift =
        frame_fixed_normalization_shift(count_q16);
    energy_exponent += 8;
    int16_t mean = vocoder_normalized_divide(
        energy, energy_exponent,
        (int16_t)(frame_shift_left(
            count_q16, (unsigned int)count_shift) >> 16),
        15 - count_shift, &energy_exponent);
    uint16_t root = (uint16_t)vocoder_sqrt_scaled(
        (uint32_t)(uint16_t)mean << 16,
        energy_exponent, 4);

    uint32_t gain_q16 =
        (uint32_t)(uint16_t)reference->gain << 16;
    int32_t comparison = frame_saturate_i32(
        (int64_t)(int32_t)gain_q16 - 0x08000000);
    comparison = frame_saturate_i32(
        (int64_t)comparison -
        (int64_t)(uint16_t)root * 0x10000);
    if (comparison < 0) {
        int32_t smoothed =
            (int32_t)reference->gain * 0xfae0 +
            ((int32_t)(int16_t)root * 0x51eb >> 4);
        reference->gain =
            (int16_t)((uint32_t)smoothed >> 16);
    }
    else {
        reference->gain =
            (int16_t)((gain_q16 - 0x00140000U) >> 16);
    }

    if (frame_class <= 3 && mode <= 1) {
        return;
    }
    int16_t padded[57];
    padded[0] = reference->spectrum[0];
    for (int index = 1; index < 57; ++index) {
        padded[index] = reference->spectrum[index - 1];
    }

    uint32_t reference_pitch_q16 =
        (uint32_t)(int32_t)reference->pitch << 16;
    uint32_t input_pitch_q16 =
        (uint32_t)(int32_t)buffer->pitch << 16;
    int32_t reference_shift =
        frame_fixed_normalization_shift(reference_pitch_q16);
    int32_t input_shift =
        frame_fixed_normalization_shift(input_pitch_q16);
    int32_t ratio_exponent;
    int16_t ratio = vocoder_normalized_divide(
        (int32_t)frame_shift_left(
            input_pitch_q16, (unsigned int)input_shift),
        -4 - input_shift,
        (int16_t)(frame_shift_left(
            reference_pitch_q16,
            (unsigned int)reference_shift) >> 16),
        -4 - reference_shift,
        &ratio_exponent);

    uint32_t mode_term =
        frame_arithmetic_shift_right(
            (uint32_t)mode << 27, 1);
    int32_t threshold = frame_saturate_i32(
        (int64_t)reference->gain * 0x10000 +
        (int64_t)(int32_t)(0x2c000000U - mode_term));

    int32_t count = buffer->count;
    int32_t ratio_multiple = ratio;
    for (int32_t harmonic = 1;
            harmonic <= count; ++harmonic) {
        uint32_t position = frame_scale_i32(
            (uint32_t)(ratio_multiple * 2), ratio_exponent);
        uint32_t fraction = (position & 0xffffU) >> 1;
        int32_t position_index = (int32_t)
            frame_arithmetic_shift_right(position, 16);
        int16_t inverse_fraction = (int16_t)
            frame_arithmetic_shift_right(
                0x7fff0000U - fraction * 0x10000U, 16);
        int32_t interpolated = (int32_t)(
            ((int64_t)padded[position_index + 1] * fraction +
             (int64_t)inverse_fraction *
                 padded[position_index]) * 2 >> 16);
        int32_t sample = buffer->samples[harmonic - 1];
        int32_t sample_q16 = (int32_t)(
            (uint32_t)sample << 16);
        int32_t difference = frame_saturate_i32(
            (int64_t)sample_q16 -
            (int64_t)interpolated * 0x10000);
        difference = frame_saturate_i32(
            (int64_t)difference - 0x10000000);
        int32_t harmonic_limit =
            (harmonic * 6 * (int32_t)buffer->pitch * 0x100) >> 16;
        if (difference > 0) {
            if (harmonic_limit > 0x0fff) {
                harmonic_limit = 0x1000;
            }
            int32_t margin = frame_saturate_i32(
                (int64_t)sample_q16 -
                ((int64_t)(threshold >> 16) * 0x10000 -
                 (int64_t)harmonic_limit * 0x10000));
            if (margin > 0) {
                int32_t factor = margin >> 16;
                if (factor > 0x03ff) {
                    factor = 0x0400;
                }
                int32_t delta = frame_saturate_i32(
                    (int64_t)interpolated * 0x10000 -
                    (int64_t)sample * 0x10000);
                delta = frame_saturate_i32(
                    (int64_t)(delta >> 16) * factor * 2);
                delta = frame_saturate_i32((int64_t)delta * 16);
                int32_t adjusted = frame_saturate_i32(
                    (int64_t)sample * 0x10000 + delta);
                int32_t result = adjusted >> 16;
                if (result < -0x77ff) {
                    result = -0x77ff;
                }
                if (result > 0x77ff) {
                    result = 0x77ff;
                }
                buffer->samples[harmonic - 1] =
                    (int16_t)result;
            }
        }
        ratio_multiple = ratio * (harmonic + 1);
    }
}

int16_t vocoder_get_second_i16(const int16_t values[2])
{
    return values[1];
}

void vocoder_store_u32(uint32_t *destination, uint32_t value)
{
    *destination = value;
}

void vocoder_increment_counter_100(void *state)
{
    int16_t *counter = (int16_t *)((uint8_t *)state + 0x86);
    if (*counter < 100) {
        ++*counter;
    }
}

int32_t vocoder_test_state_flag(void *state, int32_t index)
{
    uint8_t *bytes = (uint8_t *)state;
    int16_t flags;
    memcpy(&flags, bytes + 0x44, sizeof(flags));
    uint32_t mask = 3U << (unsigned int)(index + 2);
    if (((uint32_t)(int32_t)flags & mask) == 0U) {
        return 0;
    }
    uint32_t value = 0x000fa000U;
    memcpy(bytes + 0x40, &value, sizeof(value));
    return 0x400;
}

void vocoder_scale_i16_vector(
    int16_t *output,
    const int16_t *input,
    int32_t count,
    int16_t multiplier)
{
    for (int32_t index = 0; index < count; ++index) {
        uint32_t product = (uint32_t)(
            (int32_t)multiplier * input[index]) * 2U;
        output[index] = (int16_t)(product >> 16);
    }
}

void vocoder_complex_magnitude_squared(
    int32_t *output,
    const uint32_t *input,
    int32_t count)
{
    for (int32_t index = 0; index < count; ++index) {
        int16_t real = (int16_t)input[index];
        int16_t imaginary = (int16_t)(input[index] >> 16);
        uint32_t power = (uint32_t)(
            (int32_t)real * real +
            (int32_t)imaginary * imaginary) * 2U;
        output[index] = (int32_t)power;
    }
}

int16_t *vocoder_write_bits_msb(
    int32_t value,
    int16_t *output,
    int32_t count)
{
    for (int32_t index = count; index > 0; --index) {
        output[index - 1] = (int16_t)(value & 1);
        value = (int32_t)frame_arithmetic_shift_right(
            (uint32_t)value, 1);
    }
    return output + count;
}

uint32_t vocoder_read_bits_msb(
    const int16_t *input,
    int32_t count)
{
    uint32_t value = 0;
    for (int32_t index = 0; index < count; ++index) {
        value = (value << 1) |
            (uint32_t)(uint16_t)input[index];
    }
    return value;
}

int32_t vocoder_argmax_i16(
    const int16_t *values,
    int32_t count)
{
    int16_t maximum = INT16_MIN;
    int32_t result = 0;
    for (int32_t index = 0; index < count; ++index) {
        if (maximum < values[index]) {
            maximum = values[index];
            result = index;
        }
    }
    return result;
}

int32_t vocoder_argmax_i32(
    const int32_t *values,
    int32_t count)
{
    int32_t maximum = INT32_MIN;
    int32_t result = 0;
    for (int32_t index = 0; index < count; ++index) {
        if (maximum < values[index]) {
            maximum = values[index];
            result = index;
        }
    }
    return result;
}

int32_t vocoder_compare_scaled_less(
    int16_t left,
    int32_t left_exponent,
    int16_t right,
    int32_t right_exponent)
{
    return vocoder_compare_scaled(
        right, right_exponent, left, left_exponent);
}

int32_t vocoder_complex_peak_normalization_shift(
    const uint32_t *values,
    int32_t count)
{
    uint32_t maximum = 0x01000000U;
    for (int32_t index = 0; index < count; ++index) {
        int16_t real = (int16_t)values[index];
        int16_t imaginary = (int16_t)(values[index] >> 16);
        uint32_t power = (uint32_t)(
            (int32_t)real * real +
            (int32_t)imaginary * imaginary);
        if ((int32_t)maximum < (int32_t)power) {
            maximum = power;
        }
    }
    return (int32_t)frame_count_leading_zeros(maximum) - 2;
}

void vocoder_write_bits_cursor(
    int16_t **cursor,
    int32_t count,
    int32_t value)
{
    *cursor = vocoder_write_bits_msb(value, *cursor, count);
}

int32_t vocoder_i32_maximum_magnitude_shift(
    const int32_t *values,
    int32_t count)
{
    uint32_t maximum = 0;
    for (int32_t index = 0; index < count; ++index) {
        if (values[index] == INT32_MIN) {
            return 0;
        }
        uint32_t magnitude = values[index] < 0
            ? (uint32_t)-values[index]
            : (uint32_t)values[index];
        if (maximum < magnitude) {
            maximum = magnitude;
        }
    }
    return maximum == 0U ? 0 :
        1 - (int32_t)frame_count_leading_zeros(maximum);
}

void vocoder_shift_i16_between_exponents(
    int16_t *output,
    const int16_t *input,
    int32_t count,
    int32_t output_exponent,
    int32_t input_exponent)
{
    int32_t difference = output_exponent - input_exponent;
    for (int32_t index = 0; index < count; ++index) {
        uint32_t value = (uint32_t)(int32_t)input[index];
        output[index] = (int16_t)frame_scale_i32(
            value, difference);
    }
}

void vocoder_multiply_i16_between_exponents(
    int16_t *output,
    const int16_t *input,
    int32_t count,
    int32_t multiplier,
    int32_t product_exponent,
    int32_t output_exponent)
{
    int32_t shift =
        product_exponent - output_exponent - 15;
    for (int32_t index = 0; index < count; ++index) {
        uint32_t product = (uint32_t)(
            multiplier * (int32_t)input[index]);
        output[index] = (int16_t)frame_scale_i32(
            product, shift);
    }
}

int16_t vocoder_divide_i32_i16_q14(
    int32_t numerator,
    int16_t denominator)
{
    int32_t sign = numerator ^ (int32_t)denominator;
    uint32_t numerator_magnitude;
    if (numerator == INT32_MIN) {
        numerator_magnitude = INT32_MAX;
    }
    else {
        numerator_magnitude = numerator < 0
            ? (uint32_t)-numerator : (uint32_t)numerator;
    }
    int32_t denominator_value = denominator;
    uint32_t denominator_magnitude =
        denominator == INT16_MIN
        ? INT16_MAX
        : (uint32_t)(denominator_value < 0
            ? -denominator_value : denominator_value);
    int32_t result;
    if (numerator_magnitude < denominator_magnitude) {
        result = 0;
    }
    else if ((numerator_magnitude >> 16) <
             denominator_magnitude) {
        result = (int32_t)(
            numerator_magnitude / denominator_magnitude) >> 1;
    }
    else {
        result = INT16_MAX;
    }
    return (int16_t)(sign < 0 ? -result : result);
}

void vocoder_spread_two_bit_fields(
    uint32_t *low,
    uint32_t *high,
    uint32_t packed)
{
    uint32_t low_result = 0;
    uint32_t high_result = 0;
    uint32_t upper = (uint32_t)(
        (int32_t)packed >> 16);
    for (int32_t shift = 30; shift >= 2; shift -= 4) {
        uint32_t low_pair = packed & 3U;
        uint32_t high_pair = upper & 3U;
        low_result |=
            (low_pair << (unsigned int)shift) |
            (low_pair << (unsigned int)(shift - 2));
        high_result |=
            (high_pair << (unsigned int)shift) |
            (high_pair << (unsigned int)(shift - 2));
        packed = frame_arithmetic_shift_right(packed, 2);
        upper = frame_arithmetic_shift_right(upper, 2);
    }
    *low = low_result;
    *high = high_result;
}

uint32_t vocoder_select_voicing_codeword(
    int32_t mode,
    uint32_t *low_fields,
    uint32_t *high_fields,
    const int16_t left[16],
    const int16_t right[16],
    const int16_t weights[16],
    int32_t code_bits)
{
    static const uint32_t codebook[128] = {
        0x55555555U, 0x55555551U, 0x55545555U, 0x55505555U,
        0x55555554U, 0x55515555U, 0x55515551U, 0x55555556U,
        0x55545554U, 0x55455555U, 0x55505554U, 0x55555550U,
        0x55555545U, 0x55405555U, 0x55155555U, 0x55955555U,
        0x55505550U, 0x55505500U, 0x55055505U, 0x59555155U,
        0x51555155U, 0x55505540U, 0x55155515U, 0x51555955U,
        0x54555455U, 0x55445544U, 0x55555540U, 0x55515550U,
        0x55455545U, 0x55545544U, 0x55545550U, 0x56555455U,
        0x55005500U, 0x55005550U, 0x55015501U, 0x55405544U,
        0x55405540U, 0x55405550U, 0x55415541U, 0x55405554U,
        0x54005400U, 0x55005400U, 0x54005500U, 0x55005540U,
        0x54015400U, 0x55405400U, 0x55405500U, 0x55005504U,
        0x50005000U, 0x50005500U, 0x50005400U, 0x50005200U,
        0x54005000U, 0x50005800U, 0x55005000U, 0x51005000U,
        0x40004000U, 0x50004000U, 0x50000000U, 0x52005000U,
        0x54000000U, 0x00005400U, 0x40005000U, 0x58005000U,
        0x00000000U, 0x00000100U, 0x40000000U, 0x00000001U,
        0x00004000U, 0x10000000U, 0x00005000U, 0x00001000U,
        0x80000000U, 0x0a800000U, 0xa8000000U, 0x02a00000U,
        0xa0000000U, 0xaa000000U, 0x02aa0000U, 0x0a000000U,
        0xaaaa0000U, 0x00800000U, 0x0aaa0000U, 0xaa800000U,
        0x08000000U, 0xaaa00000U, 0x2aaa0000U, 0xaaa80000U,
        0x00020000U, 0x00080000U, 0x000a0000U, 0x00200000U,
        0x002a0000U, 0x00aa0000U, 0x02000000U, 0x02800000U,
        0xaaaaaaaaU, 0xaaaa0aaaU, 0xa800a800U, 0x02aa02aaU,
        0x0aaa0aaaU, 0xaaaaa800U, 0xa000a000U, 0x08000800U,
        0x00008000U, 0x00000280U, 0x00000200U, 0x00000aa8U,
        0x00000800U, 0x00000a00U, 0x00000a80U, 0x000002a0U,
        0x0000aaaaU, 0x0000aa80U, 0x0000a800U, 0x00002aaaU,
        0x0000a000U, 0x000002aaU, 0x00000aaaU, 0x0000aaa0U,
        0x00000002U, 0x0000002aU, 0x00000008U, 0x00000020U,
        0x0000000aU, 0x000000aaU, 0x00000080U, 0x000000a0U
    };

    if (mode != 1) {
        *low_fields = 0;
        *high_fields = 0;
        return 0x40U >> ((7U - (uint32_t)code_bits) & 0xffU);
    }

    int16_t costs[3][16];
    for (int32_t index = 0; index < 16; ++index) {
        int32_t left_value = left[index];
        int32_t right_value = right[index];
        int32_t left_complement = 0x7fff - left_value;
        int32_t left_square = left_value * left_value >> 15;
        int32_t left_complement_square =
            left_complement * left_complement >> 15;
        int16_t right_square = (int16_t)(
            (uint32_t)(right_value * right_value * 2) >> 16);
        int16_t right_complement = (int16_t)(
            0x7fff - right_value);
        int16_t right_complement_square = (int16_t)(
            (uint32_t)(
                (int32_t)right_complement *
                right_complement * 2) >> 16);
        int16_t inverse_left_square = (int16_t)(
            0x7fff - left_square);
        int16_t inverse_left_complement_square = (int16_t)(
            0x7fff - left_complement_square);
        costs[0][index] = (int16_t)(
            (uint32_t)(
                left_complement_square * 0x10000 +
                (int32_t)inverse_left_complement_square *
                right_square) >> 16);
        costs[1][index] = (int16_t)(
            (uint32_t)(
                left_square * 0x10000 +
                (int32_t)inverse_left_square *
                right_complement_square) >> 16);
        costs[2][index] = (int16_t)(
            (uint32_t)(
                left_square * 0x10000 +
                (int32_t)inverse_left_square *
                right_square) >> 16);
    }

    uint32_t candidate_count =
        (uint16_t)(1U << ((uint32_t)code_bits & 0xffU));
    uint32_t selected = 0;
    int16_t best_exponent = 0x7fff;
    int16_t best_mantissa = 0x7fff;
    uint32_t stride =
        1U << ((7U - (uint32_t)code_bits) & 0xffU);
    for (uint32_t candidate = 0;
            candidate < candidate_count; ++candidate) {
        uint32_t packed = codebook[candidate * stride];
        int64_t sum = 0;
        for (int32_t index = 15; index >= 0; --index) {
            uint32_t selector = packed & 3U;
            packed = (uint32_t)(
                (int32_t)packed >> 2);
            int32_t cost = selector == 1U
                ? costs[0][index]
                : selector == 2U
                    ? costs[1][index]
                    : costs[2][index];
            sum += (int64_t)cost * weights[index];
        }
        if (sum == 0) {
            selected = candidate;
            break;
        }

        uint64_t magnitude = (uint64_t)sum;
        if (sum < 0) {
            magnitude = ~magnitude;
        }
        unsigned int leading = magnitude >> 32 != 0U
            ? frame_count_leading_zeros(
                (uint32_t)(magnitude >> 32))
            : frame_count_leading_zeros(
                (uint32_t)magnitude) + 32U;
        int32_t shift = (int32_t)leading - 33;
        uint32_t normalized;
        if (shift < 0) {
            normalized = (uint32_t)magnitude >>
                (uint32_t)-shift;
            normalized |= (uint32_t)(magnitude >> 32)
                << ((leading - 1U) & 31U);
        }
        else {
            normalized =
                (uint32_t)magnitude << (uint32_t)shift;
        }
        int16_t exponent = (int16_t)(1 - shift);
        int16_t mantissa = (int16_t)(normalized >> 16);
        if (exponent < best_exponent ||
                (exponent == best_exponent &&
                 mantissa < best_mantissa)) {
            best_exponent = exponent;
            best_mantissa = mantissa;
            selected = candidate;
        }
    }

    vocoder_spread_two_bit_fields(
        low_fields, high_fields,
        codebook[selected * stride]);
    return selected;
}

void vocoder_prepare_voicing_frame_pair(
    int16_t output[49],
    int16_t **cursor,
    int32_t configuration_index,
    vocoder_frame_state *left,
    vocoder_frame_state *right,
    int32_t unused,
    int32_t repeated_code,
    const int16_t levels[2],
    int32_t mode,
    int16_t configuration[13],
    const int16_t vectors[48],
    int16_t pitch_bias)
{
    static const uint8_t partition_modes[7] = {
        49, 48, 47, 46, 45, 45, 45
    };
    (void)unused;
    vocoder_initialize_partition_config(
        configuration, configuration[0],
        49 - partition_modes[configuration_index]);
    vocoder_fill_i16(output, 0, 49);
    *cursor = output;

    int16_t *left_words = (int16_t *)(void *)left;
    int16_t *right_words = (int16_t *)(void *)right;
    right_words[0] = (int16_t)mode;
    left_words[0] = (int16_t)mode;
    if (mode == 3) {
        vocoder_write_repeated_frame_code(
            repeated_code, levels, configuration, cursor);
        return;
    }

    left_words[1] = 0xff;
    uint32_t voicing_code = vocoder_select_voicing_codeword(
        mode,
        (uint32_t *)(void *)(right_words + 4),
        (uint32_t *)(void *)(left_words + 4),
        vectors, vectors + 16, vectors + 32,
        configuration[4]);
    uint16_t combined = 0;
    int16_t changed = vocoder_combine_harmonic_indices(
        &combined, right, left,
        right_words[7], left_words[7],
        (uint32_t)(uint16_t)configuration[5], 1);
    if (changed == 0) {
        combined = (uint16_t)vocoder_quantize_pitch_index(
            left_words + 6, left_words + 2,
            mode, configuration[5], pitch_bias);
    }
    else {
        right_words[6] = 0x1079;
        right_words[2] = 0x38;
        left_words[6] = 0x1079;
        left_words[2] = 0x38;
    }
    vocoder_write_bits_cursor(
        cursor, configuration[5], combined);
    vocoder_write_bits_cursor(
        cursor, configuration[4], (int32_t)voicing_code);
}

void vocoder_reorder_spectral_blocks(
    int16_t *values,
    int32_t inverse,
    const int16_t configuration[13])
{
    int32_t lengths[9] = {
        configuration[5], configuration[4],
        configuration[6], configuration[7],
        configuration[8], configuration[9],
        configuration[10], configuration[11],
        configuration[12]
    };
    int32_t offsets[9] = {0};
    for (int32_t index = 1; index < 9; ++index) {
        offsets[index] =
            offsets[index - 1] + lengths[index - 1];
    }
    int32_t total = configuration[2];
    int16_t source[256];
    int16_t destination[256];
    vocoder_copy_i16(source, values, total);
    vocoder_copy_i16(destination, values, total);

    int32_t packed = 0;
#define VOCODER_MAP_BLOCK(segment, start, count)                         \
    do {                                                              \
        int32_t map_count = (count);                                  \
        for (int32_t map_index = 0; map_index < map_count;            \
                ++map_index) {                                        \
            int32_t layout = offsets[(segment)] + (start) + map_index;\
            if (inverse == 0) {                                       \
                destination[packed + map_index] = source[layout];      \
            }                                                         \
            else {                                                    \
                destination[layout] = source[packed + map_index];      \
            }                                                         \
        }                                                             \
        packed += map_count;                                          \
    } while (0)

    VOCODER_MAP_BLOCK(0, 0, 4);
    VOCODER_MAP_BLOCK(1, 0, 4);
    VOCODER_MAP_BLOCK(2, 0, 4);
    VOCODER_MAP_BLOCK(3, 0, 8);
    VOCODER_MAP_BLOCK(4, 0, 4);
    VOCODER_MAP_BLOCK(5, 0, 4);
    VOCODER_MAP_BLOCK(6, 0, 3);
    VOCODER_MAP_BLOCK(7, 0, 3);
    VOCODER_MAP_BLOCK(8, 0, 1);
    VOCODER_MAP_BLOCK(1, 4, lengths[1] - 4);
    VOCODER_MAP_BLOCK(2, 4, lengths[2] - 4);
    VOCODER_MAP_BLOCK(0, 4, lengths[0] - 4);
    VOCODER_MAP_BLOCK(4, 4, lengths[4] - 4);
    VOCODER_MAP_BLOCK(3, 8, lengths[3] - 8);
    VOCODER_MAP_BLOCK(5, 4, lengths[5] - 4);
    VOCODER_MAP_BLOCK(6, 3, lengths[6] - 3);
    VOCODER_MAP_BLOCK(7, 3, lengths[7] - 3);
    VOCODER_MAP_BLOCK(8, 1, lengths[8] - 1);
#undef VOCODER_MAP_BLOCK

    vocoder_copy_i16(values, destination, total);
}

void vocoder_quantize_residual_spectrum(
    int16_t *reference,
    int16_t *frame,
    int16_t average,
    const int16_t *configuration,
    int16_t **cursor,
    int32_t encode)
{
    int32_t count = frame[2];
    int32_t reference_count = reference[2];
    int16_t predicted[56];
    int16_t coefficients[8] = {0};
    int32_t partitions[4];

    int16_t ratio = (int16_t)vocoder_fixed_divide_half(
        0x80000, (int16_t)count);
    int16_t scaled_reference = (int16_t)(
        ((uint32_t)(int32_t)reference_count << 25) >> 16);
    int32_t step =
        (int32_t)scaled_reference * ratio * 2 >> 12;
    int32_t phase = step;
    int32_t sum = 0;
    for (int32_t index = 0; index < count; ++index) {
        int32_t source_index = phase >> 16;
        if (source_index > 55) {
            source_index = 55;
        }
        int32_t value;
        int32_t interpolated_q16;
        if (source_index == 0) {
            value = reference[8];
            interpolated_q16 = (int32_t)((uint32_t)value << 16);
        }
        else {
            int32_t current = reference[source_index + 7];
            int32_t next = reference[source_index + 8];
            int16_t fraction = (int16_t)(
                (uint16_t)phase >> 1);
            uint32_t interpolated =
                (uint32_t)current << 16;
            uint32_t current_product = (uint32_t)(
                (int32_t)fraction * current);
            int32_t negated_product =
                (current_product & 0x7fffffffU) ==
                        0x40000000U
                    ? INT32_MAX
                    : (int32_t)(0U - current_product * 2U);
            interpolated += (uint32_t)negated_product;
            interpolated += (uint32_t)(
                (int32_t)fraction * next * 2);
            interpolated_q16 = (int32_t)interpolated;
            value = (int16_t)(interpolated >> 16);
        }
        predicted[index] = (int16_t)value;
        sum += interpolated_q16 >> 6;
        phase += step;
    }

    int16_t mean = (int16_t)vocoder_fixed_divide_half(
        sum,
        (int16_t)(((uint32_t)count << 25) >> 16));
    int32_t mean_offset =
        mean == (int16_t)0x8000
            ? INT32_MAX
            : (int32_t)mean * -0xa666 >> 1;
    for (int32_t index = 0; index < count; ++index) {
        int32_t combined =
            (int32_t)predicted[index] * 0x5333 +
            mean_offset;
        uint32_t doubled = (uint32_t)combined * 2U;
        if (combined != (int32_t)
                frame_arithmetic_shift_right(doubled, 1U)) {
            doubled = ((uint32_t)(combined >> 31)) ^
                (uint32_t)INT32_MAX;
        }
        predicted[index] = (int16_t)
            frame_arithmetic_shift_right(doubled, 16U);
    }

    uint32_t packed_partitions = (uint16_t)
        vocoder_partition_layout[count - 9];
    for (int32_t group = 0; group < 4; ++group) {
        partitions[group] =
            (int32_t)(packed_partitions & 0xfU) + 2;
        packed_partitions = (uint32_t)(
            (int32_t)packed_partitions >> 4);
    }

    int16_t *spectrum = frame + 8;
    if (encode != 0) {
        for (int32_t index = 0; index < count; ++index) {
            int32_t residual =
                spectrum[index] - predicted[index];
            if (residual < -0x7fff) {
                residual = -0x7fff;
            }
            if (residual > 0x7fff) {
                residual = 0x7fff;
            }
            spectrum[index] = (int16_t)residual;
        }
        int32_t offset = 0;
        for (int32_t group = 0; group < 4; ++group) {
            int32_t size = partitions[group];
            vocoder_inverse_cosine_transform(
                spectrum + offset, size, 6);
            coefficients[group * 2] =
                spectrum[offset];
            coefficients[group * 2 + 1] =
                spectrum[offset + 1];
            vocoder_fill_i16(
                spectrum + offset + 6, 0, size - 6);
            offset += size;
        }
        for (int32_t group = 0; group < 4; ++group) {
            int32_t first = coefficients[group * 2];
            int32_t second = coefficients[group * 2 + 1];
            int32_t upper =
                (first * 0x8000 + second * 0xb504) >> 15;
            int32_t lower =
                (first * 0x8000 - second * 0xb504) >> 15;
            if (upper < -0x7fff) {
                upper = -0x7fff;
            }
            if (upper > 0x7fff) {
                upper = 0x7fff;
            }
            if (lower < -0x7fff) {
                lower = -0x7fff;
            }
            if (lower > 0x7fff) {
                lower = 0x7fff;
            }
            coefficients[group * 2] = (int16_t)upper;
            coefficients[group * 2 + 1] = (int16_t)lower;
        }
        vocoder_inverse_cosine_transform(
            coefficients, 8, 8);
    }

    coefficients[0] = 0;
    (void)vocoder_vector_quantize_i16(
        coefficients + 1, 3,
        (uint32_t)(uint16_t)configuration[7],
        vocoder_vq_primary_3, 9, 3, cursor, encode);
    (void)vocoder_vector_quantize_i16(
        coefficients + 4, 4,
        (uint32_t)(uint16_t)configuration[8],
        vocoder_vq_primary_4, 7, 4, cursor, encode);

    static const int16_t *const tables[4] = {
        vocoder_vq_5bit, vocoder_vq_4bit_a,
        vocoder_vq_4bit_b, vocoder_vq_3bit
    };
    static const uint8_t code_bits[4] = {5, 4, 4, 3};
    int32_t offset = 0;
    for (int32_t group = 0; group < 4; ++group) {
        int32_t dimension = partitions[group] - 2;
        if (dimension > 4) {
            dimension = 4;
        }
        if (dimension > 0) {
            (void)vocoder_vector_quantize_i16(
                spectrum + offset + 2, dimension,
                (uint32_t)(uint16_t)configuration[group + 9],
                tables[group], code_bits[group], 4,
                cursor, encode);
        }
        else if (encode != 0) {
            vocoder_write_bits_cursor(
                cursor, configuration[group + 9], 0);
        }
        else {
            (void)vocoder_read_bits_cursor(
                (const int16_t **)cursor,
                configuration[group + 9]);
        }
        offset += partitions[group];
    }

    vocoder_cosine_transform(coefficients, 8, 8);
    for (int32_t group = 0; group < 4; ++group) {
        int32_t first = coefficients[group * 2];
        int32_t second = coefficients[group * 2 + 1];
        coefficients[group * 2] = (int16_t)(
            (uint32_t)(first * 0x8000 +
                       second * 0x8000) >> 16);
        int16_t difference = (int16_t)(
            (uint32_t)(first * 0x8000 -
                       second * 0x8000) >> 16);
        coefficients[group * 2 + 1] = (int16_t)(
            (int32_t)difference * 0x5a82 >> 15);
    }

    offset = 0;
    for (int32_t group = 0; group < 4; ++group) {
        spectrum[offset] = coefficients[group * 2];
        spectrum[offset + 1] =
            coefficients[group * 2 + 1];
        vocoder_cosine_transform(
            spectrum + offset, partitions[group], 6);
        offset += partitions[group];
    }

    int16_t count_log = vocoder_log2_scaled(
        (uint16_t)count, 15, 5);
    int32_t bias =
        count * (int32_t)(int16_t)(average - count_log) * 2;
    for (int32_t group = 0; group < 4; ++group) {
        bias -= partitions[group] *
            (int32_t)coefficients[group * 2] * 2;
    }
    bias = vocoder_fixed_divide_half(
        bias, (int16_t)count);
    for (int32_t index = 0; index < count; ++index) {
        int32_t combined = (int32_t)(
            ((uint32_t)bias << 15) +
            (uint32_t)((int32_t)spectrum[index] * 0x8000));
        uint32_t doubled = (uint32_t)combined * 2U;
        if (combined != (int32_t)
                frame_arithmetic_shift_right(doubled, 1U)) {
            doubled = ((uint32_t)(combined >> 31)) ^
                (uint32_t)INT32_MAX;
        }
        spectrum[index] = (int16_t)
            frame_arithmetic_shift_right(doubled, 16U);
    }
    int32_t last = 0;
    for (int32_t index = 0; index < count; ++index) {
        int32_t combined =
            (int32_t)predicted[index] * 0x8000 +
            (int32_t)spectrum[index] * 0x8000;
        uint32_t doubled = (uint32_t)combined * 2U;
        if (combined != (int32_t)
                frame_arithmetic_shift_right(doubled, 1U)) {
            doubled = ((uint32_t)(combined >> 31)) ^
                (uint32_t)INT32_MAX;
        }
        last = (int32_t)
            frame_arithmetic_shift_right(doubled, 16U);
        if (last < -0x7800) {
            last = -0x7800;
        }
        if (last > 0x77ff) {
            last = 0x77ff;
        }
        spectrum[index] = (int16_t)last;
    }
    vocoder_fill_i16(
        spectrum + count, last, 56 - count);
}

void vocoder_quantize_spectral_frame(
    void *reference_pointer,
    vocoder_frame_state *frame,
    const int16_t configuration[13],
    int16_t **cursor,
    int32_t encode)
{
    uint8_t *reference = (uint8_t *)reference_pointer;
    int16_t average[3];
    vocoder_quantize_log_average(
        average, (int16_t)frame->header[0],
        frame->spectrum, (int16_t)frame->header[2],
        (int16_t *)(void *)(reference + 0x84),
        (uint16_t)configuration[6], cursor, encode);
    vocoder_quantize_residual_spectrum(
        (int16_t *)reference, (int16_t *)(void *)frame,
        average[0], configuration, cursor, encode);
}

void vocoder_encode_and_prepare_frame(
    int16_t *spectral_values,
    int16_t **cursor,
    vocoder_frame_state *frame,
    vocoder_extended_frame *destination,
    int32_t mode,
    const int16_t configuration[13])
{
    if (mode != 3) {
        vocoder_quantize_spectral_frame(
            destination, frame, configuration, cursor, 1);
        vocoder_reorder_spectral_blocks(
            spectral_values, 0, configuration);
    }
    vocoder_prepare_extended_frame(destination, frame, 0);
}

static void process_spectral_frame_pair_native(
    int16_t *spectral_values,
    int32_t configuration_index,
    vocoder_frame_state *left,
    vocoder_frame_state *right,
    vocoder_extended_frame *destination,
    int32_t repeated_code,
    const int16_t levels[2],
    const int16_t *copy_source,
    int32_t mode,
    int16_t configuration[13],
    const vocoder_band_candidate_config *candidate_config,
    const vocoder_spectral_config *frame_config,
    const int16_t *frame_indices,
    int16_t pitch_bias)
{
    int32_t spectrum_index = configuration[1];
    right->header[7] =
        (uint16_t)frame_indices[spectrum_index + 1];
    left->header[7] =
        (uint16_t)frame_indices[spectrum_index];

    int16_t work[48];
    if (mode == 1) {
        vocoder_select_frame_band_candidate(
            right, work + 8, work + 40, work + 24,
            1, spectrum_index, candidate_config);
        vocoder_select_frame_band_candidate(
            left, work, work + 32, work + 16,
            0, spectrum_index, candidate_config);
    }

    int16_t *cursor = spectral_values;
    if (configuration[0] != 0) {
        vocoder_prepare_voicing_frame_pair(
            spectral_values, &cursor, configuration_index,
            left, right, 0, repeated_code, levels,
            mode, configuration, work, pitch_bias);
    }
    if (mode != 3) {
        vocoder_populate_spectral_frame(
            right, (vocoder_spectral_band_state *)destination,
            1, spectrum_index, frame_config, copy_source);
        vocoder_populate_spectral_frame(
            left, (vocoder_spectral_band_state *)destination,
            0, spectrum_index, frame_config, copy_source);
    }
    if (configuration[0] != 0) {
        vocoder_encode_and_prepare_frame(
            spectral_values, &cursor, left, destination,
            mode, configuration);
    }
}

void vocoder_process_spectral_frame_pair(
    int16_t *spectral_values,
    int32_t configuration_index,
    vocoder_frame_state *left,
    uintptr_t unused_3,
    vocoder_frame_state *right,
    uintptr_t unused_5,
    vocoder_extended_frame *destination,
    int32_t repeated_code,
    const int16_t levels[2],
    uintptr_t unused_9,
    const int16_t *copy_source,
    int32_t mode,
    int16_t configuration[13],
    void *spectral_configuration,
    int16_t pitch_bias)
{
    (void)unused_3;
    (void)unused_5;
    (void)unused_9;
    uint8_t *config_bytes =
        (uint8_t *)spectral_configuration;
    int16_t target;
    int16_t band_mask;
    int16_t left_neighbor;
    int16_t right_neighbor;
    int16_t callback_selector;
    int16_t callback_enabled;
    int16_t logarithm_bias;
    int16_t callback_mode;
    memcpy(&target, config_bytes + 0x10, sizeof(target));
    memcpy(&band_mask, config_bytes + 0x30, sizeof(band_mask));
    memcpy(&left_neighbor, config_bytes + 0x44, sizeof(left_neighbor));
    memcpy(&right_neighbor, config_bytes + 0x46, sizeof(right_neighbor));
    memcpy(&callback_selector, config_bytes + 0x30,
           sizeof(callback_selector));
    memcpy(&callback_enabled, config_bytes + 0x32,
           sizeof(callback_enabled));
    memcpy(&logarithm_bias, config_bytes + 0x40,
           sizeof(logarithm_bias));
    memcpy(&callback_mode, config_bytes + 0x42,
           sizeof(callback_mode));
    vocoder_band_candidate_config candidate_config = {
        .band_values = frame_load_packed_pointer(config_bytes, 0x00),
        .band_basis = frame_load_packed_pointer(config_bytes, 0x04),
        .candidates = frame_load_packed_pointer(config_bytes, 0x08),
        .scores = frame_load_packed_pointer(config_bytes, 0x0c),
        .target = target,
        .quantized_inputs = frame_load_packed_pointer(config_bytes, 0x28),
        .quantized_exponents = frame_load_packed_pointer(config_bytes, 0x2c),
        .band_mask = (uint16_t)band_mask,
        .left_neighbor = left_neighbor,
        .right_neighbor = right_neighbor
    };
    vocoder_spectral_config frame_config = {
        .spectra = frame_load_packed_pointer(config_bytes, 0x14),
        .exponents = frame_load_packed_pointer(config_bytes, 0x18),
        .callback_argument_5 = (uintptr_t)
            frame_load_packed_pointer(config_bytes, 0x1c),
        .callback_argument_6 = (uintptr_t)
            frame_load_packed_pointer(config_bytes, 0x20),
        .callback_selector = callback_selector,
        .callback_enabled = callback_enabled,
        .callback_argument_4 = (uintptr_t)
            frame_load_packed_pointer(config_bytes, 0x34),
        .callback_context = (uintptr_t)
            frame_load_packed_pointer(config_bytes, 0x38),
        .callback = (vocoder_spectral_callback)(uintptr_t)
            frame_load_packed_pointer(config_bytes, 0x3c),
        .logarithm_bias = logarithm_bias,
        .callback_mode = callback_mode,
        .callback_argument_3 = (uintptr_t)
            frame_load_packed_pointer(config_bytes, 0x48)
    };
    const int16_t *frame_indices;
    memcpy(&frame_indices, config_bytes + 0x24,
           sizeof(frame_indices));
    process_spectral_frame_pair_native(
        spectral_values, configuration_index, left, right,
        destination, repeated_code, levels, copy_source, mode,
        configuration, &candidate_config, &frame_config,
        frame_indices, pitch_bias);
}

int32_t vocoder_decode_and_prepare_frame(
    vocoder_frame_record *left,
    uintptr_t unused_1,
    vocoder_frame_state *interpolated,
    int16_t fallback_status,
    vocoder_extended_frame *reference,
    int16_t *input,
    int32_t configuration_index,
    uintptr_t unused_7,
    const void *decoder_state,
    int16_t configuration[13],
    uintptr_t unused_10,
    uintptr_t unused_11,
    int16_t pitch_bias)
{
    static const uint8_t partition_modes[7] = {
        49, 48, 47, 46, 45, 45, 45
    };
    (void)unused_1;
    (void)unused_7;
    (void)unused_10;
    (void)unused_11;
    if (configuration[0] == 0) {
        return fallback_status;
    }

    vocoder_initialize_partition_config(
        configuration, configuration[0],
        49 - partition_modes[configuration_index]);
    const int16_t *cursor = input;
    int32_t prefix = 0;
    for (int32_t index = 0; index < 6; ++index) {
        prefix = cursor[index] + prefix * 2;
    }
    if (configuration[0] == 0) {
        left->state.header[0] = 0;
        left->state.header[1] = 0xff;
        interpolated->header[0] = 0;
        interpolated->header[1] = 0xff;
        fallback_status = 4;
    }
    else if (prefix == 0x3f) {
        left->state.header[0] = 3;
        left->state.header[1] = 0xff;
        interpolated->header[0] = 3;
        interpolated->header[1] = 0xff;
        int32_t result = vocoder_decode_spectral_frame_block(
            (int16_t *)(void *)left, interpolated,
            &cursor, configuration);
        if (result != 0) {
            fallback_status = (int16_t)result;
            goto finish;
        }
        goto prepare;
    }
    else {
        int32_t mode;
        if (prefix == 0x3e) {
            mode = 2;
        }
        else if (prefix < 0x3c) {
            mode = 1;
        }
        else {
            left->state.header[0] = 0;
            left->state.header[1] = 0xff;
            interpolated->header[0] = 0;
            interpolated->header[1] = 0xff;
            fallback_status = 4;
            goto finish;
        }
        left->state.header[0] = (uint16_t)mode;
        left->state.header[1] = 0xff;
        interpolated->header[0] = (uint16_t)mode;
        interpolated->header[1] = 0xff;
        vocoder_reorder_spectral_blocks(input, 1, configuration);
        cursor = input;
        uint16_t combined = (uint16_t)vocoder_read_bits_cursor(
            &cursor, configuration[5]);
        uint32_t codeword = vocoder_read_bits_cursor(
            &cursor, configuration[4]);
        vocoder_decode_spread_codeword(
            mode, (uint32_t *)(void *)(interpolated->header + 4),
            (uint32_t *)(void *)(left->state.header + 4),
            (int32_t)codeword, configuration[4]);
        int16_t changed = vocoder_combine_harmonic_indices(
            &combined, interpolated, &left->state,
            0, 0, (uint32_t)(uint16_t)configuration[5], 0);
        if (changed == 0) {
            vocoder_decode_pitch_parameters(
                mode, (int16_t *)(void *)(left->state.header + 6),
                (int16_t *)(void *)(left->state.header + 2),
                combined, configuration[5], pitch_bias);
        }
        else {
            interpolated->header[6] = 0x1079;
            interpolated->header[2] = 0x38;
            left->state.header[6] = 0x1079;
            left->state.header[2] = 0x38;
        }
        int16_t *mutable_cursor = (int16_t *)cursor;
        vocoder_quantize_spectral_frame(
            reference, &left->state, configuration,
            &mutable_cursor, 0);
        const uint8_t *decoder =
            (const uint8_t *)decoder_state;
        int16_t frame_class;
        int16_t decoder_mode;
        memcpy(&frame_class, decoder + 0x30,
               sizeof(frame_class));
        memcpy(&decoder_mode, decoder + 0x24,
               sizeof(decoder_mode));
        vocoder_adjust_harmonic_samples(
            (vocoder_harmonic_buffer *)(void *)left,
            (vocoder_harmonic_reference *)(void *)reference,
            frame_class, decoder_mode);
    }

prepare:
    vocoder_interpolate_frame_state(
        interpolated, left,
        (const vocoder_frame_record *)(const void *)reference);
    vocoder_prepare_extended_frame(
        reference, &left->state, 0);
    fallback_status = 0;
finish:
    {
        int16_t marker = -3;
        memcpy((uint8_t *)(void *)reference + 0x88,
               &marker, sizeof(marker));
    }
    return fallback_status;
}

uint32_t vocoder_decode_frame_record(
    int16_t *input,
    int32_t configuration_index,
    uint32_t flags,
    const void *decoder_state,
    vocoder_frame_record *frame,
    vocoder_frame_record *output,
    void *state_pointer)
{
    uint8_t *state = (uint8_t *)state_pointer;
    int16_t state_flags;
    memcpy(&state_flags, state + 0x64a,
           sizeof(state_flags));
    flags |= (uint16_t)state_flags;
    uint16_t history;
    memcpy(&history, state + 0x7be, sizeof(history));
    history = (uint16_t)(history << 1);
    memcpy(state + 0x7be, &history, sizeof(history));

    int32_t bad_without_eight = 0;
    int32_t bad_with_eight = 0;
    if ((flags & 0xcU) == 0U) {
        int16_t setting_10;
        int16_t setting_11;
        memcpy(&setting_10, state + 0x7c4,
               sizeof(setting_10));
        memcpy(&setting_11, state + 0x7c6,
               sizeof(setting_11));
        uint32_t result = (uint32_t)
            vocoder_decode_and_prepare_frame(
                frame, 0x44, &output->state, 0x44,
                (vocoder_extended_frame *)(void *)(state + 0x584),
                input, configuration_index,
                (uintptr_t)decoder_state,
                state + 0x618,
                (int16_t *)(void *)(state + 0x7a4),
                (uintptr_t)setting_10,
                (uintptr_t)setting_11,
                (int16_t)(flags & 0x80U));
        if (result != 0U) {
            bad_with_eight = (result & 8U) != 0U;
            bad_without_eight = bad_with_eight == 0;
        }
    }

    uint16_t sync = frame->state.header[1];
    int32_t tracked = vocoder_track_sync_word(
        &sync, (uint16_t *)(void *)(state + 0x7c8), 0);
    int16_t level;
    if ((flags & 0x4000U) == 0U) {
        if (tracked == 0) {
            if ((flags & 8U) == 0U &&
                    (flags & 4U) == 0U) {
                if (bad_with_eight != 0) {
                    goto use_second_history;
                }
                if (bad_without_eight == 0) {
                    int16_t mode =
                        (int16_t)frame->state.header[0];
                    if (mode == 2) {
                        goto return_zero;
                    }
                    if (mode == 3) {
                        goto return_bad;
                    }
                    if (mode == 1) {
                        int16_t enabled;
                        memcpy(&enabled, state + 0x7a4,
                               sizeof(enabled));
                        if (enabled != 0) {
                            goto return_two;
                        }
                        if ((int16_t)frame->state.header[1] ==
                                0xff) {
                            goto return_two;
                        }
                        goto return_bad;
                    }
                }
            }
            memcpy(&history, state + 0x7be,
                   sizeof(history));
            if ((history & 0xeU) != 0xeU) {
                vocoder_copy_frame_record(
                    frame,
                    (const vocoder_frame_record *)(const void *)
                        (state + 0x474));
                goto copied_history;
            }
use_second_history:
            vocoder_copy_frame_record(
                frame,
                (const vocoder_frame_record *)(const void *)
                    (state + 0x4fc));
copied_history:
            vocoder_copy_frame_record(output, frame);
            memcpy(&history, state + 0x7be,
                   sizeof(history));
            history |= 1U;
            memcpy(state + 0x7be, &history,
                   sizeof(history));
            vocoder_increment_counter_100(state + 0x584);
            return 0x20U;
        }
        uint8_t kind = (uint8_t)vocoder_classify_code(
            (int16_t)sync);
        int32_t left_index =
            vocoder_lookup_partition_value(
                kind, (int16_t)sync, 0);
        kind = (uint8_t)vocoder_classify_code(
            (int16_t)sync);
        int32_t right_index =
            vocoder_lookup_partition_value(
                kind, (int16_t)sync, 1);
        level = (int16_t)(
            (uint32_t)(
                ((int32_t)frame->state.spectrum[left_index] +
                 frame->state.spectrum[right_index]) *
                0x8000) >> 16);
    }
    else {
        memcpy(&sync, state + 0x7a0, sizeof(sync));
        memcpy(&level, state + 0x7a2, sizeof(level));
    }
    (void)vocoder_initialize_spectral_frame(
        (int16_t *)(void *)frame,
        (uint16_t)sync, level, level, 0);
    vocoder_copy_frame_record(output, frame);
return_bad:
    {
        uint32_t result = 0x8002U;
        int16_t zero = 0;
        memcpy(state + 0x7c0, &zero, sizeof(zero));
        vocoder_increment_counter_100(state + 0x584);
        return result;
    }
return_two:
    {
        uint32_t result = 2U;
        int16_t zero = 0;
        memcpy(state + 0x7c0, &zero, sizeof(zero));
        vocoder_increment_counter_100(state + 0x584);
        return result;
    }
return_zero:
    {
        uint32_t result = 0U;
        int16_t zero = 0;
        memcpy(state + 0x7c0, &zero, sizeof(zero));
        vocoder_increment_counter_100(state + 0x584);
        return result;
    }
}

uint32_t vocoder_encode_frame_controller(
    int16_t *spectral_values,
    int32_t configuration_index,
    uint32_t flags,
    int16_t parameter,
    void *state_pointer,
    int16_t bias)
{
    uint8_t *state = (uint8_t *)state_pointer;
    int16_t *configuration =
        (int16_t *)(void *)(state + 0x138a);
    if (configuration[0] == 0) {
        flags &= ~0x800U;
    }

    vocoder_band_candidate_config candidate_config = {
        .band_values = (const int16_t *)(const void *)(state + 0x64e),
        .band_basis = (const int16_t *)(const void *)(state + 0x69e),
        .candidates = (const int16_t *)(const void *)(state + 0x63a),
        .scores = (const int16_t *)(const void *)(state + 0x644),
        .target = *(int16_t *)(void *)(state + 0x630),
        .quantized_inputs =
            (const int16_t *)(const void *)(state + 0x710),
        .quantized_exponents =
            (const int16_t *)(const void *)(state + 0x740),
        .band_mask = *(uint16_t *)(void *)(state + 0x97c),
        .left_neighbor = *(int16_t *)(void *)(state + 0x980),
        .right_neighbor = *(int16_t *)(void *)(state + 0xa72)
    };
    vocoder_spectral_config frame_config = {
        .spectra = (const int32_t *)(const void *)(state + 0x18),
        .exponents = (const int16_t *)(const void *)(state + 0x624),
        .callback_argument_5 = (uintptr_t)(state + 0x746),
        .callback_argument_6 = (uintptr_t)(state + 0x926),
        .callback_selector = *(int16_t *)(void *)(state + 0x97c),
        .callback_enabled = (int16_t)((flags >> 6) & 1U),
        .callback_argument_4 = (uintptr_t)(state + 0xa2e),
        .callback_context = (uintptr_t)(state + 0x122e),
        .callback = vocoder_update_encoder_spectral_callback,
        .logarithm_bias = 0,
        .callback_mode = parameter,
        .callback_argument_3 = (uintptr_t)(state + 0x938)
    };
    const int16_t *frame_indices =
        (const int16_t *)(const void *)(state + 0x6f8);

    int16_t energy[16];
    int16_t levels[2];
    int32_t repeated_code;
    int32_t mode;
    int32_t result = vocoder_test_state_flag(
        state + 0x938, vocoder_get_second_i16(configuration));
    if ((flags & 0x4000U) == 0U) {
        if (vocoder_select_candidate(
                &repeated_code, levels,
                (const vocoder_candidate_pair *)(const void *)
                    (state + 0x1370)) == 0) {
            *(int16_t *)(void *)(state + 0x1370) = 0;
            if (result == 0 && (flags & 0x800U) != 0U) {
                result = vocoder_analyze_energy_trend(
                    energy,
                    (vocoder_energy_state *)(void *)(state + 0x938));
                if (bias != 0) {
                    for (int index = 0; index < 16; ++index) {
                        energy[index] =
                            (int16_t)((uint16_t)energy[index] +
                                      (uint16_t)bias);
                    }
                }
                mode = 2;
            }
            else {
                frame_config.logarithm_bias = bias;
                result |= 2;
                mode = 1;
            }
            goto process;
        }
        *(int16_t *)(void *)(state + 0x1370) = 0;
    }
    else {
        memcpy(&levels[0], state + 0x1372,
               sizeof(levels[0]));
        levels[1] = levels[0];
        {
            int16_t code;
            memcpy(&code, state + 0x1374, sizeof(code));
            repeated_code = code;
        }
        *(int16_t *)(void *)(state + 0x1370) = 1;
    }
    result |= 0x8002;
    if (bias != 0) {
        levels[0] =
            (int16_t)((uint16_t)levels[0] + (uint16_t)bias);
        levels[1] =
            (int16_t)((uint16_t)levels[1] + (uint16_t)bias);
    }
    mode = 3;

process:
    {
        uint16_t tracked = (uint16_t)repeated_code;
        vocoder_track_sync_word(
            &tracked, (uint16_t *)(void *)(state + 0x16ba), 1);
        repeated_code = (int16_t)tracked;
    }
    /* Firmware happens to receive zeroed scratch bytes here; make the
     * otherwise untouched header fields deterministic in portable C. */
    vocoder_frame_state left = {0};
    vocoder_frame_state right = {0};
    process_spectral_frame_pair_native(
        spectral_values, configuration_index, &left, &right,
        (vocoder_extended_frame *)(void *)(state + 0xabc),
        repeated_code, levels, energy, mode, configuration,
        &candidate_config, &frame_config, frame_indices,
        (int16_t)(flags & 0x80U));
    return (uint32_t)result;
}

void vocoder_saturating_shift_i16(
    int16_t *output,
    const int16_t *input,
    int32_t count,
    int32_t output_exponent,
    int32_t input_exponent)
{
    int32_t difference = output_exponent - input_exponent;
    for (int32_t index = 0; index < count; ++index) {
        int32_t value = input[index];
        if (difference <= 0) {
            output[index] = (int16_t)
                frame_arithmetic_shift_right(
                    (uint32_t)value,
                    (unsigned int)(-difference));
        }
        else {
            int64_t shifted = (int64_t)value *
                ((int64_t)1 << (unsigned int)difference);
            if (shifted > INT16_MAX) {
                shifted = INT16_MAX;
            }
            if (shifted < INT16_MIN) {
                shifted = INT16_MIN;
            }
            output[index] = (int16_t)shifted;
        }
    }
}

uint32_t vocoder_read_bits_cursor(
    const int16_t **cursor,
    int32_t count)
{
    uint32_t result = vocoder_read_bits_msb(*cursor, count);
    *cursor += count;
    return result;
}

int32_t vocoder_refine_table_index_q16(
    int32_t index,
    const int16_t values[32])
{
    int32_t base_index;
    int32_t integer_index;
    if (index == 0) {
        base_index = 0;
        integer_index = 1;
    }
    else if (index == 31) {
        base_index = 29;
        integer_index = 30;
    }
    else {
        base_index = index - 1;
        integer_index = index;
    }
    int32_t left_q16 =
        (int32_t)((uint32_t)(int32_t)values[base_index] << 16);
    int32_t right_q16 =
        (int32_t)((uint32_t)(int32_t)values[base_index + 2] << 16);
    int32_t curvature = (int32_t)(
        (int64_t)(right_q16 >> 1) +
        (left_q16 >> 1) -
        (int64_t)values[base_index + 1] * 0x10000);
    if ((curvature >> 16) >= 0) {
        return index << 16;
    }
    int32_t numerator =
        (left_q16 >> 2) - (right_q16 >> 2);
    int16_t fraction = vocoder_divide_i32_i16_q14(
        numerator, (int16_t)(curvature >> 16));
    return integer_index * 0x10000 +
        (int32_t)fraction * 2;
}

int16_t vocoder_smooth_if_mask_majority(
    int16_t current,
    int16_t previous,
    uint32_t mask)
{
    uint32_t selected = mask & 0x55555555U;
    unsigned int count = 0;
    while (selected != 0U) {
        count += selected & 1U;
        selected >>= 2;
    }
    if (count > 7U) {
        int32_t mixed =
            (int32_t)current * 0xe666 +
            ((int32_t)previous * 0x6666 >> 2);
        return (int16_t)frame_arithmetic_shift_right(
            (uint32_t)mixed, 16);
    }
    return current;
}

int32_t vocoder_select_candidate(
    int32_t *identifier,
    int16_t output[2],
    const vocoder_candidate_pair *candidates)
{
    *identifier = 0xff;
    int index;
    if (candidates->identifiers[0] == 0xff) {
        if (candidates->identifiers[1] == 0xff) {
            return 0;
        }
        index = 1;
    }
    else {
        index = 0;
    }
    *identifier = candidates->identifiers[index];
    output[0] = candidates->values[index * 2];
    output[1] = candidates->values[index * 2 + 1];
    return *identifier != 0xff;
}

int32_t vocoder_sum_i32_shifted(
    const int32_t *values,
    int32_t count,
    int16_t shift)
{
    int64_t sum = 0;
    for (int32_t index = 0; index < count; ++index) {
        sum += values[index];
    }
    if (shift <= 0) {
        return (int32_t)frame_shift_left(
            (uint32_t)sum, (unsigned int)(-(int32_t)shift));
    }
    if (shift >= 64) {
        return sum < 0 ? -1 : 0;
    }
    uint64_t shifted = (uint64_t)sum >> (unsigned int)shift;
    if (sum < 0) {
        shifted |= UINT64_MAX << (64U - (unsigned int)shift);
    }
    return (int32_t)(uint32_t)shifted;
}

void vocoder_select_maximum_five_sum(
    int32_t *maximum,
    int32_t *selected_index,
    const int32_t *values,
    const uint8_t offsets[4])
{
    int32_t best_value = 0;
    int32_t best_index = 0;
    for (int32_t index = 0; index < 4; ++index) {
        int32_t candidate = vocoder_sum_i32_shifted(
            values + offsets[index], 5, 3);
        if (best_value <= candidate) {
            best_value = candidate;
            best_index = index;
        }
    }
    *maximum = best_value;
    *selected_index = best_index;
}

static uint32_t frame_pack_butterfly(
    int32_t low,
    int32_t high_difference)
{
    return ((uint32_t)low & 0xffffU) |
        ((uint32_t)high_difference * 0x8000U & 0xffff0000U);
}

void vocoder_complex_butterfly_forward(
    uint32_t *forward,
    uint32_t *backward,
    int32_t count)
{
    for (int32_t index = 0; index < count; ++index) {
        int16_t left_real = (int16_t)*forward;
        int16_t left_imaginary = (int16_t)(*forward >> 16);
        int16_t right_real = (int16_t)*backward;
        int16_t right_imaginary = (int16_t)(*backward >> 16);
        *forward++ = frame_pack_butterfly(
            ((int32_t)left_real + right_real) >> 1,
            (int32_t)left_imaginary - right_imaginary);
        *backward-- = frame_pack_butterfly(
            ((int32_t)left_imaginary + right_imaginary) >> 1,
            (int32_t)right_real - left_real);
    }
}

void vocoder_complex_butterfly_inverse(
    uint32_t *forward,
    uint32_t *backward,
    int32_t count)
{
    for (int32_t index = 0; index < count; ++index) {
        int16_t left_real = (int16_t)*forward;
        int16_t left_imaginary = (int16_t)(*forward >> 16);
        int16_t right_real = (int16_t)*backward;
        int16_t right_imaginary = (int16_t)(*backward >> 16);
        *forward++ = frame_pack_butterfly(
            ((int32_t)left_real + right_real) >> 1,
            (int32_t)right_imaginary - left_imaginary);
        *backward-- = frame_pack_butterfly(
            ((int32_t)left_imaginary + right_imaginary) >> 1,
            (int32_t)left_real - right_real);
    }
}

void vocoder_initialize_partition_config(
    int16_t config[13],
    int32_t enabled,
    int32_t mode)
{
    if (enabled == 0) {
        return;
    }
    if (mode > 4) {
        mode = 4;
    }
    if (mode < 0) {
        mode = 0;
    }
    config[0] = 1;
    config[1] = 1;
    config[3] = (int16_t)mode;
    config[4] = 5;
    config[5] = 7;
    config[6] = 5;
    config[7] = 9;
    config[8] = 7;
    config[9] = 5;
    config[10] = 4;
    config[11] = 4;
    config[12] = 3;
    switch (mode) {
    case 1:
        config[12] = 2;
        break;
    case 2:
        config[12] = 1;
        break;
    case 3:
        config[11] = 3;
        config[12] = 1;
        break;
    case 4:
        config[10] = 3;
        config[11] = 3;
        config[12] = 1;
        break;
    default:
        break;
    }
    int16_t total = 12;
    for (int index = 6; index <= 12; ++index) {
        total = (int16_t)(total + config[index]);
    }
    config[2] = total;
}

static int32_t frame_decrease_spectral_value(int32_t value)
{
    int32_t q16 = (int32_t)(
        (uint32_t)value << 16);
    int32_t decreased = frame_saturate_i32(
        (int64_t)q16 - 0x04000000);
    decreased >>= 16;
    return decreased < -0x77ff ? -0x77ff : decreased;
}

void vocoder_spread_spectral_peak(
    int16_t spectrum[56],
    int16_t center,
    int32_t value)
{
    if (value < -0x77ff) {
        value = -0x77ff;
    }
    int32_t current = value;
    for (int32_t index = (int32_t)center - 1;
            index >= 0; --index) {
        if (spectrum[index] < current) {
            spectrum[index] = (int16_t)current;
        }
        current = frame_decrease_spectral_value(current);
    }
    current = value;
    for (int32_t index = (int32_t)center + 1;
            index < 56; ++index) {
        if (spectrum[index] < current) {
            spectrum[index] = (int16_t)current;
        }
        current = frame_decrease_spectral_value(current);
    }
}

void vocoder_convert_i32_to_i16_rounded(
    int16_t *output,
    int32_t input_exponent,
    const int32_t *input,
    int32_t output_exponent,
    int32_t count)
{
    int32_t difference = output_exponent - input_exponent;
    for (int32_t index = 0; index < count; ++index) {
        uint32_t rounded;
        if (difference == 0) {
            int32_t value = frame_saturate_i32(
                (int64_t)input[index] + 0x8000);
            rounded = (uint32_t)value;
        }
        else if (difference < 0) {
            rounded = frame_arithmetic_shift_right(
                (uint32_t)input[index],
                (unsigned int)(-difference)) + 0x8000U;
        }
        else {
            int32_t shifted;
            if (difference >= 32) {
                shifted = input[index] == 0 ? 0 :
                    (input[index] < 0 ? INT32_MIN : INT32_MAX);
            }
            else {
                int64_t scale = (int64_t)1 <<
                    (unsigned int)difference;
                shifted = frame_saturate_i32(
                    (int64_t)input[index] * scale);
            }
            shifted = frame_saturate_i32(
                (int64_t)shifted + 0x8000);
            rounded = (uint32_t)shifted;
        }
        output[index] = (int16_t)(rounded >> 16);
    }
}

void vocoder_normalize_i32_to_i16(
    int16_t *output,
    const int32_t *input,
    int32_t count,
    int32_t *exponent)
{
    int32_t shift = vocoder_i32_maximum_magnitude_shift(
        input, count);
    vocoder_convert_i32_to_i16_rounded(
        output, shift, input, 0, count);
    *exponent += shift;
}

int16_t vocoder_lookup_partition_value(
    uint32_t kind,
    int32_t index,
    int32_t selector)
{
    static const uint8_t table[36][2] = {
        {0x0c, 0x11}, {0x04, 0x07}, {0x0a, 0x13}, {0x08, 0x11},
        {0x07, 0x0b}, {0x04, 0x07}, {0x0b, 0x15}, {0x0c, 0x11},
        {0x07, 0x0b}, {0x04, 0x07}, {0x06, 0x0e}, {0x08, 0x11},
        {0x0c, 0x17}, {0x04, 0x07}, {0x07, 0x09}, {0x07, 0x0b},
        {0x0c, 0x11}, {0x04, 0x07}, {0x09, 0x11}, {0x07, 0x0f},
        {0x07, 0x0b}, {0x04, 0x07}, {0x0a, 0x13}, {0x0a, 0x0e},
        {0x07, 0x0b}, {0x08, 0x0e}, {0x06, 0x0e}, {0x08, 0x11},
        {0x0b, 0x15}, {0x08, 0x0e}, {0x07, 0x09}, {0x07, 0x0b},
        {0x04, 0x05}, {0x06, 0x07}, {0x04, 0x05}, {0x05, 0x07}
    };
    if (kind == 0U) {
        uint32_t product =
            (uint32_t)index * 5U << 10;
        int32_t sum = frame_saturate_i32(
            (int64_t)(int32_t)product + 0xffff);
        uint16_t raw = (uint16_t)(
            (uint32_t)sum >> 16);
        int16_t value = (int16_t)(uint16_t)(raw - 1U);
        return value < 0 ? 0 : value;
    }
    if (kind > 3U) {
        return 0;
    }
    return (int16_t)(
        (int32_t)table[index - 128][selector] - 1);
}

int64_t vocoder_arithmetic_shift_right_i64(
    int64_t value,
    uint8_t shift)
{
    if (shift == 0U) {
        return value;
    }
    if (shift >= 64U) {
        return value < 0 ? -1 : 0;
    }
    uint64_t result =
        (uint64_t)value >> (unsigned int)shift;
    if (value < 0) {
        result |= UINT64_MAX <<
            (64U - (unsigned int)shift);
    }
    return (int64_t)result;
}

uint64_t vocoder_shift_left_u64(
    uint64_t value,
    uint8_t shift)
{
    return shift < 64U
        ? value << (unsigned int)shift
        : 0U;
}

int16_t vocoder_squared_vector_difference(
    int16_t *result_exponent,
    const int16_t *left,
    int32_t left_exponent,
    const int16_t *right,
    int16_t right_exponent,
    int16_t packed_count)
{
    const int16_t *primary = left;
    const int16_t *secondary = right;
    int32_t primary_exponent = left_exponent;
    int32_t secondary_exponent = right_exponent;
    if (primary_exponent < secondary_exponent) {
        primary = right;
        secondary = left;
        int32_t temporary = primary_exponent;
        primary_exponent = secondary_exponent;
        secondary_exponent = temporary;
    }
    int32_t shift =
        primary_exponent + 1 - secondary_exponent;
    int32_t count =
        ((int32_t)packed_count & 0x7fff) * 2;
    int64_t energy = 0;
    for (int32_t index = 0; index < count; ++index) {
        uint32_t primary_q =
            (uint32_t)((int32_t)primary[index] * 0x8000);
        uint32_t secondary_q =
            (uint32_t)(int32_t)secondary[index] << 16;
        secondary_q = frame_arithmetic_shift_right(
            secondary_q, (unsigned int)shift);
        int16_t difference = (int16_t)(
            (primary_q - secondary_q) >> 16);
        energy += (int32_t)difference * difference;
    }
    int32_t exponent = primary_exponent * 2 + 3;
    int32_t mantissa = frame_normalize_i64_to_i32(
        energy, &exponent);
    *result_exponent = (int16_t)exponent;
    return (int16_t)frame_arithmetic_shift_right(
        (uint32_t)mantissa, 16);
}

int32_t vocoder_complex_weighted_correlation(
    int16_t output[2],
    const int16_t *weights,
    const uint32_t *input,
    int32_t input_exponent,
    const uint32_t *reference,
    int32_t reference_exponent,
    int32_t count)
{
    int64_t real_sum = 0;
    int64_t imaginary_sum = 0;
    for (int32_t index = 0; index < count; ++index) {
        int16_t input_real = (int16_t)input[index];
        int16_t input_imaginary =
            (int16_t)(input[index] >> 16);
        int32_t real_product =
            (int32_t)input_real * weights[index];
        uint32_t real_bits =
            ((uint32_t)real_product & 0x3fffffffU) >> 14;
        uint32_t imaginary_bits = (uint32_t)(
            (int32_t)input_imaginary * weights[index]) * 4U;
        int16_t scaled_real = (int16_t)real_bits;
        int16_t scaled_imaginary = (int16_t)
            frame_arithmetic_shift_right(
                imaginary_bits, 16);
        int16_t reference_real =
            (int16_t)reference[index];
        int16_t reference_imaginary =
            (int16_t)(reference[index] >> 16);
        real_sum +=
            (int32_t)scaled_real * reference_real +
            (int32_t)scaled_imaginary * reference_imaginary;
        imaginary_sum +=
            (int32_t)reference_real * scaled_imaginary -
            (int32_t)reference_imaginary * scaled_real;
    }

    int32_t base_exponent =
        input_exponent + reference_exponent;
    int32_t real_exponent = base_exponent;
    int32_t real_mantissa = frame_normalize_i64_to_i32(
        real_sum * 2, &real_exponent);
    int32_t imaginary_exponent = base_exponent;
    int32_t imaginary_mantissa = frame_normalize_i64_to_i32(
        imaginary_sum * 2, &imaginary_exponent);
    int32_t common_exponent =
        real_exponent < imaginary_exponent
        ? imaginary_exponent : real_exponent;
    if (real_exponent < common_exponent) {
        real_mantissa = (int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)real_mantissa,
                (unsigned int)(common_exponent - real_exponent));
    }
    if (imaginary_exponent < common_exponent) {
        imaginary_mantissa = (int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)imaginary_mantissa,
                (unsigned int)(
                    common_exponent - imaginary_exponent));
    }
    output[0] = (int16_t)frame_arithmetic_shift_right(
        (uint32_t)real_mantissa, 16);
    output[1] = (int16_t)frame_arithmetic_shift_right(
        (uint32_t)imaginary_mantissa, 16);
    return common_exponent;
}

int32_t vocoder_test_weighted_spectral_threshold(
    const int16_t *values,
    int32_t value_exponent,
    int32_t scale,
    int32_t target_exponent,
    int16_t multiplier,
    int32_t bias,
    int32_t threshold_multiplier)
{
    if (scale < 1) {
        return 0;
    }
    uint32_t weighted = 0;
    for (int32_t index = 1; index <= 4; ++index) {
        weighted += (uint32_t)(
            index * (int32_t)values[index]) * 2U;
    }
    weighted = frame_scale_i32(
        weighted, value_exponent - target_exponent);
    uint32_t scaled_product =
        (uint32_t)scale * (uint32_t)(int32_t)multiplier * 2U;
    int32_t reference = (int32_t)
        frame_arithmetic_shift_right(scaled_product, 9);
    uint32_t biased_product =
        (uint32_t)bias * (uint32_t)scale * 2U;
    int32_t difference = (int32_t)(
        weighted + biased_product - (uint32_t)reference);
    if (difference == INT32_MIN) {
        return 0;
    }
    if (difference < 0) {
        difference = -difference;
    }
    int64_t threshold_product =
        (int64_t)threshold_multiplier * reference;
    int32_t threshold = (int32_t)
        vocoder_arithmetic_shift_right_i64(
            threshold_product, 20);
    return difference < threshold;
}

int16_t vocoder_test_two_spectral_neighborhoods(
    int32_t left_scale,
    int32_t right_scale,
    int32_t target_exponent,
    const int32_t *values,
    int32_t value_exponent,
    int32_t left_index,
    int16_t left_multiplier,
    int32_t right_index,
    int16_t right_multiplier,
    int16_t threshold_multiplier)
{
    int16_t left_values[5];
    int16_t right_values[5];
    int32_t left_exponent = value_exponent;
    int32_t right_exponent = value_exponent;
    vocoder_normalize_i32_to_i16(
        left_values, values + left_index, 5, &left_exponent);
    vocoder_normalize_i32_to_i16(
        right_values, values + right_index, 5, &right_exponent);
    if (vocoder_test_weighted_spectral_threshold(
            left_values, left_exponent, left_scale,
            target_exponent, left_multiplier, left_index,
            threshold_multiplier) == 0) {
        return 0;
    }
    return (int16_t)(
        vocoder_test_weighted_spectral_threshold(
            right_values, right_exponent, right_scale,
            target_exponent, right_multiplier, right_index,
            threshold_multiplier) != 0);
}

void vocoder_write_repeated_frame_code(
    int32_t code,
    const int16_t levels[2],
    const void *configuration,
    int16_t **cursor)
{
    vocoder_write_bits_cursor(cursor, 6, 0x3f);
    int32_t encoded = 0x3f;
    uint32_t flag = 1;
    int32_t level;
    if (code < 0x80) {
        level = levels[1];
    }
    else {
        uint32_t sum = (uint32_t)(int32_t)levels[0] +
            (uint32_t)(int32_t)levels[1];
        level = (int32_t)
            frame_arithmetic_shift_right(sum, 1);
        if (level >= 0x7000) {
            goto write_code;
        }
        if (level < -0x5000) {
            level = -0x5000;
        }
    }

    int32_t scaled = level * 0x220 + 0x110000;
    int32_t quantized = (int32_t)
        frame_arithmetic_shift_right((uint32_t)scaled, 17);
    if (quantized <= 0x7f) {
        if (quantized < 0) {
            quantized = 0;
        }
        encoded = (int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)quantized, 1);
        flag = (uint32_t)quantized & 1U;
    }

write_code:
    vocoder_write_bits_cursor(cursor, 6, encoded);
    int16_t frame_length;
    memcpy(&frame_length,
           (const uint8_t *)configuration + 4,
           sizeof(frame_length));
    int32_t repetitions = (int32_t)
        frame_arithmetic_shift_right(
            (uint32_t)((int32_t)frame_length - 13), 3);
    for (int32_t index = 0; index < repetitions; ++index) {
        vocoder_write_bits_cursor(cursor, 8, code);
    }
    vocoder_write_bits_cursor(cursor, 1, (int32_t)flag);
}

void vocoder_decode_spread_codeword(
    int32_t enabled,
    uint32_t *low,
    uint32_t *high,
    int32_t value,
    int32_t bit_count)
{
    static const uint32_t codewords[128] = {
        0x55555555U, 0x55555551U, 0x55545555U, 0x55505555U,
        0x55555554U, 0x55515555U, 0x55515551U, 0x55555556U,
        0x55545554U, 0x55455555U, 0x55505554U, 0x55555550U,
        0x55555545U, 0x55405555U, 0x55155555U, 0x55955555U,
        0x55505550U, 0x55505500U, 0x55055505U, 0x59555155U,
        0x51555155U, 0x55505540U, 0x55155515U, 0x51555955U,
        0x54555455U, 0x55445544U, 0x55555540U, 0x55515550U,
        0x55455545U, 0x55545544U, 0x55545550U, 0x56555455U,
        0x55005500U, 0x55005550U, 0x55015501U, 0x55405544U,
        0x55405540U, 0x55405550U, 0x55415541U, 0x55405554U,
        0x54005400U, 0x55005400U, 0x54005500U, 0x55005540U,
        0x54015400U, 0x55405400U, 0x55405500U, 0x55005504U,
        0x50005000U, 0x50005500U, 0x50005400U, 0x50005200U,
        0x54005000U, 0x50005800U, 0x55005000U, 0x51005000U,
        0x40004000U, 0x50004000U, 0x50000000U, 0x52005000U,
        0x54000000U, 0x00005400U, 0x40005000U, 0x58005000U,
        0x00000000U, 0x00000100U, 0x40000000U, 0x00000001U,
        0x00004000U, 0x10000000U, 0x00005000U, 0x00001000U,
        0x80000000U, 0x0a800000U, 0xa8000000U, 0x02a00000U,
        0xa0000000U, 0xaa000000U, 0x02aa0000U, 0x0a000000U,
        0xaaaa0000U, 0x00800000U, 0x0aaa0000U, 0xaa800000U,
        0x08000000U, 0xaaa00000U, 0x2aaa0000U, 0xaaa80000U,
        0x00020000U, 0x00080000U, 0x000a0000U, 0x00200000U,
        0x002a0000U, 0x00aa0000U, 0x02000000U, 0x02800000U,
        0xaaaaaaaaU, 0xaaaa0aaaU, 0xa800a800U, 0x02aa02aaU,
        0x0aaa0aaaU, 0xaaaaa800U, 0xa000a000U, 0x08000800U,
        0x00008000U, 0x00000280U, 0x00000200U, 0x00000aa8U,
        0x00000800U, 0x00000a00U, 0x00000a80U, 0x000002a0U,
        0x0000aaaaU, 0x0000aa80U, 0x0000a800U, 0x00002aaaU,
        0x0000a000U, 0x000002aaU, 0x00000aaaU, 0x0000aaa0U,
        0x00000002U, 0x0000002aU, 0x00000008U, 0x00000020U,
        0x0000000aU, 0x000000aaU, 0x00000080U, 0x000000a0U
    };
    if (enabled != 1) {
        *low = 0;
        *high = 0;
        return;
    }
    uint32_t shifted = frame_shift_left(
        (uint32_t)value,
        (unsigned int)(7 - bit_count));
    int16_t index = (int16_t)shifted;
    vocoder_spread_two_bit_fields(
        low, high, codewords[index]);
}

void vocoder_apply_complex_swap_table(
    int16_t *values,
    int32_t size,
    int32_t unused,
    const int16_t *table)
{
    static const int16_t default_table[] = {
        56, 2, 128, 1, -65, 1, 127, 1, -161, 1, 127, 1,
        -65, 1, 127, 3, -81, 1, -65, 1, 127, 1, -161, 1,
        127, 1, -65, 1, 127, 3, -105, 1, -65, 1, 127, 3,
        -33, 1, -65, 1, 127, 3, -81, 1, -65, 1, 127, 3,
        -33, 1, -65, 1, 127, 3, -117, 3, 63, 3, -33, 1,
        -65, 1, 127, 3, -81, 3, 63, 3, -33, 1, -65, 1,
        127, 3, -105, 3, 63, 3, -33, 3, 63, 3, -81, 3,
        63, 3, -33, 3, 63, 7, -59, 3, -33, 3, 63, 7,
        -17, 3, -33, 3, 63, 7, -41, 7, 31, 7, -17, 7,
        31, 15, -21, 15, 15
    };
    static const int16_t table_32[] = {
        12, 2, 32, 1, -17, 1, 31, 3, -9, 1, -17, 1, 31,
        3, -21, 3, 15, 3, -9, 3, 15, 7, -11, 7, 7
    };
    (void)unused;
    if (size == 32) {
        table = table_32;
    }
    else if (size != 8 && size != 16 && size != 64) {
        table = default_table;
    }

    int32_t count = table[0];
    const int16_t *indices = table + 1;
    uint8_t *left_base = (uint8_t *)values;
    uint8_t *right_base = (uint8_t *)values;
    for (int32_t index = 0; index < count; ++index) {
        left_base += (ptrdiff_t)indices[index * 2] * 2;
        right_base += (ptrdiff_t)indices[index * 2 + 1] * 2;
        uint32_t left_value;
        uint32_t right_value;
        memcpy(&left_value, left_base, sizeof(left_value));
        memcpy(&right_value, right_base, sizeof(right_value));
        memcpy(left_base, &right_value, sizeof(right_value));
        memcpy(right_base, &left_value, sizeof(left_value));
        left_base += 2;
        right_base += 2;
    }
}

static const uint32_t frame_twiddles[256] = {
        0x00007fffU, 0xfe6e7ffeU, 0xfcdc7ff6U, 0xfb4a7feaU,
        0xf9b87fd9U, 0xf8277fc2U, 0xf6957fa7U, 0xf5057f87U,
        0xf3747f62U, 0xf1e47f38U, 0xf0557f0aU, 0xeec67ed6U,
        0xed387e9dU, 0xebab7e60U, 0xea1e7e1eU, 0xe8927dd6U,
        0xe7077d8aU, 0xe57d7d3aU, 0xe3f47ce4U, 0xe26d7c89U,
        0xe0e67c2aU, 0xdf617bc6U, 0xdddc7b5dU, 0xdc597aefU,
        0xdad87a7dU, 0xd9587a06U, 0xd7d9798aU, 0xd65c790aU,
        0xd4e17885U, 0xd36777fbU, 0xd1ef776cU, 0xd07976d9U,
        0xcf047642U, 0xcd9275a6U, 0xcc217505U, 0xcab27460U,
        0xc94673b6U, 0xc7db7308U, 0xc6737255U, 0xc50d719eU,
        0xc3a970e3U, 0xc2487023U, 0xc0e96f5fU, 0xbf8c6e97U,
        0xbe326dcaU, 0xbcda6cf9U, 0xbb856c24U, 0xba336b4bU,
        0xb8e36a6eU, 0xb796698cU, 0xb64c68a7U, 0xb50567bdU,
        0xb3c066d0U, 0xb27f65deU, 0xb14064e9U, 0xb00563efU,
        0xaecc62f2U, 0xad9761f1U, 0xac6560ecU, 0xab365fe4U,
        0xaa0a5ed7U, 0xa8e25dc8U, 0xa7bd5cb4U, 0xa69c5b9dU,
        0xa57e5a82U, 0xa4635964U, 0xa34c5843U, 0xa238571eU,
        0xa12955f6U, 0xa01c54caU, 0x9f14539bU, 0x9e0f5269U,
        0x9d0e5134U, 0x9c114ffbU, 0x9b174ec0U, 0x9a224d81U,
        0x99304c40U, 0x98434afbU, 0x975949b4U, 0x9674486aU,
        0x9592471dU, 0x94b545cdU, 0x93dc447bU, 0x93074326U,
        0x923641ceU, 0x91694074U, 0x90a13f17U, 0x8fdd3db8U,
        0x8f1d3c57U, 0x8e623af3U, 0x8dab398dU, 0x8cf83825U,
        0x8c4a36baU, 0x8ba0354eU, 0x8afb33dfU, 0x8a5a326eU,
        0x89be30fcU, 0x89272f87U, 0x88942e11U, 0x88052c99U,
        0x877b2b1fU, 0x86f629a4U, 0x86762827U, 0x85fa26a8U,
        0x85832528U, 0x851123a7U, 0x84a32224U, 0x843a209fU,
        0x83d61f1aU, 0x83771d93U, 0x831c1c0cU, 0x82c61a83U,
        0x827618f9U, 0x822a176eU, 0x81e215e2U, 0x81a01455U,
        0x816312c8U, 0x812a113aU, 0x80f60fabU, 0x80c80e1cU,
        0x809e0c8cU, 0x80790afbU, 0x8059096bU, 0x803e07d9U,
        0x80270648U, 0x801604b6U, 0x800a0324U, 0x80020192U,
        0x80010000U, 0x8002fe6eU, 0x800afcdcU, 0x8016fb4aU,
        0x8027f9b8U, 0x803ef827U, 0x8059f695U, 0x8079f505U,
        0x809ef374U, 0x80c8f1e4U, 0x80f6f055U, 0x812aeec6U,
        0x8163ed38U, 0x81a0ebabU, 0x81e2ea1eU, 0x822ae892U,
        0x8276e707U, 0x82c6e57dU, 0x831ce3f4U, 0x8377e26dU,
        0x83d6e0e6U, 0x843adf61U, 0x84a3dddcU, 0x8511dc59U,
        0x8583dad8U, 0x85fad958U, 0x8676d7d9U, 0x86f6d65cU,
        0x877bd4e1U, 0x8805d367U, 0x8894d1efU, 0x8927d079U,
        0x89becf04U, 0x8a5acd92U, 0x8afbcc21U, 0x8ba0cab2U,
        0x8c4ac946U, 0x8cf8c7dbU, 0x8dabc673U, 0x8e62c50dU,
        0x8f1dc3a9U, 0x8fddc248U, 0x90a1c0e9U, 0x9169bf8cU,
        0x9236be32U, 0x9307bcdaU, 0x93dcbb85U, 0x94b5ba33U,
        0x9592b8e3U, 0x9674b796U, 0x9759b64cU, 0x9843b505U,
        0x9930b3c0U, 0x9a22b27fU, 0x9b17b140U, 0x9c11b005U,
        0x9d0eaeccU, 0x9e0fad97U, 0x9f14ac65U, 0xa01cab36U,
        0xa129aa0aU, 0xa238a8e2U, 0xa34ca7bdU, 0xa463a69cU,
        0xa57ea57eU, 0xa69ca463U, 0xa7bda34cU, 0xa8e2a238U,
        0xaa0aa129U, 0xab36a01cU, 0xac659f14U, 0xad979e0fU,
        0xaecc9d0eU, 0xb0059c11U, 0xb1409b17U, 0xb27f9a22U,
        0xb3c09930U, 0xb5059843U, 0xb64c9759U, 0xb7969674U,
        0xb8e39592U, 0xba3394b5U, 0xbb8593dcU, 0xbcda9307U,
        0xbe329236U, 0xbf8c9169U, 0xc0e990a1U, 0xc2488fddU,
        0xc3a98f1dU, 0xc50d8e62U, 0xc6738dabU, 0xc7db8cf8U,
        0xc9468c4aU, 0xcab28ba0U, 0xcc218afbU, 0xcd928a5aU,
        0xcf0489beU, 0xd0798927U, 0xd1ef8894U, 0xd3678805U,
        0xd4e1877bU, 0xd65c86f6U, 0xd7d98676U, 0xd95885faU,
        0xdad88583U, 0xdc598511U, 0xdddc84a3U, 0xdf61843aU,
        0xe0e683d6U, 0xe26d8377U, 0xe3f4831cU, 0xe57d82c6U,
        0xe7078276U, 0xe892822aU, 0xea1e81e2U, 0xebab81a0U,
        0xed388163U, 0xeec6812aU, 0xf05580f6U, 0xf1e480c8U,
        0xf374809eU, 0xf5058079U, 0xf6958059U, 0xf827803eU,
        0xf9b88027U, 0xfb4a8016U, 0xfcdc800aU, 0xfe6e8002U
};

void vocoder_radix4_twiddle_butterflies(
    uint32_t *left,
    uint32_t *right,
    int32_t twiddle_step,
    int32_t count)
{
    int32_t twiddle_index = twiddle_step;
    for (int32_t index = 0; index < count; ++index) {
        uint32_t coefficient = frame_twiddles[twiddle_index];
        int32_t coefficient_real =
            (int16_t)(uint16_t)coefficient;
        int32_t coefficient_imag =
            (int16_t)(uint16_t)(coefficient >> 16);
        int32_t right_real =
            (int16_t)(uint16_t)*right;
        int32_t right_imag =
            (int16_t)(uint16_t)(*right >> 16);
        uint32_t cross = (uint32_t)(
            right_real * coefficient_imag) +
            (uint32_t)(right_imag * coefficient_real);
        uint32_t difference = (uint32_t)(
            right_real * coefficient_real) -
            (uint32_t)(right_imag * coefficient_imag);
        uint32_t left_value = *left;
        int32_t left_real =
            (int16_t)(uint16_t)left_value;
        int32_t left_imag =
            (int16_t)(uint16_t)(left_value >> 16);
        uint32_t sum_imag =
            cross + ((uint32_t)left_imag << 15);
        uint32_t sum_real =
            difference + ((uint32_t)left_real << 15);
        uint32_t other_real =
            ((uint32_t)left_real << 15) - difference;
        uint32_t other_imag =
            cross - ((uint32_t)left_imag << 15);
        *left = (sum_imag & 0xffff0000U) |
            ((sum_real >> 16) & 0xffffU);
        *right = (other_imag & 0xffff0000U) |
            ((other_real >> 16) & 0xffffU);
        ++left;
        --right;
        twiddle_index += twiddle_step;
    }
}

static uint32_t frame_pack_twiddle_term(
    uint32_t real_term,
    uint32_t imaginary_term,
    unsigned int shift)
{
    uint32_t real = frame_arithmetic_shift_right(
        real_term, shift);
    uint32_t imaginary = frame_arithmetic_shift_right(
        imaginary_term, shift);
    return (real & 0xffffU) |
        ((imaginary & 0xffffU) << 16);
}

void vocoder_radix4_matrix_stage(
    uint32_t *left,
    uint32_t *right,
    int32_t twiddle_step,
    int32_t group_size,
    int32_t group_count,
    int32_t scaling_mode)
{
    unsigned int shift = scaling_mode == -1 ? 16U : 15U;
    for (int32_t group = 0; group < group_count; ++group) {
        int32_t twiddle_index = 0;
        for (int32_t index = 0;
                index < group_size; ++index) {
            uint32_t coefficient =
                frame_twiddles[twiddle_index];
            int32_t coefficient_real =
                (int16_t)(uint16_t)coefficient;
            int32_t coefficient_imaginary =
                (int16_t)(uint16_t)(coefficient >> 16);
            uint32_t right_value = right[index];
            int32_t right_real =
                (int16_t)(uint16_t)right_value;
            int32_t right_imaginary =
                (int16_t)(uint16_t)(right_value >> 16);
            uint32_t difference =
                (uint32_t)(right_real * coefficient_real) -
                (uint32_t)(
                    right_imaginary * coefficient_imaginary);
            uint32_t cross =
                (uint32_t)(
                    right_real * coefficient_imaginary) +
                (uint32_t)(
                    right_imaginary * coefficient_real);
            uint32_t left_value = left[index];
            int32_t left_real =
                (int16_t)(uint16_t)left_value;
            int32_t left_imaginary =
                (int16_t)(uint16_t)(left_value >> 16);
            uint32_t left_real_q15 =
                (uint32_t)left_real << 15;
            uint32_t left_imaginary_q15 =
                (uint32_t)left_imaginary << 15;
            left[index] = frame_pack_twiddle_term(
                left_real_q15 + difference,
                left_imaginary_q15 + cross, shift);
            right[index] = frame_pack_twiddle_term(
                left_real_q15 - difference,
                left_imaginary_q15 - cross, shift);
            twiddle_index += twiddle_step;
        }
        left += group_size * 2;
        right += group_size * 2;
    }
}

int32_t vocoder_complex_fft_32(
    uint32_t values[32],
    int32_t exponent,
    int32_t log2_size)
{
    int32_t size = (int32_t)frame_shift_left(
        1U, (unsigned int)log2_size);
    int32_t peak_shift =
        vocoder_complex_peak_normalization_shift(values, size);
    int32_t exponent_adjustment;
    if (peak_shift == -1) {
        exponent_adjustment = -2;
    }
    else if (peak_shift == 0 || peak_shift == 1) {
        exponent_adjustment = -1;
    }
    else {
        exponent_adjustment = 0;
    }

    vocoder_apply_complex_swap_table(
        (int16_t *)(void *)values, size, 0, NULL);
    vocoder_pairwise_complex_butterflies(
        values, size, exponent_adjustment);

    int32_t group_count = size / 4;
    int32_t current_shift = peak_shift - 2;
    int32_t twiddle_step = 128;
    int32_t group_size = 2;
    for (int32_t stage = 1; stage < log2_size; ++stage) {
        if ((stage & 1) == 0) {
            current_shift =
                vocoder_complex_peak_normalization_shift(
                    values, size);
        }
        int32_t scaling_mode =
            current_shift <= 1 ? -1 : 0;
        vocoder_radix4_matrix_stage(
            values, values + group_size,
            twiddle_step, group_size, group_count,
            scaling_mode);
        exponent_adjustment += scaling_mode;
        twiddle_step /= 2;
        group_count /= 2;
        group_size *= 2;
        current_shift -= 2;
    }
    return (int32_t)(
        (uint32_t)exponent -
        (uint32_t)exponent_adjustment);
}

int32_t vocoder_real_fft_forward_64(
    int16_t values[66],
    int32_t exponent,
    int32_t log2_size,
    int32_t half_scale_mode)
{
    int16_t complex_log2 =
        (int16_t)(log2_size - 1);
    int32_t result = vocoder_complex_fft_32(
        (uint32_t *)(void *)values,
        exponent, complex_log2);
    result = vocoder_transform_half_spectrum_stage(
        values, result, log2_size, 0, half_scale_mode);
    if (half_scale_mode == 0) {
        values[1] = 0;
    }
    return result;
}

int32_t vocoder_real_fft_inverse_64(
    int16_t values[66],
    int32_t exponent,
    int32_t log2_size,
    int32_t half_scale_mode)
{
    if (half_scale_mode == 0) {
        values[1] = 0;
    }
    int32_t result = vocoder_transform_half_spectrum_stage(
        values, exponent, log2_size, 1, half_scale_mode);
    int16_t complex_log2 =
        (int16_t)(log2_size - 1);
    result = vocoder_complex_fft_32(
        (uint32_t *)(void *)values,
        result, complex_log2);
    return (int32_t)(
        (uint32_t)result +
        (uint32_t)(1 - log2_size));
}

int32_t vocoder_windowed_real_fft_energy(
    int32_t *energy,
    const int16_t *right,
    int32_t right_exponent,
    const int16_t *left,
    int32_t left_exponent,
    int32_t active_count,
    int32_t center_output,
    int16_t *workspace,
    int32_t log2_size,
    int32_t half_scale_mode)
{
    int32_t output_count = (int32_t)frame_shift_left(
        1U, (unsigned int)log2_size);
    vocoder_multiply_symmetric(
        workspace, right, output_count, center_output,
        left, active_count);
    int32_t result = vocoder_real_fft_forward_64(
        workspace, (int32_t)(
            (uint32_t)right_exponent +
            (uint32_t)left_exponent),
        log2_size, half_scale_mode);
    if (energy != NULL) {
        result = (int32_t)((uint32_t)result << 1);
        int16_t magnitude_count = (int16_t)(
            half_scale_mode + output_count / 2);
        vocoder_complex_magnitude_squared(
            energy, (const uint32_t *)(const void *)workspace,
            magnitude_count);
    }
    return result;
}

int32_t vocoder_normalized_windowed_fft_energy(
    int32_t *energy,
    const int16_t *input,
    const int16_t *left,
    int32_t left_exponent,
    int32_t active_count,
    int32_t center_output,
    int16_t *workspace,
    int32_t log2_size,
    int32_t half_scale_mode)
{
    int16_t normalized[256];
    int32_t normalized_exponent = 0;
    vocoder_copy_i16(
        normalized, input, (int16_t)active_count);
    vocoder_normalize_i16_vector(
        normalized, normalized, active_count,
        &normalized_exponent);
    return vocoder_windowed_real_fft_energy(
        energy, normalized, normalized_exponent,
        left, left_exponent, active_count,
        center_output, workspace, log2_size,
        half_scale_mode);
}

static int16_t frame_half_sum_i16(
    int16_t left, int16_t right)
{
    uint32_t sum = (uint32_t)(int32_t)left +
        (uint32_t)(int32_t)right;
    return (int16_t)(int32_t)
        frame_arithmetic_shift_right(sum, 1);
}

static int16_t frame_saturating_add_i16(
    int16_t left, int16_t right)
{
    int32_t result = (int32_t)left + (int32_t)right;
    if (result > INT16_MAX) {
        result = INT16_MAX;
    }
    if (result < INT16_MIN) {
        result = INT16_MIN;
    }
    return (int16_t)result;
}

static int16_t frame_saturating_subtract_i16(
    int16_t left, int16_t right)
{
    int32_t result = (int32_t)left - (int32_t)right;
    if (result > INT16_MAX) {
        result = INT16_MAX;
    }
    if (result < INT16_MIN) {
        result = INT16_MIN;
    }
    return (int16_t)result;
}

void vocoder_pairwise_complex_butterflies(
    uint32_t *values,
    int32_t packed_count,
    int32_t scaling_mode)
{
    for (int32_t index = 0; index < packed_count; index += 2) {
        uint32_t first = values[index];
        uint32_t second = values[index + 1];
        int16_t first_real = (int16_t)(uint16_t)first;
        int16_t first_imag =
            (int16_t)(uint16_t)(first >> 16);
        int16_t second_real = (int16_t)(uint16_t)second;
        int16_t second_imag =
            (int16_t)(uint16_t)(second >> 16);
        int16_t sum_real;
        int16_t sum_imag;
        int16_t difference_real;
        int16_t difference_imag;
        if (scaling_mode == 0) {
            sum_real = frame_saturating_add_i16(
                first_real, second_real);
            sum_imag = frame_saturating_add_i16(
                first_imag, second_imag);
            difference_real = frame_saturating_subtract_i16(
                first_real, second_real);
            difference_imag = frame_saturating_subtract_i16(
                first_imag, second_imag);
        }
        else {
            sum_real = frame_half_sum_i16(
                first_real, second_real);
            sum_imag = frame_half_sum_i16(
                first_imag, second_imag);
            uint32_t real_difference =
                (uint32_t)(int32_t)first_real -
                (uint32_t)(int32_t)second_real;
            uint32_t imag_difference =
                (uint32_t)(int32_t)first_imag -
                (uint32_t)(int32_t)second_imag;
            difference_real = (int16_t)(int32_t)
                frame_arithmetic_shift_right(
                    real_difference, 1);
            difference_imag = (int16_t)(int32_t)
                frame_arithmetic_shift_right(
                    imag_difference, 1);
            if (scaling_mode == -2) {
                sum_real = frame_half_sum_i16(sum_real, 0);
                sum_imag = frame_half_sum_i16(sum_imag, 0);
                difference_real =
                    frame_half_sum_i16(difference_real, 0);
                difference_imag =
                    frame_half_sum_i16(difference_imag, 0);
            }
        }
        values[index] =
            (uint32_t)(uint16_t)sum_real |
            ((uint32_t)(uint16_t)sum_imag << 16);
        values[index + 1] =
            (uint32_t)(uint16_t)difference_real |
            ((uint32_t)(uint16_t)difference_imag << 16);
    }
}

int32_t vocoder_transform_half_spectrum_stage(
    int16_t *values,
    int32_t exponent,
    int32_t log2_size,
    int32_t inverse,
    int32_t half_scale_mode)
{
    int32_t size = (int32_t)frame_shift_left(
        1U, (unsigned int)log2_size);
    int16_t first_real = values[0];
    int16_t first_imag = values[1];
    if (half_scale_mode == 0) {
        int32_t real = (int32_t)first_real * 0x8000;
        int32_t imag = (int32_t)first_imag * 0x8000;
        values[0] = (int16_t)(int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)real + (uint32_t)imag, 16);
        values[1] = (int16_t)(int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)real - (uint32_t)imag, 16);
    }
    else if (inverse != 0) {
        int16_t opposite = values[size];
        int32_t real = (int32_t)first_real * 0x4000;
        int32_t imag = (int32_t)opposite * 0x4000;
        values[0] = (int16_t)(int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)real + (uint32_t)imag, 16);
        values[1] = (int16_t)(int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)real - (uint32_t)imag, 16);
    }
    else {
        int32_t real = (int32_t)first_real * 0x8000;
        int32_t imag = (int32_t)first_imag * 0x8000;
        values[0] = (int16_t)(int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)real + (uint32_t)imag, 16);
        values[size] = (int16_t)(int32_t)
            frame_arithmetic_shift_right(
                (uint32_t)real - (uint32_t)imag, 16);
        values[1] = 0;
        values[size + 1] = 0;
    }

    int32_t count = size / 4 - 1;
    uint32_t *forward = (uint32_t *)(void *)(values + 2);
    uint32_t *backward =
        (uint32_t *)(void *)(values + size - 2);
    if (inverse != 0) {
        vocoder_complex_butterfly_inverse(
            forward, backward, count);
    }
    else {
        vocoder_complex_butterfly_forward(
            forward, backward, count);
    }
    int32_t twiddle_step = 512 / size;
    vocoder_radix4_twiddle_butterflies(
        forward, backward, twiddle_step, count);

    int32_t center = size / 2;
    values[center] = (int16_t)(int32_t)
        frame_arithmetic_shift_right(
            (uint32_t)(int32_t)values[center], 1);
    uint32_t negated =
        (uint32_t)(int32_t)values[center + 1] << 16;
    negated = negated == 0x80000000U
        ? 0x7fffffffU
        : 0U - negated;
    values[center + 1] = (int16_t)(int32_t)
        frame_arithmetic_shift_right(negated, 17);
    return (int32_t)((uint32_t)exponent + 1U);
}
