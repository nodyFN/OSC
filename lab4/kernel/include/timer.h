#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
extern uint64_t TIMERBASE_FREQ;

static inline uint64_t get_time() {
    uint64_t time;
    __asm__ volatile("csrr %0, time" : "=r"(time));
    return time;
}
void timer_init();
void set_next_timer(int second);

#endif