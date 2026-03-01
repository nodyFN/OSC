/* kernel/src/uart.c */
#include <stdint.h>
#include "uart.h"
#include "timer.h"
#include "task.h" 
#include "riscv.h"
#include "stdio.h"

#define BUF_SIZE 256
static char rx_buf[BUF_SIZE];
static volatile int rx_head = 0;
static volatile int rx_tail = 0;

// Bottom Half 任務 (QEMU 與板子共用)
void uart_rx_bottom_half(void *arg) {
    char c = (char)(uintptr_t)arg; 
    int next_head = (rx_head + 1) % BUF_SIZE;
    if (next_head != rx_tail) {
        rx_buf[rx_head] = c;
        rx_head = next_head;
    }
}

// void uart_init() {
// #ifdef QEMU
//     *UART_FCR = 0x07; 
//     *UART_MCR = 0x0B;
//     *UART_IER = 0x01; 
// #else
//     // 💥 實體板子 (SpacemiT K1)：依照最新 Spec，只需設定 IER！
//     // 1. 等待 Bootloader 把最後的 Log 印完，避免 Race Condition
//     for (volatile int i = 0; i < 5000000; i++);
//     while ((*UART_LSR & LSR_TX_IDLE) == 0);

//     // 2. 疊加設定 RX 中斷 (IER = 0x01)，絕對不要碰 MCR！
//     *UART_IER |= 0x01; 
// #endif
// }
void uart_init() {
    // 💥 雙平台統一：在 S-mode 下，我們儘量不要重置 UART 硬體
    // 只需要確保中斷位元被打開即可
#ifdef QEMU
    // QEMU 的 S-mode 比較寬容，但保險起見，我們只動 IER
    *UART_IER = 0x01; 
#else
    // 板子的部分我們之前改過了，維持溫柔初始化
    for (volatile int i = 0; i < 5000000; i++);
    while ((*UART_LSR & LSR_TX_IDLE) == 0);
    *UART_IER |= 0x01; 
#endif
}

void uart_isr() {
    while (1) {
        uint32_t iir = *UART_IIR;  // 使用 uint32_t 容錯板子的 32-bit
        if (iir & 0x01) break;     // 中斷處理完畢
        
        uint32_t id = (iir >> 1) & 0x0F;
        
        if (id == 2 || id == 6) { 
            // 收到外部 RX 中斷！
            while (*UART_LSR & LSR_RX_READY) {
                char c = (char)(*UART_RBR); 
                // 將搬運任務推遲到 Queue 裡面，優先權 1
                add_task(uart_rx_bottom_half, (void *)(uintptr_t)c, 1);
            }
        }
    }
}

void check_interrupt_pending() {
    // uint64_t sip_val = read_csr(sip);
    // if (sip_val & (1 << 9)) {
    //     // 如果有噴出這一行，代表 PLIC 到 CPU 的電路是通的，只是 CPU 不肯跳進 Trap
    //     printf(">> [DEBUG] SEIP is PENDING! sip: 0x%lx\n", sip_val);
    // }
}

char uart_getc() {
    // 雙平台統一：無字元時檢查 Timer
    while (rx_head == rx_tail) {
        check_timer_events(); 
        
        // 💥 在這裡加入診斷代碼
#ifndef QEMU
        check_interrupt_pending(); 
#endif

        // 稍微延遲一下，避免噴太快看不清
        for (volatile int i = 0; i < 100000; i++); 
    }
    char c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % BUF_SIZE;
    return c;
}

// // 既然我們重啟了中斷，uart_getc 也要恢復成雙平台共用的 Ring Buffer 模式！
// char uart_getc() {
//     // 雙平台統一：無字元時檢查 Timer，有字元就從 Buffer 拿！
//     while (rx_head == rx_tail) {
//         check_timer_events(); 
//     }
//     char c = rx_buf[rx_tail];
//     rx_tail = (rx_tail + 1) % BUF_SIZE;
//     return c;
// }

// char uart_getc() {
// #ifdef QEMU
//     // QEMU 擁有完美的 UART 中斷，可以享受 Ring Buffer
//     while (rx_head == rx_tail) {
//         check_timer_events(); 
//     }
//     char c = rx_buf[rx_tail];
//     rx_tail = (rx_tail + 1) % BUF_SIZE;
//     return c;
// #else
//     // 💥 實體板子 (SpacemiT K1) 終極回退：安全的 Polling 模式
//     // 因為板子的 S-mode 沒有收到 PLIC 中斷，所以我們直接死盯著硬體看！
//     while ((*UART_LSR & LSR_RX_READY) == 0) {
//         check_timer_events(); // 依然能檢查 Timer，支援 setTimeout
//         for (volatile int i = 0; i < 1000; i++); // 稍微延遲避免卡死 CPU
//     }
//     return (char)(*UART_RBR); // 直接從硬體拿字元
// #endif
// }

void uart_putc(char c) {
    if (c == '\n') uart_putc('\r');
    while ((*UART_LSR & LSR_TX_IDLE) == 0);
    *UART_THR = c;
}

void uart_puts(const char *str) {
    while (*str) {
        uart_putc(*str++);
    }
}

void uart_hex(unsigned long value) {
    const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int shift = (sizeof(unsigned long) * 8) - 4; shift >= 0; shift -= 4) {
        unsigned long nibble = (value >> shift) & 0xF;
        uart_putc(hex[nibble]);
    }
    uart_puts("\r\n");
}

void uart_hex_32(uint32_t value){
    const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int shift = (sizeof(uint32_t) * 8) - 4; shift >= 0; shift -= 4) {
        uint32_t nibble = (value >> shift) & 0xF;
        uart_putc(hex[nibble]);
    }
    uart_puts("\r\n");
}

void uart_hex_no_newline(unsigned long value) {
    const char hex[] = "0123456789ABCDEF";
    for (int shift = (sizeof(unsigned long) * 8) - 4; shift >= 0; shift -= 4) {
        unsigned long nibble = (value >> shift) & 0xF;
        uart_putc(hex[nibble]);
    }
}

void uart_hex_no_newline_32(uint32_t value){
    const char hex[] = "0123456789ABCDEF";
    for (int shift = 28; shift >= 0; shift -= 4) {
        uint32_t nibble = (value >> shift) & 0xF;
        uart_putc(hex[nibble]);
    }
}