#ifndef _PIC_H
#define _PIC_H

#include "../../../libc/include/types.h"
#include "io.h"

void PIC_configure(uint8_t offset_pic1, uint8_t offset_pic2);
void PIC_mask(int irq);
void PIC_unmask(int irq);
uint16_t PIC_readIRQ_request_reg();
uint16_t PIC_readIRQ_inservice_reg();
void PIC_end_of_int(int irq);
#endif