#ifndef _PIT_H
#define _PIT_H

#include "io.h"
#include "idt.h"
#include "irq.h"

#ifdef __cplusplus
extern "C" {
#endif
void PIT_set_freq(int hz);
void IRQ_time_handler();
void PIT_init();
void PIT_sleep(uint64_t ms);
#ifdef __cplusplus
}
#endif


#endif