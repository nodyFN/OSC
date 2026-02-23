/* kernel/src/trap.c */
#include <stdint.h>
#include "stdio.h"
#include "riscv.h"

extern void timer_handler();

void trap_handler() {
    uint64_t mcause = read_csr(mcause);
    uint64_t mepc = read_csr(mepc);

    if (mcause & 0x8000000000000000) {
        uint64_t exception_code = mcause & 0x7FFFFFFFFFFFFFFF;
        if (exception_code == 7) {
            timer_handler();
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