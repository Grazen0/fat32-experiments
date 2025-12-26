#include "control.h"
#include "numeric.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static constexpr size_t CHS_SIZE = 3;

typedef struct {
    u8 boot;
    u8 start_chs[CHS_SIZE];
    u8 part_type;
    u8 end_chs[CHS_SIZE];
    u32 start;
    u32 size;
} PartitionEntry;

static_assert(sizeof(PartitionEntry) == 16, "PartitionEntry must be 16 bytes long");

static constexpr size_t BLOCK_SIZE = 512;

static constexpr size_t MBR_CODE_BASE = 0x000;
static constexpr size_t MBR_CODE_SIZE = 0x1BE;

static constexpr size_t MBR_PART_TABLE_BASE = MBR_CODE_BASE + MBR_CODE_SIZE;
static constexpr size_t MBR_PART_ENTRIES = 4;
static constexpr size_t MBR_PART_ENTRY_SIZE = 16;
static constexpr size_t MBR_PART_TABLE_SIZE = MBR_PART_ENTRY_SIZE * MBR_PART_ENTRIES;

static constexpr size_t MBR_MAGIC_BASE = MBR_PART_TABLE_BASE + MBR_PART_TABLE_SIZE;
static constexpr size_t MBR_MAGIC_SIZE = 0x002;

static constexpr size_t MBR_SIZE = MBR_MAGIC_BASE + MBR_MAGIC_SIZE;

static constexpr u16 MBR_MAGIC = 0xAA55;

typedef enum : u8 {
    PART_EMPTY = 0x00,
    PART_FAT32_LBA = 0x0C,
} PartitionType;

static const char *part_type_label[256] = {
    [0x00] = "empty",
    [PART_FAT32_LBA] = "FAT32 (LBA)",
};

typedef struct {
    u16 cylinder;
    u8 head;
    u8 sector;
} ChsFields;

static_assert(MBR_SIZE <= BLOCK_SIZE, "MBR must fit inside a single block");

ChsFields parse_chs(const u8 chs[CHS_SIZE])
{
    const u16 extra_cyl_bits = chs[1] & 0xC0;

    return (ChsFields){
        .cylinder = (u16)chs[2] | (extra_cyl_bits << 2),
        .head = chs[0],
        .sector = chs[1] & 0x3F,
    };
}

static u8 block_buf[BLOCK_SIZE];

static inline u16 get_u16_le(const u8 data[], const size_t idx)
{
    const u8 lo = data[idx];
    const u8 hi = data[idx + 1];
    return ((u16)hi << 8) | lo;
}

FILE *file = nullptr;

static size_t load_block(const size_t block_addr)
{
    fseek(file, block_addr * BLOCK_SIZE, SEEK_SET);
    const size_t len = fread(block_buf, sizeof(block_buf[0]), MBR_SIZE, file);

    if (len == 0) {
        perror("Error reading file");
        PANIC();
    }

    return len;
}

static void parse_fat32(const PartitionEntry *entry)
{
    printf("Contents: \n");
    printf("\n");

    for (size_t b = 0; b < 2; ++b) {
        const size_t len = load_block(entry->start + b);

        for (size_t i = 0; i < len; ++i) {
            printf("%02X ", block_buf[i]);

            if (((i + 1) % 16) == 0)
                printf("\n");
        }

        printf("\n");
    }
}

int main(void)
{
    file = fopen("/dev/sda", "rb");

    if (file == nullptr) {
        perror("Error opening file");
        return 1;
    }

    const size_t len = load_block(0);

    if (len < MBR_SIZE) {
        fprintf(stderr, "File is too small. (Must be at least %zu bytes)\n", sizeof(MBR_SIZE));
        return 1;
    }

    const u16 magic = get_u16_le(block_buf, MBR_MAGIC_BASE);

    if (magic != MBR_MAGIC) {
        fprintf(stderr, "Partition table is not DOS.\n");
        return 1;
    }

    printf("\n");

    for (size_t e = 0; e < MBR_PART_ENTRIES; ++e) {
        PartitionEntry entry;
        memcpy(&entry, &block_buf[MBR_PART_TABLE_BASE + (e * sizeof(entry))], sizeof(entry));

        printf("Partition %zu ===========================\n", e + 1);

        if (entry.part_type == PART_EMPTY) {
            printf("[Empty]\n");
            printf("\n");
            continue;
        }

        printf("boot:      0x%02X\n", entry.boot);
        printf("type:      0x%02X [%s]\n", entry.part_type, part_type_label[entry.part_type]);
        printf("start:     0x%04X\n", entry.start);
        printf("size:      0x%04X\n", entry.size);
        printf("\n");

        if (entry.part_type == PART_FAT32_LBA)
            parse_fat32(&entry);
    }

    fclose(file);
    file = nullptr;

    return 0;
}
