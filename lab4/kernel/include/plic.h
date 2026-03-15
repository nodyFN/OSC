#ifndef __PLIC_H__
#define __PLIC_H__

#include <stdint.h>

#define PLIC_PRIORITY(irq)   ((volatile unsigned int *)(PLIC_BASE + (irq) * 4))
#define PLIC_ENABLE(hart)    ((volatile unsigned int *)(PLIC_BASE + 0x002080 + (hart) * 0x0100))
#define PLIC_THRESHOLD(hart) ((volatile unsigned int *)(PLIC_BASE + 0x201000 + (hart) * 0x2000))
#define PLIC_CLAIM(hart)     ((volatile unsigned int *)(PLIC_BASE + 0x201004 + (hart) * 0x2000))

extern uint64_t PLIC_BASE;
extern uint64_t UART_IRQ;

void plic_init();
int plic_claim();
void plic_complete(int irq);

#endif