#ifndef __RAMFS_H__
#define __RAMFS_H__

#include "vfs.h"
#include <stddef.h>

#define RAMFS_MAX_FILE_NAME 32
#define RAMFS_MAX_DIR_ENTRY 64

enum ramfs_node_type {
    RAMFS_TYPE_DIR,
    RAMFS_TYPE_FILE,
};

struct ramfs_node {
    enum ramfs_node_type type;
    char name[RAMFS_MAX_FILE_NAME];
    
    char* data;
    size_t size;

    struct vnode* entries[RAMFS_MAX_DIR_ENTRY];
    int nr_entries;
};

int ramfs_setup_mount(struct filesystem* fs, struct mount* mount);

#endif