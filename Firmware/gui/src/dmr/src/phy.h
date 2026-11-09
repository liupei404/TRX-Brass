#ifndef DMR_PHY_H
#define DMR_PHY_H

#include <dmr/burst.h>

#include <complex.h>
#include <stddef.h>

typedef struct dmr_phy_rx dmr_phy_rx_t;
typedef struct dmr_phy_tx dmr_phy_tx_t;

enum {
    DMR_PHY_SAMPLES_PER_SYMBOL = 4,
    DMR_PHY_SLOT_SAMPLES = 576
};

typedef void (*dmr_phy_rx_burst_cb)(void *context,
                                    const dmr_burst_t *burst);

typedef struct {
    dmr_phy_rx_burst_cb burst_callback;
    void *callback_context;
    /* Signed normalized correlation in [0, 1]. Zero selects the default. */
    float sync_threshold;
    /* RRC group delay in symbols. 3/4/5 select 25/33/41 taps; zero selects
     * the default of 4. Other values are currently rejected by create(). */
    unsigned int matched_filter_delay_symbols;
    /* After a sync-bearing burst, extract good-quality normal bursts every
     * 30 ms and label the two relative TDMA phases as timeslots 1 and 2. */
    int track_timeslots;
} dmr_phy_rx_config_t;

dmr_phy_rx_t *dmr_phy_rx_create(const dmr_phy_rx_config_t *config);
void dmr_phy_rx_destroy(dmr_phy_rx_t *rx);
void dmr_phy_rx_reset(dmr_phy_rx_t *rx);

void dmr_phy_rx_process(dmr_phy_rx_t *rx,
                        const float complex *samples,
                        size_t count);

typedef struct {
    /* Constant IQ magnitude. Zero selects the default of 1.0. */
    float amplitude;
} dmr_phy_tx_config_t;

dmr_phy_tx_t *dmr_phy_tx_create(const dmr_phy_tx_config_t *config);
void dmr_phy_tx_destroy(dmr_phy_tx_t *tx);
void dmr_phy_tx_reset(dmr_phy_tx_t *tx);

/* Overwrites the central 24 dibits with the selected Tier II sync.
 * Other payload dibits are unchanged. Returns zero for an invalid kind. */
int dmr_phy_tx_insert_sync(
    uint8_t data[DMR_BURST_BYTE_COUNT],
    dmr_sync_kind_t kind);

/* Generates four IQ samples per consumed dibit. At most capacity / 4 dibits
 * are consumed; the return value is the number of IQ samples written.
 * Continuous calls are phase- and filter-continuous. Invalid dibits or NULL
 * pointers with non-zero counts return SIZE_MAX without changing state. */
size_t dmr_phy_tx_process(dmr_phy_tx_t *tx,
                          const uint8_t *dibits,
                          size_t dibit_count,
                          float complex *samples,
                          size_t capacity);

/* Generates one independent 30 ms physical slot: a 6-symbol ramp-up,
 * all 132 input dibits unchanged, and a 6-symbol ramp-down. The output starts
 * and ends at zero magnitude. This resets and changes tx state; do not
 * interleave it with a continuous dmr_phy_tx_process() stream. Returns
 * DMR_PHY_SLOT_SAMPLES, or SIZE_MAX for invalid arguments/dibits. */
size_t dmr_phy_tx_generate_slot(
    dmr_phy_tx_t *tx,
    const uint8_t data[DMR_BURST_BYTE_COUNT],
    float complex samples[DMR_PHY_SLOT_SAMPLES]);

#endif
