/**
 * @file io.h
 * @brief x86 port io (in/out instructions)
 */

#ifndef _IO_H
#define _IO_H

#include "../../libc/include/types.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief writes a byte to a port (outb)
 * @param port the io port
 * @param data the byte to write
 */
void port_write_byte(uint16_t port, uint8_t data);

/**
 * @brief writes a word to a port (outw)
 * @param port the io port
 * @param data the word to write
 */
void port_write_word(uint16_t port, uint16_t data);

/**
 * @brief writes a long to a port (outl)
 * @param port the io port
 * @param data the long to write
 */
void port_write_long(uint16_t port, uint32_t data);

/**
 * @brief reads a byte from a port (inb)
 * @param port the io port
 * @return the byte read
 */
uint8_t port_read_byte(uint16_t port);

/**
 * @brief reads a word from a port (inw)
 * @param port the io port
 * @return the word read
 */
uint16_t port_read_word(uint16_t port);

/**
 * @brief reads a long from a port (inl)
 * @param port the io port
 * @return the long read
 */
uint32_t port_read_long(uint16_t port);

/**
 * @brief same as port_write_byte but with two jmps after the
 *  out so old hardware has time to catch up
 * @param port the io port
 * @param data the byte to write
 */
void port_write_slow_byte(uint16_t port, uint8_t data);

/**
 * @brief burns a tiny bit of time by writing to port 0x80,
 *  which nothing uses
 */
void io_wait();
#ifdef __cplusplus
}
#endif


#endif
