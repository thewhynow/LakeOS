/**
 * @file pic.h
 * @brief the 8259 programmable interrupt controller (master + slave)
 */

#ifndef _PIC_H
#define _PIC_H

#include "../../libc/include/types.h"
#include "io.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief runs the init sequence on both PICs, which remaps their
 *  vectors and masks every irq afterwards
 * @param offset_pic1 vector for IRQ0 - IRQ7 (master)
 * @param offset_pic2 vector for IRQ8 - IRQ15 (slave)
 */
void PIC_configure(uint8_t offset_pic1, uint8_t offset_pic2);

/**
 * @brief masks an irq so the PIC stops forwarding it
 * @param irq irq number, 0 - 15
 */
void PIC_mask(int irq);

/**
 * @brief unmasks an irq
 * @param irq irq number, 0 - 15
 */
void PIC_unmask(int irq);

/**
 * @brief reads the interrupt request register (irqs that are raised)
 * @return slave in the low byte, master in the high byte
 */
uint16_t PIC_readIRQ_request_reg();

/**
 * @brief reads the in-service register (irqs that are being handled)
 * @return slave in the low byte, master in the high byte
 */
uint16_t PIC_readIRQ_inservice_reg();

/**
 * @brief tells the PIC the irq is done being handled
 *  has to be called at the end of every irq handler or
 *  that line never fires again
 * @param irq irq number, 0 - 15. the slave only gets the signal if irq >= 8
 */
void PIC_end_of_int(int irq);
#ifdef __cplusplus
}
#endif


/* io ports of the master and slave PIC */
#define PIC1_COMMAND_PORT 0x20
#define PIC1_DATA_PORT    0x21
#define PIC2_COMMAND_PORT 0xA0
#define PIC2_DATA_PORT    0xA1

#endif
