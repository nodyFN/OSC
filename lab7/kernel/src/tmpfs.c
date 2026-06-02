#include "tmpfs.h"
#include "mm.h"
#include "string.h"

int tmpfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name);
int tmpfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name);
int tmpfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name);

int tmpfs_open(struct vnode* file_node, struct file** target);
int tmpfs_close(struct file* file);
int tmpfs_read(struct file* file, void* buf, size_t len);
int tmpfs_write(struct file* file, const void* buf, size_t len);
long tmpfs_lseek64(struct file* file, long offset, int whence);

struct file_operations tmpfs_f_ops = {
    .open = tmpfs_open,
    .close = tmpfs_close,
    .read = tmpfs_read,
    .write = tmpfs_write,
    .lseek64 = tmpfs_lseek64
};

struct vnode_operations tmpfs_v_ops = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,
    .mkdir = tmpfs_mkdir
};

struct vnode* tmpfs_create_vnode(enum tmpfs_node_type type, const char* name) {
    struct vnode *node = kmalloc(sizeof(struct vnode));
    node->v_ops = &tmpfs_v_ops;
    node->f_ops = &tmpfs_f_ops;
    
    struct tmpfs_node *internal = kmalloc(sizeof(struct tmpfs_node));
    internal->type = type;
    
    int len = strlen(name);
    if (len >= TMPFS_MAX_FILE_NAME) len = TMPFS_MAX_FILE_NAME - 1;
    memcpy(internal->name, name, len);
    internal->name[len] = '\0';
    
    internal->size = 0;
    internal->nr_entries = 0;
    
    node->internal = internal;
    return node;
}

int tmpfs_setup_mount(struct filesystem* fs, struct mount* mount) {
    mount->fs = fs;
    mount->root = tmpfs_create_vnode(TMPFS_TYPE_DIR, "/");
    mount->root->mount = mount;
    return 0;
}

int tmpfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    struct tmpfs_node *dir = dir_node->internal;
    if (dir->type != TMPFS_TYPE_DIR) return -1;
    
    for (int i = 0; i < dir->nr_entries; i++) {
        struct vnode *child_vnode = dir->entries[i];
        struct tmpfs_node *child = child_vnode->internal;
        if (strcmp(child->name, component_name) == 0) {
            *target = child_vnode;
            return 0;
        }
    }
    return -1;
}

int tmpfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    struct tmpfs_node *dir = dir_node->internal;
    if (dir->type != TMPFS_TYPE_DIR) return -1;
    if (dir->nr_entries >= TMPFS_MAX_DIR_ENTRY) return -1;
    
    struct vnode* dummy;
    if (tmpfs_lookup(dir_node, &dummy, component_name) == 0) {
        return -1;
    }
    
    struct vnode *new_node = tmpfs_create_vnode(TMPFS_TYPE_FILE, component_name);
    new_node->mount = dir_node->mount;
    
    dir->entries[dir->nr_entries++] = new_node;
    *target = new_node;
    
    return 0;
}

int tmpfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    struct tmpfs_node *dir = dir_node->internal;
    if (dir->type != TMPFS_TYPE_DIR) return -1;
    if (dir->nr_entries >= TMPFS_MAX_DIR_ENTRY) return -1;
    
    struct vnode* dummy;
    if (tmpfs_lookup(dir_node, &dummy, component_name) == 0) {
        return -1;
    }
    
    struct vnode *new_node = tmpfs_create_vnode(TMPFS_TYPE_DIR, component_name);
    new_node->mount = dir_node->mount;
    
    dir->entries[dir->nr_entries++] = new_node;
    *target = new_node;
    return 0;
}

int tmpfs_open(struct vnode* file_node, struct file** target) {
    return 0;
}

int tmpfs_close(struct file* file) {
    return 0;
}

int tmpfs_read(struct file* file, void* buf, size_t len) {
    struct tmpfs_node *node = file->vnode->internal;
    if (node->type != TMPFS_TYPE_FILE) return -1;
    
    size_t f_pos = file->f_pos;
    if (f_pos >= node->size) return 0;
    
    size_t readable = node->size - f_pos;
    if (len > readable) len = readable;
    
    memcpy(buf, node->data + f_pos, len);
    file->f_pos += len;
    
    return len;
}

int tmpfs_write(struct file* file, const void* buf, size_t len) {
    struct tmpfs_node *node = file->vnode->internal;
    if (node->type != TMPFS_TYPE_FILE) return -1;
    
    size_t f_pos = file->f_pos;
    if (f_pos + len > TMPFS_MAX_FILE_SIZE) {
        len = TMPFS_MAX_FILE_SIZE - f_pos;
    }
    
    memcpy(node->data + f_pos, buf, len);
    file->f_pos += len;
    
    if (file->f_pos > node->size) {
        node->size = file->f_pos;
    }
    
    return len;
}

long tmpfs_lseek64(struct file* file, long offset, int whence) {
    if (whence == 0) {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}