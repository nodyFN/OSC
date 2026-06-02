#ifndef __TMPFS_H__
#define __TMPFS_H__

#include "vfs.h"

#define TMPFS_MAX_FILE_NAME 16
#define TMPFS_MAX_DIR_ENTRY 16
#define TMPFS_MAX_FILE_SIZE 4096

enum tmpfs_node_type {
    TMPFS_TYPE_DIR,
    TMPFS_TYPE_FILE,
};

struct tmpfs_node {
    enum tmpfs_node_type type;
    char name[TMPFS_MAX_FILE_NAME];
    
    char data[TMPFS_MAX_FILE_SIZE];
    size_t size;
    
    struct vnode* entries[TMPFS_MAX_DIR_ENTRY];
    int nr_entries;
};

int tmpfs_setup_mount(struct filesystem* fs, struct mount* mount);

extern struct file_operations tmpfs_f_ops;
extern struct vnode_operations tmpfs_v_ops;

#endif