#include "task.h"
#include "mm.h"
#include "utils.h"

LIST_HEAD(task_list);

void task_init() {
    INIT_LIST_HEAD(&task_list);
}

void add_task(task_callback_t callback, void *arg, int priority) {
    struct task_event *new_task = (struct task_event *)kmalloc(sizeof(struct task_event));
    new_task->callback = callback;
    new_task->arg = arg;
    new_task->priority = priority;

    uint64_t irq_flags;
    local_irq_save(irq_flags);

    struct list_head *curr;
    struct task_event *entry;
    int inserted = 0;

    list_for_each(curr, &task_list) {
        entry = list_entry(curr, struct task_event, list);
        if (new_task->priority < entry->priority) {
            list_add_tail(&new_task->list, curr);
            inserted = 1;
            break;
        }
    }

    if (!inserted) {
        list_add_tail(&new_task->list, &task_list);
    }

    local_irq_restore(irq_flags);
}

int current_task_priority = 9999;

void run_tasks() {
    while (1) {
        uint64_t irq_flags;
        local_irq_save(irq_flags);

        if (list_empty(&task_list)) {
            local_irq_restore(irq_flags);
            break;
        }

        struct task_event *first = list_entry(task_list.next, struct task_event, list);
        if (first->priority >= current_task_priority) {
            local_irq_restore(irq_flags);
            break;
        }
        list_del(&first->list);

        int prev_task_priority = current_task_priority;
        current_task_priority = first->priority;

        local_irq_restore(irq_flags);

        asm volatile("csrsi sstatus, 2");

        if (first->callback) {
            first->callback(first->arg);
        }

        asm volatile("csrci sstatus, 2");

        local_irq_save(irq_flags);
        current_task_priority = prev_task_priority;
        local_irq_restore(irq_flags);

        kfree(first);
    }
}