#ifndef FAT32_EXPERIMENTS_MACROS_H
#define FAT32_EXPERIMENTS_MACROS_H

#include <stdio.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define PANIC(...)                                              \
    do {                                                        \
        fprintf(stderr, "PANIC (%s:%d)\n", __FILE__, __LINE__); \
        __VA_OPT__(fprintf(stderr, __VA_ARGS__);)               \
        __VA_OPT__(fprintf(stderr, "\n");)                      \
        exit(1);                                                \
    } while (0)

#define PANIC_IF(cond, ...)     \
    do {                        \
        if (cond) {             \
            PANIC(__VA_ARGS__); \
        }                       \
    } while (0)

#endif
