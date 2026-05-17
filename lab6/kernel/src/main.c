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
#include "thread.h"
#include "system_call.h"
#include "ecall_helper.h"
#include "video.h"
#include "vm.h"

struct KernelInfo kernel_info;

void init_process() {
    int ret = exec("osctest.bin");
    if (ret == -1) {
        uart_write("Failed to load osctest.bin\n", 27);
    }
}

void irq_enable() {
    asm volatile("csrsi sstatus, (1 << 1)");
}

void main(uint64_t hartid, void *dtb) {
    dtb = (void*)(PA_TO_VA((unsigned long)dtb));

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
    ecall_helper_commit();
    video_init();

    asm volatile("move tp, %0" : : "r"(kthread_create(thread_idle)));
    // kthread_create(test_kernel_shell);
    // for (int i = 0; i < 3; i++)
    //     kthread_create(thread_foo);
    user_process_create("osctest.bin");
    irq_enable();
    thread_idle();

    // int32_t pid = 1;
    // while(1){
    //     runAShell(++pid);
    // }

}