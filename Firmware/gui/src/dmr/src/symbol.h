#ifndef DMR_SYMBOL_H
#define DMR_SYMBOL_H

#include <stdint.h>

/* Dibits are stored numerically as (b1 << 1) | b0. */
float dmr_dibit_to_level(uint8_t dibit);

#endif
