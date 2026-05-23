#include <stdio.h>
#include <string.h>

#ifdef __is_libk

#include <kernel/tty.h>
#include <kernel/ps2.h>

int getchar(){
    char* buff = PS2_read();
    terminal_putchar('\n');
    char c = buff[0];
    memmove(buff, buff + 1, PS2_STDIN_SIZE - 1);
    return c;
}

#else

/* todo: implement files XD */

#endif
