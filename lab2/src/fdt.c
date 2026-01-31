#include "fdt.h"
#include "uart.h"
#include "utils.h"
#include "string.h"
#include "stdio.h"

#include <stddef.h>

// void list_all_nodes(const void *fdt){
//     struct fdt_header *header = (struct fdt_header *)fdt;
//     uint32_t off_dt_struct = toLittleEndian(header->off_dt_struct);
//     uart_puts("DT Struct Offset: ");
//     uart_hex(off_dt_struct);
//     uint32_t* struct_base = (uint32_t *)((uint64_t)fdt + off_dt_struct);
//     uart_puts("struct_base: ");
//     uart_hex((uint32_t)struct_base);
//     uint32_t size_dt_struct = toLittleEndian(header->size_dt_struct);
//     uart_puts("DT Struct Size: ");
//     uart_hex(size_dt_struct);

//     uint32_t* current = struct_base;

//     uint32_t* struct_end = (uint32_t *)((uint32_t)struct_base + size_dt_struct);
//     uart_puts("struct_end: ");
//     uart_hex((uint32_t)struct_end);
//     uart_hex(toLittleEndian(*(struct_end-1)));

//     uint32_t* string_base = (uint32_t *)((uint64_t)fdt + toLittleEndian(header->off_dt_strings));



//     int indent_level = 0;
//     int limit = 10000;
//     int l = 0;
//     while(toLittleEndian(*current) != FDT_END && l < limit){
//         if(toLittleEndian(*current) == FDT_BEGIN_NODE){
//             // uart_puts("Begin Node: \n");
//             for(int i=0; i<indent_level; i++){
//                 uart_puts("  ");
//             }
//             indent_level++;
//             current++;
//             char* name_ptr = (char*)current;
//             int name_len = 1;
//             while(*name_ptr != '\0'){
//                 uart_putc(*name_ptr);
//                 name_ptr++;
//                 name_len++;
//             }
//             if(name_len == 1){
//                 uart_puts("/");

//             }
//             uart_putc('\n');
//             // 計算 padded length
//             int padded_len = (name_len + 3) & ~3;
//             current = (uint32_t *)((uint8_t *)current + padded_len);

//         }else if(toLittleEndian(*current) == FDT_END_NODE){
//             indent_level--;
//             // uart_puts("End Node: \n");
//             current++;

//         }else if(toLittleEndian(*current) == FDT_PROP){
//             // uart_puts("Property: \n");
//             for(int i=0; i<indent_level; i++){
//                 uart_puts("  ");
//             }
//             uart_puts(" ");
//             uint32_t prop_len = toLittleEndian(*(current + 1));
//             uint32_t prop_nameoff = toLittleEndian(*(current + 2));

//             uint32_t prop_name_addr = (uint32_t)string_base + prop_nameoff;
//             char* prop_name = (char *)prop_name_addr;
//             while(*prop_name != '\0'){
//                 uart_putc(*prop_name);
//                 prop_name++;
//             }
//             // uart_puts(": ");

//             current+=3; // 移到 property value 開頭
//             char* prop_value = (char *)current;

//             /*
//             for(int i=0; i < prop_len; i++){
//                 uart_putc(prop_value[i]);
//             }
//             uart_putc('\n');
//             */
//             // === 判斷要印字串還是印 Hex Array ===
//             // 簡單啟發法：檢查第一個字元是否為可列印字元 (且不是空屬性)
//             char *str_ptr = (char*)prop_value;
//             int is_string = 0;
            
//             // 如果長度大於0，且第一個字是英文字母或數字，且最後一個字是 \0，我們就猜它是字串
//             if (prop_len > 0 && 
//                ((str_ptr[0] >= 'a' && str_ptr[0] <= 'z') || 
//                 (str_ptr[0] >= 'A' && str_ptr[0] <= 'Z') ||
//                 str_ptr[0] == '/') && // 為了 compatible 或 device_type
//                str_ptr[prop_len - 1] == '\0') {
//                 is_string = 1;
//             }

//             // === 分支 1: 印字串 (如 compatible = "riscv,virt") ===
//             // === 分支 1: 印字串 (支援 String List) ===
//             if (is_string) {
//                 uart_puts(" = \"");
                
//                 // 使用一個游標 index 來追蹤目前印到哪裡
//                 int str_idx = 0;
//                 while (str_idx < prop_len) {
//                     // 印出目前的字串 (會自動停在 \0)
//                     uart_puts(str_ptr + str_idx);
                    
//                     // 移動游標：目前字串長度 + 1 (那個 \0)
//                     int current_str_len = strlen(str_ptr + str_idx) + 1;
//                     str_idx += current_str_len;
                    
