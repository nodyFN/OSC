#include "uart.h"
#include <stdint.h>

void uart_init() {

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