#include <stdint.h>
#include "plic.h"
#include "fdt.h"
#include "stdio.h"
#include "utils.h"
#include "uart.h"

extern struct KernelInfo kernel_info;

uint64_t PLIC_BASE = 0;
uint64_t UART_IRQ = 0;

void get_plic_base(){
    int plic_offset = -1;
    #ifdef QEMU
        plic_offset = fdt_path_offset(kernel_info.dtb_addr, "/soc/plic");
    #else
        plic_offset = fdt_path_offset(kernel_info.dtb_addr, "/soc/interrupt-controller");
    #endif
    if(plic_offset == -1){
        printf("[Warning] can not find plic [1]\n");
    }else{
        int len;
        uint32_t* prop = (uint32_t*)fdt_getprop(kernel_info.dtb_addr, plic_offset, "reg", &len);
        if (!prop){
            printf("[Warning] can not find plic [2]\n");
        }else{
            PLIC_BASE = ((uint64_t)toLittleEndian((*prop))<<32 | (uint64_t)toLittleEndian(*(prop+1)));
            // printf("PLIC_BASE: %lx\n", PLIC_BASE);
        }
    }
}

void get_uart_irq(){
    int uart0_offset = fdt_path_offset(kernel_info.dtb_addr, "/soc/serial");
    if(uart0_offset == -1){
        printf("[Warning] can not find uart0 [1]\n");
    }else{
        int len;
        uint32_t* prop = (uint32_t*)fdt_getprop(kernel_info.dtb_addr, uart0_offset, "interrupts", &len);
        if (!prop){
            printf("[Warning] can not find uart0 [2]\n");
        }else{
            UART_IRQ = (uint64_t)toLittleEndian((*prop));
            // printf("UART_IRQ: %lx\n", UART_IRQ);
        }
    }
}

void enable_external_interrupt() {
    __asm__ volatile(
        "csrs sie, %0"
        :
        : "r"(1 << 9)
    );
}

void plic_init(){
    get_plic_base();
    get_uart_irq();

    // (1) Set UART interrupt priority
    *PLIC_PRIORITY(UART_IRQ) = 1;
    volatile uint32_t *enable_reg_array = PLIC_ENABLE(kernel_info.hartid);
    
    // (2) Set UART interrupt enable for the boot hart
    int reg_index = UART_IRQ / 32;
    int bit_offset = UART_IRQ % 32;
    enable_reg_array[reg_index] |= (1U << bit_offset);

    // (3) Set threshold for the boot hart
    *PLIC_THRESHOLD(kernel_info.hartid) = 0;

    // (4) Enable external interrupts
    enable_external_interrupt();
}

int plic_claim() {
    return *PLIC_CLAIM(kernel_info.hartid);
}

void plic_complete(int irq) {
    *PLIC_CLAIM(kernel_info.hartid) = irq;
}