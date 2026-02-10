#include "initrd.h"
#include "string.h"
#include "stdio.h"
#include "fdt.h"
#include "utils.h"

uint32_t _string_to_hex32_helper(const char* str){
    uint32_t hex = 0;
    for(int i = 0; i < 8; i++){
        char c = str[i];
        uint32_t value = 0;
        if(c >= '0' && c <= '9'){
            value = c - '0';
        } else if(c >= 'A' && c <= 'F'){
            value = c - 'A' + 10;
        } else if(c >= 'a' && c <= 'f'){
            value = c - 'a' + 10;
        } else if(c == '\0'){
            break;
        } else{
            // Invalid character
            hex = 0;
            break;
        }
        hex = (hex << 4) | value;
    }
    return hex;
}

void initrd_list(const void* initrd_start, const void* initrd_end){
    char* current = (char*)initrd_start;

    while(1) { 
        struct cpio_newc_header* header = (struct cpio_newc_header*)current;
        if(strncmp(header->c_magic, CPIO_NEWC_MAGIC, 6) != 0){
            printf("Error: Invalid CPIO magic at %p\n", current);
            return;
        }

        uint32_t namesize = _string_to_hex32_helper(header->c_namesize);
        uint32_t filesize = _string_to_hex32_helper(header->c_filesize);

        char* filename = current + sizeof(struct cpio_newc_header);
        if(strncmp(filename, CPIO_NEWC_END, sizeof(CPIO_NEWC_END) - 1) == 0){
            break;
        }

        printf("%d ", filesize);
        for(int i=0; i<namesize-1; i++){
            printf("%c", filename[i]);
        }
        printf("\n");

        uint32_t offset = ALIGN4(ALIGN4(sizeof(struct cpio_newc_header) + namesize) + filesize);

        current += offset;
    }
}

void initrd_cat(const void* initrd_start, const void* initrd_end, const char* cat_filename){
    char* current = (char*)initrd_start;

    while(1) { 
        struct cpio_newc_header* header = (struct cpio_newc_header*)current;
        if(strncmp(header->c_magic, CPIO_NEWC_MAGIC, 6) != 0){
            printf("Error: Invalid CPIO magic at %p\n", current);
            return;
        }

        uint32_t namesize = _string_to_hex32_helper(header->c_namesize);
        uint32_t filesize = _string_to_hex32_helper(header->c_filesize);

        char* filename = current + sizeof(struct cpio_newc_header);
        if(strncmp(filename, CPIO_NEWC_END, sizeof(CPIO_NEWC_END) - 1) == 0){
            printf("initrd_cat: %s: No such file\n", cat_filename);
            break;
        }

        if(strcmp(cat_filename, filename) == 0){
            char* file_content = current + ALIGN4(sizeof(struct cpio_newc_header) + namesize);
            for(uint32_t i = 0; i < filesize; i++){
                printf("%c", file_content[i]);
            }
            printf("\n");
            return;
        }


        uint32_t offset = ALIGN4(ALIGN4(sizeof(struct cpio_newc_header) + namesize) + filesize);

        current += offset;
    }
}

int get_initrd_info(const void* dtb_addr, uint64_t* initrd_start_addr, uint64_t* initrd_end_addr){
    int offset = fdt_path_offset(dtb_addr, "/chosen");
    if(offset == -1){
        printf("Path /chosen not found in FDT.\n");
        return -1;
    }else{
        int len = 0;
        const void *prop = fdt_getprop(dtb_addr, offset, "linux,initrd-start", &len);
        if(!prop){
            printf("Property 'linux,initrd-start' not found.\n");
            return -1;
        }
        uint32_t* initrd_start_prop = (uint32_t *)prop;
        *initrd_start_addr = (uint64_t)((uint64_t)toLittleEndian((*initrd_start_prop))<<32 | (uint64_t)toLittleEndian(*(initrd_start_prop+1)));

        prop = fdt_getprop(dtb_addr, offset, "linux,initrd-end", &len);
        if(!prop){
            printf("Property 'linux,initrd-end' not found.\n");
            return -1;
        }
        uint32_t* initrd_end_prop = (uint32_t *)prop;
        *initrd_end_addr = (uint64_t)((uint64_t)toLittleEndian((*initrd_end_prop))<<32 | (uint64_t)toLittleEndian(*(initrd_end_prop+1)));
    } 
    return 0;
}