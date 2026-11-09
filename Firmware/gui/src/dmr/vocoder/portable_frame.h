#ifndef PORTABLE_FRAME_H
#define PORTABLE_FRAME_H

#include <stddef.h>
#include <stdint.h>
#include <stddef.h>

enum {
    VOCODER_SPECTRUM_SIZE = 56
};

typedef struct {
    uint16_t header[8];
    int16_t spectrum[VOCODER_SPECTRUM_SIZE];
} vocoder_frame_state;

_Static_assert(sizeof(vocoder_frame_state) == 128,
               "VOCODER frame-state layout must be 128 bytes");

typedef struct {
    vocoder_frame_state state;
    uint16_t metadata[4];
} vocoder_frame_record;

_Static_assert(sizeof(vocoder_frame_record) == 136,
               "VOCODER frame-record layout must be 136 bytes");

typedef struct {
    vocoder_frame_state state;
    uint16_t working[6];
} vocoder_extended_frame;

_Static_assert(sizeof(vocoder_extended_frame) == 140,
               "extended frame layout must be 140 bytes");

typedef struct {
    uint8_t reserved_00[64];
    int32_t previous_sum;
    uint8_t reserved_44[14];
    int16_t values[16];
    int16_t exponents[16];
} vocoder_energy_state;

_Static_assert(offsetof(vocoder_energy_state, previous_sum) == 0x40,
               "energy-state previous_sum offset must be 0x40");
_Static_assert(offsetof(vocoder_energy_state, values) == 0x52,
               "energy-state values offset must be 0x52");
_Static_assert(offsetof(vocoder_energy_state, exponents) == 0x72,
               "energy-state exponents offset must be 0x72");

typedef struct {
    uint16_t header[8];
    int16_t values[56];
    int16_t *active;
} vocoder_harmonic_state;

_Static_assert(offsetof(vocoder_harmonic_state, active) == 0x80,
               "harmonic-state active pointer offset must be 0x80");

typedef struct {
    uint8_t reserved_00[0x4a];
    int16_t reference;
    uint8_t reserved_4c[4];
    int16_t slope;
    uint8_t reserved_52[0xa4];
    int16_t activity;
} vocoder_encoder_metric_state;

_Static_assert(offsetof(vocoder_encoder_metric_state, reference) == 0x4a,
               "encoder metric reference offset must be 0x4a");
_Static_assert(offsetof(vocoder_encoder_metric_state, slope) == 0x50,
               "encoder metric slope offset must be 0x50");
_Static_assert(offsetof(vocoder_encoder_metric_state, activity) == 0xf6,
               "encoder metric activity offset must be 0xf6");

typedef struct {
    uint8_t reserved_00[10];
    int16_t indicator;
} vocoder_encoder_aux_state;

typedef struct {
    int16_t value[11];
} vocoder_encoder_metric_parameters;

typedef struct {
    uint8_t reserved_00[0x80];
    int16_t smoothed_pitch;
} vocoder_spectral_band_state;

typedef struct {
    int16_t bands[10][8];
    int16_t exponents[5];
} vocoder_spectral_history;

_Static_assert(offsetof(vocoder_spectral_history, exponents) == 0xa0,
               "spectral history exponent offset must be 0xa0");
_Static_assert(sizeof(vocoder_spectral_history) == 0xaa,
               "spectral history layout must be 170 bytes");

typedef struct {
    uint8_t reserved_00[4];
    int16_t count;
    uint8_t reserved_06[6];
    int16_t pitch;
    uint8_t reserved_0e[2];
    int16_t samples[56];
} vocoder_harmonic_buffer;

typedef struct {
    uint8_t reserved_00[0x0c];
    int16_t pitch;
    uint8_t reserved_0e[2];
    int16_t spectrum[57];
    int16_t gain;
} vocoder_harmonic_reference;

typedef struct {
    uint8_t reserved_00[0x0a];
    int16_t identifiers[2];
    int16_t values[4];
} vocoder_candidate_pair;

