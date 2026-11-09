#include <stdint.h>
#include <string.h>

#include "portable_frame.h"

static const int16_t encoder_analysis_window[199] = {
    2390,2398,2419,2453,2502,2564,2639,2729,
    2831,2948,3077,3219,3375,3544,3725,3919,
    4125,4343,4573,4814,5067,5331,5607,5892,
    6188,6494,6810,7135,7469,7812,8163,8523,
    8889,9264,9645,10033,10427,10826,11231,11641,
    12056,12474,12896,13321,13750,14180,14614,15048,
    15483,15919,16355,16791,17226,17661,18093,18524,
    18952,19378,19800,20219,20633,21042,21447,21848,
    22242,22629,23010,23384,23751,24110,24461,24804,
    25139,25464,25779,26086,26381,26667,26942,27206,
    27459,27701,27930,28149,28355,28549,28730,28898,
    29054,29196,29326,29443,29546,29635,29711,29773,
    29821,29856,29876,29883,381,789,1223,1685,
    2174,2693,2859,3030,3204,3382,3564,3749,
    3939,4132,4329,4530,4734,4942,5154,5369,
    5588,5810,6035,6264,6495,6730,6968,7210,
    7454,7701,7950,8203,8458,8715,8976,9238,
    9503,9770,10039,10310,10583,10857,11134,11412,
    11691,11972,12254,12537,12821,13106,13392,13679,
    13966,14253,14541,14829,15117,15405,15693,15980,
    16267,16554,16839,17124,17408,17691,17972,18252,
    18531,18808,19084,19357,19628,19898,20165,20429,
    20691,20951,21208,21461,21712,21960,22204,22445,
    22682,22916,23146,23372,23594,23812,24026,24236,
    24441,24641,24837,25029,25215,25397,25573
};

static int16_t load_i16(const uint8_t *base, size_t offset)
{
    int16_t value;
    memcpy(&value, base + offset, sizeof(value));
    return value;
}

static void store_i16(uint8_t *base, size_t offset, int16_t value)
{
    memcpy(base + offset, &value, sizeof(value));
}

void vocoder_run_encoder_analysis(
    void *opaque_state, const int16_t *new_samples,
    int32_t sample_count, uint32_t flags, int16_t enabled)
{
    uint8_t *state = (uint8_t *)opaque_state;
    int16_t *sample_history = (int16_t *)(void *)(state + 0x13a4);
    int16_t *encoder_state = (int16_t *)(void *)(state + 0x18);
    int32_t retained = (int16_t)(0x17f - (int16_t)sample_count);
    int16_t pitch_flag = 0;
    uint32_t scratch[130];

    vocoder_copy_i16(sample_history, sample_history + sample_count, retained);
    int16_t *tail = sample_history + retained;
    if ((flags & 0x100U) == 0U) {
        vocoder_filter_i16_state6(
            (int32_t *)(void *)state, tail, new_samples, sample_count);
    }
    else {
        vocoder_copy_i16(tail, new_samples, sample_count);
    }

    vocoder_copy_i16(
        (int16_t *)(void *)(state + 0x420),
        (const int16_t *)(const void *)(state + 0x21c), 0x102);
    store_i16(state, 0x628, load_i16(state, 0x626));
    vocoder_copy_i16(
        (int16_t *)(void *)(state + 0x21c), encoder_state, 0x102);
    store_i16(state, 0x626, load_i16(state, 0x624));
    int32_t spectrum_exponent = vocoder_normalized_windowed_fft_energy(
        (int32_t *)(void *)encoder_state,
        (const int16_t *)(const void *)(state + 0x13dc),
        encoder_analysis_window, -7, 199, 0,
        (int16_t *)(void *)scratch, 8, 1);
    store_i16(state, 0x624, (int16_t)spectrum_exponent);

    vocoder_analyze_encoder_frame(
        state + 0xb50, encoder_state, enabled, sample_history,
        sample_count, state + 0x938, (int16_t)(flags & 0x80U),
        &pitch_flag, state + 0x16b8);
    vocoder_update_complex_spectral_analysis(
        encoder_state, enabled, sample_history, scratch,
        sample_count, state + 0x938);
    vocoder_update_spectral_classification(
        state + 0x1370, (const int32_t *)(const void *)encoder_state,
        load_i16(state, 0x624), flags, enabled,
        load_i16(state, 0xab8));
}

int32_t vocoder_encode_frame_portable(
    int16_t *bit_output, int32_t configuration_index,
    const int16_t *samples, int32_t sample_count,
    uint32_t flags, int16_t enabled, int16_t parameter,
    void *state)
{
    if (sample_count < 0x4c) {
        sample_count = 0x4c;
    }
    if (sample_count > 0x54) {
        sample_count = 0x54;
    }
    vocoder_run_encoder_analysis(
        state, samples, sample_count, flags, enabled);
    if ((enabled & 1) == 0) {
        return 0;
    }
    int16_t bias = (int16_t)(flags & 0x2000U);
    if ((flags & 0x2000U) != 0U) {
        uint8_t *bytes = (uint8_t *)state;
        int16_t metric = vocoder_update_encoder_metric(
            (const vocoder_encoder_metric_state *)(const void *)(bytes + 0x938),
            (const vocoder_encoder_aux_state *)(const void *)(bytes + 0x63a),
            (vocoder_encoder_metric_parameters *)(void *)(bytes + 0x16a2));
        bias = (int16_t)(
            ((uint32_t)(int32_t)metric * 0x550bU & 0x3fffffffU) >> 14);
    }
    return (int16_t)vocoder_encode_frame_controller(
        bit_output, configuration_index, flags, parameter, state, bias);
}

int32_t vocoder_decode_audio_portable(
    int16_t *output, int32_t sample_count,
    int16_t *bit_input, int32_t configuration_index,
    uint32_t flags, int16_t second_half, void *state)
{
    if (sample_count < 0x4c) {
        sample_count = 0x4c;
    }
    if (sample_count > 0x54) {
        sample_count = 0x54;
    }
    uint8_t *bytes = (uint8_t *)state;
    store_i16(bytes, 0x174, 0x50);
    if (second_half == 0) {
        vocoder_frame_record frame;
        memset(&frame, 0, sizeof(frame));
        uint32_t status = vocoder_decode_frame_record(
            bit_input, configuration_index, flags,
            (const void *)(uintptr_t)(uint32_t)sample_count,
            (vocoder_frame_record *)(void *)(bytes + 0x3ec),
            &frame, state);
        vocoder_synthesize_audio_frame(
            &frame, output, sample_count, 0, state);
        return (int16_t)status;
    }
    vocoder_synthesize_audio_frame(
        (vocoder_frame_record *)(void *)(bytes + 0x3ec),
        output, sample_count, second_half, state);
    return 0;
}
