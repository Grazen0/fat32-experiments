#ifndef AOC_LIB_LIB_NUMERIC_H
#define AOC_LIB_LIB_NUMERIC_H

#include <stddef.h>
#include <stdint.h>

static constexpr size_t BLOCK_SIZE = 512;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

u16 get_u16_le(const u8 data[], size_t idx);

u32 get_u32_le(const u8 data[], size_t idx);

#endif
