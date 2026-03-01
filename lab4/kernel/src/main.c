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
#ifdef QEMU
    write_csr(mscratch, (uint64_t)_stack_top);
    write_csr(mepc, (uint64_t)shell_task);

    uint64_t status = read_csr(mstatus);
    status &= ~(3UL << 11); // U-mode
    status |= (1UL << 7);   // MPIE = 1
    write_csr(mstatus, status);

    // QEMU 跑在 M-mode，必須解鎖 PMP
    asm volatile("csrw pmpaddr0, %0" :: "r"(0x3FFFFFFFFFFFFFULL));
    asm volatile("csrw pmpcfg0, %0" :: "r"(0x1F));
#else
    // 實體板子跑在 S-mode
    write_csr(sscratch, (uint64_t)_stack_top);
    write_csr(sepc, (uint64_t)shell_task);

    uint64_t status = read_csr(sstatus);
    status &= ~(1UL << 8);  // SPP = 0 (代表降級到 U-mode)
    status |= (1UL << 5);   // SPIE = 1
    write_csr(sstatus, status);
    
    // S-mode 沒有權限設定 PMP，且 U-Boot 已經設定好了，所以不需要
#endif

    void *user_stack = kmalloc(4096);
    memset(user_stack, 0, 4096); 

    printf("\n[Kernel] Switching to U-mode...\n");

    asm volatile(
        "mv sp, %0\n"
#ifdef QEMU
        "mret\n"   // M-mode 返回
#else
        "sret\n"   // S-mode 返回
#endif
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

    // struct sbiret version_ret = sbi_ecall(0x10, 0, 0, 0, 0, 0, 0, 0);
    // uart_puts("SBI Version: ");
    // uart_hex(version_ret.value);

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

#ifdef QEMU
    plic_init(); 
    write_csr(mtvec, (uint64_t)trap_vector);
    timer_init();
    set_csr(mie, 1 << 11); // MEIE
#else
    // 💥 依然保持先設定 stvec 以策安全
    write_csr(stvec, (uint64_t)trap_vector);

    // 💥 重啟 PLIC 初始化！
    plic_init(); 
    timer_init();
    
    // 💥 依照最新 Spec，同時開啟 SIE 與 SEIE (Supervisor External Interrupt)
    set_csr(sstatus, 1 << 1); 
    set_csr(sie, 1 << 9);     
#endif

    jump_to_user_mode();

}