#include <dmr/voice.h>

#include <string.h>

static size_t payload_bit(size_t index)
{
    return index < 108u ? index : index + 48u;
}

int dmr_voice_burst_extract(
    const dmr_burst_t *burst,
    uint8_t frames[DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BYTES])
{
    if (!burst || !frames)
        return 0;
    memset(frames, 0, DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BYTES);
    for (size_t i = 0; i < DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BITS; ++i) {
        const uint8_t bit = dmr_burst_get_bit(burst->data, payload_bit(i));
        if (bit == UINT8_MAX)
            return 0;
        frames[(i / DMR_VOICE_FRAME_BITS) * DMR_VOICE_FRAME_BYTES +
               (i % DMR_VOICE_FRAME_BITS) / 8u] |=
            (uint8_t)(bit << (7u - (i % 8u)));
    }
    return 1;
}

int dmr_voice_burst_insert(
    dmr_burst_t *burst,
    const uint8_t frames[DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BYTES])
{
    if (!burst || !frames)
        return 0;
    for (size_t i = 0; i < DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BITS; ++i) {
        const uint8_t bit = (uint8_t)((frames[
            (i / DMR_VOICE_FRAME_BITS) * DMR_VOICE_FRAME_BYTES +
            (i % DMR_VOICE_FRAME_BITS) / 8u] >> (7u - (i % 8u))) & 1u);
        if (!dmr_burst_set_bit(burst->data, payload_bit(i), bit))
            return 0;
    }
    return 1;
}
