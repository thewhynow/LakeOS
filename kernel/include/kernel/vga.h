/**
 * @file vga.h
 * @brief vga text mode colors and cell packing
 */

#ifndef ARCH_I386_VGA_H
#define ARCH_I386_VGA_H

#include "../../libc/include/types.h"

/**
 * the 16 vga text mode colors, the value is
 *  the number the hardware wants
 */
typedef enum {
    /** used as the background for all text */
    VGA_BLACK,
    VGA_BLUE,
    VGA_GREEN,
    VGA_DARK_CYAN,
    VGA_RED,
    VGA_MAGENTA,
    VGA_ORANGE,
    VGA_LIGHT_GRAY,
    VGA_DARK_GRAY,
    VGA_PURPLE,
    VGA_LIGHT_GREEN,
    VGA_LIGHT_CYAN,
    VGA_LIGHT_RED,
    VGA_PINK,
    VGA_YELLOW,
    /** used (on black) for all text */
    VGA_WHITE
} vga_color_t;

/**
 * @brief packs a foreground and background into one color byte
 * @param fg text color, low nibble
 * @param bg background color, high nibble
 * @return the color byte
 */
static inline uint8_t vga_color(vga_color_t fg, vga_color_t bg){
    return fg | bg << 4;
}

/**
 * @brief packs a char and color byte into one cell of the vga buffer
 * @param c the character
 * @param color color byte from vga_color
 * @return the 16 bit cell, char in the low byte
 */
static inline uint16_t vga_entry(char c, uint8_t color){
    return (uint16_t)c | (uint16_t)color << 8;
}

#endif
