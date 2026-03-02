/* kernel/src/plic.c */
#include <stdint.h>
#include "plic.h"
#include "utils.h"

extern struct KernelInfo kernel_info;

#ifdef QEMU
    #define PLIC_BASE 0x0c000000L
    #define UART0_IRQ 10
#else
    #define PLIC_BASE 0xe0000000L
    #define UART0_IRQ 42 // 0x2a
#endif

// 💥 導入助教的 SpacemiT K1 專屬公式
static inline uint64_t get_plic_enable_addr(uint64_t hart) {
    return PLIC_BASE + 0x002080 + (hart * 0x0100);
}

static inline uint64_t get_plic_threshold_addr(uint64_t hart) {
    return PLIC_BASE + 0x201000 + (hart * 0x2000);
}

static inline uint64_t get_plic_claim_addr(uint64_t hart) {
    return PLIC_BASE + 0x201004 + (hart * 0x2000);
}

void plic_init() {
    uint64_t hart = kernel_info.hartid;

    // 1. 設定 UART0 優先權
    *(volatile uint32_t *)(PLIC_BASE + UART0_IRQ * 4) = 1; 

    // 2. 設定 Enable (精確位址)
    uint64_t en_addr = get_plic_enable_addr(hart) + (UART0_IRQ / 32) * 4;
    *(volatile uint32_t *)en_addr |= (1 << (UART0_IRQ % 32));

    // 3. 設定 Threshold
    *(volatile uint32_t *)get_plic_threshold_addr(hart) = 0; 
}

uint32_t plic_claim() {
    return *(volatile uint32_t *)get_plic_claim_addr(kernel_info.hartid);
}

void plic_complete(uint32_t irq) {
    *(volatile uint32_t *)get_plic_claim_addr(kernel_info.hartid) = irq;
}