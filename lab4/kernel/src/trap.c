/* kernel/src/trap.c */
#include <stdint.h>
#include "stdio.h"
#include "riscv.h"
#include "plic.h" // 💥 引入 PLIC

extern void timer_handler();
extern void uart_isr(); // 💥 引入 UART ISR

void trap_handler() {
    uint64_t mcause = read_csr(mcause);
    uint64_t mepc = read_csr(mepc);

    if (mcause & 0x8000000000000000) {
        uint64_t exception_code = mcause & 0x7FFFFFFFFFFFFFFF;
        if (exception_code == 7) {
            timer_handler();
        } 
        else if (exception_code == 11) {
            // 💥 捕捉到外部中斷 (Machine External Interrupt)
            uint32_t irq = plic_claim();
            
            if (irq == 10) { 
                uart_isr(); // 處理 UART 收發
            }
            
            if (irq) {
                plic_complete(irq); // 告訴 PLIC 處理完畢
            }
        }
    } else {
        if (mcause == 8) {
            printf("\n[Trap] ecall from U-mode captured! mepc: 0x%lx\n", mepc);
            write_csr(mepc, mepc + 4);
        } else {
            printf("\n[Exception] Code: %ld, mepc: 0x%lx\n", mcause, mepc);
            while(1); 
        }
    }
}