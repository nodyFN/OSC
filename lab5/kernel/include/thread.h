#ifndef __THREAD_H__
#define __THREAD_H__

enum TASK_STATUS{
    RUNNING, READY, TERMINATED
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
    unsigned long stack;
    int stack_page_order;
    struct task_struct* next;
}; 

struct task_struct* kthread_create(void (*threadfn)());
void thread_foo();
void thread_idle();
void schedule();
void thread_exit();
void kill_zombies();

#endif
