#include <dmr/symbol.h>

float dmr_dibit_to_level(uint8_t dibit)
{
    static const float levels[4] = {+1.0f, +3.0f, -1.0f, -3.0f};
    return levels[dibit & 0x03u];
}
