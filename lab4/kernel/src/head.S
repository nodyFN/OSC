/* kernel/src/head.S */
.section .text.boot
.global _start
.align 4

_start:
    # 💥 雙平台現在都跑在 S-mode，統一關閉 Supervisor 模式的中斷 (sie)
    # 不管是 QEMU 還是板子，都不准碰 mie！
    csrw sie, zero

    # 暫存 a0 (hartid) 和 a1 (dtb_addr)，這由 OpenSBI/Bootloader 傳入
    mv s0, a0
    mv s1, a1

    # 設定 Stack
    la sp, _stack_top

    # 清除 BSS 段
    la t0, __bss_start
    la t1, __bss_end

bss_clear_loop:
    bge t0, t1, bss_done
    sb zero, 0(t0)
    addi t0, t0, 1
    j bss_clear_loop

bss_done:
    # 恢復參數並跳轉到 main
    mv a0, s0
    mv a1, s1
    call main

halt:
    # 在 S-mode 下 wfi 是合法的（除非被 OpenSBI 禁用，但通常可用）
    wfi
    j halt