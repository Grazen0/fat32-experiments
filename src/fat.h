#ifndef FIRMWARE_FAT_H
#define FIRMWARE_FAT_H

#include "numeric.h"
#include <stddef.h>
#include <stdio.h>

typedef enum : u8 {
    FATTR_READ_ONLY = 1 << 0,
    FATTR_HIDDEN = 1 << 1,
    FATTR_SYSTEM = 1 << 2,
    FATTR_VOLUME_ID = 1 << 3,
    FATTR_DIRECTORY = 1 << 4,
    FATTR_ARCHIVE = 1 << 5,
    FATTR_LONG_NAME = 0x0F,
} FileAttribute;

static constexpr size_t FILENAME_SIZE = 11;
static constexpr size_t FILENAME_DELETED = 0xE5;

typedef struct {
    unsigned char filename[FILENAME_SIZE];
    u8 attrs;
    u8 _padding;
    u8 created_ds;
    u8 created_hms;
    u16 created_ymd;
    u16 last_access_ymd;
    u16 clus_hi;
    u16 modified_hms;
    u16 modified_ymd;
    u16 clus_lo;
    u32 size;
} DirEntry;

static_assert(sizeof(DirEntry) == 32, "DirectoryEntry must be 32 bytes long");

typedef struct {
    u16 bytes_per_sec;
    u8 secs_per_clus;
    u16 reserved_secs;
    u8 num_fats;
    u32 total_secs_32;
    u32 secs_per_fat_32;
    u32 root_clus;
} ExtendedBpb;

typedef struct {
    ExtendedBpb bpb;
    u32 eoc_marker;
    u32 first_sec;
    u32 first_fat_sec;
    u32 first_data_sec;
} Fat32;

typedef enum {
    FAT32_OK,
    FAT32_INVALID_BPB,
} Fat32Result;

[[nodiscard]] Fat32Result fat32_parse(FILE *file, u32 first_sec, Fat32 *out_fat);

#endif
