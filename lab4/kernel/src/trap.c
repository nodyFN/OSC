/* kernel/src/trap.c */
#include <stdint.h>
#include "stdio.h"
#include "riscv.h"
#include "plic.h" // 💥 引入 PLIC

extern void timer_handler();
extern void uart_isr(); // 💥 引入 UART ISR

void trap_handler() {
#ifdef QEMU
    uint64_t cause = read_csr(mcause);
    uint64_t epc = read_csr(mepc);
#else
    uint64_t cause = read_csr(scause);
    uint64_t epc = read_csr(sepc);
#endif

    if (cause & 0x8000000000000000) {
        uint64_t exception_code = cause & 0x7FFFFFFFFFFFFFFF;
        // Exception Code 7 是 M-Timer，5 是 S-Timer
        if (exception_code == 7 || exception_code == 5) {
            timer_handler();
        } 
#ifdef QEMU
        else if (exception_code == 11) {
            uint32_t irq = plic_claim();
            if (irq == 10) uart_isr();
            if (irq) plic_complete(irq);
        }
#endif
    } else {
        if (cause == 8) {
            // printf("\n[Trap] ecall from U-mode captured! epc: 0x%lx\n", epc);
#ifdef QEMU
            write_csr(mepc, epc + 4);
#else
            write_csr(sepc, epc + 4);
#endif
        } else {
            printf("\n[Exception] Code: %ld, epc: 0x%lx\n", cause, epc);
            while(1); 
        }
    }
}