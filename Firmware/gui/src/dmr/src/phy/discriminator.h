#ifndef DMR_PHY_DISCRIMINATOR_H
#define DMR_PHY_DISCRIMINATOR_H

#include <complex.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    float complex previous;
    bool has_previous;
} dmr_discriminator_t;

void dmr_discriminator_reset(dmr_discriminator_t *discriminator);

/*
 * Produces instantaneous frequency in hertz. The first valid IQ sample after
 * reset establishes phase history and produces no output. A non-finite or
 * zero sample breaks phase continuity and is otherwise ignored.
 */
size_t dmr_discriminator_process(dmr_discriminator_t *discriminator,
                                 const float complex *samples,
                                 size_t count,
                                 float *frequency_hz);

#endif
