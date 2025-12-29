#include "parttable.h"
#include "numeric.h"
#include <stddef.h>
#include <string.h>

static const char *ptable_type_labels[] = {
    [PTABLE_UNKNOWN] = "unknown",
    [PTABLE_MBR] = "mbr",
    [PTABLE_GPT] = "gpt",
};

static_assert(MBR_SIZE == BLOCK_SIZE);

static constexpr u16 MBR_MAGIC = 0xAA55;

PartTableType ptable_detect_type(const u8 block[BLOCK_SIZE])
{
    const u16 magic = get_u16_le(block, MBR_MAGIC_BASE);

    if (magic == MBR_MAGIC)
        return PTABLE_MBR;

    return PTABLE_UNKNOWN;
}

const char *ptable_type_str(const PartTableType ptable_type)
{
    return ptable_type_labels[ptable_type];
}

MbrEntry ptable_mbr_get_entry(const u8 mbr[MBR_SIZE], const size_t idx)
{
    MbrEntry entry;
    memcpy(&entry, &mbr[MBR_ENTRIES_BASE + (idx * sizeof(entry))], sizeof(entry));
    return entry;
}

static const char *part_type_label[256] = {
    [MBRPART_EMPTY] = "empty",
    [MBRPART_FAT32_LBA] = "FAT32 (LBA)",
};

const char *ptable_mbr_ptype_str(const MbrPartitionType part_type)
{
    const char *const str = part_type_label[part_type];

    if (str == nullptr)
        return "unknown";

    return str;
}
