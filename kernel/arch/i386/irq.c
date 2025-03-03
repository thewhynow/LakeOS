#include "../../include/kernel/irq.h"

#define PIC_REMAP_OFFSET 0x20

IRQ_handler_t IRQ_handlers[16];

void IRQ_handler(registers_t* regs){
    int irq = regs->int_num - PIC_REMAP_OFFSET;

    if (IRQ_handlers[irq]){
        // handle interrupts
        IRQ_handlers[irq](regs);
    }
    else {
        printf("undefined IRQ: %i", irq);
    }
}

void IRQ_init() {
    PIC_configure(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

    for (int i = 0; i < 16; ++i)
        IRQ_reg_handle(PIC_REMAP_OFFSET + i, IRQ_handler);

    asm("sti");
}

void IRQ_reg_handle(int irq, IRQ_handler_t handler){
    IRQ_handlers[irq] = handler;
}