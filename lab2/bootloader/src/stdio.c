#include <stdarg.h>
#include "stdio.h"
#include "uart.h"

void _putchar(char c) {
    if (c == '\n') {
        uart_putc('\r');
    }
    uart_putc(c);
}

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
        value = -value;
    }
    
    if (value == 0) {
        _putchar('0');
        return;
    }

    char buffer[20];
    int i = 0;

    while (value > 0) {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }

    while (i-- > 0) {
        _putchar(buffer[i]);
    }

}


void printf(const char *format, ...){
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format != '%') {
            _putchar(*format);
            format++;
            continue;
        }

        format++; 
        
        switch (*format) {
            case 'd':
                _print_decimal(va_arg(args, int64_t));
                break;
            case 'x':
                _print_hex(va_arg(args, uint32_t), 32);
                break;
            case 'p':
                _print_hex(va_arg(args, uint32_t), 32);
                break;
            case 's':
                _puts(va_arg(args, char *));
                break;
            case 'c':
                _putchar((char)va_arg(args, int));
                break;
            case '%':
                _putchar('%');
                break;
            case 'b':
                _print_binary(va_arg(args, uint32_t), 32);
                break;
            case 'l':
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
                    _putchar('%');
                    _putchar('l');
                    _putchar(*format);
                }
                break;
            default:
                _putchar('%');
                _putchar(*format);
                break;
        }
        format++;
    }

    va_end(args);
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