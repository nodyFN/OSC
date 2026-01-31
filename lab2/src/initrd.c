#include "initrd.h"
#include "string.h"
#include "stdio.h"

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

        // uint32_t offset = sizeof(struct cpio_newc_header) + namesize;
        // offset = ALIGN4(offset);
        // offset += filesize;
        // offset = ALIGN4(offset);
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
            // printf("Found file: %s\n", filename);
            // printf("File size: %d bytes\n", filesize);
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