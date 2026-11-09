#include <dmr/data.h>
#include <dmr/phy.h>

#include <string.h>

static unsigned int popcount32(uint32_t value)
{
    unsigned int count = 0;
    while (value) { value &= value - 1u; ++count; }
    return count;
}

static uint32_t slot_codeword(unsigned int value)
{
    static const uint32_t rows[8] = {
        0x800f6u, 0x40d99u, 0x206cdu, 0x10367u,
        0x08dc6u, 0x04a97u, 0x0293eu, 0x018ebu
    };
    uint32_t word = 0;
    for (unsigned int i = 0; i < 8u; ++i)
        if ((value >> (7u - i)) & 1u)
            word ^= rows[i];
    return word;
}

uint32_t dmr_slot_type_encode(unsigned int color_code,
                              unsigned int data_type)
{
    if (color_code > 15u || data_type > 15u)
        return UINT32_MAX;
    return slot_codeword((color_code << 4) | data_type);
}

int dmr_slot_type_decode(uint32_t received, unsigned int *color_code,
                         unsigned int *data_type,
                         unsigned int *corrected_bits)
{
    if (received > 0xfffffu || !color_code || !data_type)
        return 0;
    unsigned int best_value = 0, best_distance = 21u;
    for (unsigned int value = 0; value < 256u; ++value) {
        const unsigned int distance = popcount32(
            received ^ slot_codeword(value));
        if (distance < best_distance) {
            best_distance = distance;
            best_value = value;
        }
    }
    if (best_distance > 3u)
        return 0;
    *color_code = best_value >> 4;
    *data_type = best_value & 15u;
    if (corrected_bits)
        *corrected_bits = best_distance;
    return 1;
}

static uint8_t get_bit(const uint8_t *bytes, unsigned int bit)
{
    return (bytes[bit / 8u] >> (7u - bit % 8u)) & 1u;
}

static void set_bit(uint8_t *bytes, unsigned int bit, uint8_t value)
{
    const uint8_t mask = (uint8_t)(0x80u >> (bit % 8u));
    if (value) bytes[bit / 8u] |= mask;
    else bytes[bit / 8u] &= (uint8_t)~mask;
}

static void hamming_15_11(uint8_t *d)
{
    d[11]=d[0]^d[1]^d[2]^d[3]^d[5]^d[7]^d[8];
    d[12]=d[1]^d[2]^d[3]^d[4]^d[6]^d[8]^d[9];
    d[13]=d[2]^d[3]^d[4]^d[5]^d[7]^d[9]^d[10];
    d[14]=d[0]^d[1]^d[2]^d[4]^d[6]^d[7]^d[10];
}

static void hamming_13_9(uint8_t *d)
{
    d[9]=d[0]^d[1]^d[3]^d[5]^d[6];
    d[10]=d[0]^d[1]^d[2]^d[4]^d[6]^d[7];
    d[11]=d[0]^d[1]^d[2]^d[3]^d[5]^d[7]^d[8];
    d[12]=d[0]^d[2]^d[4]^d[5]^d[8];
}

int dmr_bptc_196_96_encode(const uint8_t information[12],
                           uint8_t encoded[25])
{
    if (!information || !encoded)
        return 0;
    uint8_t matrix[196] = {0};
    unsigned int input = 0;
    for (unsigned int i = 4u; i < 12u; ++i)
        matrix[i] = get_bit(information, input++);
    for (unsigned int row = 1u; row < 9u; ++row)
        for (unsigned int col = 0; col < 11u; ++col)
            matrix[row * 15u + 1u + col] = get_bit(information, input++);
    for (unsigned int row = 0; row < 9u; ++row)
        hamming_15_11(matrix + 1u + row * 15u);
    for (unsigned int col = 0; col < 15u; ++col) {
        uint8_t column[13] = {0};
        for (unsigned int row = 0; row < 9u; ++row)
            column[row] = matrix[1u + row * 15u + col];
        hamming_13_9(column);
        for (unsigned int row = 9u; row < 13u; ++row)
            matrix[1u + row * 15u + col] = column[row];
    }
    memset(encoded, 0, 25u);
    for (unsigned int i = 0; i < 196u; ++i)
        set_bit(encoded, (i * 181u) % 196u, matrix[i]);
    return 1;
}

