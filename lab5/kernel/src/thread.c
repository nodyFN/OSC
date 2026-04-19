#include "thread.h"
#include "stdio.h"
#include "mm.h"
#include "trap.h"
#include "string.h"
#include "utils.h"

int nr_threads = 0;
struct task_struct* run_queue = 0;

void enqueue(struct task_struct** queue, struct task_struct* task) {
    if (*queue == 0) {
        *queue = task;
        task->next = task;
    } else {
        struct task_struct* tail = (*queue)->next;
        (*queue)->next = task;
        task->next = tail;
    }
}

struct task_struct* get_current() {
    register struct task_struct* current asm("tp");
    return current;
}

extern void switch_to(struct task_struct* prev, struct task_struct* next);
void schedule() {
    uint64_t irq_flags;
    local_irq_save(irq_flags);

    struct task_struct* prev = get_current();
    
    struct task_struct* next = prev->next;

    struct task_struct* head = next;
    while(next->status == TERMINATED || next->status == WAITING || next->status == ABORTED){
        next = next->next;
        if (next == head) {
            next = run_queue; 
            break;
        }
    }

    if(prev->status == RUNNING){
        prev->status = READY;
    }
    next->status = RUNNING;

    if (prev != next) {
        // printf("from pid=%d to pid=%d\n", prev->pid, next->pid);
        switch_to(prev, next);
    }

    local_irq_restore(irq_flags);
}

void thread_idle() {
    while (1) {
        asm volatile("csrsi sstatus, 2");
        // asm volatile("wfi");
        // for (int i = 0; i < 100000000; i++);
        // printf("Idling...\n");
        kill_zombies();
        schedule();
    }
}

void thread_foo() {
    for (int i = 0; i < 5; i++) {
        printf("Process ID: %d, %d\n", get_current()->pid, i);
        for (int i = 0; i < 100000000; i++);
        schedule();
    }
    thread_exit();
}

struct task_struct* kthread_create(void (*threadfn)()) {
    struct task_struct* task = kmalloc(sizeof(struct task_struct));
    memset(task, 0, sizeof(struct task_struct));
    task->pid = nr_threads++;
    task->kernel_stack = (unsigned long)kmalloc(KERNEL_STACK_SIZE);
    task->thread.ra = (unsigned long)threadfn;
    task->thread.sp = task->kernel_stack + KERNEL_STACK_SIZE;

    task->parent = NULL;
    task->waiting_pid = -1;

    enqueue(&run_queue, task);
    return task;
}

void thread_exit(){
    struct task_struct* target = get_current();
    target->status = TERMINATED;
    if(target->parent != NULL && target->parent->status == WAITING && target->parent->waiting_pid == target->pid){
        target->parent->status = READY;
        target->parent->waiting_pid = -1;
    }
    schedule();
}

void kill_zombies(){
    uint64_t irq_flags;
    local_irq_save(irq_flags);

    struct task_struct *current = run_queue->next;
    struct task_struct *prev = run_queue;
    while(current != run_queue){
        if(current->status == TERMINATED){
            current->status = ABORTED; // prevent from becoming a dangling process
            prev->next = current->next;
            kfree((void*)current->kernel_stack);           
            if (current->user_stack != 0) {
                kfree((void*)current->user_stack);
            }
            kfree(current);
            current = prev->next;
        }else{
            prev = current;
            current = current->next;
        }
    }

    local_irq_restore(irq_flags);
}

extern void ret_from_exception();

struct task_struct* user_process_create(void (*user_func)()){
    struct task_struct* task = kmalloc(sizeof(struct task_struct));
    memset(task, 0, sizeof(struct task_struct));
    task->pid = nr_threads++;
    task->status = READY;

    task->stack_page_order = 3;
    task->kernel_stack = (unsigned long)kmalloc(KERNEL_STACK_SIZE);
    task->kernel_sp = task->kernel_stack + KERNEL_STACK_SIZE;

    task->user_stack_page_order = 0;
    task->user_stack = (unsigned long)kmalloc(USER_STACK_SIZE);
    task->user_sp = task->user_stack + USER_STACK_SIZE;

    struct pt_regs *regs = (struct pt_regs *)(task->kernel_sp - sizeof(struct pt_regs));

    for (int i = 0; i < sizeof(struct pt_regs); i++) {
        ((char*)regs)[i] = 0;
    }

    regs->tp = (uint64_t)task;
    regs->sepc = (uint64_t)user_func;
    regs->sp = task->user_sp;
    regs->sstatus |= (1 << 5);
    regs->sstatus |= (1 << 13);

    task->thread.ra = (unsigned long)ret_from_exception;
    task->thread.sp = (unsigned long)regs;

    task->parent = run_queue;
    task->waiting_pid = -1;

    enqueue(&run_queue, task);
    return task;
}