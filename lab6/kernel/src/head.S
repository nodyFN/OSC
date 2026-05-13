.section .text.boot
.global _start

_start:
    la sp, _stack_top
    la t0, __bss_start
    la t1, __bss_end

bss_clear_loop:
    bge t0, t1, bss_done
    sd zero, 0(t0)
    addi t0, t0, 8
    j bss_clear_loop

bss_done:
    la a2, handle_exception
    csrw stvec, a2
    call main
    j .