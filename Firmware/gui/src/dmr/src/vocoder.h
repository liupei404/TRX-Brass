#ifndef DMR_VOCODER_H
#define DMR_VOCODER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    DMR_VOCODER_PCM_SAMPLES = 160,
    DMR_VOCODER_FRAME_BITS = 49,
    DMR_VOCODER_FRAME_BYTES = 7,
    DMR_VOCODER_FEC_BITS = 72,
    DMR_VOCODER_FEC_BYTES = 9
};

typedef struct vocoder_state vocoder_state_t;

vocoder_state_t *vocoder_state_create(void);
void vocoder_state_destroy(vocoder_state_t *state);
void vocoder_state_reset(vocoder_state_t *state);
void vocoder_state_decode(vocoder_state_t *state, const uint8_t *vocoder49,
                          int16_t *pcm);
void vocoder_state_encode(vocoder_state_t *state, uint8_t *vocoder49,
                          const int16_t *pcm);
void vocoder_state_decode_fec(vocoder_state_t *state, const uint8_t *vocoder,
                              int16_t *pcm);
void vocoder_state_encode_fec(vocoder_state_t *state, uint8_t *vocoder,
                              const int16_t *pcm);
/* Returns corrected Golay bits, or -1 outside the correction radius.  PCM
 * concealment is still generated from the nearest codewords. */
int vocoder_state_decode_fec_checked(vocoder_state_t *state,
                                     const uint8_t *vocoder, int16_t *pcm);

/* Legacy process-global API. Prefer vocoder_state_* in new code. */
int vocoder_init(void);
void vocoder_decode(const uint8_t *vocoder49, int16_t *pcm);
void vocoder_encode(uint8_t *vocoder49, const int16_t *pcm);
void vocoder_decode_fec(const uint8_t *vocoder, int16_t *pcm);
void vocoder_encode_fec(uint8_t *vocoder, const int16_t *pcm);

#ifdef __cplusplus
}
#endif

#endif
