#include "thread.h"
#include "stdio.h"
#include "mm.h"
#include "trap.h"
#include "string.h"
#include "utils.h"
#include "vm.h"
#include "initrd.h"
#include "list.h"
#include "vfs.h"

int nr_threads = 0;
struct task_struct* run_queue = 0;
extern struct KernelInfo kernel_info;

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
    while(next->status == TERMINATED || next->status == WAITING){
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
        uint64_t next_satp = MAKE_SATP(VA_TO_PA((uint64_t)next->pgd));
        asm volatile("csrw satp, %0" : : "r"(next_satp));
        asm volatile("sfence.vma");
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
    task->pgd = kernel_info.new_pgd;
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
            prev->next = current->next;
            if (current->kernel_stack != 0) {
                kfree((void*)current->kernel_stack);           
            }

            if (current->user_stack != 0) {
                kfree((void*)current->user_stack);
            }

            if (current->signal_stack_page != 0) {
                kfree((void*)current->signal_stack_page);
            }
            if (current->trampoline_code_page != 0) {
                kfree((void*)current->trampoline_code_page);
            }

            struct list_head *pos, *n;
            struct vma_struct *vma;
            list_for_each_safe(pos, n, &current->vma_list) {
                vma = list_entry(pos, struct vma_struct, list);
                list_del(pos);
                
                kfree(vma);
            }

            if (current->pgd != NULL) {
                kfree((void*)current->pgd);
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
struct task_struct* user_process_create(const char* filename){
    struct task_struct* task = kmalloc(sizeof(struct task_struct));
    memset(task, 0, sizeof(struct task_struct));
    task->pid = nr_threads++;
    task->status = READY;

    task->pgd = (uint64_t*)kmalloc(PAGE_SIZE);
    memset(task->pgd, 0, PAGE_SIZE);
    memcpy(&task->pgd[256], &kernel_info.new_pgd[256], 256 * sizeof(uint64_t));

    uint64_t initrd_start_va = PA_TO_VA(kernel_info.initrd_start_addr);
    uint64_t initrd_end_va   = PA_TO_VA(kernel_info.initrd_end_addr);

    void* file_content = get_file_address((void*)initrd_start_va, (void*)initrd_end_va, filename);
    uint32_t file_size = get_file_size((void*)initrd_start_va, (void*)initrd_end_va, filename);

    if (file_content == NULL) {
        printf("[Error] Cannot find %s in initrd\n", filename);
        return NULL;
    }

    uint64_t num_code_pages = (file_size + PAGE_SIZE - 1) / PAGE_SIZE;

    INIT_LIST_HEAD(&task->vma_list);
    struct vma_struct* new_vma = add_vma(task, 0x0, num_code_pages * PAGE_SIZE, PROT_CODE, 0);
    new_vma->file_content = file_content;
    new_vma->filesize = file_size;

    // user stack
    task->user_sp = USER_STACK_VA;
    uint64_t stack_va = USER_STACK_VA - USER_STACK_SIZE;
    add_vma(task, stack_va, USER_STACK_VA, PROT_STACK, 0);


    // kernel stack
    task->kernel_stack = (unsigned long)kmalloc(KERNEL_STACK_SIZE);
    task->kernel_sp = task->kernel_stack + KERNEL_STACK_SIZE;

    struct pt_regs *regs = (struct pt_regs *)(task->kernel_sp - sizeof(struct pt_regs));

    for (int i = 0; i < sizeof(struct pt_regs); i++) {
        ((char*)regs)[i] = 0;
    }

    regs->tp = (uint64_t)task;
    regs->sepc = 0x0;
    regs->sp = task->user_sp;
    regs->sstatus |= (1 << 5);
    regs->sstatus |= (1 << 13);
    regs->sstatus &= ~(1 << 8);

    regs->sstatus |= (1 << 18);

    task->thread.ra = (unsigned long)ret_from_exception;
    task->thread.sp = (unsigned long)regs;

    task->parent = run_queue;
    task->waiting_pid = -1;

    task->curr_dir = rootfs->root;

    struct file *f_stdin, *f_stdout, *f_stderr;
    vfs_open("/dev/uart", 0, &f_stdin);
    vfs_open("/dev/uart", 0, &f_stdout);
    vfs_open("/dev/uart", 0, &f_stderr);
    
    task->fd_table[0] = f_stdin;
    task->fd_table[1] = f_stdout;
    task->fd_table[2] = f_stderr;

    enqueue(&run_queue, task);
    return task;
}

struct vma_struct* add_vma(struct task_struct* task, uint64_t start_address, uint64_t end_address, int prot, int flags){
    struct vma_struct* new_vma = (struct vma_struct*)kmalloc(sizeof(struct vma_struct));
    new_vma->start_address = start_address;
    new_vma->end_address = end_address;
    new_vma->prot = prot;
    new_vma->flags = flags;
    new_vma->file_content = NULL;
    new_vma->filesize = 0;
    
    list_add_tail(&new_vma->list, &task->vma_list);

    return new_vma;
}