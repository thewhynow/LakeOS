/**
 * @file sys.h
 * @brief syscalls, userspace gets here with int 0x80. each sys_*
 *  is the kernel side of one, the handler unpacks the registers
 *  and puts the result in eax
 */

#ifndef _SYS_H
#define _SYS_H

#include <stdio.h>
#include <stdlib.h>
#include <types.h>

#include <kernel/vfs.h>
#include <kernel/isr.h>

/**
 * syscall numbers, same as linux x86_64 ones
 *
 * for future reference:
 *  register argument order:
 *  [0] eax // the syscall number
 *  [1] ebx
 *  [2] ecx
 *  [3] edx
 *  [4] esi
 *  [5] edi
 *  [6] ebp
 */
typedef enum {
    SYSCALL_READ   = 0,
    SYSCALL_WRITE  = 1,
    SYSCALL_OPEN   = 2,
    SYSCALL_CLOSE  = 3,
    SYSCALL_STAT   = 4,
    SYSCALL_FSTAT  = 5,
    SYSCALL_LSEEK  = 8,
    SYSCALL_MMAP   = 9,
    SYSCALL_MUNMAP = 11,
    SYSCALL_EXEC   = 59,
    SYSCALL_EXIT   = 60
} e_SYSCALL_NUMS;

/**
 * modes for sys_open, same as VFS_FILE_* in vfs.h
 */
typedef enum {
    SYSCALL_OPEN_READ  = 0b00000001,
    SYSCALL_OPEN_WRITE = 0b00000010,
    SYSCALL_OPEN_APPND = 0b00000100,
    SYSCALL_OPEN_EOF   = 0b00001000,
} e_SYSCALL_OPEN_MODE;

/**
 * where sys_lseek counts the offset from
 */
typedef enum {
    /** the start of the file */
    SYSCALL_LSEEK_SET = 0,
    /** the current position */
    SYSCALL_LSEEK_CUR = 1,
    SYSCALL_LSEEK_END = 2,
} e_SYSCALL_LSEEK_WHENCE;

/**
 * protection for sys_mmap, same as UMM_BLOCK_PROT_* in umm.h
 */
typedef enum {
    SYSCALL_MMAP_PROT_NONE  = 0x0,
    SYSCALL_MMAP_PROT_READ  = 0x1,
    SYSCALL_MMAP_PROT_WRITE = 0x2,
    SYSCALL_MMAP_PROT_EXEC  = 0x4,
} e_SYSCALL_MMAP_PROT;

/**
 * flags for sys_mmap, same as UMM_BLOCK_FLAG_* in umm.h
 */
typedef enum {
    /** mutually exclusive with PRIVATE */
    SYSCALL_MMAP_FLAG_SHARED    = 0x01,
    /** mutually exclusive with SHARED */
    SYSCALL_MMAP_FLAG_PRIVATE   = 0x02,

    /** not mutually exclusive, no file backing it, the memory comes zeroed */
    SYSCALL_MMAP_FLAG_ANONYMOUS = 0x20,
    /** not mutually exclusive, map at addr exactly instead of wherever fits */
    SYSCALL_MMAP_FLAG_FIXED     = 0x10,
    /** not mutually exclusive */
    SYSCALL_MMAP_FLAG_GROWSDOWN = 0x0100,
} e_SYSCALL_MMAP_FLAG;

/**
 * @brief the int 0x80 handler, picks the syscall from eax, passes the
 *  args from the other registers, and puts the return value in eax
 *  unknown numbers return -1
 * @param regs the registers saved when the process trapped in
 */
void ISR_syscall_handler(registers_t *regs);

/**
 * @brief read from an open file
 * @param fd the file descriptor
 * @param buff where the data goes
 * @param count how many bytes to read
 * @return how many bytes were read
 */
size_t sys_read(int fd, void *buff, size_t count);

/**
 * @brief write to an open file
 * @param fd the file descriptor
 * @param buff the data
 * @param count how many bytes to write
 * @return how many bytes were written
 */
size_t sys_write(int fd, const void *buff, size_t count);

/**
 * @brief opens a file
 * @param filename full path
 * @param mode VFS_FILE_* or'd together
 * @return the file descriptor
 */
int sys_open(const char *filename, uint8_t mode);

/**
 * @brief closes a file descriptor
 * @param fd the file descriptor
 * @return 0
 */
int sys_close(int fd);

/**
 * @brief gets info about a file by path
 * @param filename full path
 * @param statbuff where the info goes
 * @return 0
 */
int sys_stat(const char *filename, t_FileStat *statbuff);

/**
 * @brief gets info about an open file
 * @param fd the file descriptor
 * @param statbuff where the info goes
 * @return 0
 */
int sys_fstat(int fd, t_FileStat *statbuff);

/**
 * @brief moves the position in an open file, writing zeros
 *  to the file if that's past the end
 * @param fd the file descriptor
 * @param offset how far to move, relative to whence
 * @param whence SYSCALL_LSEEK_*
 * @return the new position, -1 on failure
 */
size_t sys_lseek(int fd, ssize_t offset, int whence);

/**
 * @brief maps memory into the current process, the pages are
 *  allocated right away. anonymous memory is zeroed, otherwise
 *  the file is read into it
 * @param addr where to map it, page aligned. only matters with FIXED
 * @param length how many bytes, rounded up to a page
 * @param prot SYSCALL_MMAP_PROT_*
 * @param flags SYSCALL_MMAP_FLAG_*
 * @param fd the file to map, ignored for anonymous
 * @param offset where in the file the mapping starts
 * @return the address, (void*)-1 on failure (NULL if addr isn't
 *  page aligned)
 */
void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, size_t offset);

/**
 * @brief unmaps memory from the current process
 * @param addr start of the range, page aligned
 * @param length how many bytes, rounded up to a page
 * @return 0, -1 if addr isn't page aligned
 */
int   sys_munmap(void *addr, size_t length);

/**
 * @brief loads an elf from the filesystem and runs it as a new process,
 *  doesn't return
 * @param path full path of the elf
 * @param argc argument count
 * @param argv arguments
 * @return nothing really, never gets there
 */
int  sys_exec(const char *path, int argc, char **argv);

/**
 * @brief exits the current process and goes back to the one that exec'd it
 * @param status the exit status, becomes the return value of its sys_exec
 */
void sys_exit(int status);

#endif
