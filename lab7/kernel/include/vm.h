#ifndef __VM_H__
#define __VM_H__

#include <stdint.h>

/* Memory map */
#define PAGE_OFFSET   0xffffffc000000000UL
#define PMD_SIZE      (1UL << 21)
#define PGD_SIZE      (1UL << 30)

/* VA bit-field shifts (Sv39) */
#define PGD_SHIFT     30 
#define PMD_SHIFT     21
#define PTE_SHIFT     12

#define ENTRIES_PER_TABLE  512

#define KERNEL_PGD_INDEX   ((PAGE_OFFSET >> PGD_SHIFT) & 0x1FF)

#define LINEAR_MAP_GIB     4

/* PTE descriptor bits (Sv39) */
#define PTE_V  (1UL << 0)  
#define PTE_R  (1UL << 1)
#define PTE_W  (1UL << 2)
#define PTE_X  (1UL << 3)
#define PTE_U  (1UL << 4)
#define PTE_G  (1UL << 5)
#define PTE_A  (1UL << 6)
#define PTE_D  (1UL << 7)

#define PROT_KERNEL  (PTE_V | PTE_R | PTE_W | PTE_X | PTE_G | PTE_A | PTE_D)
#define PROT_MMIO   (PTE_V | PTE_R | PTE_W | PTE_G | PTE_A | PTE_D)
#define PROT_CODE   (PTE_U | PTE_R | PTE_X | PTE_V | PTE_A | PTE_D)
#define PROT_TRAMPOLINE_CODE (PTE_U | PTE_R | PTE_X | PTE_V | PTE_A | PTE_D)
#define PROT_STACK  (PTE_U | PTE_R | PTE_W | PTE_V | PTE_A | PTE_D)
#define PROT_SIG_STACK (PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_A | PTE_D)

#define SATP_SV39           (8UL << 60)
#define MAKE_SATP(pgd_pa)   (SATP_SV39 | ((unsigned long)(pgd_pa) >> 12))

#define MAKE_PTE(pa, flags) ((((unsigned long)(pa)) >> 12) << 10 | (flags))

#define VA_TO_PA(x) (((unsigned long)(x) >= PAGE_OFFSET) ? ((unsigned long)(x) - PAGE_OFFSET) : (unsigned long)(x))
#define PA_TO_VA(x) (((unsigned long)(x) <= PAGE_OFFSET) ? ((unsigned long)(x) + PAGE_OFFSET) : (unsigned long)(x))

#define USER_SIGNAL_STACK_VA  (USER_STACK_VA - USER_STACK_SIZE - PAGE_SIZE)

#define PFN_DOWN(x) ((x) >> 12)

void setup_vm();
void drop_identity_map();
void setup_finer_granularity_paging();
void map_pages(uint64_t *pgd, unsigned long va, unsigned long size, unsigned long pa, unsigned long prot);

#endif
