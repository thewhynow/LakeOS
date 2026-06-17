#include <stdio.h>

#ifdef __is_libk

#include <kernel/tty.h>

int putchar(int c){
    terminal_putchar(c);		
    return c;
}

#else

/**
 * implement userspace (no biggie)
 */

#endif

