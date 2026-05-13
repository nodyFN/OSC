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
    ecall_helper_list[5] = waitpid_ecall_helper;
    ecall_helper_list[6] = exit_ecall_helper;
    ecall_helper_list[7] = stop_ecall_helper;
    ecall_helper_list[8] = display_ecall_helper;
    ecall_helper_list[9] = usleep_ecall_helper;
    ecall_helper_list[10] = signal_ecall_helper;
    ecall_helper_list[11] = sigreturn_ecall_helper;
    ecall_helper_list[12] = kill_ecall_helper;
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

    for(int i=0;i<count;i++){
        char c = uart_getc();
        *(buf + i) = c;
        ret_count++;
    }

    regs->a0 = ret_count;
    regs->sepc += 4;
    return;            
}

void uart_write_ecall_helper(struct pt_regs* regs){
    // 2 uart_write
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
    regs->sp = current->user_stack + USER_STACK_SIZE;
    return;
}

void fork_ecall_helper(struct pt_regs* regs){
    // 4 fork
    struct task_struct* parent = get_current();
    struct task_struct* child = kmalloc(sizeof(struct task_struct));
    memcpy((void*)child, (void*)parent, sizeof(*parent));

    child->sigpending = 0;
    child->is_handling_signal = 0;
    child->signal_stack_page = 0;

    child->pid = nr_threads++; 
    child->status = READY;

    child->kernel_stack = (unsigned long)kmalloc(KERNEL_STACK_SIZE);
    child->user_stack = (unsigned long)kmalloc(USER_STACK_SIZE);
    memcpy((void*)child->kernel_stack, (void*)parent->kernel_stack, KERNEL_STACK_SIZE);
    memcpy((void*)child->user_stack, (void*)parent->user_stack, USER_STACK_SIZE);
    child->kernel_sp = child->kernel_stack + KERNEL_STACK_SIZE;
    child->user_sp = child->user_stack + USER_STACK_SIZE;

    struct pt_regs *child_regs = (struct pt_regs *)(child->kernel_sp - sizeof(struct pt_regs));

    regs->a0 = child->pid;
    child_regs->a0 = 0;
    child_regs->tp = (uint64_t)child;
    child_regs->sepc += 4;

    unsigned long offset = child->user_stack - parent->user_stack;
    child_regs->sp = child_regs->sp + offset;
    if (child_regs->s0 >= parent->user_stack && child_regs->s0 < parent->user_stack + USER_STACK_SIZE) {
        child_regs->s0 = child_regs->s0 + offset;
    }

    child->thread.ra = (unsigned long)ret_from_exception;
    child->thread.sp = (unsigned long)child_regs;

    child->parent = parent;
    child->waiting_pid = -1;

    enqueue(&run_queue, child);

    regs->sepc += 4;
    return;
}

void waitpid_ecall_helper(struct pt_regs* regs){
    // 5 waitpid
    int waiting_pid = regs->a0;
    struct task_struct* current_process = get_current();

    // printf("PID %d is waits for PID %d\n", current_process->pid, regs->a0);

    int found = 0;
    int is_terminated = 0;

    if (run_queue != 0) {
        struct task_struct* task = run_queue;
        do {
            if (task->pid == waiting_pid) {
                found = 1;
                if (task->status == TERMINATED) {
                    is_terminated = 1;
                }
                break;
            }
            task = task->next;
        } while(task != run_queue);
    }

    if (!found) {
        regs->a0 = -1; 
    } else if (is_terminated) {
        regs->a0 = waiting_pid; 
    } else {
        current_process->status = WAITING;
        current_process->waiting_pid = waiting_pid;
        schedule();
        regs->a0 = waiting_pid; 
    }
    
    regs->sepc += 4;
    return;
}

void exit_ecall_helper(struct pt_regs* regs){
    // printf("PID %d exit\n", get_current()->pid);
    // 6 exit
    // int status = regs->a0; 
    // printf("[Kernel] Process %d exited with status %d\n", get_current()->pid, status);
    thread_exit();
}

void stop_ecall_helper(struct pt_regs* regs){
    // 7 stop
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

void unknown_ecall_helper(struct pt_regs* regs){
    if (regs->scause == 8) {
        printf("[Kernel Warning] Unhandled Syscall! a7 = %ld\n", regs->a7);    
    } else {
        printf("sepc: %lx, scause: %lx, stval: %lx\n", regs->sepc, regs->scause, regs->stval);
    }
    regs->sepc += 4;

    return;
}

void display_ecall_helper(struct pt_regs* regs) {
    // 8: display
    unsigned int* bmp_image = (unsigned int*)regs->a0;
    unsigned int width = regs->a1;
    unsigned int height = regs->a2;
    video_bmp_display(bmp_image, width, height);

    regs->sepc += 4;
}

extern uint64_t TIMERBASE_FREQ; 
void usleep_ecall_helper(struct pt_regs* regs) {
    // 9 usleep
    unsigned int usec = regs->a0;
    
    unsigned long wait_ticks = (unsigned long)usec * (TIMERBASE_FREQ / 1000000);
    unsigned long start_time, current_time;

    asm volatile("csrr %0, time" : "=r"(start_time));

    // asm volatile("csrsi sstatus, 2");

    do {
        schedule(); 
        asm volatile("csrr %0, time" : "=r"(current_time));
    } while ((current_time - start_time) < wait_ticks);

    asm volatile("csrci sstatus, 2");

    regs->a0 = 0;
    regs->sepc += 4;
}

void signal_ecall_helper(struct pt_regs* regs) {
    // 10 signal
    int signum = regs->a0;
    uint64_t handler = regs->a1;

    if (signum >= 0 && signum < 32) {
        get_current()->signal_handler[signum] = handler;
        regs->a0 = 0;
    } else {
        regs->a0 = -1;
    }

    regs->sepc += 4;
}

void sigreturn_ecall_helper(struct pt_regs* regs) {
    // 11 sigreturn
    struct task_struct *current = get_current();

    // struct page* sig_page = phys_to_page(current->signal_stack_page);
    // free_pages(sig_page, 0); 
    kfree((void*)current->signal_stack_page);
    
    *regs = current->signal_saved_regs;

    current->is_handling_signal = 0;
}

void kill_ecall_helper(struct pt_regs* regs) {
    // 12 kill
    int target_pid = regs->a0;
    int signum = regs->a1;
    
    if (signum < 0 || signum >= 32 || target_pid == 0) {
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
                found = 1;
                if (task->signal_handler[signum] != 0) {
                    task->sigpending |= (1 << signum);
                } else {
                    task->status = TERMINATED;
                    if (task->parent != NULL && task->parent->status == WAITING && task->parent->waiting_pid == task->pid) {
                        
                        task->parent->status = READY;
                        task->parent->waiting_pid = -1;
                    }
                }
                break;
            }
            task = task->next;
        } while (task != head);
    }

    if (found) {
        regs->a0 = 0;
    } else {
        regs->a0 = -1;
    }
    
    regs->sepc += 4;
}