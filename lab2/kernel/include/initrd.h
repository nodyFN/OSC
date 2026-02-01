#ifndef __INITRD_H__
#define __INITRD_H__

#define CPIO_NEWC_MAGIC "070701"
#define CPIO_NEWC_END "TRAILER!!!"
#define ALIGN4(n) (((n) + 3) & ~3)

#include <stdint.h>

struct cpio_newc_header {
    char c_magic[6];
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];
    char c_check[8];
};

void initrd_list(const void* initrd_start, const void* initrd_end);
void initrd_cat(const void* initrd_start, const void* initrd_end, const char* cat_filename);
uint32_t _string_to_hex32_helper(const char* str);
int get_initrd_info(const void* dtb_addr, uint64_t* initrd_start_addr, uint64_t* initrd_end_addr);


#endif