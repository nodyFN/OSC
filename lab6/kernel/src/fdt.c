#include "fdt.h"
#include "uart.h"
#include "utils.h"
#include "string.h"
#include "stdio.h"
#include "mm.h"

#include <stddef.h>

void remove_at(const char str[256], char nstr[256]){
    int idx = 0;
    int len = strlen(str);
    int b = 1;
    for(int i=0;i<len;i++){
        if(str[i] == '@'){
            b = 0;
        }else if(str[i] == '/'){
            b = 1;
        }
        if(b){
            nstr[idx] = str[i];
            idx++;
        }
    }
    nstr[idx] = '\0';
}

void fdt_prop_value_printer(const void* prop_start, const int prop_len){
    char* prop_value = (char *)prop_start;

    char *str_ptr = (char*)prop_value;
    int is_string = 0;
    
    if (prop_len > 0 && 
       ((str_ptr[0] >= 'a' && str_ptr[0] <= 'z') || 
        (str_ptr[0] >= 'A' && str_ptr[0] <= 'Z') ||
        str_ptr[0] == '/') &&
       str_ptr[prop_len - 1] == '\0') {
        is_string = 1;
    }

    if (is_string) {
        uart_puts("\"");
        int str_idx = 0;
        while (str_idx < prop_len) {
            uart_puts(str_ptr + str_idx);

            int current_str_len = strlen(str_ptr + str_idx) + 1;
            str_idx += current_str_len;

            if (str_idx < prop_len) {
                uart_puts("\", \"");
            }
        }
        uart_puts("\"");
    }

    else if (prop_len > 0) {
        uart_puts("<");

        uint32_t *val_ptr = (uint32_t *)prop_value;
        int cell_count = prop_len / 4; 
        for (int k = 0; k < cell_count; k++) {
            uint32_t val = toLittleEndian(val_ptr[k]);
            printf("%x", val);
            if (k < cell_count - 1) {
                uart_puts(" ");
            }
        }
        uart_puts(">");
    }
    printf("\n");
}

void list_all_nodes(const void *fdt){
    printf("========================== FDT Nodes List ==========================\n");

    printf("FDT Address: %lp\n", (uint64_t)fdt);

    struct fdt_header *header = (struct fdt_header *)fdt;
    printf("Magic Number: %x\n", toLittleEndian(header->magic));

    uint32_t off_dt_struct = toLittleEndian(header->off_dt_struct);
    printf("DT Struct Offset: %x\n", off_dt_struct);

    uint32_t* struct_base = (uint32_t *)((uint64_t)fdt + off_dt_struct);
    printf("Struct Base Address: %p\n", struct_base);
    
    uint32_t size_dt_struct = toLittleEndian(header->size_dt_struct);
    printf("Size of DT Struct: %x\n", size_dt_struct);

    uint32_t* string_base = (uint32_t *)((uint64_t)fdt + toLittleEndian(header->off_dt_strings));
    // printf("String Base Address: %x\n", (uint32_t)string_base);
    printf("String Base Address: %p\n", string_base);


    char current_path[256];
    for(int _=0; _<256; _++){
        current_path[_] = '\0';
    }
    int path_index = 0;

    uint32_t* current = struct_base;
    int indent_level = 0;
    while(toLittleEndian(*current) != FDT_END){
        if(toLittleEndian(*current) == FDT_BEGIN_NODE){
            printf("\n");
            for(int i=0; i<indent_level; i++){
                printf("    ");
            }
            indent_level++;

            current++;
            char* name_ptr = (char*)current;
            int name_len = 1;
            if(*name_ptr != '\0'){
                current_path[path_index] = '/';
                path_index++;
            }
            while(*name_ptr != '\0'){
                current_path[path_index] = *name_ptr;
                path_index++;
                name_ptr++;
                name_len++;
            }

            printf("%s\n", current_path);

            int padded_len = (name_len + 3) & ~3;
            current = (uint32_t *)((uint8_t *)current + padded_len);

        }else if(toLittleEndian(*current) == FDT_END_NODE){
            current_path[--path_index] = '\0';
            while(path_index >= 0 && current_path[path_index] != '/'){
                current_path[path_index--] = '\0';
            }
            indent_level--;
            current++;
        }else if(toLittleEndian(*current) == FDT_PROP){
            for(int i=0; i<indent_level-1; i++){
                printf("    ");
            }
            printf("  ");
            uint32_t prop_len = toLittleEndian(*(current + 1));
            uint32_t prop_nameoff = toLittleEndian(*(current + 2));

            char* prop_name = (char *)string_base + prop_nameoff;
            while(*prop_name != '\0'){
                printf("%c", *prop_name);
                prop_name++;
            }

            current+=3;
            char* prop_value = (char *)current;

            char *str_ptr = (char*)prop_value;
            int is_string = 0;

            if (prop_len > 0 && 
               ((str_ptr[0] >= 'a' && str_ptr[0] <= 'z') || 
                (str_ptr[0] >= 'A' && str_ptr[0] <= 'Z') ||
                str_ptr[0] == '/') &&
               str_ptr[prop_len - 1] == '\0') {
                is_string = 1;
            }

            if (is_string) {
                printf(" = \"");

                int str_idx = 0;
                while (str_idx < prop_len) {
                    printf("%s", str_ptr + str_idx);
                    int current_str_len = strlen(str_ptr + str_idx) + 1;
                    str_idx += current_str_len;

                    if (str_idx < prop_len) {
                        printf("\", \"");
                    }
                }
                printf("\"");
            }

            else if (prop_len > 0) {
                printf(" = <");
                
                uint32_t *val_ptr = (uint32_t *)prop_value;
                int cell_count = prop_len / 4;

                for (int k = 0; k < cell_count; k++) {

                    uint32_t val = toLittleEndian(val_ptr[k]);
                    
                    printf("0x");
                    uart_hex_no_newline(val); 

                    if (k < cell_count - 1) {
                        printf(" ");
                    }
                }
                printf(">");
            }

            printf("\n");

            int padded_len = (prop_len + 3) & ~3;
            current = (uint32_t *)((uint8_t *)current + padded_len);

        }else if(toLittleEndian(*current) == FDT_NOP){
            current++;
        }else{
            printf("Unknown: \n");
            current++;
        }
    }
    printf("==================================================================\n");
}

