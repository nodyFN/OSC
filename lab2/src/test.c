#include <stddef.h>
#include "test.h"
#include "sbi.h"
#include "stdio.h"
#include "fdt.h"
#include "utils.h"
#include "initrd.h"
#include "relocate.h"


void test_lab1(){
    printf("\n============================== Lab 1 Test Start ==============================\n");

    // 1. 取得 SBI Spec 版本 (Base Ext 0x10, FID 0)
    struct sbiret version_ret = sbi_ecall(0x10, 0, 0, 0, 0, 0, 0, 0);
    printf("SBI Version: %lx\n", (uint64_t)version_ret.value);

    // 2. 探測 Timer Extension (Lab 要求)
    long has_timer = sbi_probe_extension(0x54494D45);
    printf("Probe Timer: %lx\n", (uint64_t)has_timer);

    // 3. 探測 Shutdown Extension (Lab 要求)
    long has_shutdown = sbi_probe_extension(0x53525354);
    printf("Probe Shutdown: %lx\n", (uint64_t)has_shutdown);

    printf("============================== Lab 1 Test End ==============================\n");
}

void test_lab2(const struct fdt_test_info* info){
    const void* dtb_addr = info->dtb_addr;
    const char* path = info->path;
    const char* prop_name = info->prop_name;
    const int list_node = info->list_node;

    if (dtb_addr == NULL || path == NULL || prop_name == NULL) {
        printf("Invalid input for test_lab2.\n");
        return;
    }

    printf("\n============================== Lab 2 Test Start ==============================\n");

    int basic_info_test = 0;
    int fdt_test = 1;
    int initrd_test = 1;
    int relocate_test = 1;

    if(basic_info_test){
        printf("DTB is at: %lp\n", (uint64_t)dtb_addr);

        struct fdt_header *header = (struct fdt_header *)dtb_addr;
        uint32_t magic = toLittleEndian(header->magic);
        
        printf("FDT Magic: %x\n", magic);

        uint32_t version = toLittleEndian(header->version);
        printf("FDT Version: %x\n", version);
    }
    

    if(list_node){
        list_all_nodes(dtb_addr);
    }

    if(fdt_test){
        int offset = fdt_path_offset(dtb_addr, path);
        if(offset != -1){
            // printf("Offset of %s: %d\n", path, offset);
            int len = 0;
            const void *prop = fdt_getprop(dtb_addr, offset, prop_name, &len);
            if (prop) {
                printf("%s = ", prop_name);
                fdt_prop_value_printer(prop, len);
            } else {
                printf("Property '%s' not found.\n", prop_name);
            }
        }else{
            printf("Path %s not found in FDT.\n", path);
        }
    }
    

    if(initrd_test){   
        int offset = fdt_path_offset(dtb_addr, "/chosen");
        if(offset == -1){
            printf("Path /chosen not found in FDT.\n");
        }else{
            int len = 0;
            const void *prop = fdt_getprop(dtb_addr, offset, "linux,initrd-start", &len);
            if(!prop){
                printf("Property 'linux,initrd-start' not found.\n");
                return;
            }
            uint32_t* initrd_start_prop = (uint32_t *)prop;
            uint64_t* initrd_start_addr = (uint64_t*)((uint64_t)toLittleEndian((*initrd_start_prop))<<32 | (uint64_t)toLittleEndian(*(initrd_start_prop+1)));
            // printf("Initrd Start Address: %lp\n", initrd_start_addr);

            prop = fdt_getprop(dtb_addr, offset, "linux,initrd-end", &len);
            if(!prop){
                printf("Property 'linux,initrd-end' not found.\n");
                return;
            }
            uint32_t* initrd_end_prop = (uint32_t *)prop;
            uint64_t* initrd_end_addr = (uint64_t*)((uint64_t)toLittleEndian((*initrd_end_prop))<<32 | (uint64_t)toLittleEndian(*(initrd_end_prop+1)));
            // printf("Initrd End Address: %lp\n", initrd_end_addr);

            initrd_list((void*)initrd_start_addr, (void*)initrd_end_addr);
            printf("\n");
            initrd_cat((void*)initrd_start_addr, (void*)initrd_end_addr, "d.txt");
            initrd_cat((void*)initrd_start_addr, (void*)initrd_end_addr, "e.txt");
        }
    }

    if(relocate_test){
        relocate((void*)dtb_addr);
    }

    printf("============================== Lab 2 Test End ==============================\n");
}
