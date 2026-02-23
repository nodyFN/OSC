/* kernel/src/timer.c */
#include <stdint.h>
#include "stdio.h"
#include "riscv.h"

#define CLINT_BASE 0x2000000
#define MTIME      (CLINT_BASE + 0xBFF8)
#define MTIMECMP   (CLINT_BASE + 0x4000)
#define TIMEBASE_FREQ 10000000

uint64_t boot_time_seconds = 0;

void timer_init() {
    printf("[Timer] Initializing CLINT...\n");
    uint64_t current_time = *(volatile uint64_t*)MTIME;
    *(volatile uint64_t*)MTIMECMP = current_time + TIMEBASE_FREQ;
    
    // 💥 絕對不要在這裡設定 mstatus 的 MIE！等切換 U-mode 時硬體會自動打開
    set_csr(mie, 1 << 7); // 只開啟 MTIE
}

void timer_handler() {
    boot_time_seconds++;
    printf("[Timer] System uptime: %ld seconds\n", boot_time_seconds);
    uint64_t current_time = *(volatile uint64_t*)MTIME;
    *(volatile uint64_t*)MTIMECMP = current_time + TIMEBASE_FREQ;
}