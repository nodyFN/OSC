#include "relocate.h"
#include "fdt.h"
#include "string.h"
#include "stdio.h"
#include "utils.h"

void relocate(void* fdt) {
    unsigned long bl_size = (unsigned long)_bl_end - (unsigned long)_bl_start;

    int offset;
    #ifdef QEMU
        offset = fdt_path_offset(fdt, "/memory@80000000");
    #else
        offset = fdt_path_offset(fdt, "/memory@0");
    #endif
    if(offset == -1){
        printf("Path /memory@80000000 not found in FDT.\n");
        return;

    }
    int prop_len = 0;
    uint32_t* prop = (uint32_t*)fdt_getprop(fdt, offset, "reg", &prop_len);
    uint64_t ram_base = ((uint64_t)toLittleEndian(prop[0]) << 32) | (uint64_t)toLittleEndian(prop[1]);
    uint64_t ram_size = ((uint64_t)toLittleEndian(prop[2]) << 32) | (uint64_t)toLittleEndian(prop[3]);

    printf("ram_base: 0x%lp, ram_size: 0x%lp\n", ram_base, ram_size);
    printf("bl_size: 0x%lp\n", bl_size);


}