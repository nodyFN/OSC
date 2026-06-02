#include "ramfs.h"
#include "mm.h"
#include "string.h"
#include "initrd.h"
#include "utils.h"
#include "vm.h"

extern struct KernelInfo kernel_info;

int ramfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name);
int ramfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name);
int ramfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name);
int ramfs_open(struct vnode* file_node, struct file** target);
int ramfs_close(struct file* file);
int ramfs_read(struct file* file, void* buf, size_t len);
int ramfs_write(struct file* file, const void* buf, size_t len);
long ramfs_lseek64(struct file* file, long offset, int whence);

struct file_operations ramfs_f_ops = {
    .open = ramfs_open,
    .close = ramfs_close,
    .read = ramfs_read,
    .write = ramfs_write,
    .lseek64 = ramfs_lseek64
};

struct vnode_operations ramfs_v_ops = {
    .lookup = ramfs_lookup,
    .create = ramfs_create,
    .mkdir = ramfs_mkdir
};

struct vnode* ramfs_create_vnode(enum ramfs_node_type type, const char* name) {
    struct vnode *node = kmalloc(sizeof(struct vnode));
    node->v_ops = &ramfs_v_ops;
    node->f_ops = &ramfs_f_ops;
    node->mount = NULL;
    
    struct ramfs_node *internal = kmalloc(sizeof(struct ramfs_node));
    memset(internal, 0, sizeof(struct ramfs_node));
    
    internal->type = type;
    int len = strlen(name);
    if (len >= RAMFS_MAX_FILE_NAME) len = RAMFS_MAX_FILE_NAME - 1;
    memcpy(internal->name, name, len);
    internal->name[len] = '\0';
    
    node->internal = internal;
    return node;
}

int ramfs_setup_mount(struct filesystem* fs, struct mount* mount) {
    mount->fs = fs;
    mount->root = ramfs_create_vnode(RAMFS_TYPE_DIR, "/");
    
    uint64_t initrd_start_va = PA_TO_VA(kernel_info.initrd_start_addr);
    char* current = (char*)initrd_start_va;
    
    struct ramfs_node* root_internal = mount->root->internal;

    while(1) { 
        struct cpio_newc_header* header = (struct cpio_newc_header*)current;
        if(strncmp(header->c_magic, CPIO_NEWC_MAGIC, 6) != 0){
            break;
        }

        uint32_t namesize = _string_to_hex32_helper(header->c_namesize);
        uint32_t filesize = _string_to_hex32_helper(header->c_filesize);

        char* filename = current + sizeof(struct cpio_newc_header);
        if(strncmp(filename, CPIO_NEWC_END, sizeof(CPIO_NEWC_END) - 1) == 0){
            break;
        }

        char* file_content = current + ALIGN4(sizeof(struct cpio_newc_header) + namesize);
        
        struct vnode* child = ramfs_create_vnode(RAMFS_TYPE_FILE, filename);
        struct ramfs_node* child_internal = child->internal;
        child_internal->data = file_content;
        child_internal->size = filesize;
        child->parent = mount->root;

        if (root_internal->nr_entries < RAMFS_MAX_DIR_ENTRY) {
            root_internal->entries[root_internal->nr_entries++] = child;
        }

        uint32_t offset = ALIGN4(ALIGN4(sizeof(struct cpio_newc_header) + namesize) + filesize);
        current += offset;
    }
    
    return 0;
}

int ramfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name) {
    struct ramfs_node *dir = dir_node->internal;
    if (dir->type != RAMFS_TYPE_DIR) return -1;
    
    for (int i = 0; i < dir->nr_entries; i++) {
        struct vnode *child_vnode = dir->entries[i];
        struct ramfs_node *child = child_vnode->internal;
        if (strcmp(child->name, component_name) == 0) {
            *target = child_vnode;
            return 0;
        }
    }
    return -1;
}

int ramfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name) { return -1; }
int ramfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name) { return -1; }
int ramfs_write(struct file* file, const void* buf, size_t len) { return -1; }

int ramfs_open(struct vnode* file_node, struct file** target) { return 0; }
int ramfs_close(struct file* file) { return 0; }

int ramfs_read(struct file* file, void* buf, size_t len) {
    struct ramfs_node *node = file->vnode->internal;
    if (node->type != RAMFS_TYPE_FILE) return -1;
    
    size_t f_pos = file->f_pos;
    if (f_pos >= node->size) return 0;
    
    size_t readable = node->size - f_pos;
    if (len > readable) len = readable;
    
    memcpy(buf, node->data + f_pos, len);
    file->f_pos += len;
    
    return len;
}

long ramfs_lseek64(struct file* file, long offset, int whence) {
    if (whence == 0) {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}