#include "trap.h"
#include "stdio.h"
#include "uart.h"
#include "timer.h"
#include "plic.h"
#include "task.h"
#include "ecall_helper.h"
#include "thread.h"
#include "mm.h"
#include "string.h"

extern void (*ecall_helper_list[256])(struct pt_regs*);
extern char sigreturn_stub[];
extern char sigreturn_stub_end[];

void check_signals(struct pt_regs *regs) {
    struct task_struct *current = get_current();

    if (current && current->sigpending != 0 && current->is_handling_signal == 0) {
        for (int i = 0; i < 32; i++) {
            if (current->sigpending & (1 << i)) {
                current->sigpending &= ~(1 << i);
                current->is_handling_signal = 1;
                
                current->signal_saved_regs = *regs;
                
                struct page* sig_page = alloc_pages(0); 
                current->signal_stack_page = page_to_phys(sig_page);

                uint32_t *trampoline = (uint32_t *)(current->signal_stack_page + 4096 - 16);
                size_t stub_size = sigreturn_stub_end - sigreturn_stub;
                memcpy(trampoline, sigreturn_stub, stub_size);
                
                __asm__ volatile("fence.i");

                regs->ra = (uint64_t)trampoline;
                regs->sp = (uint64_t)trampoline;
                regs->sepc = current->signal_handler[i];
                
                break;
            }
        }
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
            ecall_helper_list[regs->a7](regs);
            // return;
        }else {
            printf("\n[Kernel Panic] Unhandled Exception!\n");
            printf("PID: %d, scause: 0x%lx, sepc: 0x%lx, stval: 0x%lx\n", 
                    get_current()->pid, regs->scause, regs->sepc, regs->stval);
            printf("Killing the crashing thread...\n");
            thread_exit();
            return;
        }
    }
    check_signals(regs);

    run_tasks();
}