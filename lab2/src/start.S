/* src/start.S */
.section .text.boot
.global _start

_start:
    /* * [S-Mode 限制]
     * 我們不能讀取 mhartid。通常 OpenSBI 會把 hartid 放在 a0 暫存器傳進來。
     * 但在這個簡單的 Lab 中，我們可以假設是單核心執行，或者不處理多核同步。
     * 直接開始初始化。
     */

    /* ==========================================
     * TODO 3: Set up stack pointer
     * ==========================================
     * Spec 要求: Stack pointer must be 16-byte aligned.
     * linker.ld 已經幫我們算好 _stack_top 了，且有 ALIGN(16)。
     */
    la sp, _stack_top

    /* ==========================================
     * TODO 2: Clear the .bss section
     * ==========================================
     * 你的邏輯完全正確，這裡保留你的寫法。
     */
    la t0, __bss_start        # BSS 開始
    la t1, __bss_end          # BSS 結束

bss_loop:
    bge t0, t1, bss_done      # 如果 t0 >= t1，結束 (改用 bge 防止 t0 超過 t1)
    sd zero, 0(t0)            # 寫入 0
    addi t0, t0, 8            # 移動 8 bytes
    j bss_loop

bss_done:
    /* * 跳轉到 C 語言入口 
     * OpenSBI 已經幫我們處理好大部分中斷設定，直接跳進去即可。
     */
    call main

    /* 防呆迴圈：如果 main 意外返回，卡在這裡 */
loop:
    wfi
    j loop