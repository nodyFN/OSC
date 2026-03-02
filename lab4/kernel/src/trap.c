/* kernel/src/trap.c */
#include <stdint.h>
#include "stdio.h"
#include "riscv.h"
#include "plic.h" 
#include "task.h" // 引入 Event Queue 任務排程器

extern void timer_handler();
extern void uart_isr(); 

extern volatile int uart_interrupts_ready;

void trap_handler() {
    // printf("\n[Trap] Entered trap handler!\n");
    uint64_t scause = read_csr(scause);
    uint64_t sepc = read_csr(sepc);
    uint64_t sstatus = read_csr(sstatus);

    // 💥 檢查是否為中斷 (Interrupt bit 為 1)
    if ((int64_t)scause < 0) { 
        uint64_t exception_code = scause & 0x7FFFFFFFFFFFFFFF;

        // Timer 中斷在 S-mode 統一是 5
        if (exception_code == 5) {
            timer_handler();
        } 
        // 外部中斷 (PLIC) 在 S-mode 統一是 9
        else if (exception_code == 9) { 
            uint32_t irq = plic_claim();
#ifdef QEMU
            if (irq == 10) uart_isr();
#else
            if (irq == 42) uart_isr();
#endif
            if (irq) plic_complete(irq);
        }
    } else {
        // 💥 這裡才是真正的 Exception (異常)
        if (scause == 8) { // ecall from U-mode
            sepc += 4;
            write_csr(sepc, sepc); // 💥 確保這一行存在，否則會無限迴圈執行同一條 ecall
        } else {
            // 💥 死亡保險：系統崩潰時，強制退回 Polling 模式！
            // 否則 printf 會卡死在 Non-blocking Buffer 裡，你什麼死因都看不到！
            uart_interrupts_ready = 0; 
            
            printf("\n[Exception] Code: %ld, epc: 0x%lx\n", scause, sepc);
            while(1); 
        }
    }

    // 處理 Task Queue (Bottom Half)
    set_csr(sstatus, 1 << 1); // 開啟 SIE
    process_tasks();
    clear_csr(sstatus, 1 << 1);

    write_csr(sepc, sepc);
    write_csr(sstatus, sstatus);
}