#include "uart.h"
#include <stdint.h>

/* * Spec: You do not need to initialize the UART manually.
 * U-Boot 已經幫你設好 Baud rate 了。
 */
void uart_init() {
    // 這裡留空即可
}

/* 輸出一個字元 */
void uart_putc(char c) {
    // 1. 等待直到 Transmitter Empty (LSR 的第 5 bit 變為 1)
    while ((*UART_LSR & LSR_TX_IDLE) == 0);
    
    // 2. 將字元寫入 THR
    *UART_THR = c;
}

/* 輸出字串 */
void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r'); // 先回到行首
        }
        uart_putc(*s++);
    }
}

/* 讀取一個字元 (Optional, Shell 用) */
char uart_getc() {
    // 1. 等待直到 Data Ready (LSR 的第 0 bit 變為 1)
    while ((*UART_LSR & LSR_RX_READY) == 0);
    
    // 2. 讀取 RBR
    return (char)(*UART_RBR & 0xFF);
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
    // 只印數字，不印 \r\n
    const char hex[] = "0123456789ABCDEF";
    for (int shift = (sizeof(unsigned long) * 8) - 4; shift >= 0; shift -= 4) {
        unsigned long nibble = (value >> shift) & 0xF;
        uart_putc(hex[nibble]);
    }
}

void uart_hex_no_newline_32(uint32_t value){
    // 只印數字，不印 \r\n
    const char hex[] = "0123456789ABCDEF";
    for (int shift = 28; shift >= 0; shift -= 4) {
        uint32_t nibble = (value >> shift) & 0xF;
        uart_putc(hex[nibble]);
    }
}