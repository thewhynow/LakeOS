#include <stdio.h>
#include <string.h>

#ifdef __is_libk

#include <kernel/ps2.h>
#include <kernel/tty.h>

char *gets(char *str) {
  char *buff = PS2_read();
  terminal_putchar('\n');

  size_t len = strlen(buff);
  memcpy(str, buff, len);

  memmove(buff, buff + len, PS2_STDIN_SIZE - len);

  return str;
}
#else

#endif
