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

// 💥 宣告外部的 UART 狀態開關
extern volatile int uart_interrupts_ready;

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
    void *user_stack = kmalloc(4096);
    memset(user_stack, 0, 4096); 

    // 💥 1. 先印字！這時候中斷隨便它觸發，我們的「登機證 (sepc)」還沒印出來。
    printf("\n[Kernel] Switching to U-mode...\n");

    // 💥 2. 關閉全域中斷！(進入 Critical Section)
    // 確保接下來修改 sepc 和 sstatus 的過程中，絕對不會有硬體中斷跑來攪局！
    clear_csr(sstatus, 1 << 1); 

    // 💥 3. 安全地設定返回位址與降級狀態
    write_csr(sscratch, (uint64_t)_stack_top);
    write_csr(sepc, (uint64_t)shell_task);

    uint64_t status = read_csr(sstatus);
    status &= ~(1UL << 8);  // SPP = 0 (降級到 U-mode)
    status |= (1UL << 5);   // SPIE = 1 (保證 sret 之後，U-mode 的中斷會自動開啟)
    write_csr(sstatus, status);

    // 💥 4. 完美起飛
    asm volatile(
        "mv sp, %0\n"
        "sret\n"   
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
    set_csr(sstatus, 1 << 1); // sstatus.SIE = 1
    set_csr(sie, 1 << 9);     // sie.SEIE = 1

    // 💥 終極關鍵：告訴 UART，中斷管線已經全部準備就緒！可以開始 Non-blocking 了！
    uart_interrupts_ready = 1;

    jump_to_user_mode();

}