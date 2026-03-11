#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>

#ifdef QEMU
    #define UART_BASE 0x10000000

    #define UART_THR  (volatile uint8_t *)(UART_BASE + 0x00) // 0x00
    #define UART_RBR  (volatile uint8_t *)(UART_BASE + 0x00) // 0x00
    #define UART_LSR  (volatile uint8_t *)(UART_BASE + 0x05) // 0x05 (標準 Offset)

#else
    #define UART_BASE 0xd4017000

    /* * SpacemiT K1 UART
     * Register Width = 4 (uint32_t)
     * Register Shift = 2 (Offset * 4)
     */
    #define UART_THR  (volatile uint32_t *)(UART_BASE + 0x00) // 0x00
    #define UART_RBR  (volatile uint32_t *)(UART_BASE + 0x00) // 0x00
    #define UART_LSR  (volatile uint32_t *)(UART_BASE + 0x14) // 0x05 * 4 = 0x14

#endif

#define LSR_RX_READY 0x01
#define LSR_TX_IDLE  0x20

void uart_init();
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