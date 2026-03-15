#include <stdint.h>
#include "plic.h"
#include "fdt.h"
#include "stdio.h"
#include "utils.h"

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
            printf("PLIC_BASE: %lx\n", PLIC_BASE);
        }
    }
}

void get_uart_irq(){
    printf("Getting uart_irq!!!\n");
    int uart0_offset = fdt_path_offset(kernel_info.dtb_addr, "/soc/serial");
    printf("[A]\n");
    if(uart0_offset == -1){
        printf("[B]\n");
        printf("[Warning] can not find uart0 [1]\n");
        printf("[C]\n");
    }else{
        printf("[D]\n");
        int len;
        printf("[E]\n");
        uint32_t* prop = (uint32_t*)fdt_getprop(kernel_info.dtb_addr, uart0_offset, "interrupts", &len);
        printf("[F]\n");
        if (!prop){
            printf("[G]\n");
            printf("[Warning] can not find uart0 [2]\n");
            printf("[H]\n");
        }else{
            printf("[I]\n");
            UART_IRQ = (uint64_t)toLittleEndian((*prop));
            printf("[J]\n");
            printf("UART_IRQ: %lx\n", UART_IRQ);
            printf("[K]\n");
        }
    }
    uart_flush();
}

void enable_external_interrupt() {
    asm volatile(
        "li t0, (1 << 9);"
        "csrs sie, t0;");
}

void plic_init(){
    get_plic_base();
    get_uart_irq();

    // (1) Set UART interrupt priority
    *PLIC_PRIORITY(UART_IRQ) = 1;

    // (2) Set UART interrupt enable for the boot hart
    // *PLIC_ENABLE(kernel_info.hartid) |= (1 << UART_IRQ);
    if (UART_IRQ > 0) {
        // 1. 設定優先權 (Priority)
        // PRIORITY 陣列從 PLIC_BASE + 0x0 開始，每個 IRQ 佔 4 Bytes (32-bit)
        // 這裡寫入 1 代表設定為最低優先權 (非 0 即可)
        *PLIC_PRIORITY(UART_IRQ) = 1;

        // 2. 開啟中斷 (Enable)
        // ENABLE 陣列從 PLIC_BASE + 0x2000 開始
        // 我們要設定給 Hart 0 的 S-mode (通常是 Context 1)
        // 因為 UART_IRQ 是 42，所以要寫入 ENABLE 陣列的第二個元素 (Index 1)
        volatile uint32_t *enable_reg_array = PLIC_ENABLE(kernel_info.hartid);
        
        // 計算這顆 IRQ 應該落在哪一個 32-bit 暫存器，以及推擠幾格
        int reg_index = UART_IRQ / 32;   // 例如: 42 / 32 = 1
        int bit_offset = UART_IRQ % 32;  // 例如: 42 % 32 = 10
        
        // 🚨 安全的位元操作：先確保 1 是 32-bit unsigned 整數再位移
        enable_reg_array[reg_index] |= (1U << bit_offset);

        printf("[PLIC] Enabled IRQ %d for Hart %d (Reg[%d] bit %d)\n", 
               (int)UART_IRQ, (int)kernel_info.hartid, reg_index, bit_offset);
               
    } else {
        printf("[Error] UART_IRQ is 0, skipping PLIC setup!\n");
    }
    uart_flush();

    // (3) Set threshold for the boot hart
    printf("[PLIC] Setting Threshold...\n"); 
    uart_flush();
    *PLIC_THRESHOLD(kernel_info.hartid) = 0;

    // (4) Enable external interrupts
    printf("[PLIC] Enabling external interrupts...\n"); 
    uart_flush();
    enable_external_interrupt();

    printf("[PLIC] Init complete!\n"); 
    uart_flush();
}

int plic_claim() {
    return *PLIC_CLAIM(kernel_info.hartid);
}

void plic_complete(int irq) {
    *PLIC_CLAIM(kernel_info.hartid) = irq;
}