/**
 * @file ps2.h
 * @brief ps2 keyboard (scancode set 1) and the stdin buffer it fills
 */

#ifndef _PS2_H
#define _PS2_H

#include "pic.h"
#include "irq.h"
#include "../../libc/include/string.h"
#include "tty.h"

/* size of the internal PS2 stdin buffer */
#define PS2_STDIN_SIZE 0xFF

#ifdef __cplusplus
extern "C" {
#endif
/**
 * scancode -> ascii, indexed by the raw scancode
 *  0 for keys that don't print anything
 *  no shift, so everything is lowercase
 */
extern char KEYBOARD_MAP[0x58];

/**
 * @brief IRQ1 handler, grabs the scancode from the keyboard
 *  and sends the end of interrupt
 */
void IRQ_keyboard_handler();

/**
 * @brief blocks until enter is pressed, echoing every key to
 *  the terminal and handling backspace
 *  returns stdin after reading input
 * @return the internal stdin buffer, not a copy, and it is
 *  not cleared until PS2_flush
 */
char* PS2_read();

/**
 * @brief flushes the internal stdin buffer
 */
void PS2_flush();

/**
 * @brief reads input in a non psychotic manner
 *  does a PS2_read then copies what it got into buff
 * @param buff where to copy the line to
 * @param bytes the most that will be copied
 * @return meant to be the amount copied
 */
size_t PS2_read_bytes(void *buff, size_t bytes);

#ifdef __cplusplus
}
#endif


#endif
