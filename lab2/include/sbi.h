#ifndef __SBI_H__
#define __SBI_H__

#include <stdint.h> // 為了使用 long (在 64-bit 系統是 64-bit)

/* 定義回傳結構：SBI 會同時回傳 error 和 value */
struct sbiret {
    long error;
    long value;
};

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
                        unsigned long arg1, unsigned long arg2,
                        unsigned long arg3, unsigned long arg4,
                        unsigned long arg5);

long sbi_get_spec_version();

long sbi_probe_extension(long extension_id);

void sbi_legacy_reboot();

#endif