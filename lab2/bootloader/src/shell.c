#include <stdint.h>
#include "shell.h"
#include "uart.h"
#include "string.h"
#include "initrd.h"
#include "fdt.h"
#include "utils.h"
#include "stdio.h"

extern struct KernelInfo kernel_info;
extern void* global_dtb;

extern int should_load_kernel;

void getCommand(char* buffer, int max_len) {
    int cursor_idx = 0;
    int length = 0;
    
    for(int i=0; i<max_len; i++) buffer[i] = '\0';

    while(1) {
        char c = uart_getc();

        if (c == KEY_ESC) {
            char next1 = uart_getc();
            if (next1 == '[') {
                char next2 = uart_getc();
                if (next2 == 'D') { // Left
                    if (cursor_idx > 0) {
                        cursor_idx--;
                        uart_puts("\033[D");
                    }
                } else if (next2 == 'C') { // Right
                    if (cursor_idx < length) {
                        cursor_idx++;
                        uart_puts("\033[C");
                    }
                }
            }
            continue;
        }


        else if (c == KEY_BACKSPACE || c == '\b' || c == 127) {
            if (cursor_idx > 0) {
                
                int delete_idx = cursor_idx - 1;
                
                for (int i = delete_idx; i < length - 1; i++) {
                    buffer[i] = buffer[i + 1];
                }
                
                length--;
                buffer[length] = '\0';
                cursor_idx--;

                
                uart_putc('\b');

                for (int i = cursor_idx; i < length; i++) {
                    uart_putc(buffer[i]);
                }

                uart_putc(' ');

                for (int i = 0; i < (length - cursor_idx) + 1; i++) {
                    uart_putc('\b');
                }
            }
        } 
        
        else if (c == KEY_ENTER || c == '\r' || c == '\n') {
            uart_puts("\r\n");
            buffer[length] = '\0';
            break;
        } 
        
        else {
            if (length < max_len - 1) {

                for (int i = length; i > cursor_idx; i--) {
                    buffer[i] = buffer[i - 1];
                }

                buffer[cursor_idx] = c;
                
                length++;
                buffer[length] = '\0'; 

                uart_putc(c); 
                
                for (int i = cursor_idx + 1; i < length; i++) {
                    uart_putc(buffer[i]);
                }

                int distance_to_back = length - (cursor_idx + 1);
                
                for (int i = 0; i < distance_to_back; i++) {
                     uart_puts("\033[D"); 
                }

                cursor_idx++;
            }
        }
    }
}

void processCommand(shell_t* shell) {
    if(strcmp(shell->command, "load") == 0){
        should_load_kernel = 1;
    }else if(strcmp(shell->command, "info") == 0){
        uint64_t ram_base = 0, ram_size = 0;
        uint64_t initrd_start = 0, initrd_end = 0;

        if (fdt_get_memory_info(global_dtb, &ram_base, &ram_size) == -1) {
            printf("[Failed] Failed to parse memory node.\n");
        }

        printf("Detected RAM: Base = 0x%lx, Size = 0x%lx\n", ram_base, ram_size);

        if (fdt_get_initrd_range(global_dtb, &initrd_start, &initrd_end) != -1) {
            printf("Detected Initrd: Start = 0x%lx, End = 0x%lx\n", initrd_start, initrd_end);
        }
    }else if(strcmp(shell->command, "ls") == 0) {
        uint64_t initrd_start_addr = 0, initrd_end_addr = 0;
        if(get_initrd_info(global_dtb, &initrd_start_addr, &initrd_end_addr) == -1){
            printf("[Failed] Failed to get initrd info.\n");
            return;
        }
        initrd_list((void*)initrd_start_addr, (void*)initrd_end_addr);
    }else if(strncmp(shell->command, "cat ", 4) == 0) {
        uint64_t initrd_start_addr = 0, initrd_end_addr = 0;
        if(get_initrd_info(global_dtb, &initrd_start_addr, &initrd_end_addr) == -1){
            printf("[Failed] Failed to get initrd info.\n");
            return;
        }
        char* filename = shell->command + 4;
        initrd_cat((void*)initrd_start_addr, (void*)initrd_end_addr, filename);
    }else{
        uart_puts("Command not found: ");
        uart_puts(shell->command);
        uart_puts("\n");
    }
}

void runAShell(int32_t pid) {
    
    char* prompt_start = "bootloader> ";
    shell_t shell; 
    char command_buffer[128]; 
    shell.pid = pid;
    uart_puts(prompt_start);

    getCommand(command_buffer, 128);
    

    shell.command = command_buffer; 

    return processCommand(&shell); 
}