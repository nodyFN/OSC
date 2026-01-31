#include "sbi.h"
#include <stdint.h>



/* * SBI Ecall 實作
 * 使用 Inline Assembly 觸發 ecall 指令
 */
struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
                        unsigned long arg1, unsigned long arg2,
                        unsigned long arg3, unsigned long arg4,
                        unsigned long arg5) {
    struct sbiret ret;

    // GCC Inline Assembly 語法
    __asm__ volatile (
        // 1. 把 C 語言的參數搬到 RISC-V 指定的暫存器
        "mv a7, %[ext]\n"      // Extension ID 放入 a7
        "mv a6, %[fid]\n"      // Function ID 放入 a6
        "mv a0, %[arg0]\n"     // 參數 0 放入 a0
        "mv a1, %[arg1]\n"     // 參數 1 放入 a1
        "mv a2, %[arg2]\n"     // 參數 2 放入 a2
        "mv a3, %[arg3]\n"     // 參數 3 放入 a3
        "mv a4, %[arg4]\n"     // 參數 4 放入 a4
        "mv a5, %[arg5]\n"     // 參數 5 放入 a5
        
        // 2. 觸發 Trap，跳入 M-Mode (OpenSBI)
        "ecall\n"

        // 3. OpenSBI 處理完返回，把回傳值搬回 C 語言變數
        "mv %[err], a0\n"      // a0 是錯誤碼
        "mv %[val], a1\n"      // a1 是回傳值

        // --- 輸出入限制與破壞列表 (Constraints) ---
        // Outputs: 告訴編譯器哪些變數被寫入了
        : [err] "=r" (ret.error), [val] "=r" (ret.value)
        
        // Inputs: 告訴編譯器輸入變數來自哪裡
        : [ext] "r" (ext), [fid] "r" (fid),
          [arg0] "r" (arg0), [arg1] "r" (arg1), [arg2] "r" (arg2),
          [arg3] "r" (arg3), [arg4] "r" (arg4), [arg5] "r" (arg5)
        
        // Clobbers: 告訴編譯器我們弄髒了哪些暫存器，請它不要把重要資料暫存在這
        : "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "memory"
    );

    return ret;
}

// 輔助函式：取得 SBI Spec 版本
long sbi_get_spec_version() {
    // EID=0x10 (Base), FID=0 (Get Spec Version)
    struct sbiret ret = sbi_ecall(0x10, 0, 0, 0, 0, 0, 0, 0);
    return ret.value;
}

// 輔助函式：探測某個 Extension 是否存在
// long sbi_probe_extension(long extension_id) {
//     // EID=0x10 (Base), FID=3 (Probe Extension)
//     // arg0 放我們要查的 extension_id
//     struct sbiret ret = sbi_ecall(0x10, 3, extension_id, 0, 0, 0, 0, 0);
//     return ret.value; // 如果回傳 0 代表不支援，非 0 代表支援
// }

long sbi_probe_extension(long extension_id) {
    /* * 根據 SBI Spec Chapter 4:
     * EID (a7) = 0x10  (Base Extension)
     * FID (a6) = 3     (Probe SBI Extension Function)
     * arg0     = extension_id (我們要查的那個 ID)
     */
    struct sbiret ret = sbi_ecall(0x10, 3, extension_id, 0, 0, 0, 0, 0);
    
    return ret.value;
}

void sbi_legacy_reboot() {
    // EID=0x08, FID=0 (忽略), 沒有參數
    sbi_ecall(0x08, 0, 0, 0, 0, 0, 0, 0);
}