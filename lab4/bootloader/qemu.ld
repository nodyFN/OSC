ENTRY(_start)

SECTIONS
{
    /* 💥 關鍵修改：讓 Bootloader 直接降落在 RAM 的最開頭！ */
    . = 0x80000000;

    _bl_start = .;

    .text : {
        KEEP(*(.text.boot))
        *(.text .text.*)
    }

    .rodata : {
        . = ALIGN(8);
        *(.rodata .rodata.*)
    }

    .data : {
        . = ALIGN(8);
        *(.data .data.*)
    }

    .bss : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    . = ALIGN(16);
    _bl_end = .;

    . += 0x4000;
    _stack_top = .;
}