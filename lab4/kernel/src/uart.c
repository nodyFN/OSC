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

int uart_async_enabled = 0;

void uart_flush() {
    // 0x40 (二進位 0100 0000) 是 LSR 暫存器的 TEMT (Transmitter Empty) 位元
    // 當它變成 1 時，代表不僅軟體緩衝區空了，連 UART 硬體底層的「實體移位暫存器」也把最後一個 bit 送出去了。
    while ((*UART_LSR & 0x40) == 0) {
        // 死等，直到硬體傳輸完全結束
    }
}

void uart_init() {
    uart_puts("[UART_INIT]: enable rx interrupt\n");
    uart_flush();
    // Enable RX and TX interrupt
    *UART_IER = 0x01;

    uart_puts("[UART_INIT]: enable MCR\n");
    uart_flush();
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
    if (!uart_async_enabled) {
        // 【早期模式】死等硬體發送完畢，確保文字一定印得出來！
        while ((*UART_LSR & LSR_TX_IDLE) == 0);
        *UART_THR = c;
    } else {
        // 【非同步模式】中斷系統 Ready 後才使用這套
        tx_buf.data[tx_buf.head] = c;
        tx_buf.head = (tx_buf.head + 1) % BUF_SIZE;
        *UART_IER |= 0x02;
    }
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