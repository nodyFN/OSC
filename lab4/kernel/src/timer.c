#include "timer.h"
#include "sbi.h"
#include "fdt.h"
#include "utils.h"
#include "stdio.h"
#include "mm.h"
#include "task.h"

uint64_t TIMERBASE_FREQ = 0x989680;

extern struct KernelInfo kernel_info;

struct list_head timer_list;

void enable_timer_interrupt(){
    __asm__ volatile(
        "csrs sie, %0"
        :
        : "r"(1 << 5)
    );
}

int get_time_after_boot(){
    uint64_t current_time = get_time();
    uint64_t seconds_after_boot = current_time / TIMERBASE_FREQ;
    return seconds_after_boot;
}

void periodic_tick_handler(){
    // printf("Tick! %d seconds after booting.\n", get_time_after_boot());
    add_timer(periodic_tick_handler, NULL, PERIODIC_TIME_SLOT_SECOND);
}

void one_shot_alert_callback(void* arg){
    printf("%s\n", (char*)arg);
    kfree(arg);
}

void set_next_timer(int second) {
    uint64_t current_time = get_time();
    sbi_set_timer(current_time + TIMERBASE_FREQ * second);
}

void timer_init(){
    int offset = fdt_path_offset(kernel_info.dtb_addr, "/cpus");
    if(offset == -1){
        printf("[FAILED] Can not find cpu frequency [1].\n");
    }else{
        int len;
        uint32_t* prop = (uint32_t*)fdt_getprop(kernel_info.dtb_addr, offset, "timebase-frequency", &len);
        if (!prop){
            printf("[FAILED] Can not find cpu frequency [2].\n");
        }else{
            TIMERBASE_FREQ = toLittleEndian(*prop);
            // printf("TIMERBASE_FREQ: %d\n", TIMERBASE_FREQ);
        }
    }

    INIT_LIST_HEAD(&timer_list);

    add_timer(periodic_tick_handler, NULL, PERIODIC_TIME_SLOT_SECOND);
    enable_timer_interrupt();
}

void set_next_timer_absolute(uint64_t absolute_tick){
    sbi_set_timer(absolute_tick);
}

void add_timer(timer_callback_t callback, void *arg, double second) {
    struct timer_event *new_timer = (struct timer_event *)kmalloc(sizeof(struct timer_event));
    
    uint64_t current_time = get_time();
    uint64_t timeout_ticks = second * TIMERBASE_FREQ;
    new_timer->expire_time = current_time + timeout_ticks;
    new_timer->callback = callback;
    new_timer->arg = arg;

    uint64_t irq_flags;
    local_irq_save(irq_flags);

    struct list_head *curr;
    struct timer_event *entry;
    int inserted = 0;

    list_for_each(curr, &timer_list) {
        entry = list_entry(curr, struct timer_event, list);
        if (new_timer->expire_time < entry->expire_time) {
            list_add_tail(&new_timer->list, curr);
            inserted = 1;
            break;
        }
    }

    if (!inserted) {
        list_add_tail(&new_timer->list, &timer_list);
    }

    if (timer_list.next == &new_timer->list) {
        set_next_timer_absolute(new_timer->expire_time); 
    }

    local_irq_restore(irq_flags);
}

void timer_handler(){
    uint64_t current_time = get_time();

    while (!list_empty(&timer_list)) {
        struct timer_event *first = list_entry(timer_list.next, struct timer_event, list);

        if (first->expire_time <= current_time) {
            list_del(&first->list);

            if (first->callback) {
                add_task(first->callback, first->arg, 5);
            }

            kfree(first);
        } else {
            break; 
        }
    }

    if (!list_empty(&timer_list)) {
        struct timer_event *next_timer = list_entry(timer_list.next, struct timer_event, list);
        set_next_timer_absolute(next_timer->expire_time);
    } else {
        set_next_timer_absolute(-1ULL);
    }
}