_Static_assert(offsetof(vocoder_harmonic_reference, gain) == 0x82,
               "harmonic reference gain offset must be 0x82");

typedef void (*vocoder_spectral_callback)(
    int32_t selector,
    uintptr_t context,
    vocoder_frame_state *frame,
    uintptr_t argument_3,
    uintptr_t argument_4,
    uintptr_t argument_5,
    uintptr_t argument_6,
    int32_t slice_index,
    int32_t spectrum_index,
    int32_t mode);

void vocoder_filter_encoder_band_history(
    int16_t history[41], int16_t first[40], int16_t second[40],
    int16_t input);
void vocoder_update_encoder_spectral_callback(
    int32_t selector, uintptr_t context,
    vocoder_frame_state *frame, uintptr_t analysis,
    uintptr_t spectral_state, uintptr_t exponent_table,
    uintptr_t sample_table, int32_t slice_index,
    int32_t spectrum_index, int32_t mode);

typedef struct {
    const int32_t *spectra;
    const int16_t *exponents;
    uintptr_t callback_argument_5;
    uintptr_t callback_argument_6;
    int16_t callback_selector;
    int16_t callback_enabled;
    uintptr_t callback_argument_4;
    uintptr_t callback_context;
    vocoder_spectral_callback callback;
    int16_t logarithm_bias;
    int16_t callback_mode;
    uintptr_t callback_argument_3;
} vocoder_spectral_config;

typedef struct {
    const int16_t *band_values;
    const int16_t *band_basis;
    const int16_t *candidates;
    const int16_t *scores;
    int16_t target;
    uint8_t reserved_12[22];
    const int16_t *quantized_inputs;
    const int16_t *quantized_exponents;
    uint16_t band_mask;
    uint8_t reserved_32[18];
    int16_t left_neighbor;
    int16_t right_neighbor;
} vocoder_band_candidate_config;

void vocoder_copy_i16(
    int16_t *destination,
    const int16_t *source,
    int32_t count);
int16_t *vocoder_fill_i16(
    int16_t *destination,
    int32_t value,
    int32_t count);
void vocoder_expand_log_spectrum(
    int16_t spectrum[56],
    int32_t *exponent,
    int32_t harmonic_count);
void vocoder_multiply_symmetric(
    int16_t *output,
    const int16_t *right,
    int32_t output_count,
    int32_t center_output,
    const int16_t *left,
    int32_t active_count);
void vocoder_shift_i16(
    int16_t *output,
    const int16_t *input,
    int32_t count,
    int32_t shift);
void vocoder_copy_frame_record(
    vocoder_frame_record *destination,
    const vocoder_frame_record *source);
void vocoder_prepare_extended_frame(
    vocoder_extended_frame *destination,
    const vocoder_frame_state *source,
    int32_t tag);
void vocoder_expand_voicing_bands(
    int16_t output[128],
    const int16_t thresholds[8],
    const int16_t values[8],
    int32_t exponent);
void vocoder_generate_voicing_flags(vocoder_harmonic_state *state);
int32_t vocoder_track_sync_word(
    uint16_t *symbol,
    uint16_t history[18],
    int32_t sync_mode);
void vocoder_cosine_transform(
    int16_t *values,
    int32_t output_count,
    int32_t input_count);
void vocoder_inverse_cosine_transform(
    int16_t *values,
    int32_t transform_count,
    int32_t output_count);
void vocoder_compute_band_weights(
    int16_t output[8],
    int16_t lower_bounds[8],
    const int16_t *band_values,
    const int16_t *band_basis,
    uint32_t band_mask,
    int32_t band,
    int32_t neighbor_distance,
    uint32_t *shape_flag);
void vocoder_filter_log_spectrum(
    int16_t output[56],
    const int16_t *input,
    int32_t count);
void vocoder_populate_spectral_frame(
    vocoder_frame_state *frame,
    vocoder_spectral_band_state *state,
    int32_t slice_index,
    int32_t spectrum_index,
    const vocoder_spectral_config *config,
    const int16_t *copy_source);
