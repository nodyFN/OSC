#include "trap.h"
#include "stdio.h"
#include "uart.h"
#include "timer.h"
#include "plic.h"

void kernel_resume() {
    printf("\nKernel resumed! Back to idle loop.\n");
    while (1) {
        uart_putc(uart_getc()); 
    }
}

void do_trap(struct pt_regs* regs){
    if(regs->scause & (1UL << 63)) { // interrupt
        uint64_t exception_code = regs->scause & ~(1ULL << 63);

        if (exception_code == 5) { // Supervisor Timer Interrupt
            timer_handler();
        }else if(exception_code == 9){ // PLIC
            int irq = plic_claim();
            if(irq == UART_IRQ){
                uart_trap_handler();
            }
            if(irq){
                plic_complete(irq);
            }
        }
    }else{
        if(regs->scause == 8){
            // ecall
            if (regs->a7 == 93) {
                printf("User program finished execution.\n");
                regs->sstatus |= (1 << 8); 
                regs->sepc = (uint64_t)kernel_resume;
                
                return;
            }
        }
        printf("sepc: %lx, scause: %lx, stval: %lx\n", regs->sepc, regs->scause, regs->stval);

        regs->sepc += 4;
    }
}