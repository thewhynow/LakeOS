/**
 * @file pit.h
 * @brief the programmable interval timer (channel 0), also keeps
 *  the wall clock in rtc.h ticking
 */

#ifndef _PIT_H
#define _PIT_H

#include "io.h"
#include "idt.h"
#include "irq.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief sets how often channel 0 fires IRQ0
 * @param hz ticks per second
 */
void PIT_set_freq(int hz);

/**
 * @brief IRQ0 handler, counts a tick and every 1000 of them
 *  adds a second to the time in rtc.h
 */
void IRQ_time_handler();

/**
 * @brief sets the timer to 1000hz (1 tick == 1ms) and unmasks IRQ0
 */
void PIT_init();

/**
 * @brief busy-waits for the given time
 * @param ms milliseconds to wait, assumes PIT_init was called
 */
void PIT_sleep(uint64_t ms);
#ifdef __cplusplus
}
#endif


#endif

#ifdef _PIT_H_INTERNAL

/**
 * @brief days in a month, used by the rtc code as well
 * @param year the full year, this decides if february is 28 or 29
 * @param month 1 - 12
 * @return the number of days
 */
uint8_t max_monthday(uint16_t year, uint8_t month);

#endif
