#ifndef DMR_PHY_SYNC_H
#define DMR_PHY_SYNC_H

#include <dmr/burst.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { DMR_SYNC_SYMBOL_COUNT = 24 };

bool dmr_sync_word(dmr_sync_kind_t kind, uint64_t *word);

typedef struct {
    float offset_hz;
    float gain_hz_per_level;
    float residual_rms_hz;
    float score;
} dmr_sync_fit_t;

void dmr_sync_expected_levels(dmr_sync_kind_t kind,
                              float levels[DMR_SYNC_SYMBOL_COUNT]);

/* Fits observed = offset + gain * expected. A valid voice sync has positive
 * gain; score is the signed normalized correlation in [-1, +1]. */
bool dmr_sync_correlate(const float observed[DMR_SYNC_SYMBOL_COUNT],
                        dmr_sync_kind_t kind,
                        dmr_sync_fit_t *fit);

#endif
