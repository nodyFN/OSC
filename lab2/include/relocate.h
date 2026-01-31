#ifndef __RELOCATE_H_
#define __RELOCATE_H_

#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))

extern char _bl_start[];
extern char _bl_end[];

void relocate(void* fdt);




#endif