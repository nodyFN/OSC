#include <stdarg.h>
#include "stdio.h"
#include "uart.h"

// 1. 基礎輸出函式 wrapper
void _putchar(char c) {
    if (c == '\n') {
        uart_putc('\r'); // 處理換行，補上回車
    }
    uart_putc(c);
}

// 2. 印出字串
void _puts(char *s) {
    if (!s) s = "(null)";
    while (*s) {
        _putchar(*s++);
    }
}

void _print_binary(uint64_t value, int bits) {
    char buffer[64];
    for(int j=0; j<64; j++){
        buffer[j] = '0';
    }

    _puts("0b");
    if(value == 0){
        for(int j=0; j<bits; j++){
            _putchar('0');
        }
        return;
    }
    int i=0;
    while(value){
        if(value & 1){
            buffer[63-i] = '1';
        }else{
            buffer[63-i] = '0';
        }
        value >>= 1;
        i++;
    }
    
    for(int j=64-bits; j<64; j++){
        _putchar(buffer[j]);
    }
    return;
}

void _print_hex(uint64_t value, int bits) {
    _puts("0x");
    if(bits == 64){     
        uart_hex_no_newline(value);
    }else{
        uart_hex_no_newline_32((uint32_t)value);
    }

}

void _print_decimal(int64_t value) {
    if (value < 0) {
        _putchar('-');
        value = -value; // 轉成正數處理
    }
    
    // 特殊情況：如果是 0，直接印出 '0'
    if (value == 0) {
        _putchar('0');
        return;
    }

    char buffer[20]; // 足夠存放 64-bit 整數的字串
    int i = 0;

    // 將數字轉成字串（反向存儲）
    while (value > 0) {
        buffer[i++] = (value % 10) + '0'; // 將餘數轉成字元
        value /= 10;
    }

    // 反向印出字串
    while (i-- > 0) {
        _putchar(buffer[i]);
    }

}

// // 3. 核心：數字轉字串 (Integer to String)
// // val: 數值
// // base: 進位制 (10 或 16)
// // sign: 是否為有號數 (1 為有號, 0 為無號)
// void _print_num(long val, int base, int sign) {
//     char buf[32]; // 暫存 buffer，足夠存 64-bit 二進位
//     int i = 0;
//     unsigned long uval;

//     if (sign && (val < 0)) {
//         uval = -val;
//         _putchar('-');
//     } else {
//         uval = (unsigned long)val;
//     }

//     // 處理特殊情況：0
//     if (uval == 0) {
//         _putchar('0');
//         return;
//     }

//     // 數字轉字串 (注意：算出來是反的)
//     while (uval > 0) {
//         int d = uval % base;
//         // 如果大於 9 (16進位)，轉成 A-F，否則轉成 0-9
//         buf[i++] = (d >= 10) ? (d - 10 + 'A') : (d + '0');
//         uval /= base;
//     }

//     // 反向印出
//     while (i-- > 0) {
//         _putchar(buf[i]);
//     }
// }

// void printf(const char *format, ...){
//     va_list args;
//     va_start(args, format); // 初始化參數列表

//     while (*format) {
//         // 如果不是 %，直接印出
//         if (*format != '%') {
//             _putchar(*format);
//             format++;
//             continue;
//         }

//         // 遇到 %，看下一個字元是什麼
//         format++; 
        
//         // 處理格式
//         switch (*format) {
//             case 'd': // 整數 (有號)
//                 _print_num(va_arg(args, int), 10, 1);
//                 break;
//             case 'x': // 16進位 (無號, 小寫通常用同一套邏輯，這裡偷懶全印大寫)
//             case 'X':
//             case 'p': // 指標 (通常也印 16 進位)
//                 _print_num(va_arg(args, unsigned long), 16, 0);
//                 break;
//             case 's': // 字串
//                 _puts(va_arg(args, char *));
//                 break;
//             case 'c': // 字元
//                 // 注意：va_arg 抓 char 會自動升級成 int
//                 _putchar((char)va_arg(args, int));
//                 break;
//             case '%': // 印出 '%' 本身
//                 _putchar('%');
//                 break;
//             case 'l': // 支援 %ld (long int)
//                 format++;
//                 if (*format == 'd') {
//                     _print_num(va_arg(args, long), 10, 1);
//                 } else if (*format == 'x' || *format == 'X') {
//                     _print_num(va_arg(args, unsigned long), 16, 0);
//                 } else {
//                     // 不支援的格式，原樣印出
//                     _putchar('%');
//                     _putchar('l');
//                     _putchar(*format);
//                 }
//                 break;
//             default: // 不支援的格式，原樣印出
//                 _putchar('%');
//                 _putchar(*format);
//                 break;
//         }
//         format++;
//     }

//     va_end(args); // 清理
// }

void printf(const char *format, ...){
    va_list args;
    va_start(args, format); // 初始化參數列表

    while (*format) {
        // 如果不是 %，直接印出
        if (*format != '%') {
            _putchar(*format);
            format++;
            continue;
        }

        // 遇到 %，看下一個字元是什麼
        format++; 
        
        // 處理格式
        switch (*format) {
            case 'd': // 整數 (有號)
                _print_decimal(va_arg(args, int64_t));
                break;
            case 'x': // 16進位 32 bits
                _print_hex(va_arg(args, uint32_t), 32);
                break;
            case 'p': // 指標 (通常也印 16 進位)
                _print_hex(va_arg(args, uint32_t), 32);
                break;
            case 's': // 字串
                _puts(va_arg(args, char *));
                break;
            case 'c': // 字元
                // 注意：va_arg 抓 char 會自動升級成 int
                _putchar((char)va_arg(args, int));
                break;
            case '%': // 印出 '%' 本身
                _putchar('%');
                break;
            case 'b': // 二進位輸出, 32 bits
                _print_binary(va_arg(args, uint32_t), 32);
                break;
            case 'l': // 支援 %ld (long int)
                format++;
                if (*format == 'd') {
                    _print_decimal(va_arg(args, int64_t));
                } else if (*format == 'x') {
                    _print_hex(va_arg(args, uint64_t), 64);
                } else if (*format == 'b') {
                    _print_binary(va_arg(args, uint64_t), 64);
                } else if (*format == 'p') {
                    _print_hex(va_arg(args, uint64_t), 64);
                
                }else {
                    // 不支援的格式，原樣印出
                    _putchar('%');
                    _putchar('l');
                    _putchar(*format);
                }
                break;
            default: // 不支援的格式，原樣印出
                _putchar('%');
                _putchar(*format);
                break;
        }
        format++;
    }

    va_end(args); // 清理
}

void printf_test(void *dtb_addr){
    printf("decimal: %d\n", 12345);
    printf("long decimal: %ld\n", 1234567890L);
    printf("hex: %x\n", 0x1234abcd);
    printf("long hex: %lx\n", 0x1234567890abcdefL);
    printf("hex(decimal): %x\n", 1234);
    printf("binary(decimal): %b\n", 1234);
    printf("binary(hex): %b\n", 0x4d2);
    printf("long binary(decimal): %lb\n", 1234567890L);
    printf("long binary(hex): %lb\n", 0x499602D2L);
    printf("string: %s\n", "Hello, Orange Pi!");

    // printf("pointer test, dtb_addr: %p\n", (uint32_t)dtb_addr);
    printf("long pointer test, dtb_addr: %lp\n", (uint64_t)dtb_addr);
    uart_puts("(uart) DTB is at: ");
    uart_hex((unsigned long)dtb_addr);
}