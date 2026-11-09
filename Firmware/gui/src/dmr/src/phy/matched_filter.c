#include "phy/matched_filter.h"

#define DMR_RRC_ROLLOFF 0.2f

bool dmr_matched_filter_init(dmr_matched_filter_t *filter)
{
    return dmr_matched_filter_init_with_delay(
        filter, DMR_MATCHED_FILTER_DELAY_SYMBOLS);
}

bool dmr_matched_filter_init_with_delay(dmr_matched_filter_t *filter,
                                        unsigned int delay_symbols)
{
    if (delay_symbols < 1u || delay_symbols > 16u)
        return false;
    const size_t tap_count = 2u * DMR_MATCHED_FILTER_SAMPLES_PER_SYMBOL *
                             delay_symbols + 1u;
    float taps[tap_count];
    liquid_firdes_prototype(LIQUID_FIRFILT_RRC,
                            DMR_MATCHED_FILTER_SAMPLES_PER_SYMBOL,
                            delay_symbols,
                            DMR_RRC_ROLLOFF, 0.0f, taps);

    /* Preserve discriminator units: a constant frequency offset in hertz
     * must remain the same constant after the matched filter. */
    float dc_gain = 0.0f;
    for (size_t i = 0; i < tap_count; ++i)
        dc_gain += taps[i];
    for (size_t i = 0; i < tap_count; ++i)
        taps[i] /= dc_gain;

    filter->fir = firfilt_rrrf_create(taps, (unsigned int)tap_count);
    return filter->fir != NULL;
}

void dmr_matched_filter_deinit(dmr_matched_filter_t *filter)
{
    if (filter->fir != NULL)
        firfilt_rrrf_destroy(filter->fir);
    filter->fir = NULL;
}

void dmr_matched_filter_reset(dmr_matched_filter_t *filter)
{
    firfilt_rrrf_reset(filter->fir);
}

void dmr_matched_filter_process(dmr_matched_filter_t *filter,
                                const float *input,
                                size_t count,
                                float *output)
{
    for (size_t i = 0; i < count; ++i)
        firfilt_rrrf_execute_one(filter->fir, input[i], &output[i]);
}
