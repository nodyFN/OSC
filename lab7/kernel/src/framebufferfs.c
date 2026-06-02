#include "framebufferfs.h"
#include "mm.h"
#include "string.h"
#include "video.h"
#include "vm.h"


static void flush_dcache(void* addr, unsigned long len) {
    unsigned long start = (unsigned long)addr & ~(CACHE_BLOCK_SIZE - 1);
    __sync_synchronize();
    for (unsigned long line = start; line < (unsigned long)addr + len;
         line += CACHE_BLOCK_SIZE) {
        cbo_flush(line);
        __sync_synchronize();
    }
}

struct framebuffer_info {
    unsigned int width;
    unsigned int height;
    unsigned int bpp;
};
#define FB_IOCTL_GET_INFO 0

int fb_write(struct file* file, const void* buf, size_t len) {
    size_t max_size = FB_WIDTH * FB_HEIGHT * FB_BPP; 
    
    if (file->f_pos >= max_size) return 0;
    
    size_t write_len = len;
    if (file->f_pos + write_len > max_size) {
        write_len = max_size - file->f_pos;
    }

    char* fb_va = (char*)PA_TO_VA(FB_BASE_PA);
    char* dest = fb_va + file->f_pos;
    
    memcpy(dest, buf, write_len);
    file->f_pos += write_len;

    flush_dcache(dest, write_len);
    
    return write_len;
}

long fb_lseek64(struct file* file, long offset, int whence) {
    if (whence == 0) {
        file->f_pos = offset;
        return file->f_pos;
    }
    return -1;
}

int fb_ioctl(struct file* file, unsigned long request, void* arg) {
    if (request == FB_IOCTL_GET_INFO) {
        struct framebuffer_info* info = (struct framebuffer_info*)arg;
        info->width = FB_WIDTH;
        info->height = FB_HEIGHT;
        info->bpp = FB_BPP; 
        return 0;
    }
    return -1;
}

int fb_open(struct vnode* file_node, struct file** target) { return 0; }
int fb_close(struct file* file) { return 0; }
int fb_read(struct file* file, void* buf, size_t len) { return -1; }
int fb_lookup(struct vnode* dir_node, struct vnode** target, const char* component_name) { return -1; }
int fb_create(struct vnode* dir_node, struct vnode** target, const char* component_name) { return -1; }
int fb_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name) { return -1; }

struct file_operations fb_f_ops = {
    .open = fb_open, 
    .close = fb_close, 
    .read = fb_read,
    .write = fb_write, 
    .lseek64 = fb_lseek64, 
    .ioctl = fb_ioctl
};

struct vnode_operations fb_v_ops = {
    .lookup = fb_lookup, 
    .create = fb_create, 
    .mkdir = fb_mkdir
};

int framebufferfs_setup_mount(struct filesystem* fs, struct mount* mount) {
    mount->fs = fs;
    struct vnode *node = kmalloc(sizeof(struct vnode));
    node->v_ops = &fb_v_ops;
    node->f_ops = &fb_f_ops;
    node->mount = NULL;
    node->parent = NULL;
    node->internal = NULL;
    mount->root = node;
    return 0;
}