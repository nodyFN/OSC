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
#include "vm.h"
#include "list.h"
#include "vfs.h"

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
    ecall_helper_list[13] = mmap_ecall_helper;
    ecall_helper_list[14] = open_ecall_helper;
    ecall_helper_list[15] = close_ecall_helper;
    ecall_helper_list[16] = read_ecall_helper;
    ecall_helper_list[17] = write_ecall_helper;
    ecall_helper_list[18] = mkdir_ecall_helper;
    ecall_helper_list[19] = mount_ecall_helper;
    ecall_helper_list[20] = chdir_ecall_helper;
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

    uint64_t initrd_start_va = PA_TO_VA(kernel_info.initrd_start_addr);
    uint64_t initrd_end_va   = PA_TO_VA(kernel_info.initrd_end_addr);

    void* file_content = get_file_address((void*)initrd_start_va, (void*)initrd_end_va, path);
    uint32_t file_size = get_file_size((void*)initrd_start_va, (void*)initrd_end_va, path);
    
    if (file_content == 0) {
        printf("[Error] exec cannot find %s\n", path);
        regs->a0 = -1;
        regs->sepc += 4;
        return;
    }

    struct task_struct *current = get_current();


    struct list_head *pos, *n;
    struct vma_struct *vma;
    list_for_each_safe(pos, n, &current->vma_list) {
        vma = list_entry(pos, struct vma_struct, list);
        list_del(pos);
        kfree(vma);
    }
    INIT_LIST_HEAD(&current->vma_list);

    for (int i = 0; i < 32; i++) {
        current->signal_handler[i] = 0;
    }
    current->sigpending = 0;
    current->is_handling_signal = 0;
    
    if (current->signal_stack_page != 0) {
        kfree((void*)current->signal_stack_page);
        current->signal_stack_page = 0;
    }

    uint64_t* new_pgd = (uint64_t*)kmalloc(PAGE_SIZE);
    memset(new_pgd, 0, PAGE_SIZE);
    memcpy(&new_pgd[256], &kernel_info.new_pgd[256], 256 * sizeof(uint64_t));

    uint64_t num_code_pages = (file_size + PAGE_SIZE - 1) / PAGE_SIZE;
    struct vma_struct* new_vma = add_vma(current, 0x0, num_code_pages * PAGE_SIZE, PROT_CODE, 0);
    new_vma->file_content = file_content;
    new_vma->filesize = file_size;

    uint64_t stack_va = USER_STACK_VA - USER_STACK_SIZE;
    add_vma(current, stack_va, USER_STACK_VA, PROT_STACK, 0);

    current->pgd = new_pgd; 
    current->user_sp = USER_STACK_VA;

    uint64_t next_satp = MAKE_SATP(VA_TO_PA((uint64_t)current->pgd));
    asm volatile("csrw satp, %0" : : "r"(next_satp));
    asm volatile("sfence.vma");

    regs->sepc = 0x0;
    regs->sp = current->user_sp;

    regs->a0 = 0;

    return;
}

