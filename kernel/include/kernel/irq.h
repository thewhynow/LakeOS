#ifndef _IRQ_H
#define _IRQ_H

#include "pic.h"
#include "isr.h"
#include "idt.h"
#include "pit.h"
#include "tty.h"
#include "pit.h"
#include "ps2.h"

typedef void (*IRQ_handler_t)();

extern void kernel_panic();

void IRQ_init();

#define PIC_REMAP_OFFSET 0x20

#endif