ENTRY(_start)

SECTIONS
{
    . = 0x40200000;

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
        *(.data .data.* .sdata .sdata.* .got .got.*)
    }

    .bss : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.* .sbss .sbss.*)
        *(COMMON)
        __bss_end = .;
    }

    . = ALIGN(16);
    _bl_end = .;

    . += 0x4000;
    _stack_top = .;
}