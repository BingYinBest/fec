/*
 * Minimal squashfs superblock definition, layout matches the on-disk
 * format (Linux uapi/linux/squashfs_fs.h). Self-contained so the fec
 * build does not need kernel headers.
 */
#ifndef SQUASHFS_FS_H
#define SQUASHFS_FS_H

#include <stdint.h>

#define SQUASHFS_MAGIC 0x73717368
#define SQUASHFS_MAGIC_SWAB 0x68737173

struct squashfs_super_block {
    uint32_t s_magic;
    uint32_t inodes;
    uint32_t mkfs_time;
    uint32_t block_size;
    uint32_t fragments;
    uint16_t compression;
    uint16_t block_log;
    uint16_t flags;
    uint16_t no_ids;
    uint16_t s_major;
    uint16_t s_minor;
    uint64_t root_inode;
    uint64_t bytes_used;
    uint64_t id_table_start;
    uint64_t xattr_id_table_start;
    uint64_t inode_table_start;
    uint64_t directory_table_start;
    uint64_t fragment_table_start;
    uint64_t lookup_table_start;
};

#endif /* SQUASHFS_FS_H */
