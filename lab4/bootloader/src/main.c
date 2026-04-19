#include "uart.h"
#include "string.h"
#include "fdt.h"
#include "utils.h"
#include "initrd.h"
#include "stdio.h"
#include "shell.h"

extern char _bl_start[];
extern char _bl_end[];
int should_load_kernel;

#ifdef QEMU
    #define KERNEL_DEST_ADDR  0x80200000UL
#else
    #define KERNEL_DEST_ADDR 0x00200000UL
#endif

void* global_dtb;


uint32_t get_dtb_size(void *dtb) {
    uint8_t *p = (uint8_t *)dtb;
    return (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
}

uint32_t uart_read_uint32() {
    uint32_t val = 0;
    for (int i = 0; i < 4; i++) {
        val |= ((uint32_t)(uint8_t)uart_getc()) << (i * 8);
    }
    return val;
}

void load_kernel_from_uart(char *dest) {
    printf(">> [Loader] Waiting for Kernel Size (4 bytes)...\n");
    
    uint32_t size = uart_read_uint32();
    printf(">> [Loader] Kernel Size: %d bytes. Ready to receive.\n", size);

    for (uint32_t i = 0; i < size; i++) {
        dest[i] = uart_getc();
    }
    printf("\n>> [Loader] Kernel Loaded Successfully.\n");

    // send a signal to send_kernel.py to indicate the transfer is done
    uart_puts("OSC_DOWNLOAD_DONE\n");
}

void main_post_reloc(uint64_t hartid, void *dtb) {
    printf(">> [High Mem] Relocation complete! Stack is safe.\n");
    printf(">> [High Mem] Address: %lx\n", (uint64_t)main_post_reloc);

    global_dtb = dtb;
    should_load_kernel = 0;
    while(1){
        runAShell(0);
        if (should_load_kernel) {
            break;
        }
    }

    // send a signal to send_kernel.py to start sending the kernel
    uart_puts("OSC_START_DOWNLOAD\n");

    char *dest = (char *)(uintptr_t)KERNEL_DEST_ADDR;
    load_kernel_from_uart(dest);
    
    __asm__ __volatile__("fence.i");

    printf(">> [High Mem] Jumping to Kernel at %lx...\n", KERNEL_DEST_ADDR);
    printf("DTB address: %lx\n", dtb);

    void (*kernel_entry)(uint64_t, void *) = (void (*)(uint64_t, void *))KERNEL_DEST_ADDR;

    kernel_entry(hartid, dtb);

    while(1); 
}

void relocate_and_jump(void *dtb, uint64_t hartid) {
    uint64_t ram_base = 0, ram_size = 0;
    uint64_t initrd_start = 0, initrd_end = 0;

    if (fdt_get_memory_info(dtb, &ram_base, &ram_size) == -1) {
        printf(">> [Failed] Failed to parse memory node.\n");
        return;
    }
    printf(">> [Reloc] Detected RAM: Base = 0x%lx, Size = 0x%lx\n", ram_base, ram_size);

    if (fdt_get_initrd_range(dtb, &initrd_start, &initrd_end) != -1) {
        printf(">> [Reloc] Detected Initrd: Start = 0x%lx, End = 0x%lx\n", initrd_start, initrd_end);
    }

    uint64_t ram_top = ram_base + ram_size;
    
    uint64_t safe_top = ram_top;
    if (initrd_start != 0 && initrd_start < safe_top && initrd_start > ram_base) {
        printf(">> [Reloc] Initrd detected at high memory. Adjusting safe top.\n");
        safe_top = initrd_start;
    }

    uint64_t bl_size = (uint64_t)_bl_end - (uint64_t)_bl_start;
    uint64_t stack_size = 0x4000; 
    uint64_t dest_addr = safe_top - stack_size - bl_size;
    dest_addr &= ~0xFFF;

    uint64_t dtb_start = (uint64_t)dtb;
    uint64_t dtb_end = dtb_start + get_dtb_size(dtb);

    if (dest_addr < dtb_end && (dest_addr + bl_size) > dtb_start) {
        printf(">> [Reloc] Warning: Calculated destination %lx overlaps with DTB (%lx - %lx)!\n", dest_addr, dtb_start, dtb_end);
        
        dest_addr = dtb_start - stack_size - bl_size;
        dest_addr &= ~0xFFF;
        printf(">> [Reloc] Adjusted destination to: %lx\n", dest_addr);
    }

    printf(">> [Reloc] Relocating Bootloader to: 0x%lx\n", dest_addr);

    memcpy((void *)dest_addr, (void *)_bl_start, bl_size);
    __asm__ __volatile__("fence.i");

    uint64_t offset = dest_addr - (uint64_t)_bl_start;
    void *new_func_entry = (void *)((uint64_t)main_post_reloc + offset);
    uint64_t new_sp = dest_addr + bl_size + stack_size;

    printf(">> [Reloc] Jumping to high memory...\n");

    __asm__ __volatile__(
        "mv a0, %2 \n\t"
        "mv a1, %3 \n\t"
        "mv sp, %0 \n\t"
        "jr %1     \n\t"
        : 
        : "r" (new_sp), "r" (new_func_entry), "r" (hartid), "r" (dtb)
        : "memory", "a0", "a1"
    );

    while(1);
}

void main(uint64_t hartid, void *dtb) {
    uart_init(dtb);
    
    // wait for QEMU to connect (if running in QEMU)
    #ifdef QEMU
        // printf(">> [Bootloader] Waiting for QEMU to connect...\n");
        uart_getc();
    #endif
    printf("DTB address: %lx\n", dtb);
    printf("\nbootloader: \n");
    printf(">> [Bootloader] Started.\n");
    relocate_and_jump(dtb, hartid);
    while(1);
}