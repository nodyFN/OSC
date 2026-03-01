/* kernel/src/trap_entry.S */
.section .rodata
debug_msg: .string "\n[DEBUG] Assembly call trap_handler!\n"

.section .text
.align 4
.global trap_vector

trap_vector:
    # 1. 檢查前一個特權模式 (Privilege Mode)
    # 我們先在當前的 SP 上借用 16 bytes 來暫存 t0, t1 (不管 SP 是 User 還是 Kernel，這都是安全的)
    addi sp, sp, -16
    sd t0, 0(sp)
    sd t1, 8(sp)

#ifdef QEMU
    csrr t0, mstatus
    li t1, 0x1800        # MPP 的遮罩 (bits 11:12)
    and t0, t0, t1
    bnez t0, from_kernel # 如果 MPP != 0，代表我們是從 M-mode 來的 (巢狀中斷)
#else
    csrr t0, sstatus
    li t1, 0x100         # SPP 的遮罩 (bit 8)
    and t0, t0, t1
    bnez t0, from_kernel # 如果 SPP != 0，代表我們是從 S-mode 來的 (巢狀中斷)
#endif

from_user:
    # 來自 U-mode：還原 t0, t1，並切換到 Kernel Stack
    ld t0, 0(sp)
    ld t1, 8(sp)
    addi sp, sp, 16
    
#ifdef QEMU
    csrrw sp, mscratch, sp
#else
    csrrw sp, sscratch, sp
#endif

    addi sp, sp, -256
    # 儲存原本的 User SP (目前存放在 scratch 裡)
#ifdef QEMU
    csrr t0, mscratch
#else
    csrr t0, sscratch
#endif
    sd t0, 2*8(sp)
    j save_common

from_kernel:
    # 來自 Kernel：還原 t0, t1，**絕對不可以交換 SP**
    ld t0, 0(sp)
    ld t1, 8(sp)
    addi sp, sp, 16

    addi sp, sp, -256
    # 儲存原本的 Kernel SP (也就是退回 256 bytes 前的 sp)
    addi t0, sp, 256
    sd t0, 2*8(sp)
    j save_common

save_common:
    # 2. 儲存剩下的所有通用暫存器
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

    # # 💥 加入 Assembly Debug 訊息 💥
    # # 由於所有暫存器都已經安全地存在 stack 上了，我們現在可以隨意使用暫存器
    # la a0, debug_msg     # 將字串的位址載入 a0 (第一個參數)
    # call printf          # 呼叫 C 的 printf


    # 3. 呼叫 C 語言的 Handler
    call trap_handler

    # 4. 準備返回，先恢復通用暫存器
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

    # 5. 檢查我們要返回哪裡
#ifdef QEMU
    csrr t0, mstatus
    li t1, 0x1800
    and t0, t0, t1
    bnez t0, ret_to_kernel
#else
    csrr t0, sstatus
    li t1, 0x100
    and t0, t0, t1
    bnez t0, ret_to_kernel
#endif

ret_to_user:
    # 要回到 U-mode：把剛剛存在 2*8(sp) 的 User SP 放回 scratch，並交換 SP
    ld t0, 2*8(sp)
#ifdef QEMU
    csrw mscratch, t0
#else
    csrw sscratch, t0
#endif
    addi sp, sp, 256

#ifdef QEMU
    csrrw sp, mscratch, sp
    mret
#else
    csrrw sp, sscratch, sp
    sret
#endif

ret_to_kernel:
    # 要回到 Kernel (巢狀中斷結束)：不需要交換，直接退回堆疊即可
    addi sp, sp, 256
#ifdef QEMU
    mret
#else
    sret
#endif