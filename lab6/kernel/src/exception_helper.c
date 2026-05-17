#include "exception_helper.h"
#include "thread.h"
#include "ecall_helper.h"
#include "mm.h"
#include "vm.h"
#include "stdio.h"
#include "string.h"

void (*exception_helper_list[256])(struct pt_regs*);

void exception_helper_commit(){
    for(int i=0;i<256;i++){
        exception_helper_list[i] = unhandle_exception_helper;
    }
    exception_helper_list[8] = ecall_exception_helper;
    exception_helper_list[12] = instruction_page_fault_helper;
    exception_helper_list[13] = load_page_fault_helper;
    exception_helper_list[15] = store_amo_page_fault_helper;
}

void ecall_exception_helper(struct pt_regs* regs){
    // scause 8
    extern void (*ecall_helper_list[256])(struct pt_regs*);
    ecall_helper_list[regs->a7](regs);
}

static void do_page_fault(struct pt_regs* regs, int required_prot) {
    uint64_t fault_addr = regs->stval;
    struct task_struct *current = get_current();

    struct list_head *pos;
    struct vma_struct *vma = NULL;

    list_for_each(pos, &current->vma_list) {
        struct vma_struct *entry = list_entry(pos, struct vma_struct, list);
        if (fault_addr >= entry->start_address && fault_addr < entry->end_address) {
            vma = entry;
            break;
        }
    }

    if (vma == NULL || (vma->prot & required_prot) == 0) {
        printf("[Segmentation fault]: Kill Process\n");
        thread_exit();
    }else{
        printf("[Translation fault]: %lx\n", fault_addr);
    }

    uint64_t page_start = fault_addr & ~(PAGE_SIZE - 1);
    void* physical_page = kmalloc(PAGE_SIZE);
    
    if (physical_page == NULL) {
        printf("[Error] OOM during Page Fault\n");
        thread_exit();
    }
    memset(physical_page, 0, PAGE_SIZE);

    if (vma->file_content != NULL) {
        uint64_t offset = page_start - vma->start_address;

        if (offset < vma->filesize) {
            uint64_t copy_size = PAGE_SIZE;
            if (offset + PAGE_SIZE > vma->filesize) {
                copy_size = vma->filesize - offset;
            }
            memcpy(physical_page, (void*)((uint64_t)vma->file_content + offset), copy_size);
        }
    }

    map_pages(current->pgd, page_start, PAGE_SIZE, VA_TO_PA((uint64_t)physical_page), vma->prot);

    asm volatile("sfence.vma");
}

void instruction_page_fault_helper(struct pt_regs* regs){
    // printf("instruction_page_fault_helper\n");
    // scause 12
    do_page_fault(regs, PTE_X);
}

void load_page_fault_helper(struct pt_regs* regs){
    // printf("load_page_fault_helper\n");
    // scause 13
    do_page_fault(regs, PTE_R);
}

void store_amo_page_fault_helper(struct pt_regs* regs){
    // printf("store_amo_page_fault_helper\n");
    // scuase 15
    do_page_fault(regs, PTE_W);
}

void unhandle_exception_helper(struct pt_regs* regs){
    printf("\n[Kernel Panic] Unhandled Exception!\n");
    printf("PID: %d, scause: %d, sepc: %lx, stval: %lx\n", get_current()->pid, regs->scause, regs->sepc, regs->stval);
    printf("Killing the crashing thread...\n");
    thread_exit();
}