/* * Spec 要求 2: The program counter must point to the correct entry address.
 * 這裡設定 _start 為整個程式的進入點標籤。
 */
ENTRY(_start)

SECTIONS
{
    /* * Spec 要求 1: All data sections must be placed at the expected memory addresses.
     * 手冊顯示 DRAM 起點為 0x0，
     * 但我們設定在 0x00200000 (2MB 處)，是為了避開 U-Boot 可能佔用的開機區域。
     */
    . = 0x00200000;

    /* .text 段：存放編譯後的機器指令 (程式碼) */
    .text : {
        KEEP(*(.text.boot)) /* 強制保留啟動組合語言，確保它排在 0x00200000 的第一行 */
        *(.text .text.*)    /* 其他 C 語言編譯出來的函式代碼 */
    }

    /* .rodata 段：存放唯讀資料，如 C 語言中的字串常數 "Hello World" */
    .rodata : {
        . = ALIGN(8);
        *(.rodata .rodata.*)
    }

    /* .data 段：存放「有初始值」的全域變數 (例如 int a = 10;) */
    .data : {
        . = ALIGN(8);
        *(.data .data.*)
    }

    /* * Spec 要求 3: The .bss segment must be zero-initialized.
     * 這裡定義 BSS 段的邊界 (__bss_start 到 __bss_end)，
     * 讓 start.S 知道要清空哪一段記憶體。
     */
    .bss : {
        . = ALIGN(16);
        __bss_start = .;     /* 記錄 BSS 開始的記憶體位址 */
        *(.bss .bss.*)       /* 收集所有未初始化的全域變數 */
        *(COMMON)
        __bss_end = .;       /* 記錄 BSS 結束的記憶體位址 */
    }

    /* * Spec 要求 4: The stack pointer must be set to a valid memory location.
     * 在 DRAM 中劃出一塊空間作為 Stack。
     */
    . = ALIGN(16);
    . += 0x4000;             /* 預留 16KB 空間給 Stack */
    _stack_top = .;          /* Stack 是由高位址往低位址長，所以 SP 要指向這塊空間的最末端 */
}