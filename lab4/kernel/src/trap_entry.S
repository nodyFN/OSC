/* kernel/src/trap_entry.S */
.section .text
.align 4
.global trap_vector

trap_vector:
    # 1. 交換 SP (取得 Kernel SP)
#ifdef QEMU
    csrrw sp, mscratch, sp
#else
    csrrw sp, sscratch, sp
#endif

    # 2. 開堆疊
    addi sp, sp, -256

    # 3. 儲存所有暫存器 (除了 x2(sp) 和 x0(zero))
    sd x1, 1*8(sp)
    sd x3, 3*8(sp)
    sd x4, 4*8(sp)
    sd x5, 5*8(sp)
    sd x6, 6*8(sp)
    sd x7, 7*8(sp)
    sd x8, 8*8(sp)
    sd x9, 9*8(sp)
    sd x10, 10*8(sp)
    sd x11, 11*8(sp)
    sd x12, 12*8(sp)
    sd x13, 13*8(sp)
    sd x14, 14*8(sp)
    sd x15, 15*8(sp)
    sd x16, 16*8(sp)
    sd x17, 17*8(sp)
    sd x18, 18*8(sp)
    sd x19, 19*8(sp)
    sd x20, 20*8(sp)
    sd x21, 21*8(sp)
    sd x22, 22*8(sp)
    sd x23, 23*8(sp)
    sd x24, 24*8(sp)
    sd x25, 25*8(sp)
    sd x26, 26*8(sp)
    sd x27, 27*8(sp)
    sd x28, 28*8(sp)
    sd x29, 29*8(sp)
    sd x30, 30*8(sp)
    sd x31, 31*8(sp)

    # 4. 把存在 scratch 裡的 User SP 拿出來存好
    # x5 (t0) 剛剛已經存進堆疊了，現在可以隨便用！
#ifdef QEMU
    csrr t0, mscratch
#else
    csrr t0, sscratch
#endif
    sd t0, 2*8(sp)

    # 5. 呼叫 C 函數
    call trap_handler

    # 6. 把原本的 User SP 讀回來放回 scratch
    # 我們再次借用 t0
    ld t0, 2*8(sp)
#ifdef QEMU
    csrw mscratch, t0
#else
    csrw sscratch, t0
#endif

    # 7. 恢復所有暫存器 (包含原本被我們借用的 t0/x5)
    ld x1, 1*8(sp)
    ld x3, 3*8(sp)
    ld x4, 4*8(sp)
    ld x5, 5*8(sp)
    ld x6, 6*8(sp)
    ld x7, 7*8(sp)
    ld x8, 8*8(sp)
    ld x9, 9*8(sp)
    ld x10, 10*8(sp)
    ld x11, 11*8(sp)
    ld x12, 12*8(sp)
    ld x13, 13*8(sp)
    ld x14, 14*8(sp)
    ld x15, 15*8(sp)
    ld x16, 16*8(sp)
    ld x17, 17*8(sp)
    ld x18, 18*8(sp)
    ld x19, 19*8(sp)
    ld x20, 20*8(sp)
    ld x21, 21*8(sp)
    ld x22, 22*8(sp)
    ld x23, 23*8(sp)
    ld x24, 24*8(sp)
    ld x25, 25*8(sp)
    ld x26, 26*8(sp)
    ld x27, 27*8(sp)
    ld x28, 28*8(sp)
    ld x29, 29*8(sp)
    ld x30, 30*8(sp)
    ld x31, 31*8(sp)

    # 8. 回收堆疊
    addi sp, sp, 256

    # 9. 再次交換！把存在 scratch 裡的 User SP 換回 sp，同時把 Kernel SP 存回 scratch
#ifdef QEMU
    csrrw sp, mscratch, sp
    mret
#else
    csrrw sp, sscratch, sp
    sret
#endif