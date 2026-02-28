#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

// 定義回呼函數的指標型態
typedef void (*timer_callback_t)(void *arg);

// Timer 事件節點 (用於 Linked List)
typedef struct timer_event {
    uint64_t expire_time;         // 任務應該被執行的絕對時間 (mtime)
    timer_callback_t callback;    // 時間到時要執行的函數
    void *arg;                    // 傳給函數的參數
    struct timer_event *next;     // 連結下一個任務
} timer_event_t;

void timer_init();
void add_timer(timer_callback_t callback, void *arg, uint64_t duration_sec);
void timer_handler();
void check_timer_events();

#endif