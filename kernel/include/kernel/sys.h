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
    SYSCALL_FSTAT = 5,
    SYSCALL_LSEEK = 8,
    SYSCALL_MMAP  = 9,
    SYSCALL_EXEC  = 59,
    SYSCALL_EXIT  = 60
} e_SYSCALL_NUMS;

typedef enum {
    SYSCALL_OPEN_READ  = 0b00000001,
    SYSCALL_OPEN_WRITE = 0b00000010,
    SYSCALL_OPEN_APPND = 0b00000100,
    SYSCALL_OPEN_EOF   = 0b00001000,
} e_SYSCALL_OPEN_MODE;

typedef enum {
    SYSCALL_LSEEK_SET = 0,
    SYSCALL_LSEEK_CUR = 1,
    SYSCALL_LSEEK_END = 2,
} e_SYSCALL_LSEEK_WHENCE;

typedef enum {
    SYSCALL_MMAP_PROT_NONE  = 0x0,
    SYSCALL_MMAP_PROT_READ  = 0x1,
    SYSCALL_MMAP_PROT_WRITE = 0x2,
    SYSCALL_MMAP_PROT_EXEC  = 0x4,
} e_SYSCALL_MMAP_PROT;

typedef enum {
    /* mutually exclusive */
    SYSCALL_MMAP_FLAG_SHARED    = 0x01,
    SYSCALL_MMAP_FLAG_PRIVATE   = 0x02,

    /* not */
    SYSCALL_MMAP_FLAG_ANONYMOUS = 0x20,
    SYSCALL_MMAP_FLAG_FIXED     = 0x10,
    SYSCALL_MMAP_FLAG_GROWSDOWN = 0x0100,
} e_SYSCALL_MMAP_FLAG;

void ISR_syscall_handler(registers_t *regs);

size_t sys_read(int fd, void *buff, size_t count);
size_t sys_write(int fd, const void *buff, size_t count);

int sys_open(const char *filename, uint8_t mode);
int sys_close(int fd);

int sys_stat(const char *filename, t_FileStat *statbuff);
int sys_fstat(int fd, t_FileStat *statbuff);

size_t sys_lseek(int fd, ssize_t offset, int whence);

void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, size_t offset);

int  sys_exec(const char *path, int argc, char **argv); 
void sys_exit(int status);

#endif
