#ifndef DMR_DATA_H
#define DMR_DATA_H

#include <dmr/burst.h>

#include <stdint.h>

enum {
    DMR_DATA_TYPE_PI_HEADER = 0,
    DMR_DATA_TYPE_VOICE_LC_HEADER = 1,
    DMR_DATA_TYPE_TERMINATOR_LC = 2,
    DMR_DATA_TYPE_CSBK = 3,
    DMR_DATA_TYPE_IDLE = 9
};

/* Encode/decode the 8-bit CC/Data Type field protected by Golay(20,8). */
uint32_t dmr_slot_type_encode(unsigned int color_code,
                              unsigned int data_type);
int dmr_slot_type_decode(uint32_t received, unsigned int *color_code,
                         unsigned int *data_type,
                         unsigned int *corrected_bits);

/* Encode 96 information bits into the interleaved 196-bit BPTC payload.
 * Both buffers are packed MSB-first. */
int dmr_bptc_196_96_encode(const uint8_t information[12],
                           uint8_t encoded[25]);
int dmr_bptc_196_96_decode(const uint8_t encoded[25],
                           uint8_t information[12]);

/* Full LC Voice Header/Terminator helpers. The 9-byte LC is protected by
 * RS(12,9), BPTC and the data-type-specific parity mask. */
int dmr_full_lc_decode(const dmr_burst_t *burst, uint8_t lc[9],
                       unsigned int *color_code, unsigned int *data_type);
int dmr_full_lc_encode(dmr_burst_t *burst, const uint8_t lc[9],
                       unsigned int color_code, unsigned int data_type,
                       dmr_sync_kind_t sync_kind);

/* Build a complete 264-bit BS Idle normal burst with BS Data Sync. */
int dmr_idle_burst_encode(unsigned int color_code, dmr_burst_t *burst);

/* Build one 24-bit CACH. AT=0 advertises the corresponding inbound slot as
 * idle; TC is 0 for TS1 and 1 for TS2. Payload is a null Short LC fragment. */
int dmr_cach_encode(int access_busy, unsigned int timeslot,
                    uint8_t cach[3]);

#endif