//                     // 如果後面還有東西，代表這是 String List，印出分隔符號 (通常用 ", " 或 space)
//                     if (str_idx < prop_len) {
//                         uart_puts("\", \"");
//                     }
//                 }
//                 uart_puts("\"");
//             }
//             // === 分支 2: 印 Hex Array (如 reg = <0x0 0x80000000>) ===
//             else if (prop_len > 0) {
//                 uart_puts(" = <");
                
//                 // 將指標轉為 uint32_t* 以便一次讀 4 bytes
//                 uint32_t *val_ptr = (uint32_t *)prop_value;
//                 int cell_count = prop_len / 4; // 計算有幾個 cell

//                 for (int k = 0; k < cell_count; k++) {
//                     // 【重點】讀出來並轉 Endian
//                     uint32_t val = toLittleEndian(val_ptr[k]);
                    
//                     uart_puts("0x");
//                     // 注意：這裡不能用你會自動換行的 uart_hex，要用不會換行的版本
//                     uart_hex_no_newline(val); 
                    
//                     // 如果不是最後一個，印個空白隔開
//                     if (k < cell_count - 1) {
//                         uart_puts(" ");
//                     }
//                 }
//                 uart_puts(">");
//             }
//             // 如果 prop_len == 0，代表是 empty property (如 "interrupt-controller;")，什麼都不印

//             uart_puts("\n");
//             // 計算 padded length
//             int padded_len = (prop_len + 3) & ~3;
//             current = (uint32_t *)((uint8_t *)current + padded_len);

//         }else if(toLittleEndian(*current) == FDT_NOP){
//             // uart_puts("Property: \n");
//             current++;
//         }else{
//             uart_puts("Unknown: \n");
//             current++;
//         }
//         // current++;
//         l++;
//     }
//     uart_puts("End of FDT Struct\n");
//     if(l >= limit){
//         uart_puts("Warning: Reached limit of 5000 iterations, stopping early.\n");
//     }
    


// }

