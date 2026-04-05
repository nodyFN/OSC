#ifndef __ECALL_HELPER__
#define __ECALL_HELPER__

#include "trap.h"

void getpid_ecall_helper(struct pt_regs* regs);
void uart_read_ecall_helper(struct pt_regs* regs);
void uart_write_ecall_helper(struct pt_regs* regs);
void exec_ecall_helper(struct pt_regs* regs);
void fork_ecall_helper(struct pt_regs* regs);
void exit_ecall_helper(struct pt_regs* regs);
void stop_ecall_helper(struct pt_regs* regs);
void display_ecall_helper(struct pt_regs* regs);
void usleep_ecall_helper(struct pt_regs* regs);
void signal_ecall_helper(struct pt_regs* regs);
void sigreturn_ecall_helper(struct pt_regs* regs);
void kill_ecall_helper(struct pt_regs* regs);

void unknown_ecall_helper(struct pt_regs* regs);

void ecall_helper_commit();

#endif