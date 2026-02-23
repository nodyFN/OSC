#include "uart.h"
#include <stdint.h>

void uart_init() {
    // QEMU virt 機器的 UART Base Address 是 0x10000000
    volatile uint8_t *uart = (uint8_t *)0x10000000;
    
    uart[1] = 0x00; // IER: 關閉所有中斷
    uart[3] = 0x80; // LCR: 開啟 DLAB (設定 Baud Rate 需要)
    uart[0] = 0x03; // DLL: 設定 Baud Rate (38400 baud)
    uart[1] = 0x00; // DLM: 
    uart[3] = 0x03; // LCR: 8 bits, 無 parity, 1 stop bit
    uart[2] = 0xC7; // FCR: 開啟 FIFO 緩衝區，並清空 TX/RX
}

void uart_putc(char c) {
    while ((*UART_LSR & LSR_TX_IDLE) == 0);
    *UART_THR = c;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

char uart_getc() {
    while ((*UART_LSR & LSR_RX_READY) == 0);

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