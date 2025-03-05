#ifndef _PS2_H
#define _PS2_H

#include "pic.h"
#include "irq.h"
#include "../../../libc/include/string.h"
#include "tty.h"
#include <stddef.h>

extern char KEYBOARD_MAP[0x58];

void IRQ_keyboard_handler();

/* size of the internal PS2 stdin buffer */
#define PS2_STDIN_SIZE 0xFF

/* returns stdin after reading input */
char* PS2_read();

/* flushes the internal stdin buffer */
void PS2_flush();

#endif