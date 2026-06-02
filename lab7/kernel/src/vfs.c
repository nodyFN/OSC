#include "vfs.h"
#include "tmpfs.h"
#include "mm.h"
#include "string.h"
#include "thread.h"

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

static int get_parent_and_basename(const char* pathname, struct vnode** parent, char* basename) {
    char path_copy[256];
    int len = strlen(pathname);
    if (len >= 256) return -1;
    memcpy(path_copy, pathname, len + 1);
    
    int last_slash = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (path_copy[i] == '/') {
            last_slash = i;
            break;
        }
    }
    
    if (last_slash == -1) { 
        *parent = rootfs->root;
        memcpy(basename, pathname, len + 1);
        return 0;
    } else if (last_slash == 0) {
        *parent = rootfs->root;
        memcpy(basename, pathname + 1, len);
        return 0;
    } else {
        path_copy[last_slash] = '\0';
        memcpy(basename, pathname + last_slash + 1, len - last_slash);
        return vfs_lookup(path_copy, parent);
    }
}

int vfs_open(const char* pathname, int flags, struct file** target) {
    struct vnode* node;
    int res = vfs_lookup(pathname, &node);
    
    if (res != 0) {
        if (flags & O_CREAT) {
            struct vnode* parent;
            char basename[16];
            res = get_parent_and_basename(pathname, &parent, basename);
            if (res != 0) return res;

            res = parent->v_ops->create(parent, &node, basename);
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

int vfs_lookup(const char* pathname, struct vnode** target) {
    struct task_struct* curr_task = get_current();
    struct vnode* curr = curr_task->curr_dir;

    if (pathname[0] == '/') {
        curr = rootfs->root;
    }

    const char* p = pathname;
    while (*p) {
        while (*p == '/') p++;
        if (*p == '\0') break;

        char comp[16] = {0};
        int i = 0;
        while (*p && *p != '/') {
            if (i < 15) comp[i++] = *p;
            p++;
        }
        comp[i] = '\0';

        if (strcmp(comp, ".") == 0) {
            continue;
        } else if (strcmp(comp, "..") == 0) {
            if (curr->parent) {
                curr = curr->parent;
            }
            continue;
        }

        if (curr->mount) {
            curr = curr->mount->root;
        }
        
        struct vnode* next_node;
        int res = curr->v_ops->lookup(curr, &next_node, comp);
        if (res != 0) return res;

        curr = next_node;
    }

    if (curr->mount) {
        curr = curr->mount->root;
    }

    *target = curr;
    return 0;
}

int vfs_mkdir(const char* pathname) {
    struct vnode* parent;
    char basename[16];
    
    int res = get_parent_and_basename(pathname, &parent, basename);
    if (res != 0) return res;
    
    struct vnode* new_dir;
    return parent->v_ops->mkdir(parent, &new_dir, basename);
}

int vfs_mount(const char* target, const char* filesystem) {
    struct vnode* target_node;
    int res = vfs_lookup(target, &target_node);
    if (res != 0) return res;
    
    struct filesystem* fs = NULL;
    for (int i = 0; i < nr_fs; i++) {
        if (strcmp(registered_fs[i]->name, filesystem) == 0) {
            fs = registered_fs[i];
            break;
        }
    }
    if (!fs) return -1;
    
    struct mount* mnt = kmalloc(sizeof(struct mount));
    res = fs->setup_mount(fs, mnt);
    if (res != 0) {
        kfree(mnt);
        return res;
    }
    
    mnt->root->parent = target_node->parent; 
    target_node->mount = mnt;
    return 0;
}

int sys_chdir(const char *path) {
    struct vnode* target;
    int res = vfs_lookup(path, &target);
    if (res == 0) {
        get_current()->curr_dir = target;
    }
    return res;
}