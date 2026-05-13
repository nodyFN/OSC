ENTRY(_start)

SECTIONS
{
    . = 0x80200000;

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

    . = ALIGN(4096);
    _end = .;

    . += 0x10000;
    _stack_top = .;
}