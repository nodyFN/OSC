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
#include "sbi.h" // 💥 記得加上

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

__attribute__((noinline, noreturn)) 
void jump_to_user_mode() {
    // 💥 雙平台統一：使用 S-mode 暫存器，不設定 PMP
    write_csr(sscratch, (uint64_t)_stack_top);
    write_csr(sepc, (uint64_t)shell_task);

    uint64_t status = read_csr(sstatus);
    status &= ~(1UL << 8);  // SPP = 0 (降級到 U-mode)
    status |= (1UL << 5);   // SPIE = 1
    write_csr(sstatus, status);

    void *user_stack = kmalloc(4096);
    memset(user_stack, 0, 4096); 

    printf("\n[Kernel] Switching to U-mode...\n");

    asm volatile(
        "mv sp, %0\n"
        "sret\n"   // 💥 雙平台統一：sret
        :: "r"((uint64_t)user_stack + 4096)
        : "memory" 
    );
    while(1);
}

void check_delegation() {
    // 試圖寫入 SEIE (bit 9)，這是外部中斷的開關
    set_csr(sie, 1 << 9); 
    
    // 再讀出來看看
    uint64_t current_sie = read_csr(sie);
    
    if (current_sie & (1 << 9)) {
        printf(">> [Check] SEIP is delegated! (Bit 9 is writable)\n");
    } else {
        printf(">> [Check] SEIP is NOT delegated. (Bit 9 is stuck at 0)\n");
    }
}

void main(uint64_t hartid, void *dtb) {
    uart_init();
    // for (volatile int i = 0; i < 5000000; i++);
    printf("\nkernel: \n");
    printf(">> [Kernel] Booted successfully!\n");
    printf(">> [Kernel] Hart ID: %lx\n", hartid);
    printf(">> [Kernel] DTB Addr: %lx\n", (uint64_t)dtb);

    struct sbiret version_ret = sbi_ecall(0x10, 0, 0, 0, 0, 0, 0, 0);
    uart_puts("SBI Version: ");
    uart_hex(version_ret.value);

    check_delegation();

    kernel_info.hartid = hartid;
    kernel_info.dtb_addr = dtb;
    if(get_initrd_info(dtb, &kernel_info.initrd_start_addr, &kernel_info.initrd_end_addr) == -1){
        printf("[Failed] Failed to get initrd info.\n");
        return;
    }else{
        printf(">> [Kernel] Initrd Start Addr: %lx\n", kernel_info.initrd_start_addr);
        printf(">> [Kernel] Initrd End Addr: %lx\n", kernel_info.initrd_end_addr);
    }
    
    // plic_init(); // 💥 初始化 PLIC 控制器

    mm_init(dtb);

    // 💥 雙平台統一：設定 stvec、初始化 PLIC 與 Timer
    write_csr(stvec, (uint64_t)trap_vector);
    plic_init(); 
    timer_init();

    printf("Testing Software-triggered Interrupt...\n");
    set_csr(sip, 1 << 9); // 手動立起 SEIP bit
    
    // 💥 雙平台統一：開啟 S-mode 全域中斷 (SIE) 與外部中斷 (SEIE)
    // 註：Timer 的 STIE 應該已經在你的 timer_init() 裡設定了
    set_csr(sstatus, 1 << 1); // sstatus.SIE = 1
    set_csr(sie, 1 << 9);     // sie.SEIE = 1

    jump_to_user_mode();

}