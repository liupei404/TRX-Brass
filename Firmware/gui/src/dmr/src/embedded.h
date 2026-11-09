#ifndef DMR_EMBEDDED_H
#define DMR_EMBEDDED_H

#include <dmr/burst.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    unsigned int color_code;
    int privacy;
    unsigned int lcss;
    unsigned int corrected_bits;
    uint8_t fragment[4];
} dmr_emb_t;

typedef struct dmr_embedded_lc_decoder dmr_embedded_lc_decoder_t;

typedef struct {
    uint8_t data[9];
    unsigned int flco;
    unsigned int feature_set_id;
    unsigned int service_options;
    uint32_t target_id;
    uint32_t source_id;
    unsigned int corrected_bits;
} dmr_embedded_lc_t;

int dmr_emb_decode(const dmr_burst_t *burst, dmr_emb_t *emb);
int dmr_emb_encode(dmr_burst_t *burst, const dmr_emb_t *emb);
int dmr_embedded_lc_encode(const uint8_t data[9], unsigned int color_code,
                           int privacy, dmr_emb_t fragments[4]);
dmr_embedded_lc_decoder_t *dmr_embedded_lc_decoder_create(void);
void dmr_embedded_lc_decoder_destroy(dmr_embedded_lc_decoder_t *decoder);
void dmr_embedded_lc_decoder_reset(dmr_embedded_lc_decoder_t *decoder);
int dmr_embedded_lc_decoder_process(dmr_embedded_lc_decoder_t *decoder,
                                    const dmr_emb_t *emb,
                                    dmr_embedded_lc_t *lc);

#endif
