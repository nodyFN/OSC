#include "trap.h"
#include "stdio.h"
#include "uart.h"
#include "timer.h"
#include "plic.h"
#include "task.h"
#include "thread.h"
#include "exception_helper.h"
#include "string.h"
#include "mm.h"
#include "vm.h"

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
                
                current->signal_stack_page = (uint64_t)kmalloc(PAGE_SIZE);
                memset((void*)current->signal_stack_page, 0, PAGE_SIZE);

                size_t stub_size = (uint64_t)sigreturn_stub_end - (uint64_t)sigreturn_stub;
                uint64_t kva_trampoline = current->signal_stack_page + PAGE_SIZE - stub_size;
                memcpy((void*)kva_trampoline, sigreturn_stub, stub_size);
                
                __asm__ volatile("fence.i");

                map_pages(current->pgd, USER_SIGNAL_STACK_VA, PAGE_SIZE, VA_TO_PA(current->signal_stack_page), PROT_SIG_STACK);
                __asm__ volatile("sfence.vma");
                uint64_t uva_trampoline = USER_SIGNAL_STACK_VA + PAGE_SIZE - stub_size;
                
                regs->ra = uva_trampoline;
                regs->sp = uva_trampoline & ~0xF; 
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
        extern void (*exception_helper_list[256])(struct pt_regs*);
        exception_helper_list[regs->scause](regs);
    }
    check_signals(regs);

    run_tasks();
}