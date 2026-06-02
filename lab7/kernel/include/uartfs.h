#ifndef __UARTFS_H__
#define __UARTFS_H__

#include "vfs.h"

int uartfs_setup_mount(struct filesystem* fs, struct mount* mount);

#endif