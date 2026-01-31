
#include <stdint.h>
#include "uart.h" 
#include "shell.h"
#include "sbi.h"
#include "string.h"


void main() {
    uart_init();
    uart_puts("Hello World!\n");
    uart_puts("This is Orange Pi RV2 Lab 1.\n");
    
    int32_t pid = 1;

    struct sbiret version_ret = sbi_ecall(0x10, 0, 0, 0, 0, 0, 0, 0);
    uart_puts("SBI Version: ");
    uart_hex(version_ret.value);

    long has_timer = sbi_probe_extension(0x54494D45);
    uart_puts("Probe Set Timer: ");
    uart_hex(has_timer); 

    long has_shutdown = sbi_probe_extension(0x53525354);
    uart_puts("Probe Shutdown:  ");
    uart_hex(has_shutdown);

    while (1) {
        runAShell(++pid);
        
    }
}