static void copy_user_page_tables(uint64_t *dst_pgd, uint64_t *src_pgd) {
    for (int i = 0; i < 256; i++) {
        if (src_pgd[i] & PTE_V) {
            uint64_t *src_pmd = (uint64_t *)PA_TO_VA((src_pgd[i] >> 10) << 12);
            uint64_t *dst_pmd = (uint64_t *)kmalloc(PAGE_SIZE);
            memset(dst_pmd, 0, PAGE_SIZE);
            dst_pgd[i] = (PFN_DOWN(VA_TO_PA((uint64_t)dst_pmd)) << 10) | PTE_V;

            for (int j = 0; j < 512; j++) {
                if (src_pmd[j] & PTE_V) {
                    uint64_t *src_pte = (uint64_t *)PA_TO_VA((src_pmd[j] >> 10) << 12);
                    uint64_t *dst_pte = (uint64_t *)kmalloc(PAGE_SIZE);
                    memset(dst_pte, 0, PAGE_SIZE);
                    dst_pmd[j] = (PFN_DOWN(VA_TO_PA((uint64_t)dst_pte)) << 10) | PTE_V;

                    for (int k = 0; k < 512; k++) {
                        if (src_pte[k] & PTE_V) {
                            uint64_t pa = (src_pte[k] >> 10) << 12;
                            uint64_t prot = src_pte[k] & 0x3FF;
                            if (prot & PTE_W) {
                                prot &= ~PTE_W;
                            }
                            src_pte[k] = (PFN_DOWN(pa) << 10) | prot; 
                            dst_pte[k] = src_pte[k];
                            
                            struct page *pp = phys_to_page(pa);
                            if (pp != NULL) {
                                if (prot & PTE_W) {
                                    prot &= ~PTE_W;
                                }
                                src_pte[k] = (PFN_DOWN(pa) << 10) | prot; 
                                dst_pte[k] = src_pte[k];
                                
                                pp->reference_count++;
                            } else {
                                dst_pte[k] = src_pte[k];
                            }
                        }
                    }
                }
            }
        }
    }
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

    child->pgd = (uint64_t*)kmalloc(PAGE_SIZE);
    memset(child->pgd, 0, PAGE_SIZE);
    memcpy(&child->pgd[256], &parent->pgd[256], 256 * sizeof(uint64_t));
    copy_user_page_tables(child->pgd, parent->pgd);

    asm volatile("sfence.vma");

    INIT_LIST_HEAD(&child->vma_list);
    struct list_head *pos;
    struct vma_struct *parent_vma;
    list_for_each(pos, &parent->vma_list) {
        parent_vma = list_entry(pos, struct vma_struct, list);
        struct vma_struct *child_vma = add_vma(child, parent_vma->start_address, parent_vma->end_address, parent_vma->prot, parent_vma->flags);

        child_vma->file_content = parent_vma->file_content;
        child_vma->filesize = parent_vma->filesize;
    }

    child->kernel_stack = (unsigned long)kmalloc(KERNEL_STACK_SIZE);
    memcpy((void*)child->kernel_stack, (void*)parent->kernel_stack, KERNEL_STACK_SIZE);
    child->kernel_sp = child->kernel_stack + KERNEL_STACK_SIZE;

    unsigned long stack_offset = (unsigned long)regs - parent->kernel_stack;
    struct pt_regs *child_regs = (struct pt_regs *)(child->kernel_stack + stack_offset);

    regs->a0 = child->pid;
    child_regs->a0 = 0;
    
    child_regs->tp = (uint64_t)child;
    child->thread.ra = (unsigned long)ret_from_exception;
    child->thread.sp = (unsigned long)child_regs;

    child->parent = parent;
    child->waiting_pid = -1;

    enqueue(&run_queue, child);

    regs->sepc += 4;
    child_regs->sepc += 4; 
    
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

static int is_overlap(struct task_struct* current, uint64_t addr, unsigned long length){
    struct list_head *curr;
    struct vma_struct *entry;
    uint64_t end_addr = addr + length;

    list_for_each(curr, &current->vma_list) {
        entry = list_entry(curr, struct vma_struct, list);
        if (addr < entry->end_address && end_addr > entry->start_address) {
            return 1;
        }
    }

    return 0;
}

void mmap_ecall_helper(struct pt_regs* regs){
    // 13 mmap
    uint64_t addr = regs->a0;
    unsigned long length = regs->a1;
    int prot = regs->a2;
    int flags = regs->a3;
    struct task_struct *current = get_current();

    unsigned long aligned_length = (length + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1));
    uint64_t mapping_addr = addr;
    if(addr == 0 || (addr & (PAGE_SIZE - 1)) != 0 || is_overlap(current, addr, aligned_length)){
        uint64_t search_addr = 0x40000000;  // 1 GB
        uint64_t search_limit = 0x80000000; // 2 GB
        int found = 0;

        while ((search_addr + aligned_length) <= search_limit) {
            if (!is_overlap(current, search_addr, aligned_length)) {
                mapping_addr = search_addr;
                found = 1;
                break;
            }
            search_addr += PAGE_SIZE;
        }

        if (!found) {
            printf("[Error] mmap: No unmapped area found!\n");
            regs->a0 = -1;
            regs->sepc += 4;
            return;
        }
    }

    uint64_t pte_flags = PTE_V | PTE_U | PTE_A | PTE_D; 
    if (prot & 1) pte_flags |= PTE_R;
    if (prot & 2) pte_flags |= PTE_W;
    if (prot & 4) pte_flags |= PTE_X;

    add_vma(current, mapping_addr, mapping_addr + aligned_length, pte_flags, flags);

    if (flags & 0x8000) {
        for (uint64_t offset = 0; offset < aligned_length; offset += PAGE_SIZE) {
            void* physical_page = kmalloc(PAGE_SIZE);
            if (physical_page != NULL) {
                memset(physical_page, 0, PAGE_SIZE);
                map_pages(current->pgd, mapping_addr + offset, PAGE_SIZE, VA_TO_PA((uint64_t)physical_page), pte_flags);
            }
        }
        asm volatile("sfence.vma");
    }

    regs->a0 = mapping_addr;
    regs->sepc += 4;
}


