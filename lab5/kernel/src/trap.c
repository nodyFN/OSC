#include "trap.h"
#include "stdio.h"
#include "uart.h"
#include "timer.h"
#include "plic.h"
#include "task.h"
#include "thread.h"
#include "mm.h"
#include "string.h"
#include "utils.h"

extern int nr_threads;
extern void ret_from_exception();
extern struct task_struct* run_queue;
extern struct KernelInfo kernel_info;

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
            if (regs->a7 == 0) {
                // getpid
                regs->a0 = get_current()->pid; 
                regs->sepc += 4;
                return;
            }else if(regs->a7 == 1){
                // uart_read
                char *buf = (char *)regs->a0;
                long count = regs->a1;

                long ret_count = 0;
                for(int i=0;i<count;i++){
                    char c = uart_getc_sync();
                    *(buf + i) = c;
                    ret_count++;
                }
                regs->a0 = ret_count;
                regs->sepc += 4;
                return;      
            }else if(regs->a7 == 2){
                // uart_write
                const char *buf = (const char *)regs->a0;
                long count = regs->a1;
                
                long ret_count = 0;
                for(int i=0;i<count;i++){
                    uart_putc(buf[i]);
                    ret_count++;
                }

                regs->a0 = ret_count;
                regs->sepc += 4;
                return;
            }else if(regs->a7 == 3){
                // exec
                const char *path = (const char *)regs->a0;
                
                if (path == 0) {
                    regs->a0 = -1;
                    regs->sepc += 4;
                    return;
                }

                void* program_entry = get_file_address((void*)kernel_info.initrd_start_addr, (void*)kernel_info.initrd_end_addr, path);
                
                if (program_entry == 0) {
                    regs->a0 = -1;
                    regs->sepc += 4;
                    return;
                }

                struct task_struct *current = get_current();
                regs->sepc = (uint64_t)program_entry;
                regs->sp = current->user_stack + (1 << current->user_stack_page_order) * 4096; 

                return;
            }else if(regs->a7 == 4){
                // fork
                struct task_struct* parent = get_current();
                struct task_struct* child = kmalloc(sizeof(struct task_struct));
                memcpy((void*)child, (void*)parent, sizeof(*parent));

                child->pid = nr_threads++; 
                child->status = READY;

                struct page* kpage = alloc_pages(child->stack_page_order);
                child->stack = page_to_phys(kpage);
                struct page* upage = alloc_pages(child->user_stack_page_order);
                child->user_stack = page_to_phys(upage);
                memcpy((void*)child->stack, (void*)parent->stack, (1 << child->stack_page_order) * 4096);
                memcpy((void*)child->user_stack, (void*)parent->user_stack, (1 << child->user_stack_page_order) * 4096);
                child->kernel_sp = child->stack + 0x8000;
                child->user_sp = child->user_stack + 0x1000;

                struct pt_regs *child_regs = (struct pt_regs *)(child->kernel_sp - sizeof(struct pt_regs));

                regs->a0 = child->pid;
                child_regs->a0 = 0;
                child_regs->tp = (uint64_t)child;
                child_regs->sepc += 4;

                unsigned long offset = child->user_stack - parent->user_stack;
                child_regs->sp = child_regs->sp + offset;
                if (child_regs->s0 >= parent->user_stack && child_regs->s0 < parent->user_stack + 4096) {
                    child_regs->s0 = child_regs->s0 + offset;
                }

                child->thread.ra = (unsigned long)ret_from_exception;
                child->thread.sp = (unsigned long)child_regs;

                enqueue(&run_queue, child);

                regs->sepc += 4;
                return;
            }else if(regs->a7 == 5){
                //exit
                int status = regs->a0; 
                // printf("[Kernel] Process %d exited with status %d\n", get_current()->pid, status);
                thread_exit();
            }else if (regs->a7 == 6) {
                // stop
                long target_pid = regs->a0;
                if (target_pid == 0) {
                    printf("[Kernel] Permission denied: Cannot stop Idle Thread.\n");
                    regs->a0 = -1;
                    regs->sepc += 4;
                    return;
                }

                int found = 0;
                if (run_queue != 0) {
                    struct task_struct *task = run_queue->next;
                    struct task_struct *head = task;
                    do {
                        if (task->pid == target_pid) {
                            task->status = TERMINATED;
                            found = 1;
                            break;
                        }
                        task = task->next;
                    } while (task != head);
                }

                if (found) {
                    if (target_pid == get_current()->pid) {
                        // printf("[Kernel] Process %ld stopped itself.\n", target_pid);
                        thread_exit();
                    } else {
                        // printf("[Kernel] Process %ld was successfully stopped.\n", target_pid);
                        regs->a0 = 0;
                    }
                } else {
                    // printf("[Kernel] Failed to stop: Process %ld not found.\n", target_pid);
                    regs->a0 = -1;
                }

                regs->sepc += 4;
                return;
            }
        }

        if (regs->scause == 8) {
            printf("[Kernel Warning] Unhandled Syscall! a7 = %ld\n", regs->a7);
        } else {
            printf("sepc: %lx, scause: %lx, stval: %lx\n", regs->sepc, regs->scause, regs->stval);
        }
        regs->sepc += 4;
    }
    // run_tasks();
}