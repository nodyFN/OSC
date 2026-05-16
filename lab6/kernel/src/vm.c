#include "vm.h"
#include "utils.h"
#include "mm.h"
#include "string.h"
#include "stdio.h"

static unsigned long __attribute__((section(".data"), aligned(PAGE_SIZE))) early_pgd[ENTRIES_PER_TABLE] = { 0 };
static unsigned long __attribute__((section(".data"), aligned(PAGE_SIZE))) early_pmd_kernel[LINEAR_MAP_GIB][ENTRIES_PER_TABLE] = { { 0 } };
static unsigned long __attribute__((section(".data"), aligned(PAGE_SIZE))) early_pmd_identity[LINEAR_MAP_GIB][ENTRIES_PER_TABLE] = { { 0 } };

void setup_vm(){
    for(int i=0;i<LINEAR_MAP_GIB;i++){
        early_pgd[i] = MAKE_PTE(VA_TO_PA((unsigned long)early_pmd_identity[i]), PTE_V);
        early_pgd[i + 256] = MAKE_PTE(VA_TO_PA((unsigned long)early_pmd_kernel[i]), PTE_V);
    }

    for(int i=0;i<LINEAR_MAP_GIB;i++){
        for(int j=0;j<ENTRIES_PER_TABLE;j++){
            early_pmd_kernel[i][j] = early_pmd_identity[i][j] = MAKE_PTE(((i * PGD_SIZE) + (j * PMD_SIZE)), PROT_KERNEL);
        }
    }

    unsigned long satp = MAKE_SATP(VA_TO_PA(early_pgd));
    __asm__ volatile("sfence.vma");
    __asm__ volatile("csrw satp, %0" : : "r"(satp));
    __asm__ volatile("sfence.vma");
}

void drop_identity_map(){
    for(int i=0;i<LINEAR_MAP_GIB;i++){
        early_pgd[i] = 0;
    }
    __asm__ volatile("sfence.vma");
}

extern struct KernelInfo kernel_info;
extern uint64_t phys_mem_start;
extern uint64_t phys_mem_end;
void setup_finer_granularity_paging(){
    kernel_info.new_pgd = (uint64_t *)kmalloc(PAGE_SIZE);
    memset(kernel_info.new_pgd, 0, PAGE_SIZE);

    for(uint64_t pa = 0; pa < LINEAR_MAP_GIB * (1Ull << 30); pa += PAGE_SIZE) {
        unsigned long prot;
        
        if(PA_TO_VA(pa) >= phys_mem_start && PA_TO_VA(pa) < phys_mem_end) {
            prot = PROT_KERNEL;
        }else {
            prot = PROT_MMIO;
        }
        map_pages(PA_TO_VA(pa), PAGE_SIZE, pa, prot);
    }

    unsigned long satp = MAKE_SATP(VA_TO_PA((unsigned long)kernel_info.new_pgd));
    __asm__ volatile("sfence.vma");
    __asm__ volatile("csrw satp, %0" : : "r"(satp));
    __asm__ volatile("sfence.vma");
}

static void pagewalk(unsigned long va, unsigned long pa, unsigned long prot) {
    unsigned long *table = kernel_info.new_pgd;

    int vpn2 = (va >> 30) & 0x1FF;
    
    if ((table[vpn2] & PTE_V) == 0) {
        void *new_page = kmalloc(PAGE_SIZE);
        memset(new_page, 0, PAGE_SIZE);
        table[vpn2] = (PFN_DOWN(VA_TO_PA(new_page)) << 10) | PTE_V;
    }

    unsigned long pa1 = (table[vpn2] >> 10) << 12;
    table = (unsigned long *)PA_TO_VA(pa1);

    int vpn1 = (va >> 21) & 0x1FF;
    
    if ((table[vpn1] & PTE_V) == 0) {
        void *new_page = kmalloc(PAGE_SIZE);
        memset(new_page, 0, PAGE_SIZE);
        
        table[vpn1] = (PFN_DOWN(VA_TO_PA(new_page)) << 10) | PTE_V;
    }

    unsigned long pa0 = (table[vpn1] >> 10) << 12;
    table = (unsigned long *)PA_TO_VA(pa0);

    int vpn0 = (va >> 12) & 0x1FF;
    
    table[vpn0] = (PFN_DOWN(pa) << 10) | prot;
}

void map_pages(unsigned long va, unsigned long size, unsigned long pa, unsigned long prot) {
    for (int i = 0; i < size; i += PAGE_SIZE){
        pagewalk(va + i, pa + i, prot);
    }  
}