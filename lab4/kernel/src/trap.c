/* kernel/src/trap.c */
#include <stdint.h>
#include "stdio.h"
#include "riscv.h"
#include "plic.h" 
#include "task.h" // 引入 Event Queue 任務排程器

extern void timer_handler();
extern void uart_isr(); 

void trap_handler() {
    // 💥 1. 第一時間把 EPC 和 Status 讀出來存進區域變數 (保護它們不被巢狀中斷覆寫！)
#ifdef QEMU
    uint64_t epc = read_csr(mepc);
    uint64_t status = read_csr(mstatus);
    uint64_t cause = read_csr(mcause);
#else
    uint64_t epc = read_csr(sepc);
    uint64_t status = read_csr(sstatus);
    uint64_t cause = read_csr(scause);
#endif

    // 💥 2. 處理硬體中斷 (Top Half：只做最緊急的事)
    if (cause & 0x8000000000000000) {
        uint64_t exception_code = cause & 0x7FFFFFFFFFFFFFFF;

        printf("\n[Trap] Interrupt! Code: %ld\n", exception_code);
        
        // Timer 中斷 (QEMU: 7, 板子: 5)
        if (exception_code == 7 || exception_code == 5) {
            timer_handler();
        } 
#ifdef QEMU
        // QEMU: M-mode 外部中斷 (Exception Code 11)
        else if (exception_code == 11) { 
            printf("\n[Interrupt] External Interrupt Received! Checking PLIC...\n");
            uint32_t irq = plic_claim();
            if (irq == 10) uart_isr(); // QEMU 的 UART0 IRQ 是 10
            if (irq) plic_complete(irq);
        }
#else
        // 實體板子 (SpacemiT K1): S-mode 外部中斷 (Exception Code 9)
        else if (exception_code == 9) { 
            printf("\n[Interrupt] External Interrupt Received! Checking PLIC...\n");
            uint32_t irq = plic_claim();
            if (irq == 42) uart_isr(); // 板子的 UART0 IRQ 是 42
            if (irq) plic_complete(irq);
        }
#endif
    } else {
        // 💥 處理例外 (Exception)
        if (cause == 8) {
            // ecall from U-mode
            // 這裡不印出 printf 訊息，避免干擾 Shell 的非同步輸出
            epc += 4; // ecall 需要 +4，直接修改存起來的區域變數
        } else {
            // 其他未預期的 Exception (例如記憶體非法存取)，印出錯誤並死機
            printf("\n[Exception] Code: %ld, epc: 0x%lx\n", cause, epc);
            while(1); 
        }
    }

    // 💥 3. 處理 Bottom Half (任務分派與巢狀中斷)
    // 離開 Trap 之前，打開全域中斷，開始消化剛剛塞進 Queue 的 Task！
    // 這時如果有新的 Timer 中斷進來，就可以完美打斷正在執行的 Bottom Half 任務
#ifdef QEMU
    set_csr(mstatus, 1 << 3);   // 開啟 MIE，允許巢狀中斷
    process_tasks();            // 執行 Task Queue 裡的任務
    clear_csr(mstatus, 1 << 3); // 消化完畢，關閉 MIE，準備安全返回
#else
    set_csr(sstatus, 1 << 1);   // 開啟 SIE，允許巢狀中斷
    process_tasks();            // 執行 Task Queue 裡的任務
    clear_csr(sstatus, 1 << 1); // 消化完畢，關閉 SIE，準備安全返回
#endif

    // 💥 4. 恢復剛剛存起來的 EPC 和 Status
#ifdef QEMU
    write_csr(mepc, epc);
    write_csr(mstatus, status);
#else
    write_csr(sepc, epc);
    write_csr(sstatus, status);
#endif
}