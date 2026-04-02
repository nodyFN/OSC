ENTRY(_start)

SECTIONS
{
    . = 0x40200000;

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
        *(.sdata .sdata.*)
    }

    .bss : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        *(.sbss .sbss.*)
        __bss_end = .;
    }

    . = ALIGN(4096);
    _end = .;

    . += 0x10000;
    _stack_top = .;
}