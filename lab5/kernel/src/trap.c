#include "trap.h"
#include "stdio.h"
#include "uart.h"
#include "timer.h"
#include "plic.h"
#include "task.h"
#include "ecall_helper.h"

extern void (*ecall_helper_list[256])(struct pt_regs*);

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
            ecall_helper_list[regs->a7](regs);
            return;
        }
    }
    run_tasks();
}