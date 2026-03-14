#include "uart.h"
#include <stdint.h>

#define BUF_SIZE 256

struct ring_buffer {
    char data[BUF_SIZE];
    volatile int head;
    volatile int tail;
};

struct ring_buffer tx_buf = { .head = 0, .tail = 0 };
struct ring_buffer rx_buf = { .head = 0, .tail = 0 };
int tx_is_empty(){ 
    return tx_buf.head == tx_buf.tail;
}

void uart_init() {
    // Enable RX and TX interrupt
    *UART_IER = 0x01;

    // Enable UART interrupt
    *UART_MCR = (1 << 3);
}

void uart_trap_handler(){
    uint8_t iir = *UART_IIR & 0x0F;

    if(iir == 0x04 || iir == 0x0C){ 
        // rx interrupt
        while(*UART_LSR & 0x01){
            char c = *UART_RBR;
            rx_buf.data[rx_buf.head] = c;
            rx_buf.head = (rx_buf.head + 1) % BUF_SIZE;
        }
    }else if(iir == 0x02){  
        // tx       
        if(!tx_is_empty()){
            char c = tx_buf.data[tx_buf.tail];
            tx_buf.tail = (tx_buf.tail + 1) % BUF_SIZE;
            *UART_THR = c; 
        }else{
            *UART_IER &= ~0x02; 
        }
    }
}

void uart_putc(char c) {
    tx_buf.data[tx_buf.head] = c;
    tx_buf.head = (tx_buf.head + 1) % BUF_SIZE;

    *UART_IER |= 0x02;
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
    while (rx_buf.head == rx_buf.tail) {
        asm volatile("wfi");
    }

    char c = rx_buf.data[rx_buf.tail];
    rx_buf.tail = (rx_buf.tail + 1) % BUF_SIZE;
    
    return c;
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