int32_t vocoder_vector_normalization_shift(
    const int16_t *values,
    int32_t count);
void vocoder_normalize_i16_vector(
    int16_t *output,
    const int16_t *input,
    int32_t count,
    int32_t *exponent);
int32_t vocoder_multiply_complex_by_real(
    int16_t *output,
    const uint32_t *complex_input,
    int32_t input_exponent,
    const int16_t *real_input,
    int32_t real_exponent,
    int32_t count);
int32_t vocoder_multiply_complex_vector(
    uint32_t *output,
    const uint32_t *coefficient,
    int32_t coefficient_exponent,
    const uint32_t *input,
    int32_t input_exponent,
    int32_t count);
int32_t vocoder_fourth_root_vector(
    int16_t *output,
    const uint32_t *input,
    int32_t input_exponent,
    int32_t count);
int32_t vocoder_adjacent_complex_products(
    uint32_t values[129],
    int32_t input_exponent);
int32_t vocoder_correlate_window_40(
    int16_t output[40],
    int16_t values[80],
    int32_t input_exponent,
    const int16_t window[80]);
int32_t vocoder_sum_i16(
    const int16_t *values,
    int32_t count);
int16_t vocoder_max_i16(
    const int16_t *values,
    int32_t count);
int32_t vocoder_compare_scaled(
    int16_t left,
    int32_t left_exponent,
    int16_t right,
    int32_t right_exponent);
int32_t vocoder_sum_i32_normalized(
    const int32_t *values,
    int32_t count,
    int32_t *exponent);
int32_t vocoder_vector_energy(
    int32_t *exponent,
    const int16_t *values,
    uint32_t count);
int16_t vocoder_align_by_exponents(
    int16_t *output,
    const int16_t *values,
    const int16_t *exponents,
    int32_t count);
int16_t vocoder_complex_vector_energy(
    int16_t *exponent,
    const uint32_t *values,
    int16_t input_exponent,
    int32_t complex_count);
int16_t vocoder_ratio_of_i32_sums(
    const int32_t numerator_values[128],
    int32_t numerator_exponent,
    const int32_t denominator_values[128],
    int32_t denominator_exponent,
    int32_t *result_exponent);
int32_t vocoder_align_two_i32_sums(
    int16_t *left,
    int16_t *right,
    const int32_t *values,
    int32_t input_exponent,
    int32_t left_index,
    int32_t right_index);
int32_t vocoder_sum_adaptive_intervals(
    const int16_t *values,
    int16_t step_mantissa,
    int32_t step_exponent,
    int32_t total_count);
int16_t vocoder_tail_sum_complement(
    int16_t numerator,
    int32_t numerator_exponent,
    const int16_t values[32],
    int16_t start_mantissa,
    int32_t start_exponent);
int16_t vocoder_atan_scaled(
    int16_t input,
    int32_t input_exponent,
    int32_t *result_exponent);
int16_t vocoder_atan2_scaled(
    int16_t numerator,
    int32_t numerator_exponent,
    int16_t denominator,
    int32_t denominator_exponent,
    int32_t *result_exponent);
int16_t vocoder_analyze_spectral_blocks(
    vocoder_spectral_history *state,
    int16_t pitch,
    const int16_t spectra[8][32],
    const int32_t exponents[8]);
void vocoder_adjust_harmonic_samples(
    vocoder_harmonic_buffer *buffer,
    vocoder_harmonic_reference *reference,
    int32_t frame_class,
    int32_t mode);
int16_t vocoder_get_second_i16(const int16_t values[2]);
void vocoder_store_u32(uint32_t *destination, uint32_t value);
void vocoder_increment_counter_100(void *state);
int32_t vocoder_test_state_flag(void *state, int32_t index);
void vocoder_scale_i16_vector(
    int16_t *output,
    const int16_t *input,
    int32_t count,
    int16_t multiplier);
void vocoder_complex_magnitude_squared(
    int32_t *output,
    const uint32_t *input,
    int32_t count);