void open_ecall_helper(struct pt_regs* regs) {
    // 14 open
    const char* pathname = (const char*)regs->a0;
    int flags = (int)regs->a1;
    
    struct task_struct* curr = get_current();
    int fd = -1;

    for (int i = 0; i < MAX_FD; i++) {
        if (curr->fd_table[i] == NULL) {
            fd = i;
            break;
        }
    }
    
    if (fd == -1) {
        regs->a0 = -1;
    } else {
        struct file* f;
        int res = vfs_open(pathname, flags, &f);
        if (res != 0) {
            regs->a0 = res;
        } else {
            curr->fd_table[fd] = f;
            regs->a0 = fd;
        }
    }
    
    regs->sepc += 4;
    return;
}


void close_ecall_helper(struct pt_regs* regs) {
    // 15 close
    int fd = (int)regs->a0;
    struct task_struct* curr = get_current();

    if (fd < 0 || fd >= MAX_FD || curr->fd_table[fd] == NULL) {
        regs->a0 = -1;
    } else {
        int res = vfs_close(curr->fd_table[fd]);
        curr->fd_table[fd] = NULL;
        regs->a0 = res;
    }
    
    regs->sepc += 4;
    return;
}


void read_ecall_helper(struct pt_regs* regs) {
    // 16 read
    int fd = (int)regs->a0;
    void* buf = (void*)regs->a1;
    size_t count = (size_t)regs->a2;
    struct task_struct* curr = get_current();
    
    if (fd < 0 || fd >= MAX_FD || curr->fd_table[fd] == NULL) {
        regs->a0 = -1;
    } else {
        regs->a0 = vfs_read(curr->fd_table[fd], buf, count);
    }
    
    regs->sepc += 4;
    return;
}


void write_ecall_helper(struct pt_regs* regs) {
    // 17 write
    int fd = (int)regs->a0;
    const void* buf = (const void*)regs->a1;
    size_t count = (size_t)regs->a2;
    struct task_struct* curr = get_current();
    
    if (fd < 0 || fd >= MAX_FD || curr->fd_table[fd] == NULL) {
        regs->a0 = -1;
    } else {
        regs->a0 = vfs_write(curr->fd_table[fd], buf, count);
    }
    
    regs->sepc += 4;
    return;
}


void mkdir_ecall_helper(struct pt_regs* regs) {
    // 18 mkdir
    const char* pathname = (const char*)regs->a0;
    regs->a0 = vfs_mkdir(pathname);
    
    regs->sepc += 4;
    return;
}


void mount_ecall_helper(struct pt_regs* regs) {
    // 19 mount
    const char* target = (const char*)regs->a1;
    const char* filesystem = (const char*)regs->a2;
    
    regs->a0 = vfs_mount(target, filesystem);
    
    regs->sepc += 4;
    return;
}

void chdir_ecall_helper(struct pt_regs* regs) {
    // 20 chdir
    const char* path = (const char*)regs->a0;
    struct vnode* target_dir;

    int res = vfs_lookup(path, &target_dir);
    if (res == 0) {
        get_current()->curr_dir = target_dir;
    }
    regs->a0 = res;
    
    regs->sepc += 4;
    return;
}