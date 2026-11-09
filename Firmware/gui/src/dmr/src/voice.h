#ifndef DMR_VOICE_H
#define DMR_VOICE_H

#include <dmr/burst.h>

#include <stddef.h>
#include <stdint.h>

enum {
    DMR_VOICE_FRAME_COUNT = 3,
    DMR_VOICE_FRAME_BITS = 72,
    DMR_VOICE_FRAME_BYTES = 9
};

/* A normal voice burst carries three 72-bit AMBE+2 channel frames.  Their
 * 216 bits occupy the two 108-bit payload regions around the 48-bit centre
 * field.  Frames are packed MSB-first in on-air order. */
int dmr_voice_burst_extract(
    const dmr_burst_t *burst,
    uint8_t frames[DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BYTES]);

/* Replaces only the two voice payload regions.  The centre field (voice
 * sync or EMB/embedded signalling) is left unchanged. */
int dmr_voice_burst_insert(
    dmr_burst_t *burst,
    const uint8_t frames[DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BYTES]);

#endif