int16_t *vocoder_write_bits_msb(
    int32_t value,
    int16_t *output,
    int32_t count);
uint32_t vocoder_read_bits_msb(
    const int16_t *input,
    int32_t count);
int32_t vocoder_argmax_i16(
    const int16_t *values,
    int32_t count);
int32_t vocoder_argmax_i32(
    const int32_t *values,
    int32_t count);
int32_t vocoder_compare_scaled_less(
    int16_t left,
    int32_t left_exponent,
    int16_t right,
    int32_t right_exponent);
int32_t vocoder_complex_peak_normalization_shift(
    const uint32_t *values,
    int32_t count);
void vocoder_write_bits_cursor(
    int16_t **cursor,
    int32_t count,
    int32_t value);
int32_t vocoder_i32_maximum_magnitude_shift(
    const int32_t *values,
    int32_t count);
void vocoder_shift_i16_between_exponents(
    int16_t *output,
    const int16_t *input,
    int32_t count,
    int32_t output_exponent,
    int32_t input_exponent);
void vocoder_multiply_i16_between_exponents(
    int16_t *output,
    const int16_t *input,
    int32_t count,
    int32_t multiplier,
    int32_t product_exponent,
    int32_t output_exponent);
int16_t vocoder_divide_i32_i16_q14(
    int32_t numerator,
    int16_t denominator);
void vocoder_spread_two_bit_fields(
    uint32_t *low,
    uint32_t *high,
    uint32_t packed);
uint32_t vocoder_select_voicing_codeword(
    int32_t mode,
    uint32_t *low_fields,
    uint32_t *high_fields,
    const int16_t left[16],
    const int16_t right[16],
    const int16_t weights[16],
    int32_t code_bits);
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
    int16_t pitch_bias);
void vocoder_reorder_spectral_blocks(
    int16_t *values,
    int32_t inverse,
    const int16_t configuration[13]);
void vocoder_quantize_residual_spectrum(
    int16_t *reference,
    int16_t *frame,
    int16_t average,
    const int16_t *configuration,
    int16_t **cursor,
    int32_t encode);
void vocoder_quantize_spectral_frame(
    void *reference,
    vocoder_frame_state *frame,
    const int16_t configuration[13],
    int16_t **cursor,
    int32_t encode);
void vocoder_encode_and_prepare_frame(
    int16_t *spectral_values,
    int16_t **cursor,
    vocoder_frame_state *frame,
    vocoder_extended_frame *destination,
    int32_t mode,
    const int16_t configuration[13]);
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
    int16_t pitch_bias);
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
    int16_t pitch_bias);
uint32_t vocoder_decode_frame_record(
    int16_t *input,
    int32_t configuration_index,
    uint32_t flags,
    const void *decoder_state,
    vocoder_frame_record *frame,
    vocoder_frame_record *output,
    void *state);
uint32_t vocoder_encode_frame_controller(
    int16_t *spectral_values,
    int32_t configuration_index,
    uint32_t flags,
    int16_t parameter,
    void *state,
    int16_t bias);
void vocoder_saturating_shift_i16(
    int16_t *output,
    const int16_t *input,
    int32_t count,
    int32_t output_exponent,
    int32_t input_exponent);
uint32_t vocoder_read_bits_cursor(
    const int16_t **cursor,
    int32_t count);
int32_t vocoder_refine_table_index_q16(
    int32_t index,
    const int16_t values[32]);
int16_t vocoder_smooth_if_mask_majority(
    int16_t current,
    int16_t previous,
    uint32_t mask);
int32_t vocoder_select_candidate(
    int32_t *identifier,
    int16_t output[2],
    const vocoder_candidate_pair *candidates);
int32_t vocoder_sum_i32_shifted(
    const int32_t *values,
    int32_t count,
    int16_t shift);
void vocoder_select_maximum_five_sum(
    int32_t *maximum,
    int32_t *selected_index,
    const int32_t *values,
    const uint8_t offsets[4]);
void vocoder_complex_butterfly_forward(
    uint32_t *forward,
    uint32_t *backward,
    int32_t count);
