#include "numeric.h"

u16 get_u16_le(const u8 data[], const size_t idx)
{
    const u16 lo = data[idx];
    const u16 hi = data[idx + 1];
    return (hi << 8) | lo;
}

u32 get_u32_le(const u8 data[], const size_t idx)
{
    const u32 a = data[idx];
    const u32 b = data[idx + 1];
    const u32 c = data[idx + 2];
    const u32 d = data[idx + 3];

    return (d << 24) | (c << 16) | (b << 8) | a;
}
