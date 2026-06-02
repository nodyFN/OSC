#include "vfs.h"
#include "tmpfs.h"
#include "mm.h"
#include "string.h"

struct mount* rootfs;
struct filesystem* registered_fs[10];
int nr_fs = 0;

int register_filesystem(struct filesystem* fs) {
    if (nr_fs >= 10) return -1;
    registered_fs[nr_fs++] = fs;
    return 0;
}

void vfs_init() {
    struct filesystem *tmpfs = kmalloc(sizeof(struct filesystem));
    tmpfs->name = "tmpfs";
    tmpfs->setup_mount = tmpfs_setup_mount;
    register_filesystem(tmpfs);

    rootfs = kmalloc(sizeof(struct mount));
    tmpfs->setup_mount(tmpfs, rootfs);
}

int vfs_open(const char* pathname, int flags, struct file** target) {
    struct vnode* node;
    const char *comp = pathname;
    
    if (comp[0] == '/') comp++;

    int res = rootfs->root->v_ops->lookup(rootfs->root, &node, comp);
    if (res != 0) {
        if (flags & O_CREAT) {
            res = rootfs->root->v_ops->create(rootfs->root, &node, comp);
            if (res != 0) return res;
        } else {
            return -1;
        }
    }

    struct file *f = kmalloc(sizeof(struct file));
    if (!f) return -1;
    f->vnode = node;
    f->f_pos = 0;
    f->f_ops = node->f_ops;
    f->flags = flags;
    
    int open_res = f->f_ops->open(node, &f);
    if (open_res != 0) {
        kfree(f);
        return open_res;
    }
    
    *target = f;
    return 0;
}

int vfs_close(struct file* file) {
    if (!file) return -1;
    int res = file->f_ops->close(file);
    kfree(file);
    return res;
}

int vfs_write(struct file* file, const void* buf, size_t len) {
    if (!file) return -1;
    return file->f_ops->write(file, buf, len);
}

int vfs_read(struct file* file, void* buf, size_t len) {
    if (!file) return -1;
    return file->f_ops->read(file, buf, len);
}