void vocoder_complex_butterfly_inverse(
    uint32_t *forward,
    uint32_t *backward,
    int32_t count);
void vocoder_initialize_partition_config(
    int16_t config[13],
    int32_t enabled,
    int32_t mode);
void vocoder_spread_spectral_peak(
    int16_t spectrum[56],
    int16_t center,
    int32_t value);
void vocoder_convert_i32_to_i16_rounded(
    int16_t *output,
    int32_t input_exponent,
    const int32_t *input,
    int32_t output_exponent,
    int32_t count);
void vocoder_normalize_i32_to_i16(
    int16_t *output,
    const int32_t *input,
    int32_t count,
    int32_t *exponent);
int16_t vocoder_lookup_partition_value(
    uint32_t kind,
    int32_t index,
    int32_t selector);
int64_t vocoder_arithmetic_shift_right_i64(
    int64_t value,
    uint8_t shift);
uint64_t vocoder_shift_left_u64(
    uint64_t value,
    uint8_t shift);
int16_t vocoder_squared_vector_difference(
    int16_t *result_exponent,
    const int16_t *left,
    int32_t left_exponent,
    const int16_t *right,
    int16_t right_exponent,
    int16_t packed_count);
int32_t vocoder_complex_weighted_correlation(
    int16_t output[2],
    const int16_t *weights,
    const uint32_t *input,
    int32_t input_exponent,
    const uint32_t *reference,
    int32_t reference_exponent,
    int32_t count);
int32_t vocoder_test_weighted_spectral_threshold(
    const int16_t *values,
    int32_t value_exponent,
    int32_t scale,
    int32_t target_exponent,
    int16_t multiplier,
    int32_t bias,
    int32_t threshold_multiplier);
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
    int16_t threshold_multiplier);
void vocoder_write_repeated_frame_code(
    int32_t code,
    const int16_t levels[2],
    const void *configuration,
    int16_t **cursor);
void *vocoder_memset_bytes(
    void *destination,
    size_t count,
    int32_t value);
void *vocoder_fill_bytes(
    void *destination,
    int32_t value,
    size_t count);
void vocoder_decode_spread_codeword(
    int32_t enabled,
    uint32_t *low,
    uint32_t *high,
    int32_t value,
    int32_t bit_count);
void vocoder_apply_complex_swap_table(
    int16_t *values,
    int32_t size,
    int32_t unused,
    const int16_t *table);
void vocoder_radix4_twiddle_butterflies(
    uint32_t *left,
    uint32_t *right,
    int32_t twiddle_step,
    int32_t count);
void vocoder_pairwise_complex_butterflies(
    uint32_t *values,
    int32_t packed_count,
    int32_t scaling_mode);
int32_t vocoder_transform_half_spectrum_stage(
    int16_t *values,
    int32_t exponent,
    int32_t log2_size,
    int32_t inverse,
    int32_t half_scale_mode);
void vocoder_radix4_matrix_stage(
    uint32_t *left,
    uint32_t *right,
    int32_t twiddle_step,
    int32_t group_size,
    int32_t group_count,
    int32_t scaling_mode);
int32_t vocoder_complex_fft_32(
    uint32_t values[32],
    int32_t exponent,
    int32_t log2_size);
int32_t vocoder_real_fft_forward_64(
    int16_t values[66],
    int32_t exponent,
    int32_t log2_size,
    int32_t half_scale_mode);
int32_t vocoder_real_fft_inverse_64(
    int16_t values[66],
    int32_t exponent,
    int32_t log2_size,
    int32_t half_scale_mode);
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
    int32_t half_scale_mode);
int32_t vocoder_normalized_windowed_fft_energy(
    int32_t *energy,
    const int16_t *input,
    const int16_t *left,
    int32_t left_exponent,
    int32_t active_count,
    int32_t center_output,
    int16_t *workspace,
    int32_t log2_size,
    int32_t half_scale_mode);
void vocoder_generate_complex_phasors(
    int16_t *output,
    const int16_t *phase_words,
    uint32_t phase_bits,
    int32_t count);
