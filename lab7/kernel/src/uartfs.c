#include "uartfs.h"
#include "mm.h"
#include "uart.h"

int uartfs_open(struct vnode* file_node, struct file** target) { return 0; }
int uartfs_close(struct file* file) { return 0; }

int uartfs_read(struct file* file, void* buf, size_t len) {
    char* cbuf = (char*)buf;
    for(size_t i = 0; i < len; i++) {
        cbuf[i] = uart_getc();
    }
    return len;
}

int uartfs_write(struct file* file, const void* buf, size_t len) {
    const char* cbuf = (const char*)buf;
    for(size_t i = 0; i < len; i++) {
        uart_putc(cbuf[i]);
    }
    return len;
}

long uartfs_lseek64(struct file* file, long offset, int whence) { return -1; }

int uartfs_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name) { return -1; }
int uartfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name) { return -1; }
int uartfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name) { return -1; }

struct file_operations uartfs_f_ops = {
    .open = uartfs_open,
    .close = uartfs_close,
    .read = uartfs_read,
    .write = uartfs_write,
    .lseek64 = uartfs_lseek64
};

struct vnode_operations uartfs_v_ops = {
    .lookup = uartfs_lookup,
    .create = uartfs_create,
    .mkdir = uartfs_mkdir
};

int uartfs_setup_mount(struct filesystem* fs, struct mount* mount) {
    mount->fs = fs;
    struct vnode *node = kmalloc(sizeof(struct vnode));
    node->v_ops = &uartfs_v_ops;
    node->f_ops = &uartfs_f_ops;
    node->mount = NULL;
    node->parent = NULL;
    node->internal = NULL;
    
    mount->root = node;
    return 0;
}