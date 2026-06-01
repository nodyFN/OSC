#ifndef __EXCEPTION_HELPER_H__
#define __EXCEPTION_HELPER_H__

#include "trap.h"

void ecall_exception_helper(struct pt_regs* regs);
void instruction_page_fault_helper(struct pt_regs* regs);
void load_page_fault_helper(struct pt_regs* regs);
void store_amo_page_fault_helper(struct pt_regs* regs);

void unhandle_exception_helper(struct pt_regs* regs);

void exception_helper_commit();

#endif