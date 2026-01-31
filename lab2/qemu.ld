/* qemu.ld - 給 QEMU 用的 */
ENTRY(_start)

SECTIONS
{
    /* QEMU 的 RAM Base 是 0x80000000 */
    /* QEMU 的 Kernel Entry 通常設在 0x80200000 (OpenSBI 跳轉位置) */
    . = 0x80200000;

    _bl_start = .;

    .text : {
        KEEP(*(.text.boot))
        *(.text)
    }

    .rodata : { *(.rodata) }
    .data : { *(.data) }

    .bss : {
        __bss_start = .;
        *(.bss)
        *(.sbss)
        __bss_end = .;
    }

    . = ALIGN(16);

    _bl_end = .;

    . += 0x4000; /* 16KB Stack size */
    _stack_top = .;
}