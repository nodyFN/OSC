#include "thread.h"
#include "stdio.h"
#include "mm.h"
#include "trap.h"

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
    struct task_struct* prev = get_current();
    
    struct task_struct* next = prev->next;

    // printf("next thread status: %d\n", next->status);

    while(next->status == TERMINATED){
        next = next->next;
    }

    if(prev->status != TERMINATED){
        prev->status = READY;
    }
    next->status = RUNNING;

    // printf("Running thread pid: %d\n", next->pid);

    if (prev != next) {
        switch_to(prev, next);
    }
}

void thread_idle() {
    while (1) {
        asm volatile("csrsi sstatus, 2"); // 2 對應到 SIE (bit 1)
        // for (int i = 0; i < 100000000; i++);
        // printf("Idling...\n");
        kill_zombies();
        schedule();
    }
}

void thread_foo() {
    for (int i = 0; i < 5; i++) {
        // uart_puts("Process ID: ");
        printf("Process ID: %d, %d\n", get_current()->pid, i);
        // uart_hex(get_current()->pid);
        // uart_puts(" ");
        // uart_hex(i);
        // uart_puts("\n");
        for (int i = 0; i < 100000000; i++)
            ;
        schedule();
    }
    // while (1)
    //     ;
    thread_exit();
}

struct task_struct* kthread_create(void (*threadfn)()) {
    struct task_struct* task = kmalloc(sizeof(struct task_struct));
    task->pid = nr_threads++;
    struct page* kpage = alloc_pages(3);
    task->stack = page_to_phys(kpage);
    task->stack_page_order = 3;
    task->thread.ra = (unsigned long)threadfn;
    #define STACK_SIZE 0x8000
    task->thread.sp = task->stack + STACK_SIZE;
    enqueue(&run_queue, task);
    return task;
}

void thread_exit(){
    struct task_struct* target = get_current();
    target->status = TERMINATED;
    schedule();
}

void kill_zombies(){
    struct task_struct *current = run_queue->next;
    struct task_struct *prev = run_queue;
    while(current != run_queue){
        if(current->status == TERMINATED){
            prev->next = current->next;
            free_pages(phys_to_page(current->stack), current->stack_page_order);
            kfree(current);
            current = prev->next;
        }else{
            prev = current;
            current = current->next;
        }
    }
}

extern void ret_from_exception();

struct task_struct* user_process_create(void (*user_func)()){
    struct task_struct* task = kmalloc(sizeof(struct task_struct));
    task->pid = nr_threads++;
    task->status = READY;

    task->stack_page_order = 3;
    struct page* kpage = alloc_pages(task->stack_page_order);
    task->stack = page_to_phys(kpage);
    task->kernel_sp = task->stack + 0x8000; // 32KB

    task->user_stack_page_order = 0;
    struct page* upage = alloc_pages(task->user_stack_page_order);
    task->user_stack = page_to_phys(upage);
    task->user_sp = task->user_stack + 0x1000;

    struct pt_regs *regs = (struct pt_regs *)(task->kernel_sp - sizeof(struct pt_regs));

    for (int i = 0; i < sizeof(struct pt_regs); i++) {
        ((char*)regs)[i] = 0;
    }

    regs->tp = (uint64_t)task;
    regs->sepc = (uint64_t)user_func;
    regs->sp = task->user_sp;
    regs->sstatus |= (1 << 5);

    task->thread.ra = (unsigned long)ret_from_exception;
    task->thread.sp = (unsigned long)regs;

    enqueue(&run_queue, task);
    return task;
}