int fdt_path_offset(const void *fdt, const char *path){
    char r_path[256];
    remove_at(path, r_path);

    char current_path[256];
    char r_current_path[256];
    for(int _=0; _<256; _++){
        current_path[_] = '\0';
    }
    remove_at(current_path, r_current_path);
    int path_index = 0;
    
    struct fdt_header *header = (struct fdt_header *)fdt;
    uint32_t off_dt_struct = toLittleEndian(header->off_dt_struct);
    uint32_t* struct_base = (uint32_t *)((uint64_t)fdt + off_dt_struct);

    uint32_t* current = struct_base;
    while(toLittleEndian(*current) != FDT_END && strcmp(r_path, r_current_path) != 0){
        if(toLittleEndian(*current) == FDT_BEGIN_NODE){
            uint32_t* token_addr = current;

            current++;
            char* name_ptr = (char*)current;
            int name_len = 1;
            if(*name_ptr != '\0'){
                current_path[path_index] = '/';
                path_index++;
            }
            while(*name_ptr != '\0'){
                current_path[path_index] = *name_ptr;
                path_index++;
                name_ptr++;
                name_len++;
            }
            remove_at(current_path, r_current_path);

            // if (strcmp(current_path, path) == 0) {
            //     return (uint64_t)token_addr - (uint64_t)fdt;
            // }
            if (strcmp(r_current_path, r_path) == 0) {
                // printf("Got path: %s\n", r_path);
                return (uint64_t)token_addr - (uint64_t)fdt;
            }else{
                // printf("Current: %s, Want: %s\n", r_current_path, r_path);
            }




            int padded_len = (name_len + 3) & ~3;
            current = (uint32_t *)((uint8_t *)current + padded_len);

        }else if(toLittleEndian(*current) == FDT_END_NODE){
            current_path[--path_index] = '\0';
            while(path_index >= 0 && current_path[path_index] != '/'){
                current_path[path_index--] = '\0';
            }
            current++;
        }else if(toLittleEndian(*current) == FDT_PROP){

            uint32_t prop_len = toLittleEndian(*(current + 1));
            
            current += 3;

            int padded_len = (prop_len + 3) & ~3;
            current = (uint32_t *)((uint8_t *)current + padded_len);
        }else if(toLittleEndian(*current) == FDT_NOP){
            current++;
        }else{
            printf("Unknown: \n");
            current++;
        }
    }
    return -1;
}

const void *fdt_getprop(const void *fdt, int nodeoffset, const char *name, int *lenp){
    struct fdt_header *header = (struct fdt_header *)fdt;
    uint32_t* string_base = (uint32_t *)((uint64_t)fdt + toLittleEndian(header->off_dt_strings));

    uint32_t* current =  (uint32_t*)(fdt + nodeoffset);
    int node_level = 0;
    while(!(node_level == 1 && toLittleEndian(*current) == FDT_END_NODE)){
        if(toLittleEndian(*current) == FDT_BEGIN_NODE){
            node_level++;
            current++;
        }else if(toLittleEndian(*current) == FDT_END_NODE){
            node_level--;
            current++;
        }else if(toLittleEndian(*current) == FDT_PROP && node_level == 1){
            uint32_t prop_len = toLittleEndian(*(current + 1));
            uint32_t prop_nameoff = toLittleEndian(*(current + 2));

            char* prop_name = (char *)string_base + prop_nameoff;

            if(strcmp(prop_name, name) == 0){

                if(lenp){
                    *lenp = prop_len;
                }
                current += 3;
                return (const void *)current;
            }

            current+=3;
            int padded_len = (prop_len + 3) & ~3;
            current = (uint32_t *)((uint8_t *)current + padded_len);
        }else if(toLittleEndian(*current) == FDT_NOP){
            current++;
        }else{
            current++;
        }
    }
    return NULL;
}

