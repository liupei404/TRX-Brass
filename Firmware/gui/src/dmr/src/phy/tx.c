#include <dmr/phy.h>
#include <dmr/symbol.h>

#include "phy/sync.h"

#include <liquid/liquid.h>

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DMR_TX_SAMPLES_PER_SYMBOL 4u
#define DMR_TX_FILTER_DELAY_SYMBOLS 4u
#define DMR_TX_FILTER_TAPS \
    (2u * DMR_TX_SAMPLES_PER_SYMBOL * DMR_TX_FILTER_DELAY_SYMBOLS + 1u)
#define DMR_TX_RRC_ROLLOFF 0.2f
#define DMR_TX_LEVEL_STEP_HZ 648.0f
#define DMR_TX_SAMPLE_RATE_HZ 19200.0f
#define DMR_TX_TWO_PI 6.28318530717958647692f

enum {
    DMR_TX_SLOT_SETTLE_SYMBOLS = 4,
    DMR_TX_SLOT_EDGE_SYMBOLS = 6,
    DMR_TX_SLOT_EDGE_SAMPLES =
        DMR_PHY_SAMPLES_PER_SYMBOL * DMR_TX_SLOT_EDGE_SYMBOLS,
    DMR_TX_SLOT_GENERATED_SYMBOLS =
        DMR_TX_SLOT_SETTLE_SYMBOLS + DMR_TX_SLOT_EDGE_SYMBOLS +
        DMR_BURST_DIBIT_COUNT + DMR_TX_SLOT_EDGE_SYMBOLS +
        DMR_TX_SLOT_SETTLE_SYMBOLS,
    DMR_TX_SLOT_GENERATED_SAMPLES =
        DMR_PHY_SAMPLES_PER_SYMBOL * DMR_TX_SLOT_GENERATED_SYMBOLS
};

struct dmr_phy_tx {
    firfilt_rrrf filter;
    float phase;
    float amplitude;
};

dmr_phy_tx_t *dmr_phy_tx_create(const dmr_phy_tx_config_t *config)
{
    const float amplitude = config == NULL || config->amplitude == 0.0f
        ? 1.0f : config->amplitude;
    if (!isfinite(amplitude) || amplitude <= 0.0f)
        return NULL;

    dmr_phy_tx_t *tx = calloc(1, sizeof(*tx));
    if (tx == NULL)
        return NULL;

    float taps[DMR_TX_FILTER_TAPS];
    liquid_firdes_prototype(LIQUID_FIRFILT_RRC, DMR_TX_SAMPLES_PER_SYMBOL,
                            DMR_TX_FILTER_DELAY_SYMBOLS,
                            DMR_TX_RRC_ROLLOFF, 0.0f, taps);
    tx->filter = firfilt_rrrf_create(taps, DMR_TX_FILTER_TAPS);
    if (tx->filter == NULL) {
        free(tx);
        return NULL;
    }
    tx->amplitude = amplitude;
    return tx;
}

void dmr_phy_tx_destroy(dmr_phy_tx_t *tx)
{
    if (tx == NULL)
        return;
    firfilt_rrrf_destroy(tx->filter);
    free(tx);
}

void dmr_phy_tx_reset(dmr_phy_tx_t *tx)
{
    if (tx == NULL)
        return;
    firfilt_rrrf_reset(tx->filter);
    tx->phase = 0.0f;
}

int dmr_phy_tx_insert_sync(
    uint8_t data[DMR_BURST_BYTE_COUNT],
    dmr_sync_kind_t kind)
{
    uint64_t word = 0;
    if (data == NULL || !dmr_sync_word(kind, &word))
        return 0;
    for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i) {
        const unsigned int shift = 46u - 2u * (unsigned int)i;
        dmr_burst_set_dibit(data, 54u + i,
                            (uint8_t)((word >> shift) & 3u));
    }
    return 1;
}

