#include "numeric.h"
#include <stddef.h>
#include <stdio.h>

#define ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static constexpr size_t CHS_SIZE = 3;

typedef struct {
    u8 boot;
    u8 start_chs[CHS_SIZE];
    u8 type;
    u8 end_chs[CHS_SIZE];
    u32 start_sector;
    u32 size;
} PartitionEntry;

static constexpr size_t MBR_CODE_SIZE = 446;
static constexpr size_t MBR_PRIMARY_TABLE_SIZE = 4;

static constexpr u16 DOS_MAGIC = 0xAA55;

typedef struct [[gnu::packed]] {
    u8 code[MBR_CODE_SIZE];
    PartitionEntry primary_table[MBR_PRIMARY_TABLE_SIZE];
    u16 magic;
} MasterBootRecord;

int main(void)
{
    FILE *file = fopen("/dev/sda", "rb");

    if (file == nullptr) {
        perror("Error opening file");
        return 1;
    }

    MasterBootRecord record;

    const size_t len = fread(&record, 1, sizeof(record), file);

    if (len == 0) {
        perror("Error reading file");
        return 1;
    }

    if (len < sizeof(record)) {
        fprintf(stderr, "File is too small. (Must be at least %zu bytes)\n", sizeof(record));
        return 1;
    }

    for (size_t i = 0; i < sizeof(record); ++i)
        printf("%02X ", ((u8 *)&record)[i]);

    printf("\n");

    if (record.magic != DOS_MAGIC) {
        fprintf(stderr, "Partition table is not DOS.\n");
        return 1;
    }

    for (size_t i = 0; i < MBR_PRIMARY_TABLE_SIZE; ++i) {
        printf("boot: %02X\n", record.primary_table[i].boot);
    }

    fclose(file);
    file = nullptr;

    return 0;
}
