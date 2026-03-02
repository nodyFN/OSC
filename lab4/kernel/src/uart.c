/* kernel/src/uart.c */
#include <stdint.h>
#include "uart.h"
#include "timer.h"
#include "task.h" 
#include "riscv.h"
#include "stdio.h"

#define BUF_SIZE 256

// 建立 RX 與 TX 緩衝區
static char rx_buf[BUF_SIZE];
static volatile int rx_head = 0;
static volatile int rx_tail = 0;

static char tx_buf[BUF_SIZE];
static volatile int tx_head = 0;
static volatile int tx_tail = 0;

// Bottom Half 任務：處理接收到的字元
void uart_rx_bottom_half(void *arg) {
    char c = (char)(uintptr_t)arg; 
    int next_head = (rx_head + 1) % BUF_SIZE;
    if (next_head != rx_tail) {
        rx_buf[rx_head] = c;
        rx_head = next_head;
    }
}

void uart_init() {
#ifdef QEMU
    *UART_IER = 0x01; 
#else
    for (volatile int i = 0; i < 5000000; i++);
    while ((*UART_LSR & LSR_TX_IDLE) == 0);
    
    // 💥 啟動 RX 中斷
    *UART_IER |= 0x01; 
    
    // 💥 開啟 OUT2，確保硬體中斷線路接通 PLIC
    *UART_MCR |= 0x08; 
#endif
}

// 💥 完美版 ISR：同時處理 RX 接收與 TX 發送
void uart_isr() {
    while (1) {
        uint32_t iir = *UART_IIR; 
        if (iir & 0x01) break; // 1 代表沒有中斷了，跳出迴圈
        
        uint32_t id = (iir >> 1) & 0x0F;
        
        if (id == 2 || id == 6) { 
            // 收到 RX 中斷 (有字元進來)
            while (*UART_LSR & LSR_RX_READY) {
                char c = (char)(*UART_RBR & 0xFF); 
                add_task(uart_rx_bottom_half, (void *)(uintptr_t)c, 1);
            }
        }
        else if (id == 1) { 
            // 💥 收到 TX 中斷 (硬體發送完畢，THR 空了)
            if (tx_head != tx_tail) {
                // Buffer 裡面還有字，拿一個出來交給硬體發送
                *UART_THR = tx_buf[tx_tail];
                tx_tail = (tx_tail + 1) % BUF_SIZE;
            } else {
                // Buffer 空了，暫時關閉 TX 中斷，避免無限觸發
                *UART_IER &= ~0x02; 
            }
        }
    }
}

// 💥 真正的 Non-blocking Input
char uart_getc() {
    // U-mode 純等待，Kernel 會在中斷背景把 rx_buf 填滿
    while (rx_head == rx_tail); 
    
    char c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % BUF_SIZE;
    return c;
}

// 💥 預設為 0，代表還在開機早期，中斷尚未啟用
volatile int uart_interrupts_ready = 0;
// void uart_putc(char c) {
//     if (c == '\n') uart_putc('\r');

//     // 💡 邏輯切換：如果中斷還沒準備好 (開機階段)，強制使用 Polling
//     if (!uart_interrupts_ready) {
// #ifdef QEMU
//         while ((*UART_LSR & 0x20) == 0); // QEMU 的 LSR_TX_IDLE
// #else
//         while ((*UART_LSR & 0x20) == 0); // 板子的 LSR_TX_IDLE
// #endif
//         *UART_THR = c;
//         return;
//     }

//     // 💡 進入 Shell 後，全面啟動 Non-blocking
//     int next_head = (tx_head + 1) % BUF_SIZE;
    
//     // 稍微阻塞等待 (避免 Buffer 溢位)
//     while (next_head == tx_tail); 
    
//     tx_buf[tx_head] = c;
//     tx_head = next_head;
    
//     // 啟動 TX 中斷
//     *UART_IER |= 0x02; 
// }
void uart_putc(char c) {
    if (c == '\n') uart_putc('\r');

    // 1. 開機早期的 Polling 模式
    if (!uart_interrupts_ready) {
        while ((*UART_LSR & 0x20) == 0);
        *UART_THR = c;
        return;
    }

    // ==========================================
    // 🚀 100% 純 Non-blocking 輸出
    // ==========================================
    int next_head = (tx_head + 1) % BUF_SIZE;
    
    // Buffer 滿了才稍作等待
    while (next_head == tx_tail); 
    
    tx_buf[tx_head] = c;
    tx_head = next_head;
    
    *UART_IER |= 0x02; // 開啟 TX 中斷
    
    // 💥 針對 QEMU (16550 UART) 的引擎點火機制！
    // 解決中斷「解除武裝」的問題，手動送出第一發子彈！
#ifdef QEMU
    if (*UART_LSR & 0x20) { // 如果硬體現在是閒置的
        if (tx_head != tx_tail) {
            *UART_THR = tx_buf[tx_tail];
            tx_tail = (tx_tail + 1) % BUF_SIZE;
        }
    }
#endif
}

// void uart_putc(char c) {
//     if (c == '\n') uart_putc('\r');
//     while ((*UART_LSR & LSR_TX_IDLE) == 0);
//     *UART_THR = c;
// }


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