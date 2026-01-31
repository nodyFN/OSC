
#include <stdint.h>
#include <stddef.h>
#include "uart.h" 
#include "shell.h"
#include "sbi.h"
#include "string.h"
#include "fdt.h"
#include "utils.h"
#include "stdio.h"
#include "initrd.h"
#include "test.h"

void main(unsigned long hartid, void *dtb_addr) {

    uart_init();

    printf("Hello World!\n");
    printf("This is Orange Pi RV2 Lab 2.\n");

    // printf_test(dtb_addr);
    
    
    int32_t pid = 1;

    test_lab1();
    test_lab2(&(struct fdt_test_info){
        .dtb_addr = dtb_addr,
        #ifdef QEMU
            .path = "/soc/virtio_mmio@10004000",
            .prop_name = "reg",
        #else
            .path = "/soc/display-subsystem-dsi",
            .prop_name = "reg",
        #endif
        .list_node = 0
    });

    
    


    while (1) {
        // 這裡不需要一直印 "this is pid 1"，移到 runAShell 裡面比較乾淨
        // uart_puts("this is pid 1\n");
        runAShell(++pid);
        
    }
}