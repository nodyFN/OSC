#include "task.h"
#include "riscv.h"
#include "stdio.h"

#define MAX_TASKS 64

static task_t task_queue[MAX_TASKS];
static int task_count = 0;

void task_queue_init() {
    task_count = 0;
}

static uint64_t lock_queue() {
#ifdef QEMU
    uint64_t status = read_csr(mstatus);
    clear_csr(mstatus, 1 << 3); // 關閉 MIE
#else
    uint64_t status = read_csr(sstatus);
    clear_csr(sstatus, 1 << 1); // 關閉 SIE
#endif
    return status;
}

static void unlock_queue(uint64_t status) {
#ifdef QEMU
    write_csr(mstatus, status);
#else
    write_csr(sstatus, status);
#endif
}

void add_task(task_callback_t callback, void *arg, int priority) {
    uint64_t status = lock_queue();

    if (task_count >= MAX_TASKS) {
        unlock_queue(status);
        return;
    }

    // 依照 Priority 排序插入 (Insertion Sort)
    int i = task_count - 1;
    while (i >= 0 && task_queue[i].priority > priority) {
        task_queue[i + 1] = task_queue[i];
        i--;
    }
    
    task_queue[i + 1].callback = callback;
    task_queue[i + 1].arg = arg;
    task_queue[i + 1].priority = priority;
    task_count++;

    unlock_queue(status);
}

void process_tasks() {
    while (1) {
        uint64_t status = lock_queue();
        
        if (task_count == 0) {
            unlock_queue(status);
            break;
        }

        // 取出優先權最高的任務
        task_t current_task = task_queue[0];
        for (int i = 1; i < task_count; i++) {
            task_queue[i - 1] = task_queue[i];
        }
        task_count--;

        // 💥 非常關鍵：取出任務後立刻解鎖！這樣執行任務時，硬體中斷才能打斷它！
        unlock_queue(status); 

        // 執行 Bottom Half 任務
        if (current_task.callback) {
            current_task.callback(current_task.arg);
        }
    }
}