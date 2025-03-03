#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../../../libc/include/string.h"
#include "../../include/kernel/tty.h"
#include "vga.h"

static const size_t TERMINAL_WIDTH = 80LU;
static const size_t TERMINAL_HEIGHT = 25LU;

static size_t terminal_row =     0;
static size_t terminal_col =     0;
static uint16_t* terminal_buff = (uint16_t*) 0xb8000;
static uint8_t terminal_color;

void terminal_init(){
    terminal_color = vga_color(VGA_WHITE, VGA_BLACK);
    for (size_t y = 0; y < TERMINAL_HEIGHT; ++y)
        for (size_t x = 0; x < TERMINAL_WIDTH; ++x)
            terminal_buff[y * TERMINAL_WIDTH + x] = vga_entry(' ', vga_color(VGA_WHITE, VGA_BLACK));
}

#define terminal_setcolor(color)\
    (terminal_color = (color))

#define terminal_putentryat(c, color, x, y)\
    (terminal_buff[(y) * TERMINAL_WIDTH + (x)] = vga_entry((c), (color)))

void terminal_scroll(){
    memcpy(
        terminal_buff + 1 * 80,
        terminal_buff + 0 * 80,
        80 * 24 * 2
    );

    memset(terminal_buff + 24 * 80, 80, vga_entry(' ', terminal_color));

    terminal_col = 0;
    terminal_row = 24;
}

void terminal_putchar(char c){
    if (c != '\n'){
        terminal_putentryat(c, terminal_color, terminal_col, terminal_row);
        if (++terminal_col == TERMINAL_WIDTH){
            terminal_col = 0;
            if (++terminal_row == TERMINAL_HEIGHT)
                terminal_scroll();
        }
    }
    else {
        terminal_col = 0;
        if (++terminal_row == TERMINAL_HEIGHT)
           terminal_scroll();
    }
}

void terminal_write(const char* str, size_t len){
    for (size_t i = 0; i < len; ++i)
        terminal_putchar(str[i]);
}

void terminal_print(const char* str){
    terminal_write(str, strlen(str));
}