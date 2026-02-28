#ifndef __PLIC_H__
#define __PLIC_H__

#include <stdint.h>

void plic_init();
uint32_t plic_claim();
void plic_complete(uint32_t irq);

#endif