size_t dmr_phy_tx_process(dmr_phy_tx_t *tx,
                          const uint8_t *dibits,
                          size_t dibit_count,
                          float complex *samples,
                          size_t capacity)
{
    if (tx == NULL || (dibit_count != 0u && dibits == NULL) ||
        (capacity != 0u && samples == NULL))
        return SIZE_MAX;

    size_t consumed = dibit_count;
    if (consumed > capacity / DMR_TX_SAMPLES_PER_SYMBOL)
        consumed = capacity / DMR_TX_SAMPLES_PER_SYMBOL;
    for (size_t i = 0; i < consumed; ++i) {
        if (dibits[i] > 3u)
            return SIZE_MAX;
    }

    size_t produced = 0;
    for (size_t i = 0; i < consumed; ++i) {
        for (size_t sample = 0; sample < DMR_TX_SAMPLES_PER_SYMBOL; ++sample) {
            const float impulse = sample == 0u
                ? dmr_dibit_to_level(dibits[i]) : 0.0f;
            float shaped = 0.0f;
            firfilt_rrrf_execute_one(tx->filter, impulse, &shaped);
            tx->phase += DMR_TX_TWO_PI * DMR_TX_LEVEL_STEP_HZ * shaped /
                         DMR_TX_SAMPLE_RATE_HZ;
            if (tx->phase > DMR_TX_TWO_PI || tx->phase < -DMR_TX_TWO_PI)
                tx->phase = remainderf(tx->phase, DMR_TX_TWO_PI);
            samples[produced++] = tx->amplitude * cexpf(I * tx->phase);
        }
    }
    return produced;
}

size_t dmr_phy_tx_generate_slot(
    dmr_phy_tx_t *tx,
    const uint8_t data[DMR_BURST_BYTE_COUNT],
    float complex samples[DMR_PHY_SLOT_SAMPLES])
{
    if (tx == NULL || data == NULL || samples == NULL)
        return SIZE_MAX;

    uint8_t dibits[DMR_BURST_DIBIT_COUNT];
    if (!dmr_burst_unpack_dibits(data, dibits))
        return SIZE_MAX;
    uint8_t symbols[DMR_TX_SLOT_GENERATED_SYMBOLS];
    float complex generated[DMR_TX_SLOT_GENERATED_SAMPLES];
    const size_t prefix = DMR_TX_SLOT_SETTLE_SYMBOLS +
                          DMR_TX_SLOT_EDGE_SYMBOLS;
    memset(symbols, dibits[0], prefix);
    memcpy(symbols + prefix, dibits, DMR_BURST_DIBIT_COUNT);
    memset(symbols + prefix + DMR_BURST_DIBIT_COUNT,
           dibits[DMR_BURST_DIBIT_COUNT - 1u],
           DMR_TX_SLOT_EDGE_SYMBOLS + DMR_TX_SLOT_SETTLE_SYMBOLS);

    dmr_phy_tx_reset(tx);
    if (dmr_phy_tx_process(tx, symbols, DMR_TX_SLOT_GENERATED_SYMBOLS,
                           generated, DMR_TX_SLOT_GENERATED_SAMPLES) !=
        DMR_TX_SLOT_GENERATED_SAMPLES)
        return SIZE_MAX;

    /* Skip both the explicit initial settling input and the causal RRC group
     * delay. This places all 132 dibits between the two amplitude ramps. */
    memcpy(samples,
           generated + DMR_PHY_SAMPLES_PER_SYMBOL *
                       (DMR_TX_SLOT_SETTLE_SYMBOLS +
                        DMR_TX_FILTER_DELAY_SYMBOLS),
           DMR_PHY_SLOT_SAMPLES * sizeof(*samples));
    for (size_t i = 0; i < DMR_TX_SLOT_EDGE_SAMPLES; ++i) {
        const float gain = (float)i /
                           (float)(DMR_TX_SLOT_EDGE_SAMPLES - 1u);
        samples[i] *= gain;
        samples[DMR_PHY_SLOT_SAMPLES - 1u - i] *= gain;
    }
    return DMR_PHY_SLOT_SAMPLES;
}