int dmr_bptc_196_96_decode(const uint8_t encoded[25],
                           uint8_t information[12])
{
    if (!encoded || !information)
        return 0;
    uint8_t matrix[196] = {0};
    for (unsigned int i = 0; i < 196u; ++i)
        matrix[i] = get_bit(encoded, (i * 181u) % 196u);
    memset(information, 0, 12u);
    unsigned int output = 0;
    for (unsigned int i = 4u; i < 12u; ++i)
        set_bit(information, output++, matrix[i]);
    for (unsigned int row = 1u; row < 9u; ++row)
        for (unsigned int col = 0; col < 11u; ++col)
            set_bit(information, output++, matrix[row * 15u + 1u + col]);
    return 1;
}

static uint8_t gf_multiply(uint8_t a, uint8_t b)
{
    uint8_t result = 0;
    while (b) {
        if (b & 1u) result ^= a;
        const int high = a & 0x80u;
        a <<= 1;
        if (high) a ^= 0x1du;
        b >>= 1;
    }
    return result;
}

static void rs_12_9(const uint8_t lc[9], uint8_t parity[3])
{
    static const uint8_t coefficients[9][3] = {
        {0x1c,0xbc,0xfd},{0x89,0x31,0x08},{0xad,0x41,0x36},
        {0x7d,0x71,0x16},{0xf3,0xa6,0x3a},{0x08,0x83,0x7b},
        {0x3f,0x6f,0x02},{0x6c,0x0d,0xa7},{0x0e,0x38,0x40}
    };
    memset(parity, 0, 3u);
    for (unsigned int i = 0; i < 9u; ++i)
        for (unsigned int p = 0; p < 3u; ++p)
            parity[p] ^= gf_multiply(lc[i], coefficients[i][p]);
}

static int extract_data_fields(const dmr_burst_t *burst, uint8_t encoded[25],
                               unsigned int *color_code,
                               unsigned int *data_type)
{
    uint32_t slot = 0;
    for (unsigned int i = 0; i < 10u; ++i)
        slot = (slot << 1) | dmr_burst_get_bit(burst->data, 98u + i);
    for (unsigned int i = 0; i < 10u; ++i)
        slot = (slot << 1) | dmr_burst_get_bit(burst->data, 156u + i);
    if (!dmr_slot_type_decode(slot, color_code, data_type, NULL))
        return 0;
    memset(encoded, 0, 25u);
    for (unsigned int i = 0; i < 98u; ++i)
        set_bit(encoded, i, dmr_burst_get_bit(burst->data, i));
    for (unsigned int i = 0; i < 98u; ++i)
        set_bit(encoded, 98u + i,
                dmr_burst_get_bit(burst->data, 166u + i));
    return 1;
}

int dmr_full_lc_decode(const dmr_burst_t *burst, uint8_t lc[9],
                       unsigned int *color_code, unsigned int *data_type)
{
    if (!burst || !lc || !color_code || !data_type)
        return 0;
    uint8_t encoded[25], information[12], expected[3];
    if (!extract_data_fields(burst, encoded, color_code, data_type) ||
        (*data_type != DMR_DATA_TYPE_VOICE_LC_HEADER &&
         *data_type != DMR_DATA_TYPE_TERMINATOR_LC) ||
        !dmr_bptc_196_96_decode(encoded, information))
        return 0;
    memcpy(lc, information, 9u);
    rs_12_9(lc, expected);
    const uint8_t mask = *data_type == DMR_DATA_TYPE_VOICE_LC_HEADER
        ? 0x96u : 0x99u;
    for (unsigned int i = 0; i < 3u; ++i)
        if ((uint8_t)(information[9u + i] ^ mask) != expected[i])
            return 0;
    return 1;
}

