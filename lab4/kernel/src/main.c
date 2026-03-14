#include <stdint.h>
#include "uart.h"
#include "stdio.h"
#include "shell.h"
#include "utils.h"
#include "initrd.h"
#include "mm.h"
#include "timer.h"
#include "plic.h"

void irq_enable() {
    asm volatile("csrsi sstatus, (1 << 1)");
}

struct KernelInfo kernel_info;

void main(uint64_t hartid, void *dtb) {
    printf("\nkernel: \n");
    printf(">> [Kernel] Booted successfully!\n");
    printf(">> [Kernel] Hart ID: %lx\n", hartid);
    printf(">> [Kernel] DTB Addr: %lx\n", (uint64_t)dtb);

    kernel_info.hartid = hartid;
    kernel_info.dtb_addr = dtb;
    if(get_initrd_info(dtb, &kernel_info.initrd_start_addr, &kernel_info.initrd_end_addr) == -1){
        printf("[Failed] Failed to get initrd info.\n");
        return;
    }else{
        printf(">> [Kernel] Initrd Start Addr: %lx\n", kernel_info.initrd_start_addr);
        printf(">> [Kernel] Initrd End Addr: %lx\n", kernel_info.initrd_end_addr);
    }
    
    mm_init(dtb);

    timer_init();
    plic_init();
    uart_init();
    irq_enable();

    int32_t pid = 1;

    while(1){
        runAShell(++pid);
    }

}