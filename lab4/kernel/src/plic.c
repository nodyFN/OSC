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
        // 設定中斷優先權
        *(volatile uint32_t *)(PLIC_BASE + UART0_IRQ * 4) = 1; 
        
        // 💥 S-mode (Context 1) 的 Enable 位址是 0x2080
        *(volatile uint32_t *)(PLIC_BASE + 0x2080) = (1 << UART0_IRQ); 
        
        // 💥 S-mode (Context 1) 的 Threshold 位址是 0x201000
        *(volatile uint32_t *)(PLIC_BASE + 0x201000) = 0; 
    }

    uint32_t plic_claim() {
        // 💥 S-mode (Context 1) 的 Claim 位址是 0x201004
        return *(volatile uint32_t *)(PLIC_BASE + 0x201004);
    }

    void plic_complete(uint32_t irq) {
        *(volatile uint32_t *)(PLIC_BASE + 0x201004) = irq;
    }
#else
    // Orange Pi RV2 (OpiRV2) 參數
    #define PLIC_BASE 0xe0000000L
    #define UART0_IRQ 0x2a // 42

    // void plic_init() {
    //     *(volatile uint32_t *)(PLIC_BASE + UART0_IRQ * 4) = 1;

    //     // 把 Context 0 到 3 的 UART0 中斷全部打開
    //     for (int ctx = 0; ctx < 4; ctx++) {
    //         uint64_t en_addr = PLIC_BASE + 0x2000 + ctx * 0x80 + (UART0_IRQ / 32) * 4;
    //         *(volatile uint32_t *)en_addr |= (1 << (UART0_IRQ % 32));

    //         uint64_t th_addr = PLIC_BASE + 0x200000 + ctx * 0x1000;
    //         *(volatile uint32_t *)th_addr = 0;
    //     }
    // }
    void plic_init() {
        // 1. 設定 UART0 (ID 42) 的優先權為最高
        *(volatile uint32_t *)(PLIC_BASE + 42 * 4) = 7; 

        // 2. 把前 16 個 Context 通通打開測試 (暴力破法)
        for (int ctx = 0; ctx < 16; ctx++) {
            // Enable register: 0x2000 + ctx * 0x80
            uint64_t en_addr = PLIC_BASE + 0x2000 + ctx * 0x80 + (42 / 32) * 4;
            *(volatile uint32_t *)en_addr |= (1 << (42 % 32));

            // Threshold register: 0x200000 + ctx * 0x1000
            uint64_t th_addr = PLIC_BASE + 0x200000 + ctx * 0x1000;
            *(volatile uint32_t *)th_addr = 0; // 門檻設為 0，不擋任何中斷
        }
    }

    static uint64_t current_claim_addr = 0;

    // uint32_t plic_claim() {
    //     for (int ctx = 0; ctx < 4; ctx++) {
    //         uint64_t claim_addr = PLIC_BASE + 0x200004 + ctx * 0x1000;
    //         uint32_t irq = *(volatile uint32_t *)claim_addr;
    //         if (irq != 0) {
    //             current_claim_addr = claim_addr;
    //             return irq;
    //         }
    //     }
    //     return 0;
    // }
    uint32_t plic_claim() {
        // 假設我們用的是 Context 1 (S-mode)
        uint32_t irq = *(volatile uint32_t *)(PLIC_BASE + 0x201004);
        return irq;
    }

    // void plic_complete(uint32_t irq) {
    //     if (current_claim_addr != 0) {
    //         *(volatile uint32_t *)current_claim_addr = irq;
    //     }
    // }
    void plic_complete(uint32_t irq) {
        *(volatile uint32_t *)(PLIC_BASE + 0x201004) = irq;
    }
#endif