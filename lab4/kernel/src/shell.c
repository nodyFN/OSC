#include <stdint.h>
#include "shell.h"
#include "uart.h"
#include "string.h"
#include "sbi.h"
#include "initrd.h"
#include "fdt.h"
#include "utils.h"
#include "stdio.h"
#include "timer.h"

extern struct KernelInfo kernel_info;

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
        
        else if (c == KEY_ENTER) {
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
    if(strcmp(shell->command, "help") == 0){
        printf("Available commands:\n");
        printf("  help  - show all commands.\n");
        printf("  hello - print Hello world.\n");
        printf("  info  - print system info.\n");
    }else if(strcmp(shell->command, "hello") == 0){
        printf("Hello world.\n");
    }else if(strcmp(shell->command, "info") == 0){
        printf("System information:\n");
        printf("   OpenSBI specification version: %lx\n", sbi_get_spec_version());
        printf("   implementation ID: %lx\n", sbi_get_impl_id());
        printf("   implementation version: %lx\n", sbi_get_impl_version());
    }else if(strcmp(shell->command, "ls") == 0) {
        initrd_list((void*)kernel_info.initrd_start_addr, (void*)kernel_info.initrd_end_addr);
    }else if(strncmp(shell->command, "cat ", 4) == 0) {
        char* filename = shell->command + 4;
        initrd_cat((void*)kernel_info.initrd_start_addr, (void*)kernel_info.initrd_end_addr, filename);
    }else if(strncmp(shell->command, "run ", 4) == 0){
        char* filename = shell->command + 4;
        initrd_exec((void*)kernel_info.initrd_start_addr, (void*)kernel_info.initrd_end_addr, filename);
    }else if(strncmp(shell->command, "setTimeout ", 11) == 0){
        int second = 0;
        char* c = shell->command + 11;
        while(*c != ' '){
            second = second * 10 + (*c - '0');
            c++;
        }
        c++;
        char* message_buffer = kmalloc(256);
        memset(message_buffer, 0, 256);
        memcpy(message_buffer, c, 256 - (c - shell->command));
        add_timer(one_shot_alert_callback, (void*)message_buffer, second);
    }else{
        printf("Unknown command: %s\n", shell->command);
        printf("Use help to get commands.\n");
    }


}

void runAShell(int32_t pid) {
    
    char* prompt_start = "OSClab> ";
    shell_t shell; 
    char command_buffer[128]; 
    shell.pid = pid;
    uart_puts(prompt_start);

    getCommand(command_buffer, 128);
    

    shell.command = command_buffer; 

    processCommand(&shell); 
}