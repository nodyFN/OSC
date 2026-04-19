#ifndef __THREAD__H__
#define __THREAD__H__

#include "trap.h"

#define KERNEL_STACK_SIZE 0x8000
#define USER_STACK_SIZE 0x8000

enum TASK_STATUS{
    RUNNING, READY, ABORTED, TERMINATED, WAITING
};

struct task_struct {
    struct thread_struct {
        unsigned long ra;
        unsigned long sp;
        unsigned long s[12];
    } thread;
    int pid;
    enum TASK_STATUS status;
    unsigned long kernel_sp;
    unsigned long user_sp;
    unsigned long kernel_stack;
    int stack_page_order;
    unsigned long user_stack;
    int user_stack_page_order;
    struct task_struct* next;
    struct task_struct* parent;
    int waiting_pid;

    uint64_t signal_handler[32];
    uint32_t sigpending;
    int is_handling_signal;
    struct pt_regs signal_saved_regs;
    uint64_t signal_stack_page;
}; 

struct task_struct* kthread_create(void (*threadfn)());
void thread_foo();
void thread_idle();
void schedule();
void thread_exit();
void kill_zombies();
struct task_struct* get_current();
void enqueue(struct task_struct** queue, struct task_struct* task);

struct task_struct* user_process_create(void (*user_func)());

#endif