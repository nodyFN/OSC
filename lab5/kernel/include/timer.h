#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include "list.h"

extern uint64_t TIMERBASE_FREQ;
typedef void (*timer_callback_t)(void *arg);

struct timer_event {
    struct list_head list;
    uint64_t expire_time;
    timer_callback_t callback;
    void *arg;
};

static inline uint64_t get_time() {
    uint64_t time;
    __asm__ volatile("csrr %0, time" : "=r"(time));
    return time;
}
void timer_init();
void set_next_timer(int second);
void add_timer(timer_callback_t callback, void *arg, uint64_t timeout_ticks);
void set_next_timer_absolute(uint64_t absolute_tick);
void timer_handler();
void timer_event_test();

#endif