int dmr_full_lc_encode(dmr_burst_t *burst, const uint8_t lc[9],
                       unsigned int color_code, unsigned int data_type,
                       dmr_sync_kind_t sync_kind)
{
    if (!burst || !lc || color_code > 15u ||
        (data_type != DMR_DATA_TYPE_VOICE_LC_HEADER &&
         data_type != DMR_DATA_TYPE_TERMINATOR_LC) ||
        (sync_kind != DMR_SYNC_MS_DATA && sync_kind != DMR_SYNC_BS_DATA))
        return 0;
    uint8_t information[12], encoded[25];
    memcpy(information, lc, 9u);
    rs_12_9(lc, information + 9u);
    const uint8_t mask = data_type == DMR_DATA_TYPE_VOICE_LC_HEADER
        ? 0x96u : 0x99u;
    for (unsigned int i = 9u; i < 12u; ++i)
        information[i] ^= mask;
    if (!dmr_bptc_196_96_encode(information, encoded))
        return 0;
    memset(burst, 0, sizeof(*burst));
    for (unsigned int i = 0; i < 98u; ++i)
        dmr_burst_set_bit(burst->data, i, get_bit(encoded, i));
    for (unsigned int i = 0; i < 98u; ++i)
        dmr_burst_set_bit(burst->data, 166u + i,
                          get_bit(encoded, 98u + i));
    const uint32_t slot = dmr_slot_type_encode(color_code, data_type);
    for (unsigned int i = 0; i < 10u; ++i)
        dmr_burst_set_bit(burst->data, 98u + i,
                          (uint8_t)((slot >> (19u - i)) & 1u));
    for (unsigned int i = 0; i < 10u; ++i)
        dmr_burst_set_bit(burst->data, 156u + i,
                          (uint8_t)((slot >> (9u - i)) & 1u));
    return dmr_phy_tx_insert_sync(burst->data, sync_kind);
}

int dmr_idle_burst_encode(unsigned int color_code, dmr_burst_t *burst)
{
    static const uint8_t fill[12] = {
        0xff,0x83,0xdf,0x17,0x32,0x09,0x4e,0xd1,0xe7,0xcd,0x8a,0x91
    };
    uint8_t payload[25];
    if (!burst || color_code > 15u ||
        !dmr_bptc_196_96_encode(fill, payload))
        return 0;
    memset(burst, 0, sizeof(*burst));
    for (unsigned int i = 0; i < 98u; ++i)
        dmr_burst_set_bit(burst->data, i, get_bit(payload, i));
    for (unsigned int i = 0; i < 98u; ++i)
        dmr_burst_set_bit(burst->data, 166u + i, get_bit(payload, 98u + i));
    const uint32_t slot = dmr_slot_type_encode(color_code,
                                               DMR_DATA_TYPE_IDLE);
    for (unsigned int i = 0; i < 10u; ++i)
        dmr_burst_set_bit(burst->data, 98u + i,
            (uint8_t)((slot >> (19u - i)) & 1u));
    for (unsigned int i = 0; i < 10u; ++i)
        dmr_burst_set_bit(burst->data, 156u + i,
            (uint8_t)((slot >> (9u - i)) & 1u));
    return dmr_phy_tx_insert_sync(burst->data, DMR_SYNC_BS_DATA);
}

int dmr_cach_encode(int access_busy, unsigned int timeslot, uint8_t cach[3])
{
    if (!cach || (access_busy != 0 && access_busy != 1) ||
        (timeslot != 1u && timeslot != 2u))
        return 0;
    const uint8_t d[4] = {(uint8_t)access_busy,
                          (uint8_t)(timeslot - 1u), 0, 0};
    const uint8_t h[3] = {d[0]^d[1]^d[2], d[1]^d[2]^d[3],
                          d[0]^d[1]^d[3]};
    uint8_t tx[24] = {0};
    tx[0]=d[0]; tx[4]=d[1]; tx[8]=d[2]; tx[12]=d[3];
    tx[14]=h[0]; tx[18]=h[1]; tx[22]=h[2];
    memset(cach, 0, 3u);
    for (unsigned int i = 0; i < 24u; ++i)
        set_bit(cach, i, tx[i]);
    return 1;
}
