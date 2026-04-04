#include <stdint.h>
#include "uart.h"
#include "stdio.h"
#include "shell.h"
#include "utils.h"
#include "initrd.h"
#include "mm.h"
#include "timer.h"
#include "plic.h"
#include "task.h"
#include "thread.h"
#include "system_call.h"

void init_process() {
    int ret = exec("osctest.bin");
    if (ret == -1) {
        uart_write("Failed to load osctest.bin\n", 27);
    }
}

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
    task_init();
    timer_init();
    plic_init();
    uart_init();
    irq_enable();

    // mm_test();
    
    uart_async_enabled = 1;

    asm volatile("move tp, %0" : : "r"(kthread_create(thread_idle)));
    user_process_create(init_process);
    thread_idle();
    
    // 3. 【刪除或註解掉舊的 Kernel Shell】
    // int32_t pid = 1;
    // while(1){
    //     runAShell(++pid);
    // }

    

}