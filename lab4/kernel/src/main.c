#include <stdint.h>
#include "uart.h"
#include "stdio.h"
#include "shell.h"
#include "utils.h"
#include "initrd.h"
#include "mm.h"
#include "task.h"
#include "timer.h"
#include "plic.h"
#include "task.h"

struct KernelInfo kernel_info;
void irq_enable() {
    asm volatile("csrsi sstatus, (1 << 1)");
}

void main(uint64_t hartid, void *dtb) {
    uart_init(dtb);

    kernel_info.hartid = hartid;
    kernel_info.dtb_addr = dtb;
    if(get_initrd_info(dtb, &kernel_info.initrd_start_addr, &kernel_info.initrd_end_addr) == -1){
        return;
    }
   
    mm_init(dtb);
    task_init();
    timer_init();
    plic_init();
    irq_enable();
    int32_t pid = 1;

    while(1){
        runAShell(++pid);
    }

}