#ifndef __FDT_H__
#define __FDT_H__
#include <stdint.h>

#define FDT_MAGIC 0xd00dfeed
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_NOP        0x00000004
#define FDT_END        0x00000009

struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};



int fdt_path_offset(const void *fdt, const char *path);
const void *fdt_getprop(const void *fdt, int nodeoffset, const char *name, int *lenp);
void fdt_prop_value_printer(const void* prop_start, const int prop_len);
void list_all_nodes(const void *fdt);
int fdt_get_memory_info(void *dtb, uint64_t *base, uint64_t *size);
int fdt_get_initrd_range(void *dtb, uint64_t *start, uint64_t *end);
uint32_t fdt_total_size(const void *fdt);
int fdt_reserve_memory(void *dtb);


#endif