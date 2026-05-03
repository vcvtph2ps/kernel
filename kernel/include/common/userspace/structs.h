#pragma once
#include <stdint.h>

typedef struct {
    long tv_sec;
    long tv_nsec;
} structs_timespec_t;

#define STRUCTS_STAT_MODE_TYPE_MASK 0x0F000
#define STRUCTS_STAT_MODE_TYPE_CHAR 0x02000
#define STRUCTS_STAT_MODE_TYPE_FILE 0x08000
#define STRUCTS_STAT_MODE_TYPE_DIR 0x04000

typedef struct {
    uint64_t st_dev;
    uint64_t st_ino;
    unsigned long st_nlink;
    unsigned int st_mode;
    unsigned int st_uid;
    unsigned int st_gid;
    unsigned int pad0;
    uint64_t st_rdev;
    int64_t st_size;
    long st_blksize;
    int64_t st_blocks;
    structs_timespec_t st_atim;
    structs_timespec_t st_mtim;
    structs_timespec_t st_ctim;
    long pad1[3];
} structs_stat_t;
