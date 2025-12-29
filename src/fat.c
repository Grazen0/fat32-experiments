#include "fat.h"
#include "control.h"
#include "numeric.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    u8 data[BLOCK_SIZE];
    u32 block_addr;
} BlockBuffer;

#define BLOCK_BUFFER_INIT {.data = {}, .block_addr = UINT32_MAX}

static BlockBuffer scratch_bbuf = BLOCK_BUFFER_INIT;
static BlockBuffer fat_bbuf = BLOCK_BUFFER_INIT;

static bool bbuf_load(BlockBuffer *const bbuf, FILE *const file, const u32 block_addr)
{
    if (bbuf->block_addr == block_addr)
        return false;

    fseek(file, block_addr * BLOCK_SIZE, SEEK_SET);
    const size_t read_len = fread(bbuf->data, sizeof(bbuf->data[0]), BLOCK_SIZE, file);

    if (read_len == 0) {
        perror("Error reading file");
        PANIC();
    }

    PANIC_IF(read_len != BLOCK_SIZE, "Partial block read (block_addr = 0x%08X, read_len = %zu)",
             block_addr, read_len);

    bbuf->block_addr = block_addr;
    return true;
}

static inline u32 fat32_clus_first_sec(const Fat32 *const fat, const size_t clus)
{
    return fat->first_data_sec + (fat->bpb.secs_per_clus * (clus - 2));
}

typedef enum {
    EBPB_OK,
    EBPB_ROOT_ENTRIES_NZ,
    EBPB_TOTAL_SECS_NZ,
    EBPB_SECS_PER_FAT_NZ,
    EBPB_ROOT_CLUS_INVALID,
    EBPB_BYTES_PER_SEC_UNSUPPORTED,
} ExtendedBpbResult;

[[nodiscard]] static inline ExtendedBpbResult fat32_parse_bpb(const u8 bpb_data[],
                                                              ExtendedBpb *const out_bpb)
{
    enum : size_t {
        BPB_BYTES_PER_SEC = 0x00,
        BPB_SECS_PER_CLUS = 0x02,
        BPB_RESERVED_SECS = 0x03,
        BPB_NUM_FATS = 0x05,
        BPB_ROOT_ENTRIES = 0x06,
        BPB_TOTAL_SECS = 0x08,
        BPB_SECS_PER_FAT = 0x0B,
        BPB_TOTAL_SECS_32 = 0x15,
        BPB_SECS_PER_FAT_32 = 0x19,
        BPB_ROOT_CLUS = 0x21,
    };

    const u16 bytes_per_sec = get_u16_le(bpb_data, BPB_BYTES_PER_SEC);

    if (bytes_per_sec != BLOCK_SIZE)
        return EBPB_BYTES_PER_SEC_UNSUPPORTED;

    const u16 root_entries = get_u16_le(bpb_data, BPB_ROOT_ENTRIES);

    if (root_entries != 0)
        return EBPB_ROOT_ENTRIES_NZ;

    const u16 secs_per_fat = get_u16_le(bpb_data, BPB_SECS_PER_FAT);

    if (secs_per_fat != 0)
        return EBPB_SECS_PER_FAT_NZ;

    const u16 total_secs = get_u16_le(bpb_data, BPB_TOTAL_SECS);

    if (total_secs != 0)
        return EBPB_TOTAL_SECS_NZ;

    const u32 root_clus = get_u32_le(bpb_data, BPB_ROOT_CLUS);

    if (root_clus < 2)
        return EBPB_ROOT_CLUS_INVALID;

    *out_bpb = (ExtendedBpb){
        .bytes_per_sec = bytes_per_sec,
        .secs_per_clus = bpb_data[BPB_SECS_PER_CLUS],
        .reserved_secs = get_u16_le(bpb_data, BPB_RESERVED_SECS),
        .num_fats = bpb_data[BPB_NUM_FATS],
        .total_secs_32 = get_u32_le(bpb_data, BPB_TOTAL_SECS_32),
        .secs_per_fat_32 = get_u32_le(bpb_data, BPB_SECS_PER_FAT_32),
        .root_clus = root_clus,
    };

    return EBPB_OK;
}

static inline void print_dir_entry(const DirEntry *const entry)
{
    const u32 clus = ((u32)entry->clus_hi << 16) | entry->clus_lo;

    if (entry->attrs == FATTR_LONG_NAME) {
        printf("long filename\n");
    } else {
        printf("filename:   ");

        if ((u8)entry->filename[0] == 0xE5)
            printf("[deleted] ");

        for (size_t i = 0; i < FILENAME_SIZE; ++i) {
            const u8 b = entry->filename[i];
            fputc(b >= ' ' && b <= '~' ? b : '.', stdout);
        }

        printf("\n");
    }

    printf("attributes: 0x%02X\n", entry->attrs);
    printf("cluster:    0x%08X\n", clus);
    printf("size:       %u\n", entry->size);
    printf("\n");
}

static inline bool dirent_is_item(const DirEntry *const entry)
{
    return entry->attrs != FATTR_LONG_NAME && entry->filename[0] != FILENAME_DELETED;
}

Fat32Result fat32_parse(FILE *const file, const u32 first_sec, Fat32 *const out_fat)
{
    bbuf_load(&scratch_bbuf, file, first_sec);

    static constexpr size_t FAT_BPB_BASE = 0xB;

    if (fat32_parse_bpb(&scratch_bbuf.data[FAT_BPB_BASE], &out_fat->bpb) != EBPB_OK)
        return FAT32_INVALID_BPB;

    out_fat->first_sec = first_sec;
    out_fat->first_fat_sec = first_sec + out_fat->bpb.reserved_secs;
    out_fat->first_data_sec =
        out_fat->first_fat_sec + (out_fat->bpb.num_fats * out_fat->bpb.secs_per_fat_32);

    bbuf_load(&fat_bbuf, file, out_fat->first_fat_sec);

    out_fat->eoc_marker = get_u32_le(fat_bbuf.data, 4);

    bbuf_load(&scratch_bbuf, file, fat32_clus_first_sec(out_fat, out_fat->bpb.root_clus));

    DirEntry entry;

    for (size_t i = 0;; ++i) {
        memcpy(&entry, &scratch_bbuf.data[i * sizeof(entry)], sizeof(entry));

        if (entry.filename[0] == '\0')
            break;

        if (dirent_is_item(&entry))
            print_dir_entry(&entry);
    }

    return FAT32_OK;
}
