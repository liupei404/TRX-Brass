#ifndef DMR_PHY_MATCHED_FILTER_H
#define DMR_PHY_MATCHED_FILTER_H

#include <liquid/liquid.h>

#include <stdbool.h>
#include <stddef.h>

enum {
    DMR_MATCHED_FILTER_SAMPLES_PER_SYMBOL = 4,
    DMR_MATCHED_FILTER_DELAY_SYMBOLS = 4,
    DMR_MATCHED_FILTER_TAP_COUNT = 33,
    DMR_MATCHED_FILTER_GROUP_DELAY_SAMPLES = 16
};

typedef struct {
    firfilt_rrrf fir;
} dmr_matched_filter_t;

bool dmr_matched_filter_init(dmr_matched_filter_t *filter);
bool dmr_matched_filter_init_with_delay(dmr_matched_filter_t *filter,
                                        unsigned int delay_symbols);
void dmr_matched_filter_deinit(dmr_matched_filter_t *filter);
void dmr_matched_filter_reset(dmr_matched_filter_t *filter);

/* Input and output may point to the same buffer. */
void dmr_matched_filter_process(dmr_matched_filter_t *filter,
                                const float *input,
                                size_t count,
                                float *output);

#endif