void vocoder_polyphase_filter_16(
    int16_t *output,
    const int16_t *input,
    int32_t output_count);
void vocoder_filter_i32_state6(
    int32_t *output,
    const int32_t *input,
    int32_t state[6],
    int32_t count);
void vocoder_filter_i16_state6(
    int32_t state[6],
    int16_t *output,
    const int16_t *input,
    int32_t count);
int32_t vocoder_test_weighted_curvature(
    int16_t threshold,
    int32_t value_exponent,
    const int16_t values[5],
    int16_t input_exponent);
void vocoder_sqrt_i32_vector(
    int16_t *output,
    int16_t output_exponent,
    const int32_t *input,
    uint16_t input_exponent,
    int32_t count);
int32_t vocoder_vector_quantize_i16(
    int16_t *values,
    int32_t dimension,
    uint32_t stride_exponent,
    const int16_t *table,
    uint32_t code_bits,
    int32_t stride,
    int16_t **cursor,
    int32_t encode);
void vocoder_quantize_log_average(
    int16_t *output,
    int32_t dimension,
    const int16_t *values,
    int32_t count,
    int16_t *previous,
    uint32_t code_bits,
    int16_t **cursor,
    int32_t encode);
int16_t vocoder_select_spectral_candidate(
    const int16_t candidates[5],
    const int16_t scores[5],
    int32_t target,
    int32_t local_index,
    int32_t fallback_index);
int32_t vocoder_initialize_spectral_frame(
    int16_t frame[64],
    uint32_t code,
    int32_t first_level,
    int32_t second_level,
    int16_t spread);
int32_t vocoder_decode_spectral_frame_block(
    int16_t frame[64],
    vocoder_frame_state *destination,
    const int16_t **cursor,
    const int16_t *config);
int32_t vocoder_log_i32_vector(
    int16_t *output,
    const int32_t *input,
    int32_t input_exponent,
    int32_t count);
void vocoder_transform_32_symmetric(int16_t values[32]);
int32_t vocoder_analyze_subframes(
    void *state,
    int16_t *filtered_output,
    int32_t energy[16],
    int32_t *consumed_samples,
    const int16_t *samples,
    int32_t available_samples,
    int32_t new_samples);
void vocoder_accumulate_interpolated_samples(
    uint32_t *phase,
    const int16_t *samples,
    int32_t phase_step,
    int32_t window_phase,
    int32_t **output_cursor,
    int16_t sample_shift,
    int16_t window_step,
    int16_t count);
void vocoder_integrate_resampled_intervals(
    uint32_t *mantissas,
    int16_t *exponents,
    int32_t spacing,
    int32_t count,
    const int32_t table[128]);
int32_t vocoder_integrate_resampled_to_end(
    uint32_t *mantissas,
    int16_t *exponents,
    int32_t spacing,
    const int32_t table[128]);
void vocoder_compare_resampled_bands(
    int16_t values[8],
    int16_t spacing,
    const int32_t table[128]);
void vocoder_synthesize_interpolated_segment(
    int32_t *output,
    int16_t amplitude,
    int32_t amplitude_exponent,
    uint16_t spacing,
    const int16_t *samples,
    int16_t output_shift);
void vocoder_select_frame_band_candidate(
    vocoder_frame_state *frame,
    int16_t *weights,
    int16_t *lower_bounds,
    int16_t quantized[8],
    int32_t slice_index,
    int32_t spectrum_index,
    const vocoder_band_candidate_config *configuration);
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
    int32_t selected_label);
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
    int32_t count);
void vocoder_synthesize_frame_transition(
    int32_t *output,
    int32_t count,
    void *synthesis_state,
    const vocoder_frame_record *current,
    const vocoder_frame_record *previous,
    int16_t pitch);
void vocoder_synthesize_excitation_frame(
    int32_t *output,
    int32_t count,
    int16_t *overlap_state,
    const vocoder_frame_record *frame,
    int16_t cutoff);
