#ifndef __ECALL_HELPER_H__
#define __ECALL_HELPER_H__

#include "trap.h"

void getpid_ecall_helper(struct pt_regs* regs);
void uart_read_ecall_helper(struct pt_regs* regs);
void uart_write_ecall_helper(struct pt_regs* regs);
void exec_ecall_helper(struct pt_regs* regs);
void fork_ecall_helper(struct pt_regs* regs);
void waitpid_ecall_helper(struct pt_regs* regs);
void exit_ecall_helper(struct pt_regs* regs);
void stop_ecall_helper(struct pt_regs* regs);
void display_ecall_helper(struct pt_regs* regs);
void usleep_ecall_helper(struct pt_regs* regs);
void signal_ecall_helper(struct pt_regs* regs);
void sigreturn_ecall_helper(struct pt_regs* regs);
void kill_ecall_helper(struct pt_regs* regs);
void mmap_ecall_helper(struct pt_regs* regs);
void open_ecall_helper(struct pt_regs* regs);
void close_ecall_helper(struct pt_regs* regs);
void read_ecall_helper(struct pt_regs* regs);
void write_ecall_helper(struct pt_regs* regs);
void mkdir_ecall_helper(struct pt_regs* regs);
void mount_ecall_helper(struct pt_regs* regs);
void chdir_ecall_helper(struct pt_regs* regs);
void lseek64_ecall_helper(struct pt_regs* regs);
void ioctl_ecall_helper(struct pt_regs* regs);

void unknown_ecall_helper(struct pt_regs* regs);

void ecall_helper_commit();

#endif