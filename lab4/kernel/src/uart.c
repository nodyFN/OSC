#include "uart.h"
#include <stdint.h>

#ifdef QEMU
    // 補上 QEMU (16550A) 中斷所需的暫存器 (直接轉成指標)
    #define UART_IER  (volatile uint8_t *)(UART_BASE + 0x01)
    #define UART_IIR  (volatile uint8_t *)(UART_BASE + 0x02)
    #define UART_FCR  (volatile uint8_t *)(UART_BASE + 0x02)
    #define UART_LCR  (volatile uint8_t *)(UART_BASE + 0x03)

    // 讀寫緩衝區 (Ring Buffer)
    #define BUF_SIZE 256
    static char rx_buf[BUF_SIZE];
    static volatile int rx_head = 0;
    static volatile int rx_tail = 0;

    static char tx_buf[BUF_SIZE];
    static volatile int tx_head = 0;
    static volatile int tx_tail = 0;
#endif

void uart_init() {
#ifdef QEMU
    *UART_IER = 0x00; // 先關閉所有中斷
    *UART_LCR = 0x80; // 開啟 DLAB，準備設定 Baud Rate
    *UART_THR = 0x03; // DLL (除頻器低位元)
    *UART_IER = 0x00; // DLM (除頻器高位元)
    *UART_LCR = 0x03; // 8 bits, 無 parity, 1 stop bit
    *UART_FCR = 0x07; // 開啟 FIFO 緩衝區
    
    // 💥 關鍵：開啟 RX (接收) 中斷 💥
    *UART_IER = 0x01; 
#else
    // 實體板子 (Orange Pi) 如果還沒要實作中斷，先維持現狀或留空
#endif
}

void uart_isr() {
#ifdef QEMU
    while (1) {
        uint8_t iir = *UART_IIR;
        if (iir & 0x01) break; // 0x01 代表沒有待處理的中斷
        
        uint8_t id = (iir >> 1) & 0x0F;
        
        if (id == 2 || id == 6) { 
            // 收到資料 (RX)
            while (*UART_LSR & LSR_RX_READY) {
                char c = *UART_RBR;
                int next_head = (rx_head + 1) % BUF_SIZE;
                if (next_head != rx_tail) {
                    rx_buf[rx_head] = c;
                    rx_head = next_head;
                }
            }
        } else if (id == 1) { 
            // 硬體準備好發送下一筆資料了 (TX)
            if (tx_head != tx_tail) {
                *UART_THR = tx_buf[tx_tail];
                tx_tail = (tx_tail + 1) % BUF_SIZE;
            } else {
                // Buffer 沒東西了，暫時關閉 TX 中斷
                *UART_IER &= ~0x02;
            }
        }
    }
#endif
}

char uart_getc() {
#ifdef QEMU
    while (rx_head == rx_tail) {
        check_timer_events(); 
    }
    char c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % BUF_SIZE;
    return c;
#else
    // 實體板子 (Polling 模式)
    while ((*UART_LSR & LSR_RX_READY) == 0) {
        // 在等待時，不斷檢查是否有鬧鐘響起
        check_timer_events(); 
        
        // 加入一個極短的延遲，避免 while 迴圈跑太快把 CPU 卡死
        for (volatile int i = 0; i < 1000; i++); 
    }
    return *UART_RBR;
#endif
}

void uart_putc(char c) {
#ifdef QEMU
    int next_head = (tx_head + 1) % BUF_SIZE;
    
    // 💥 防死鎖機制：如果 Buffer 滿了
    // 代表目前可能處於「中斷關閉」的狀態 (例如 Kernel 剛開機)
    // 我們主動檢查硬體是否閒置，如果是，就手動把最舊的字元推出去，強行騰出空間！
    while (next_head == tx_tail) {
        if (*UART_LSR & LSR_TX_IDLE) {
            *UART_THR = tx_buf[tx_tail];
            tx_tail = (tx_tail + 1) % BUF_SIZE;
        }
    }
    
    tx_buf[tx_head] = c;
    tx_head = next_head;
    
    // 寫入 Buffer 後，主動打開 TX 中斷，讓硬體自動把 Buffer 內的字元吃走
    *UART_IER |= 0x02;
#else
    while ((*UART_LSR & LSR_TX_IDLE) == 0);
    *UART_THR = c;
#endif
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
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