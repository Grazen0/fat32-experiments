#ifndef FIRMWARE_PARTTABLE_H
#define FIRMWARE_PARTTABLE_H

#include "numeric.h"
#include <assert.h>
#include <stddef.h>

typedef enum {
    PTABLE_UNKNOWN,
    PTABLE_MBR,
    PTABLE_GPT,
} PartTableType;

static constexpr size_t MBR_CHS_SIZE = 3;

typedef enum : u8 {
    MBRPART_EMPTY = 0x00,
    MBRPART_FAT32_LBA = 0x0C,
} MbrPartitionType;

typedef struct {
    u8 boot;
    u8 start_chs[MBR_CHS_SIZE];
    u8 part_type;
    u8 end_chs[MBR_CHS_SIZE];
    u32 start;
    u32 size;
} MbrEntry;

static_assert(sizeof(MbrEntry) == 16);

static constexpr size_t MBR_CODE_BASE = 0x000;
static constexpr size_t MBR_CODE_SIZE = 0x1BE;

static constexpr size_t MBR_ENTRIES_BASE = MBR_CODE_BASE + MBR_CODE_SIZE;
static constexpr size_t MBR_ENTRIES = 4;
static constexpr size_t MBR_ENTRY_SIZE = sizeof(MbrEntry);
static constexpr size_t MBR_ENTRIES_SIZE = MBR_ENTRY_SIZE * MBR_ENTRIES;

static constexpr size_t MBR_MAGIC_BASE = MBR_ENTRIES_BASE + MBR_ENTRIES_SIZE;
static constexpr size_t MBR_MAGIC_SIZE = 0x002;

static constexpr size_t MBR_SIZE = MBR_MAGIC_BASE + MBR_MAGIC_SIZE;

PartTableType ptable_detect_type(const u8 block[BLOCK_SIZE]);

const char *ptable_type_str(PartTableType ptable_type);

MbrEntry ptable_mbr_get_entry(const u8 mbr[MBR_SIZE], size_t idx);

const char *ptable_mbr_ptype_str(MbrPartitionType part_type);

#endif
