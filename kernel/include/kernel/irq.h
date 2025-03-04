#ifndef _IRQ_H
#define _IRQ_H

#include "pic.h"
#include "isr.h"
#include "idt.h"

typedef void (*IRQ_handler_t)(registers_t* regs);

void IRQ_init();
void IRQ_reg_handle(int irq, IRQ_handler_t handler);

#endif