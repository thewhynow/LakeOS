/**
 * @file tty.h
 * @brief the text mode terminal (80x25 vga buffer)
 */

#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief clears the screen and sets the color to white on black
 */
void terminal_init();

/**
 * @brief prints one char at the cursor and advances it
 *  '\n' goes to the start of the next line, scrolls
 *  when the cursor falls off the bottom of the screen
 * @param c the character to print
 */
void terminal_putchar(char c);

/**
 * @brief prints size chars of str
 * @param str the characters to print
 * @param size how many chars to print, str is not read for a null byte
 * @return size
 */
size_t terminal_write(const char *str, size_t size);

/**
 * @brief prints a null-terminated string
 * @param str the string to print
 */
void terminal_print(const char *str);

/**
 * @brief erases the char before the cursor and moves the cursor back
 *  wraps to the previous line if the cursor is at the start of one
 */
void terminal_delchar();

/**
 * @brief moves the hardware cursor to where the terminal thinks it is
 */
void terminal_update_cursor();

/**
 * @brief writes a byte to an io port
 *  same function as the one in io.h, just declared again here
 * @param port the io port
 * @param data the byte to write
 */
void port_write_byte(unsigned short port, unsigned char data);
#ifdef __cplusplus
}
#endif

#endif
