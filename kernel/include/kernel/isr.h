/**
 * @file isr.h
 * @brief cpu exceptions + software interrupts (the isr stubs in asm/isr.s)
 */

#ifndef _ISR_H
#define _ISR_H

#include "../../libc/include/types.h"
#include "../../libc/include/stdio.h"
#include "idt.h"
#include "gdt.h"

/**
 * the saved state of whatever was running when an
 *  interrupt came in, handlers get a pointer to this.
 * members are in reverse order they are pushed, so
 *  don't shuffle it
 * also doubles as the context of a process (see t_Process)
 */
typedef struct {
    uint32_t ds; ///< datasegment pushed by us
    uint32_t edi, esi, ebp, kern_esp, ebx, edx, ecx, eax; ///< general purpose register saved with "pusha"
    uint32_t int_num, error; ///< pushed int_num and error code
    uint32_t eip, cs, eflags, esp, ss; ///< pushed by CPU when issuing interrupt

} __attribute__((__packed__)) registers_t;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief points every IDT gate at its isr stub and enables them,
 *  and installs the page fault (0xE) and syscall (0x80) handlers
 */
void ISR_init();
#ifdef __cplusplus
}
#endif


/**
 * an isr handler, called with the registers
 *  saved by the stub
 */
typedef void(*ISR_handler_t)(registers_t* regs);

#endif
