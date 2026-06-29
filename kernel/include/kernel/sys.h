#ifndef _SYS_H
#define _SYS_H

#include <stdio.h>
#include <stdlib.h>
#include <types.h>

#include <kernel/vfs.h>
#include <kernel/isr.h>

typedef enum {
    SYSCALL_READ  = 0,
    SYSCALL_WRITE = 1,
    SYSCALL_OPEN  = 2,
    SYSCALL_CLOSE = 3,
    SYSCALL_STAT  = 4,
    SYSCALL_EXEC  = 59,
    SYSCALL_EXIT  = 60
} e_SYSCALL_NUMS;

void ISR_syscall_handler(registers_t *regs);

size_t sys_read(int fd, void *buff, size_t count);
size_t sys_write(int fd, const void *buff, size_t count);
int    sys_open(const char *filename, uint8_t mode);
int    sys_close(int fd);
int    sys_stat(const char *filename, const t_FileStat *statbuff);
int    sys_exec(const char *path, char *const *argv, char *const *envp);
void   sys_exit(int status);

#endif
