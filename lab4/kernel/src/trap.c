#include "trap.h"
#include "stdio.h"
#include "uart.h"
#include "timer.h"

void kernel_resume() {
    printf("\nKernel resumed! Back to idle loop.\n");
    while (1) {
        // 這裡可以繼續處理你的 UART 中斷輸入
        uart_putc(uart_getc()); 
    }
}

void do_trap(struct pt_regs* regs){
    if(regs->scause & (1UL << 63)) { // interrupt
        uint64_t exception_code = regs->scause & ~(1ULL << 63);

        if (exception_code == 5) { // Supervisor Timer Interrupt
            uint64_t current_time = get_time();
            uint64_t seconds_after_boot = current_time / TIMERBASE_FREQ;
            printf("Tick! %d seconds after booting.\n", seconds_after_boot);
            set_next_timer(2);
        }
    }else{
        if(regs->scause == 8){
            // ecall
            if (regs->a7 == 93) {
                printf("User program finished execution.\n");
                
                // 【核心魔法】
                // 1. 將 sstatus 的 SPP 位元 (bit 8) 設為 1，讓 sret 回到 S-mode
                regs->sstatus |= (1 << 8); 
                
                // 2. 將 sepc 指向 kernel_resume，讓 sret 跳去執行這個 C 函式
                regs->sepc = (uint64_t)kernel_resume;
                
                return; // 直接 return，不執行 sepc += 4
            }
        }
        printf("sepc: %lx, scause: %lx, stval: %lx\n", regs->sepc, regs->scause, regs->stval);

        regs->sepc += 4;
    }
}