int fdt_get_memory_info(void *dtb, uint64_t *base, uint64_t *size) {
    int mem_offset = fdt_path_offset(dtb, "/memory");
    if (mem_offset < 0) {
        printf("[Failed] Path /memory@... not found in FDT.\n");
        return -1;
    }

    int len;
    uint32_t* prop = (uint32_t*)fdt_getprop(dtb, mem_offset, "reg", &len);
    if (!prop) return -1;

    *base = ((uint64_t)toLittleEndian((*prop))<<32 | (uint64_t)toLittleEndian(*(prop+1)));
    *size = ((uint64_t)toLittleEndian(*(prop+2))<<32 | (uint64_t)toLittleEndian(*(prop+3)));

    return 0;
}

int fdt_get_initrd_range(void *dtb, uint64_t *start, uint64_t *end){
    int offset_initrd = fdt_path_offset(dtb, "/chosen");
    if(offset_initrd == -1){
        printf("[Failed] Path /chosen not found in FDT.\n");
        return -1;
    }

    int len = 0;
    const void *prop = fdt_getprop(dtb, offset_initrd, "linux,initrd-start", &len);
    if(!prop){
        printf("[Failed] Property 'linux,initrd-start' not found.\n");
        return -1;
    }
    uint32_t* initrd_start_prop = (uint32_t *)prop;
    *start = ((uint64_t)toLittleEndian((*initrd_start_prop))<<32 | (uint64_t)toLittleEndian(*(initrd_start_prop+1)));

    prop = fdt_getprop(dtb, offset_initrd, "linux,initrd-end", &len);
    if(!prop){
        printf("[Failed] Property 'linux,initrd-end' not found.\n");
        return -1;
    }
    uint32_t* initrd_end_prop = (uint32_t *)prop;
    *end = ((uint64_t)toLittleEndian((*initrd_end_prop))<<32 | (uint64_t)toLittleEndian(*(initrd_end_prop+1)));

    return 0;
}

uint32_t fdt_total_size(const void *fdt){
    struct fdt_header *header = (struct fdt_header *)fdt;
    return toLittleEndian(header->totalsize);
}

int fdt_reserve_memory(void *dtb) {
    int res_mem_offset = fdt_path_offset(dtb, "/reserved-memory");
    if (res_mem_offset < 0) {
        printf("[FDT] Node /reserved-memory not found. Skip dynamic reservation.\n");
        return -1;
    }

    struct fdt_header *header = (struct fdt_header *)dtb;
    uint32_t* string_base = (uint32_t *)((uint64_t)dtb + toLittleEndian(header->off_dt_strings));
    
    uint32_t* current = (uint32_t*)((uint64_t)dtb + res_mem_offset);

    int depth = 0;
    uint32_t address_cells = 2; 
    uint32_t size_cells = 2;

    while (1) {
        uint32_t tag = toLittleEndian(*current);

        if (tag == FDT_BEGIN_NODE) {
            depth++;
            current++;
            
            char* name_ptr = (char*)current;
            int name_len = strlen(name_ptr) + 1;
            int padded_len = (name_len + 3) & ~3;
            current = (uint32_t *)((uint8_t *)current + padded_len);

        } else if (tag == FDT_END_NODE) {
            depth--;
            current++;

            if (depth == 0) {
                break;
            }

        } else if (tag == FDT_PROP) {
            uint32_t prop_len = toLittleEndian(*(current + 1));
            uint32_t prop_nameoff = toLittleEndian(*(current + 2));
            char* prop_name = (char *)string_base + prop_nameoff;

            current += 3;
            void *prop_value = current;

            if (depth == 1 && strcmp(prop_name, "#address-cells") == 0) {
                address_cells = toLittleEndian(*(uint32_t*)prop_value);
            } else if (depth == 1 && strcmp(prop_name, "#size-cells") == 0) {
                size_cells = toLittleEndian(*(uint32_t*)prop_value);
            } else if (depth == 2 && strcmp(prop_name, "reg") == 0) {
                uint32_t *val_ptr = (uint32_t*)prop_value;
                uint64_t start = 0, size = 0;

                if (address_cells == 2) {
                    start = ((uint64_t)toLittleEndian(val_ptr[0]) << 32) | toLittleEndian(val_ptr[1]);
                    val_ptr += 2;
                } else {
                    start = toLittleEndian(val_ptr[0]);
                    val_ptr += 1;
                }

                if (size_cells == 2) {
                    size = ((uint64_t)toLittleEndian(val_ptr[0]) << 32) | toLittleEndian(val_ptr[1]);
                } else {
                    size = toLittleEndian(val_ptr[0]);
                }

                printf("[Reserve] /reserved-memory chunk: 0x%lx - 0x%lx (Size: 0x%lx bytes)\n", start, start + size, size);
                
                memory_reserve(start, start + size);
            }

            int padded_len = (prop_len + 3) & ~3;
            current = (uint32_t *)((uint8_t *)prop_value + padded_len);

        } else if (tag == FDT_NOP) {
            current++;
        } else if (tag == FDT_END) {
            break;
        } else {
            current++;
        }
    }
    return 0;
}