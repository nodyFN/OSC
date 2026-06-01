.section .text.boot
.global _start

_start:
    la sp, _stack_top
    mv      s0, a0
    mv      s1, a1

    call setup_vm

    la      t0, 1f
    li      t1, 0xffffffc000000000
    add     t0, t0, t1
    jr      t0

.align 2
1:
    la      sp, _stack_top

    la      t0, __bss_start
    la      t1, __bss_end

bss_clear_loop:
    bge t0, t1, bss_done
    sd zero, 0(t0)
    addi t0, t0, 8
    j bss_clear_loop

bss_done:
    call    drop_identity_map
    la a2, handle_exception
    csrw stvec, a2
    mv      a0, s0
    mv      a1, s1
    call main
    j .