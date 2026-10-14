/**
 * @file irq.h
 * @brief hardware interrupts (IRQ0 - IRQ15)
 */

#ifndef _IRQ_H
#define _IRQ_H

#include "pic.h"
#include "isr.h"
#include "idt.h"
#include "tty.h"

/**
 * a hardware interrupt handler
 *  it is responsible for sending the end of
 *  interrupt to the PIC itself
 */
typedef void (*IRQ_handler_t)();

#ifdef __cplusplus
extern "C" {
#endif
/**
 * halts the cpu, defined elsewhere
 */
extern void kernel_panic();

/**
 * @brief remaps the PIC to PIC_REMAP_OFFSET, sets up an IDT gate
 *  for each of the 16 IRQs, then enables interrupts (sti)
 *  the IRQs themselves stay masked until someone unmasks them
 */
void IRQ_init();
#ifdef __cplusplus
}
#endif


/**
 * vector the PIC maps IRQ0 to, so IRQ n lands on
 *  PIC_REMAP_OFFSET + n. 0x20 keeps them out of the way
 *  of the cpu exceptions (0 - 31)
 */
#define PIC_REMAP_OFFSET 0x20

#endif
