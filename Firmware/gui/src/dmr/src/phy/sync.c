#include "phy/sync.h"

#include <dmr/symbol.h>

#include <math.h>
#include <stdint.h>

/* ETSI TS 102 361-1, table 9.2. Bits are transmitted most-significant first. */
#define DMR_BS_VOICE_SYNC UINT64_C(0x755FD7DF75F7)
#define DMR_MS_VOICE_SYNC UINT64_C(0x7F7D5DD57DFD)
#define DMR_BS_DATA_SYNC  UINT64_C(0xDFF57D75DF5D)
#define DMR_MS_DATA_SYNC  UINT64_C(0xD5D7F77FD757)

bool dmr_sync_word(dmr_sync_kind_t kind, uint64_t *word)
{
    if (word == NULL)
        return false;
    if (kind == DMR_SYNC_MS_VOICE)
        *word = DMR_MS_VOICE_SYNC;
    else if (kind == DMR_SYNC_BS_VOICE)
        *word = DMR_BS_VOICE_SYNC;
    else if (kind == DMR_SYNC_MS_DATA)
        *word = DMR_MS_DATA_SYNC;
    else if (kind == DMR_SYNC_BS_DATA)
        *word = DMR_BS_DATA_SYNC;
    else
        return false;
    return true;
}

void dmr_sync_expected_levels(dmr_sync_kind_t kind,
                              float levels[DMR_SYNC_SYMBOL_COUNT])
{
    uint64_t word = 0;
    (void)dmr_sync_word(kind, &word);

    for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i) {
        const unsigned int shift = 46u - 2u * (unsigned int)i;
        levels[i] = dmr_dibit_to_level((uint8_t)(word >> shift));
    }
}

bool dmr_sync_correlate(const float observed[DMR_SYNC_SYMBOL_COUNT],
                        dmr_sync_kind_t kind,
                        dmr_sync_fit_t *fit)
{
    float expected[DMR_SYNC_SYMBOL_COUNT];
    float mean_expected = 0.0f;
    float mean_observed = 0.0f;

    dmr_sync_expected_levels(kind, expected);
    for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i) {
        if (!isfinite(observed[i]))
            return false;
        mean_expected += expected[i];
        mean_observed += observed[i];
    }
    mean_expected /= DMR_SYNC_SYMBOL_COUNT;
    mean_observed /= DMR_SYNC_SYMBOL_COUNT;

    float covariance = 0.0f;
    float expected_energy = 0.0f;
    float observed_energy = 0.0f;
    for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i) {
        const float x = expected[i] - mean_expected;
        const float y = observed[i] - mean_observed;
        covariance += x * y;
        expected_energy += x * x;
        observed_energy += y * y;
    }
    if (!(expected_energy > 0.0f) || !(observed_energy > 0.0f))
        return false;

    fit->gain_hz_per_level = covariance / expected_energy;
    fit->offset_hz = mean_observed - fit->gain_hz_per_level * mean_expected;
    fit->score = covariance / sqrtf(expected_energy * observed_energy);

    float squared_error = 0.0f;
    for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i) {
        const float predicted = fit->offset_hz +
                                fit->gain_hz_per_level * expected[i];
        const float error = observed[i] - predicted;
        squared_error += error * error;
    }
    fit->residual_rms_hz = sqrtf(squared_error / DMR_SYNC_SYMBOL_COUNT);

    return fit->gain_hz_per_level > 0.0f;
}
