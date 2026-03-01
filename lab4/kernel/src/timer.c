/* kernel/src/timer.c */
#include "timer.h"
#include "mm.h"
#include "stdio.h"
#include "riscv.h"
#include "sbi.h"

// ==========================================
// 硬體相關定義與抽象函數
// ==========================================

#ifdef QEMU
    #define CLINT_BASE 0x2000000L
    #define MTIME      ((volatile uint64_t *)(CLINT_BASE + 0xBFF8))
    #define MTIMECMP   ((volatile uint64_t *)(CLINT_BASE + 0x4000))
    #define TIMEBASE_FREQ 10000000 // QEMU 10MHz
#else
    #define TIMEBASE_FREQ 24000000 // Orange Pi RV2 (JH7110) 24MHz
#endif

// 1. 取得目前的系統時間 (統一使用 rdtime 指令)
// rdtime 是 S-mode 合法指令，OpenSBI 會幫我們處理
static uint64_t get_current_time() {
    uint64_t time;
    asm volatile("rdtime %0" : "=r"(time));
    return time;
}


// 1. 建立一個標準的 SBI 呼叫函數
#ifndef QEMU
static inline void sbi_set_timer(uint64_t stime_value) {
    register uint64_t a0 asm("a0") = stime_value;
    register uint64_t a6 asm("a6") = 0;          // FID = 0
    register uint64_t a7 asm("a7") = 0x54494D45; // EID = "TIME"

    asm volatile(
        "ecall"
        : "+r"(a0)
        : "r"(a6), "r"(a7)
        : "memory"
    );
}
#endif

// 2. 設定下一次的鬧鐘 (統一使用 SBI Call)
static void set_next_timer(uint64_t expire_time) {
    // 使用 SBI Extension ID: 0x00 (Legacy Set Timer)
    // 或者使用新的 0x54494D45 (TIME Extension)
    sbi_ecall(0x00, 0, expire_time, 0, 0, 0, 0, 0);
}

// // 關閉/開啟本機中斷 (保護 Linked List)
// static uint64_t lock_interrupts() {
// #ifdef QEMU
//     uint64_t status = read_csr(mstatus);
//     clear_csr(mstatus, 1 << 3); // 關閉 MIE
// #else
//     uint64_t status = read_csr(sstatus);
//     clear_csr(sstatus, 1 << 1); // 關閉 SIE
// #endif
//     return status;
// }

// static void unlock_interrupts(uint64_t status) {
// #ifdef QEMU
//     write_csr(mstatus, status);
// #else
//     write_csr(sstatus, status);
// #endif
// }



// ==========================================
// Timer Multiplexing 排程器邏輯
// ==========================================

static timer_event_t *timer_head = NULL;

void add_timer(timer_callback_t callback, void *arg, uint64_t duration_sec) {
    timer_event_t *new_node = (timer_event_t *)kmalloc(sizeof(timer_event_t));
    new_node->expire_time = get_current_time() + (duration_sec * TIMEBASE_FREQ);
    new_node->callback = callback;
    new_node->arg = arg;
    new_node->next = NULL;

    if (timer_head == NULL || timer_head->expire_time > new_node->expire_time) {
        new_node->next = timer_head;
        timer_head = new_node;
        set_next_timer(timer_head->expire_time);
    } else {
        timer_event_t *curr = timer_head;
        while (curr->next != NULL && curr->next->expire_time <= new_node->expire_time) {
            curr = curr->next;
        }
        new_node->next = curr->next;
        curr->next = new_node;
    }
}

void timer_handler() {
    uint64_t current_time = get_current_time();
    while (timer_head != NULL && timer_head->expire_time <= current_time) {
        timer_event_t *event = timer_head;
        timer_head = timer_head->next;
        if (event->callback) event->callback(event->arg);
    }
    if (timer_head != NULL) {
        set_next_timer(timer_head->expire_time);
    } else {
        set_next_timer(0xFFFFFFFFFFFFFFFFULL);
    }
}


// ==========================================
// 測試用 Callback 與 初始化
// ==========================================

static uint64_t uptime_sec = 0;
void print_uptime_callback(void *arg) {
    uptime_sec++;
    printf("[Timer] System uptime: %ld seconds\n", uptime_sec);
    add_timer(print_uptime_callback, NULL, 1);
}

void timer_init() {
    add_timer(print_uptime_callback, NULL, 1);
    
    // 💥 雙平台統一：只開啟 S-mode 的中斷
    set_csr(sie, 1 << 5);     // 開啟 STIE (Supervisor Timer Interrupt Enable)
    set_csr(sstatus, 1 << 1); // 開啟全域 SIE
}


// 讓外部主動檢查是否有任務過期 (Polling 備用方案)
void check_timer_events() {
    uint64_t current_time = get_current_time();
    // uint64_t status = lock_interrupts(); // 保護佇列

    while (timer_head != NULL && timer_head->expire_time <= current_time) {
        timer_event_t *event = timer_head;
        timer_head = timer_head->next;

        // unlock_interrupts(status); // 執行前解鎖
        if (event->callback) {
            event->callback(event->arg);
        }
        // status = lock_interrupts(); // 執行後重新上鎖
    }

    // unlock_interrupts(status);
}