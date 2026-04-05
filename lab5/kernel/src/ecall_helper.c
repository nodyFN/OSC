#include "ecall_helper.h"
#include "thread.h"
#include "uart.h"
#include "fdt.h"
#include "utils.h"
#include "initrd.h"
#include "string.h"
#include "mm.h"
#include "stdio.h"
#include "video.h"

extern struct KernelInfo kernel_info;
extern int nr_threads;
extern void ret_from_exception();
extern struct task_struct* run_queue;

void (*ecall_helper_list[256])(struct pt_regs*);

void ecall_helper_commit(){
    for(int i=0;i<256;i++){
        ecall_helper_list[i] = unknown_ecall_helper;
    }
    ecall_helper_list[0] = getpid_ecall_helper;
    ecall_helper_list[1] = uart_read_ecall_helper;
    ecall_helper_list[2] = uart_write_ecall_helper;
    ecall_helper_list[3] = exec_ecall_helper;
    ecall_helper_list[4] = fork_ecall_helper;
    ecall_helper_list[5] = exit_ecall_helper;
    ecall_helper_list[6] = stop_ecall_helper;
    ecall_helper_list[7] = display_ecall_helper;
    ecall_helper_list[8] = usleep_ecall_helper;
}

void getpid_ecall_helper(struct pt_regs* regs){
    // 0 getpid
    regs->a0 = get_current()->pid; 
    regs->sepc += 4;
    return;
}

void uart_read_ecall_helper(struct pt_regs* regs){
    // 1 uart_read
    char *buf = (char *)regs->a0;
    long count = regs->a1;
    long ret_count = 0;

    asm volatile("csrsi sstatus, 2");

    for(int i=0;i<count;i++){
        char c = uart_getc();
        *(buf + i) = c;
        ret_count++;
    }

    asm volatile("csrci sstatus, 2");

    regs->a0 = ret_count;
    regs->sepc += 4;
    return;            
}

void uart_write_ecall_helper(struct pt_regs* regs){
    // 2 uart_write
    const char *buf = (const char *)regs->a0;
    long count = regs->a1;
    long ret_count = 0;

    asm volatile("csrsi sstatus, 2");

    for(int i=0;i<count;i++){
        uart_putc(buf[i]);
        ret_count++;
    }

    asm volatile("csrci sstatus, 2");

    regs->a0 = ret_count;
    regs->sepc += 4;
    return;
}

void exec_ecall_helper(struct pt_regs* regs){
    // 3 exec
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
}

void fork_ecall_helper(struct pt_regs* regs){
    // 4 fork
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
}

void exit_ecall_helper(struct pt_regs* regs){
    // 5 exit
    // int status = regs->a0; 
    // printf("[Kernel] Process %d exited with status %d\n", get_current()->pid, status);
    thread_exit();
}

void stop_ecall_helper(struct pt_regs* regs){
    // 6 stop
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

int tem = 0;
void unknown_ecall_helper(struct pt_regs* regs){
    
    if (regs->scause == 8) {
        tem++;
        if(tem <=15){
            printf("[Kernel Warning] Unhandled Syscall! a7 = %ld\n", regs->a7);
        }
        
    } else {
        printf("sepc: %lx, scause: %lx, stval: %lx\n", regs->sepc, regs->scause, regs->stval);
    }
    regs->sepc += 4;

    return;
}

void display_ecall_helper(struct pt_regs* regs) {
    // 7: display
    unsigned int* bmp_image = (unsigned int*)regs->a0;
    unsigned int width = regs->a1;
    unsigned int height = regs->a2;

    video_bmp_display(bmp_image, width, height);
    
    regs->sepc += 4;
}

extern uint64_t TIMERBASE_FREQ; 
void usleep_ecall_helper(struct pt_regs* regs) {
    // 8 usleep
    unsigned int usec = regs->a0;
    
    unsigned long wait_ticks = (unsigned long)usec * (TIMERBASE_FREQ / 1000000);
    unsigned long start_time, current_time;

    asm volatile("csrr %0, time" : "=r"(start_time));

    asm volatile("csrsi sstatus, 2");

    do {
        schedule(); 
        asm volatile("csrr %0, time" : "=r"(current_time));
    } while ((current_time - start_time) < wait_ticks);

    asm volatile("csrci sstatus, 2");

    regs->a0 = 0;
    regs->sepc += 4;
}