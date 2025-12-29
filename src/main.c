#include "control.h"
#include "fat.h"
#include "numeric.h"
#include "parttable.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static inline void print_u8_safe(const u8 b)
{
    fputc(b >= ' ' && b <= '~' ? b : '.', stdout);
}

static u8 block_buf[BLOCK_SIZE];

static size_t load_block(FILE *const file, const size_t block_addr, u8 buf[BLOCK_SIZE])
{
    fseek(file, block_addr * BLOCK_SIZE, SEEK_SET);
    const size_t len = fread(buf, sizeof(buf[0]), BLOCK_SIZE, file);

    if (len == 0) {
        perror("Error reading file");
        PANIC();
    }

    return len;
}

static void dump_hex(const u8 data[], const size_t n)
{
    static constexpr size_t COLS = 16;

    const size_t total_rows = (n + COLS - 1) / COLS;

    for (size_t i = 0; i < total_rows; ++i) {
        const size_t rem = MIN(COLS, n - (i * COLS));

        printf("%03zX: ", COLS * i);

        for (size_t j = 0; j < rem; ++j)
            printf("%02X ", data[(COLS * i) + j]);

        printf("  ");

        for (size_t j = 0; j < rem; ++j)
            print_u8_safe(data[(COLS * i) + j]);

        printf("\n");
    }
}

int main(const int argc, const char *const argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [FILENAME]\n", argv[0]);
        return 1;
    }

    const char *const filename = argv[1];
    FILE *file = fopen(filename, "rb");

    if (file == nullptr) {
        perror("Error opening file");
        return 1;
    }

    load_block(file, 0, block_buf);

    const PartTableType ptable_type = ptable_detect_type(block_buf);

    if (ptable_type == PTABLE_UNKNOWN) {
        fprintf(stderr, "Invalid or unsupported partition table.\n");
        return 1;
    }

    if (ptable_type == PTABLE_GPT) {
        fprintf(stderr, "GPT partition tables are not supported.\n");
        return 1;
    }

    printf("Partition table type: %s\n", ptable_type_str(ptable_type));
    printf("\n");

    for (size_t e = 0; e < MBR_ENTRIES; ++e) {
        MbrEntry entry = ptable_mbr_get_entry(block_buf, e);

        if (entry.part_type == MBRPART_EMPTY)
            continue;

        printf("Partition %zu ===========================\n", e + 1);
        printf("boot:      0x%02X\n", entry.boot);
        printf("type:      0x%02X [%s]\n", entry.part_type, ptable_mbr_ptype_str(entry.part_type));
        printf("start:     0x%04X\n", entry.start);
        printf("size:      0x%04X\n", entry.size);
        printf("\n");

        if (entry.part_type == MBRPART_FAT32_LBA) {
            Fat32 fat;
            const Fat32Result fat_result = fat32_parse(file, entry.start, &fat);

            if (fat_result != FAT32_OK) {
                fprintf(stderr, "Could not parse FAT32 partition (code = %i)\n", fat_result);
                continue;
            }

            printf("Bytes per sector:           %u\n", fat.bpb.bytes_per_sec);
            printf("Sectors per cluster:        %u\n", fat.bpb.secs_per_clus);
            printf("Reserved sectors:           %u\n", fat.bpb.reserved_secs);
            printf("Number of FATs:             %u\n", fat.bpb.num_fats);
            printf("Large total sectors:        %u\n", fat.bpb.total_secs_32);
            printf("Sectors per FAT:            0x%08X\n", fat.bpb.secs_per_fat_32);
            printf("Root cluster:               0x%08X\n", fat.bpb.root_clus);
            printf("\n");

            printf("first sector:      0x%08X\n", fat.first_sec);
            printf("first fat sector:  0x%08X\n", fat.first_fat_sec);
            printf("first data sector: 0x%08X\n", fat.first_data_sec);
            printf("\n");

            printf("EOC marker: 0x%08X\n", fat.eoc_marker);
        }
    }

    fclose(file);
    file = nullptr;

    return 0;
}
