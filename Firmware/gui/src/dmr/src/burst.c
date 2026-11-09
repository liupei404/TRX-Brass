#include <dmr/burst.h>

#include <string.h>

uint8_t dmr_burst_get_dibit(
    const uint8_t data[DMR_BURST_BYTE_COUNT], size_t index)
{
    if (data == NULL || index >= DMR_BURST_DIBIT_COUNT)
        return UINT8_MAX;
    const unsigned int shift = 6u - 2u * (unsigned int)(index & 3u);
    return (uint8_t)((data[index / 4u] >> shift) & 3u);
}

int dmr_burst_set_dibit(
    uint8_t data[DMR_BURST_BYTE_COUNT], size_t index, uint8_t dibit)
{
    if (data == NULL || index >= DMR_BURST_DIBIT_COUNT || dibit > 3u)
        return 0;
    const unsigned int shift = 6u - 2u * (unsigned int)(index & 3u);
    const uint8_t mask = (uint8_t)(3u << shift);
    data[index / 4u] = (uint8_t)((data[index / 4u] & ~mask) |
                                 (dibit << shift));
    return 1;
}

uint8_t dmr_burst_get_bit(
    const uint8_t data[DMR_BURST_BYTE_COUNT], size_t index)
{
    if (data == NULL || index >= DMR_BURST_BIT_COUNT)
        return UINT8_MAX;
    return (uint8_t)((data[index / 8u] >> (7u - index % 8u)) & 1u);
}

int dmr_burst_set_bit(
    uint8_t data[DMR_BURST_BYTE_COUNT], size_t index, uint8_t bit)
{
    if (data == NULL || index >= DMR_BURST_BIT_COUNT || bit > 1u)
        return 0;
    const uint8_t mask = (uint8_t)(1u << (7u - index % 8u));
    data[index / 8u] = (uint8_t)((data[index / 8u] & ~mask) |
                                 (bit != 0u ? mask : 0u));
    return 1;
}

int dmr_burst_pack_dibits(
    const uint8_t dibits[DMR_BURST_DIBIT_COUNT],
    uint8_t data[DMR_BURST_BYTE_COUNT])
{
    if (dibits == NULL || data == NULL)
        return 0;
    for (size_t i = 0; i < DMR_BURST_DIBIT_COUNT; ++i) {
        if (dibits[i] > 3u)
            return 0;
    }
    memset(data, 0, DMR_BURST_BYTE_COUNT);
    for (size_t i = 0; i < DMR_BURST_DIBIT_COUNT; ++i)
        dmr_burst_set_dibit(data, i, dibits[i]);
    return 1;
}

int dmr_burst_unpack_dibits(
    const uint8_t data[DMR_BURST_BYTE_COUNT],
    uint8_t dibits[DMR_BURST_DIBIT_COUNT])
{
    if (data == NULL || dibits == NULL)
        return 0;
    for (size_t i = 0; i < DMR_BURST_DIBIT_COUNT; ++i)
        dibits[i] = dmr_burst_get_dibit(data, i);
    return 1;
}
