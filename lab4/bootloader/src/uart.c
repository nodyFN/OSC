#include "uart.h"
#include "fdt.h"
#include "utils.h"
#include <stdint.h>


uint64_t UART_BASE;

void uart_init(void* dtb) {
    // UART_BASE = 0xd4017000;
    int uart_offset = -1;
    uart_offset = fdt_path_offset(dtb, "/soc/serial");
    if(uart_offset < 0){
        uart_puts("[Warning] No uart address\n");
    }else{
        int len;
        uint32_t* prop = (uint32_t*)fdt_getprop(dtb, uart_offset, "reg", &len);
        if(!prop){
            uart_puts("[Warning] No uart address\n");
        }else{
            UART_BASE = ((uint64_t)toLittleEndian((*prop))<<32 | (uint64_t)toLittleEndian(*(prop+1)));
            uart_puts("uart addr: ");
            uart_hex(UART_BASE);
        }
    }

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