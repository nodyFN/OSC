/* kernel/src/timer.c */
#include "timer.h"
#include "mm.h"
#include "stdio.h"
#include "riscv.h"

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

// 取得目前的系統時間
static uint64_t get_current_time() {
#ifdef QEMU
    return *MTIME;
#else
    uint64_t time;
    asm volatile("rdtime %0" : "=r"(time));
    return time;
#endif
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

// 設定下一次的鬧鐘觸發時間
static void set_next_timer(uint64_t expire_time) {
#ifdef QEMU
    *MTIMECMP = expire_time;
#else
    sbi_set_timer(expire_time); // 💥 使用新的安全 SBI 呼叫
#endif
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

    // uint64_t status = lock_interrupts(); // 💥 進入臨界區

    if (timer_head == NULL || timer_head->expire_time > new_node->expire_time) {
        new_node->next = timer_head;
        timer_head = new_node;
        // 最快過期的任務改變了，立刻更新硬體鬧鐘
        set_next_timer(timer_head->expire_time);
    } else {
        timer_event_t *curr = timer_head;
        while (curr->next != NULL && curr->next->expire_time <= new_node->expire_time) {
            curr = curr->next;
        }
        new_node->next = curr->next;
        curr->next = new_node;
    }

    // unlock_interrupts(status); // 💥 離開臨界區
}

void timer_handler() {
    uint64_t current_time = get_current_time();

    // 處理所有已過期的任務
    while (timer_head != NULL && timer_head->expire_time <= current_time) {
        timer_event_t *event = timer_head;
        timer_head = timer_head->next;

        if (event->callback) {
            event->callback(event->arg);
        }
        
        // 此處可呼叫 kfree(event); 釋放記憶體
    }

    // 設定下一個鬧鐘
    if (timer_head != NULL) {
        set_next_timer(timer_head->expire_time);
    } else {
        // 如果沒有任務了，把鬧鐘設到無限遠
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
    
    // 再次預約 1 秒後的自己！
    add_timer(print_uptime_callback, NULL, 1);
}

void timer_init() {
    // 預約第一個任務
    add_timer(print_uptime_callback, NULL, 1);
    
#ifdef QEMU
    set_csr(mie, 1 << 7); // 開啟 MTIE
#else
    // 💥 1. 開啟 STIE (Supervisor Timer Interrupt Enable)
    set_csr(sie, 1 << 5); 

    // 💥 2. 強制開啟 S-mode 的全域中斷 (SIE 位元, mstatus/sstatus 的 bit 1)
    // 這樣就算在 S-mode 底下，也能收到中斷
    set_csr(sstatus, 1 << 1); 
#endif
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