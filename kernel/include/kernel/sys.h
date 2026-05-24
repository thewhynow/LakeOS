#ifndef _SYS_H
#define _SYS_H

#include <stdio.h>
#include <stdlib.h>
#include <types.h>

#include <kernel/vfs.h>
#include <kernel/isr.h>

typedef enum {
  SYSCALL_READ,
  SYSCALL_WRITE,
  SYSCALL_OPEN,
  SYSCALL_CLOSE,
} e_SYSCALL_NUMS;

void ISR_syscall_handler(registers_t *regs);

size_t sys_read(int fd, void *buff, size_t count);
size_t sys_write(int fd, const void *buff, size_t count);
int    sys_open(const char *filename, uint8_t mode);
int   sys_close(int fd);

#endif
