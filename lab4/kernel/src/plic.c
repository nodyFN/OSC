#include <stdint.h>
#include "plic.h"

struct KernelInfo {
    uint64_t hartid;
    void *dtb_addr;
    uint64_t initrd_start_addr;
    uint64_t initrd_end_addr;
};
extern struct KernelInfo kernel_info;

#ifdef QEMU
    #define PLIC_BASE 0x0c000000L
    #define UART0_IRQ 10

    void plic_init() {
        *(volatile uint32_t *)(PLIC_BASE + UART0_IRQ * 4) = 1; 
        *(volatile uint32_t *)(PLIC_BASE + 0x2000) = (1 << UART0_IRQ); 
        *(volatile uint32_t *)(PLIC_BASE + 0x200000) = 0; 
    }

    uint32_t plic_claim() {
        return *(volatile uint32_t *)(PLIC_BASE + 0x200004);
    }

    void plic_complete(uint32_t irq) {
        *(volatile uint32_t *)(PLIC_BASE + 0x200004) = irq;
    }
#else
    // Orange Pi RV2 (OpiRV2) 參數
    #define PLIC_BASE 0xe0000000L
    #define UART0_IRQ 0x2a // 42

    void plic_init() {
        *(volatile uint32_t *)(PLIC_BASE + UART0_IRQ * 4) = 1;

        // 把 Context 0 到 3 的 UART0 中斷全部打開
        for (int ctx = 0; ctx < 4; ctx++) {
            uint64_t en_addr = PLIC_BASE + 0x2000 + ctx * 0x80 + (UART0_IRQ / 32) * 4;
            *(volatile uint32_t *)en_addr |= (1 << (UART0_IRQ % 32));

            uint64_t th_addr = PLIC_BASE + 0x200000 + ctx * 0x1000;
            *(volatile uint32_t *)th_addr = 0;
        }
    }

    static uint64_t current_claim_addr = 0;

    uint32_t plic_claim() {
        for (int ctx = 0; ctx < 4; ctx++) {
            uint64_t claim_addr = PLIC_BASE + 0x200004 + ctx * 0x1000;
            uint32_t irq = *(volatile uint32_t *)claim_addr;
            if (irq != 0) {
                current_claim_addr = claim_addr;
                return irq;
            }
        }
        return 0;
    }

    void plic_complete(uint32_t irq) {
        if (current_claim_addr != 0) {
            *(volatile uint32_t *)current_claim_addr = irq;
        }
    }
#endif