void vocoder_synthesize_audio_frame(
    vocoder_frame_record *frame,
    int16_t *output,
    int32_t count,
    int32_t preserve_pitch,
    void *decoder_state);
int32_t vocoder_classify_spectral_frame(
    int16_t output[2],
    const int32_t values[128],
    int32_t value_exponent,
    int32_t log_bias,
    void *classifier_state,
    int16_t relaxed_threshold);
void vocoder_update_spectral_classification(
    void *classifier_state,
    const int32_t values[128],
    int32_t value_exponent,
    uint32_t flags,
    int32_t slot,
    int16_t relaxed_threshold);
void vocoder_analyze_complex_spectral_ratios(
    int16_t *state,
    uint32_t complex_values[129],
    int32_t complex_exponent,
    const int32_t spectrum[129],
    int32_t spectrum_exponent,
    const int16_t thresholds[8],
    const int16_t voicing_values[8],
    int32_t voicing_exponent);
void vocoder_update_complex_spectral_analysis(
    void *state,
    int32_t enabled,
    const int16_t *sample_history,
    uint32_t scratch[129],
    int32_t sample_count,
    const void *auxiliary_state);
int32_t vocoder_track_pitch_analysis(
    int16_t state[7],
    int16_t history[10],
    const int16_t *correlation,
    const int32_t energy[128],
    const void *encoder_state,
    int32_t enabled,
    int32_t auxiliary_value,
    int16_t *output_flag);
int32_t vocoder_update_spectral_energy_state(
    void *state,
    const int32_t values[16],
    int32_t value_exponent,
    int32_t divisor);
void vocoder_analyze_encoder_frame(
    void *subframe_state,
    void *encoder_state,
    int32_t enabled,
    const int16_t *samples,
    int32_t new_sample_count,
    void *analysis_state,
    int16_t option_flag,
    int16_t *output_flag,
    void *auxiliary_state);
void vocoder_run_encoder_analysis(
    void *state, const int16_t *new_samples,
    int32_t sample_count, uint32_t flags, int16_t enabled);
int32_t vocoder_encode_frame_portable(
    int16_t *bit_output, int32_t configuration_index,
    const int16_t *samples, int32_t sample_count,
    uint32_t flags, int16_t enabled, int16_t parameter,
    void *state);
int32_t vocoder_decode_audio_portable(
    int16_t *output, int32_t sample_count,
    int16_t *bit_input, int32_t configuration_index,
    uint32_t flags, int16_t second_half, void *state);
void vocoder_copy_frame_state(
    vocoder_frame_state *destination,
    const vocoder_frame_state *source);
void vocoder_interpolate_frame_state(
    vocoder_frame_state *destination,
    const vocoder_frame_record *left,
    const vocoder_frame_record *right);
int32_t vocoder_analyze_energy_trend(
    int16_t output[16],
    vocoder_energy_state *state);
void vocoder_apply_adaptive_spectral_attenuation(
    int16_t *samples,
    int32_t *exponent,
    int16_t *smoothed_mantissa,
    int16_t *smoothed_exponent,
    int32_t phase_step,
    int32_t count);
void vocoder_move_active_harmonic(
    vocoder_harmonic_state *state,
    int16_t new_index);
int16_t vocoder_combine_harmonic_indices(
    uint16_t *combined_index,
    vocoder_frame_state *left,
    vocoder_frame_state *right,
    int32_t left_adjustment,
    int32_t right_adjustment,
    uint32_t table_bits,
    int32_t update_indices);
int16_t vocoder_update_encoder_metric(
    const vocoder_encoder_metric_state *state,
    const vocoder_encoder_aux_state *auxiliary,
    vocoder_encoder_metric_parameters *parameters);
void vocoder_extract_log_spectral_bands(
    int16_t *output,
    int32_t spacing,
    int32_t output_count,
    int32_t flags,
    vocoder_spectral_band_state *state,
    int32_t bias,
    const int32_t *spectra,
    const int16_t *exponents,
    int32_t spectrum_index);

#endif
