/* kernel/src/plic.c */
#include "plic.h"

// QEMU Virt 機器的 PLIC 記憶體映射位址
#define PLIC_BASE 0x0c000000L
#define PLIC_PRIORITY(id)     (PLIC_BASE + (id) * 4)
#define PLIC_MENABLE(hart)    (PLIC_BASE + 0x2000 + (hart) * 0x80)
#define PLIC_MTHRESHOLD(hart) (PLIC_BASE + 0x200000 + (hart) * 0x1000)
#define PLIC_MCLAIM(hart)     (PLIC_BASE + 0x200004 + (hart) * 0x1000)

void plic_init() {
    int uart_irq = 10; // QEMU 中 UART0 的中斷號碼是 10
    int hart = 0;      // Boot hart 是 0

    // 1. 設定 UART 中斷優先級為 1 (必須大於 0 才會觸發)
    *(volatile uint32_t*)PLIC_PRIORITY(uart_irq) = 1;

    // 2. 允許 Hart 0 的 M-mode 接收此中斷
    *(volatile uint32_t*)PLIC_MENABLE(hart) = (1 << uart_irq);

    // 3. 設定門檻為 0 (接受所有大於 0 的優先級)
    *(volatile uint32_t*)PLIC_MTHRESHOLD(hart) = 0;
}

uint32_t plic_claim() {
    // 讀取 Claim 暫存器，得知是哪個設備觸發中斷
    return *(volatile uint32_t*)PLIC_MCLAIM(0);
}

void plic_complete(uint32_t irq) {
    // 寫回 Claim 暫存器，告訴 PLIC 中斷處理完畢
    *(volatile uint32_t*)PLIC_MCLAIM(0) = irq;
}