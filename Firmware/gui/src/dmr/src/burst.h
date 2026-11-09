#ifndef DMR_BURST_H
#define DMR_BURST_H

#include <stdint.h>
#include <stddef.h>

enum {
    DMR_BURST_DIBIT_COUNT = 132,
    DMR_BURST_BIT_COUNT = 264,
    DMR_BURST_BYTE_COUNT = 33
};

typedef enum {
    DMR_SYNC_BS_VOICE,
    DMR_SYNC_MS_VOICE,
    DMR_SYNC_BS_DATA,
    DMR_SYNC_MS_DATA,
    DMR_SYNC_KIND_COUNT
} dmr_sync_kind_t;

typedef struct {
    /* On-air order, four dibits per byte, first dibit in bits 7..6. */
    uint8_t data[DMR_BURST_BYTE_COUNT];
    /* Sampling point of the first dibit in the filtered 19.2 kHz stream.
     * Index zero is the first discriminator output after RX reset. */
    uint64_t first_sample_index;
    /* Relative TDMA phase: 1/2 when tracking is enabled, otherwise zero. */
    unsigned int timeslot;
    /* False for a burst extracted from the acquired TDMA grid. */
    int has_sync;
    dmr_sync_kind_t sync_kind;
    float cfo_hz;
    float deviation_scale;
    float sync_score;
    /* Burst-local timing slope selected on held-out payload symbols. */
    float sample_rate_error_ppm;
} dmr_burst_t;

/* Packed burst helpers. Index zero is the first on-air dibit. The get
 * function returns UINT8_MAX for an invalid argument/index. */
uint8_t dmr_burst_get_dibit(
    const uint8_t data[DMR_BURST_BYTE_COUNT], size_t index);
int dmr_burst_set_dibit(
    uint8_t data[DMR_BURST_BYTE_COUNT], size_t index, uint8_t dibit);
uint8_t dmr_burst_get_bit(
    const uint8_t data[DMR_BURST_BYTE_COUNT], size_t index);
int dmr_burst_set_bit(
    uint8_t data[DMR_BURST_BYTE_COUNT], size_t index, uint8_t bit);
int dmr_burst_pack_dibits(
    const uint8_t dibits[DMR_BURST_DIBIT_COUNT],
    uint8_t data[DMR_BURST_BYTE_COUNT]);
int dmr_burst_unpack_dibits(
    const uint8_t data[DMR_BURST_BYTE_COUNT],
    uint8_t dibits[DMR_BURST_DIBIT_COUNT]);

#endif
