#include <stdint.h>
#include "uart.h"
#include "stdio.h"
#include "shell.h"
#include "utils.h"
#include "initrd.h"
#include "mm.h"
#include "riscv.h"
#include "string.h"
#include "plic.h" // 💥 記得加上

struct KernelInfo kernel_info;
extern char _stack_top[];
extern void trap_vector();
extern void timer_init();

void user_task() {
    printf("\n[User] Hello from U-mode! Doing some work...\n");
    for (volatile int i = 0; i < 30000000; i++);
    printf("[User] Work done, calling ecall to ask Kernel for help!\n");
    asm volatile("ecall");
    printf("[User] Wow! I'm back from Kernel!\n");
    while(1) {
        for (volatile int i = 0; i < 50000000; i++);
    }
}

void shell_task() {
    printf("\n[User] Hello from U-mode! Doing some work...\n");
    
    // 故意延遲一下
    for (volatile int i = 0; i < 30000000; i++);

    printf("[User] Work done, calling ecall to ask Kernel for help!\n");
    asm volatile("ecall");
    printf("[User] Wow! I'm back from Kernel!\n");

    // 💥 把你的 Shell 搬來這裡！讓它在 U-mode 下執行 💥
    printf("\n[User] Starting Interactive Shell in U-mode...\n");
    int32_t pid = 1;
    while(1) {
        runAShell(++pid);
    }
}

// 💥 防禦編譯器重排的終極標籤
__attribute__((noinline, noreturn)) 
void jump_to_user_mode() {
    write_csr(mscratch, (uint64_t)_stack_top);
    // write_csr(mepc, (uint64_t)user_task);
    write_csr(mepc, (uint64_t)shell_task);

    uint64_t mstatus = read_csr(mstatus);
    mstatus &= ~(3UL << 11); // U-mode
    mstatus |= (1UL << 7);   // MPIE = 1 (切換過去時自動打開中斷)
    write_csr(mstatus, mstatus);

    // 💥 解鎖 PMP，讓 U-mode 能存取記憶體
    asm volatile("csrw pmpaddr0, %0" :: "r"(0x3FFFFFFFFFFFFFULL));
    asm volatile("csrw pmpcfg0, %0" :: "r"(0x1F));

    void *user_stack = kmalloc(4096);
    memset(user_stack, 0, 4096); // 💥 清空垃圾資料

    printf("\n[Kernel] Switching to U-mode using mret...\n");

    // 💥 加上 Memory Barrier，禁止編譯器亂移程式碼
    asm volatile(
        "mv sp, %0\n"
        "mret\n"
        :: "r"((uint64_t)user_stack + 4096)
        : "memory" 
    );
    while(1);
}

void main(uint64_t hartid, void *dtb) {
    uart_init();
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
    
    plic_init(); // 💥 初始化 PLIC 控制器

    mm_init(dtb);
    // // mm_test();

    // int32_t pid = 1;

    // while(1){
    //     runAShell(++pid);
    // }
    write_csr(mtvec, (uint64_t)trap_vector);
    timer_init();

    // 💥 開啟 MEIE (Machine External Interrupt Enable) = bit 11
    set_csr(mie, 1 << 11);

    jump_to_user_mode();

}