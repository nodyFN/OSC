#ifndef __UART_H__
#define __UART_H__

#include <stdint.h> // 建議引入這個以使用 uint8_t / uint32_t

/* ==========================================
 * QEMU 環境 (當 Makefile 傳入 -DQEMU 時啟用)
 * ========================================== */
#ifdef QEMU

    /* QEMU Virt Machine UART Base */
    #define UART_BASE 0x10000000

    /* * QEMU 模擬的是標準 16550A
     * Register Width = 1 (uint8_t)
     * Register Shift = 0 (Offset 不用乘)
     */
    #define UART_THR  (volatile uint8_t *)(UART_BASE + 0x00) // 0x00
    #define UART_RBR  (volatile uint8_t *)(UART_BASE + 0x00) // 0x00
    #define UART_LSR  (volatile uint8_t *)(UART_BASE + 0x05) // 0x05 (標準 Offset)

/* ==========================================
 * Orange Pi RV2 環境 (預設)
 * ========================================== */
#else

    /* OpiRV2 UART Base */
    #define UART_BASE 0xd4017000

    /* * SpacemiT K1 UART
     * Register Width = 4 (uint32_t)
     * Register Shift = 2 (Offset * 4)
     */
    #define UART_THR  (volatile uint32_t *)(UART_BASE + 0x00) // 0x00
    #define UART_RBR  (volatile uint32_t *)(UART_BASE + 0x00) // 0x00
    #define UART_LSR  (volatile uint32_t *)(UART_BASE + 0x14) // 0x05 * 4 = 0x14

#endif

/* LSR Bit Masks (兩者通用) */
#define LSR_RX_READY 0x01
#define LSR_TX_IDLE  0x20

/* --- 函式宣告 --- */
void uart_init();
void uart_putc(char c);
void uart_puts(const char *s);
char uart_getc();

/* 簡單的 64-bit 十六進位輸出 */
void uart_hex(unsigned long value);
void uart_hex_32(uint32_t value);
void uart_hex_no_newline(unsigned long value);
void uart_hex_no_newline_32(uint32_t value);

#define KEY_ENTER 13      // \r
#define KEY_BACKSPACE 127 // or 8 (\b)
#define KEY_ESC 27

#endif // _UART_H