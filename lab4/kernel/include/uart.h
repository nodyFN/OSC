#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>

extern uint64_t UART_BASE;

#ifdef QEMU
   typedef volatile uint8_t * uart_reg_t;
   #define UART_SHIFT 0
#else
    typedef volatile uint32_t * uart_reg_t;
    #define UART_SHIFT 2
#endif

#define UART_THR  (uart_reg_t)(UART_BASE + 0x00 * (1 << UART_SHIFT)) // 0x00
#define UART_RBR  (uart_reg_t)(UART_BASE + 0x00 * (1 << UART_SHIFT)) // 0x00
#define UART_LSR  (uart_reg_t)(UART_BASE + 0x05 * (1 << UART_SHIFT)) // 0x05
#define UART_IER  (uart_reg_t)(UART_BASE + 0x01 * (1 << UART_SHIFT)) // 0x01
#define UART_MCR  (uart_reg_t)(UART_BASE + 0x04 * (1 << UART_SHIFT)) // 0x04
#define UART_IIR  (uart_reg_t)(UART_BASE + 0x02 * (1 << UART_SHIFT)) // 0x02

#define LSR_RX_READY 0x01
#define LSR_TX_IDLE  0x20

void uart_init(void* dtb);
void uart_putc(char c);
void uart_puts(const char *s);
char uart_getc();

void uart_hex(unsigned long value);
void uart_hex_32(uint32_t value);
void uart_hex_no_newline(unsigned long value);
void uart_hex_no_newline_32(uint32_t value);

#define KEY_ENTER 13      // \r
#define KEY_BACKSPACE 127 // or 8 (\b)
#define KEY_ESC 27

#endif