.section .text.boot
.global _start

_start:
    /* a0 = hartid, a1 = dtb */
    mv      s0, a0
    mv      s1, a1
    fence.i
    la      t0, __bss_start
    la      t1, __bss_end
    
bss_loop:
    bge     t0, t1, bss_done
    sd      zero, 0(t0)
    addi    t0, t0, 8
    j       bss_loop
    
bss_done:
    la      sp, _stack_top
    mv      a0, s0
    mv      a1, s1
    call    main

hang:
    j       hang