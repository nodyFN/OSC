#include "uart.h"
#include "string.h"
#include "fdt.h"
#include "utils.h"
#include "initrd.h"
#include "stdio.h"

extern char _bl_start[];
extern char _bl_end[];

#ifdef QEMU
    #define KERNEL_DEST_ADDR  0x80200000
    #define KERNEL_STAGE_ADDR 0x82000000
#else
    #define KERNEL_DEST_ADDR  0x40200000
    #define KERNEL_STAGE_ADDR 0x42000000
#endif

#define KERNEL_COPY_SIZE  0x400000 


void main_post_reloc(uint64_t hartid, void *dtb) {
    printf(">> [High Mem] Relocation complete! Stack is safe.\n");

    printf(">> [High Mem] Copying Kernel to: %lx\n", KERNEL_DEST_ADDR);
    printf(">> [High Mem] From Staging Addr: %lx\n", KERNEL_STAGE_ADDR);
    
    char *src = (char *)(uintptr_t)KERNEL_STAGE_ADDR;
    char *dest = (char *)(uintptr_t)KERNEL_DEST_ADDR;

    memcpy(dest, src, KERNEL_COPY_SIZE);
    
    __asm__ __volatile__("fence.i");

    printf(">> [High Mem] Jumping to Kernel...\n");

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

    if (fdt_get_initrd_range(dtb, &initrd_start, &initrd_end) == -1) {
        printf(">> [Failed] No Initrd detected.\n");
        return;
    } else {
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

    if (dest_addr < KERNEL_STAGE_ADDR + KERNEL_COPY_SIZE) {
        printf(">> [Failed] Critical Warning: Destination address is dangerously low!\n");
        return;
    }

    memcpy((void *)dest_addr, (void *)_bl_start, bl_size);
    __asm__ __volatile__("fence.i");

    uint64_t offset = dest_addr - (uint64_t)_bl_start;
    void *new_func_entry = (void *)((uint64_t)main_post_reloc + offset);
    uint64_t new_sp = dest_addr + bl_size + stack_size;

    printf(">> [Reloc] Jumping to high memory...\n");

    __asm__ __volatile__(
        "mv sp, %0 \n\t"
        "mv a0, %2 \n\t"
        "mv a1, %3 \n\t"
        "jr %1     \n\t"
        : 
        : "r" (new_sp), "r" (new_func_entry), "r" (hartid), "r" (dtb)
        : "memory", "a0", "a1"
    );

    while(1);
}

void main(uint64_t hartid, void *dtb) {
    uart_init();
    printf("\nbootloader: \n");
    printf(">> [Bootloader] Started.\n");
    relocate_and_jump(dtb, hartid);
    while(1);
}