void fdt_prop_value_printer(const void* prop_start, const int prop_len){
    char* prop_value = (char *)prop_start;

    // === 判斷要印字串還是印 Hex Array ===
    // 簡單啟發法：檢查第一個字元是否為可列印字元 (且不是空屬性)
    char *str_ptr = (char*)prop_value;
    int is_string = 0;
    
    // 如果長度大於0，且第一個字是英文字母或數字，且最後一個字是 \0，我們就猜它是字串
    if (prop_len > 0 && 
       ((str_ptr[0] >= 'a' && str_ptr[0] <= 'z') || 
        (str_ptr[0] >= 'A' && str_ptr[0] <= 'Z') ||
        str_ptr[0] == '/') && // 為了 compatible 或 device_type
       str_ptr[prop_len - 1] == '\0') {
        is_string = 1;
    }

    // === 分支 1: 印字串 (如 compatible = "riscv,virt") ===
    // === 分支 1: 印字串 (支援 String List) ===
    if (is_string) {
        uart_puts("\"");
        
        // 使用一個游標 index 來追蹤目前印到哪裡
        int str_idx = 0;
        while (str_idx < prop_len) {
            // 印出目前的字串 (會自動停在 \0)
            uart_puts(str_ptr + str_idx);
            
            // 移動游標：目前字串長度 + 1 (那個 \0)
            int current_str_len = strlen(str_ptr + str_idx) + 1;
            str_idx += current_str_len;
            
            // 如果後面還有東西，代表這是 String List，印出分隔符號 (通常用 ", " 或 space)
            if (str_idx < prop_len) {
                uart_puts("\", \"");
            }
        }
        uart_puts("\"");
    }
    // === 分支 2: 印 Hex Array (如 reg = <0x0 0x80000000>) ===
    else if (prop_len > 0) {
        uart_puts("<");
        
        // 將指標轉為 uint32_t* 以便一次讀 4 bytes
        uint32_t *val_ptr = (uint32_t *)prop_value;
        int cell_count = prop_len / 4; // 計算有幾個 cell
        for (int k = 0; k < cell_count; k++) {
            // 【重點】讀出來並轉 Endian
            uint32_t val = toLittleEndian(val_ptr[k]);
            
            // uart_puts("0x");
            // 注意：這裡不能用你會自動換行的 uart_hex，要用不會換行的版本
            // uart_hex_no_newline(val);
            printf("%x", val);

            // 如果不是最後一個，印個空白隔開
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
    // printf("Struct Base Address: %x\n", (uint32_t)struct_base);
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
            // int name_len = 1;
            // printf("/");
            // while(*name_ptr != '\0'){
            //     printf("%c", *name_ptr);
            //     name_ptr++;
            //     name_len++;
            // }
            // printf("\n");
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

            // 計算 padded length
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

            current+=3; // 移到 property value 開頭
            char* prop_value = (char *)current;

            // === 判斷要印字串還是印 Hex Array ===
            // 簡單啟發法：檢查第一個字元是否為可列印字元 (且不是空屬性)
            char *str_ptr = (char*)prop_value;
            int is_string = 0;
            
            // 如果長度大於0，且第一個字是英文字母或數字，且最後一個字是 \0，我們就猜它是字串
            if (prop_len > 0 && 
               ((str_ptr[0] >= 'a' && str_ptr[0] <= 'z') || 
                (str_ptr[0] >= 'A' && str_ptr[0] <= 'Z') ||
                str_ptr[0] == '/') && // 為了 compatible 或 device_type
               str_ptr[prop_len - 1] == '\0') {
                is_string = 1;
            }

            // === 分支 1: 印字串 (如 compatible = "riscv,virt") ===
            // === 分支 1: 印字串 (支援 String List) ===
            if (is_string) {
                printf(" = \"");
                
                // 使用一個游標 index 來追蹤目前印到哪裡
                int str_idx = 0;
                while (str_idx < prop_len) {
                    // 印出目前的字串 (會自動停在 \0)
                    // uart_puts(str_ptr + str_idx);
                    printf("%s", str_ptr + str_idx);
                    
                    // 移動游標：目前字串長度 + 1 (那個 \0)
                    int current_str_len = strlen(str_ptr + str_idx) + 1;
                    str_idx += current_str_len;
                    
                    // 如果後面還有東西，代表這是 String List，印出分隔符號 (通常用 ", " 或 space)
                    if (str_idx < prop_len) {
                        printf("\", \"");
                    }
                }
                printf("\"");
            }
            // === 分支 2: 印 Hex Array (如 reg = <0x0 0x80000000>) ===
            else if (prop_len > 0) {
                printf(" = <");
                
                // 將指標轉為 uint32_t* 以便一次讀 4 bytes
                uint32_t *val_ptr = (uint32_t *)prop_value;
                int cell_count = prop_len / 4; // 計算有幾個 cell

                for (int k = 0; k < cell_count; k++) {
                    // 【重點】讀出來並轉 Endian
                    uint32_t val = toLittleEndian(val_ptr[k]);
                    
                    printf("0x");
                    // 注意：這裡不能用你會自動換行的 uart_hex，要用不會換行的版本
                    uart_hex_no_newline(val); 
                    
                    // 如果不是最後一個，印個空白隔開
                    if (k < cell_count - 1) {
                        printf(" ");
                    }
                }
                printf(">");
            }
            // 如果 prop_len == 0，代表是 empty property (如 "interrupt-controller;")，什麼都不印
            printf("\n");
            // 計算 padded length
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
    // printf("Desired Path: %s\n", path);
    // path: /abc/def/geh
    char current_path[256];
    for(int _=0; _<256; _++){
        current_path[_] = '\0';
    }
    int path_index = 0;
    
    struct fdt_header *header = (struct fdt_header *)fdt;
    uint32_t off_dt_struct = toLittleEndian(header->off_dt_struct);
    uint32_t* struct_base = (uint32_t *)((uint64_t)fdt + off_dt_struct);

    uint32_t* current = struct_base;
    while(toLittleEndian(*current) != FDT_END && strcmp(path, current_path) != 0){
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

            // 【關鍵修正】在這裡直接比對！
            if (strcmp(current_path, path) == 0) {
                // 找到了！計算相對於 fdt 開頭的 offset
                // offset = token_addr (BEGIN_NODE的位置) - fdt (Base)
                // printf("Found Path: %s\n", current_path);
                return (uint64_t)token_addr - (uint64_t)fdt;
            }




            int padded_len = (name_len + 3) & ~3;
            current = (uint32_t *)((uint8_t *)current + padded_len);
            // current++;
            // printf("(Begin) Current Path: %s\n", current_path);
        }else if(toLittleEndian(*current) == FDT_END_NODE){
            current_path[--path_index] = '\0';
            while(path_index >= 0 && current_path[path_index] != '/'){
                current_path[path_index--] = '\0';
            }
            // printf("(End) Current Path: %s\n", current_path);
            current++;
        }else if(toLittleEndian(*current) == FDT_PROP){
            // 1. 讀取長度 (位於 token 後面 4 bytes)
            uint32_t prop_len = toLittleEndian(*(current + 1));
            
            // 2. 跳過 Header (Token + Len + NameOff = 12 bytes = 3 * uint32_t)
            current += 3;

            // 3. 計算並跳過對齊後的資料長度
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
            // printf("In [fdt_getprop] find prop name: %s\n", prop_name);
            if(strcmp(prop_name, name) == 0){
                // 找到屬性了！
                if(lenp){
                    *lenp = prop_len;
                }
                current += 3; // 移到 property value 開頭
                return (const void *)current;
            }

            current+=3; // 移到 property value 開頭
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