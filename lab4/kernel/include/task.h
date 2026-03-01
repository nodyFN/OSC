#ifndef __TASK_H__
#define __TASK_H__

#include <stdint.h>

typedef void (*task_callback_t)(void *arg);

typedef struct {
    task_callback_t callback;
    void *arg;
    int priority; // 數字越小，優先權越高
} task_t;

void task_queue_init();
void add_task(task_callback_t callback, void *arg, int priority);
void process_tasks();

#endif