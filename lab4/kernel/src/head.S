/* kernel/src/head.S */
.section .text.boot
.global _start
.align 4

_start:
    # 現在我們是真正的 M-mode 老大，這行不會當機了！
    csrw mie, zero

    mv s0, a0
    mv s1, a1

    la sp, _stack_top
    la t0, __bss_start
    la t1, __bss_end

bss_clear_loop:
    bge t0, t1, bss_done
    sb zero, 0(t0)     # 使用 sb 很安全
    addi t0, t0, 1
    j bss_clear_loop

bss_done:
    mv a0, s0
    mv a1, s1
    call main

halt:
    wfi
    j halt