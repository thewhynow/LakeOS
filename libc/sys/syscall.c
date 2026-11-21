#include <sys/syscall.h>
#include <kernel/sys.h>

static inline uint32_t syscall6(
    uint32_t id, uint32_t a0, uint32_t a1,
    uint32_t a2, uint32_t a3, uint32_t a4,
    uint32_t a5
){
    uint32_t ret;

    __asm__ volatile (
        "push %[a5]         \n"
        "push %%ebp         \n"
        "mov  4(%%esp), %%ebp \n"
        "int  $0x80         \n"
        "pop  %%ebp         \n"
        "add  $4, %%esp     \n"
        : "=a" (ret)
        : "a" (id),
          "b" (a0),
          "c" (a1),
          "d" (a2),
          "S" (a3),
          "D" (a4),
          [a5] "g" (a5)
        : "memory", "cc"
    );

    return ret;
}

#define SYSCALL0(id)                    syscall6((id), 0, 0, 0, 0, 0, 0)
#define SYSCALL1(id, a)                 syscall6((id), (uint32_t)(a), 0, 0, 0, 0, 0)
#define SYSCALL2(id, a, b)              syscall6((id), (uint32_t)(a), (uint32_t)(b), 0, 0, 0, 0)
#define SYSCALL3(id, a, b, c)           syscall6((id), (uint32_t)(a), (uint32_t)(b), (uint32_t)(c), 0, 0, 0)
#define SYSCALL6(id, a, b, c, d, e, f)  syscall6((id), (uint32_t)(a), (uint32_t)(b), (uint32_t)(c), (uint32_t)(d), (uint32_t)(e), (uint32_t)(f))

size_t read(int fd, void *buff, size_t count){
#ifdef __is_libk
    return sys_read(fd, buff, count);
#else
    return SYSCALL3(SYSCALL_READ, fd, buff, count);
#endif
}

size_t write(int fd, const void *buff, size_t count){
#ifdef __is_libk
    return sys_write(fd, buff, count);
#else
    return SYSCALL3(SYSCALL_WRITE, fd, buff, count);
#endif
}

int open(const char *filename, uint8_t mode){
#ifdef __is_libk
    return sys_open(filename, mode);
#else
    return SYSCALL2(SYSCALL_OPEN, filename, mode);
#endif
}

int close(int fd){
#ifdef __is_libk
    return sys_close(fd);
#else
    return SYSCALL1(SYSCALL_CLOSE, fd);
#endif
}

int stat(const char *filename, stat_t *statbuff){
#ifdef __is_libk
    return sys_stat(filename, (void*) statbuff);
#else
    return SYSCALL2(SYSCALL_STAT, filename, statbuff);
#endif
}

int fstat(int fd, stat_t *statbuff){
#ifdef __is_libk
    return sys_fstat(fd, (void*) statbuff);
#else
    return SYSCALL2(SYSCALL_FSTAT, fd, statbuff);
#endif
}

size_t lseek(int fd, ssize_t offset, int whence){
#ifdef __is_libk
    return sys_lseek(fd, offset, whence);
#else
    return SYSCALL3(SYSCALL_LSEEK, fd, offset, whence);
#endif
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, size_t offset){
#ifdef __is_libk
    return sys_mmap(addr, length, prot, flags, fd, offset);
#else
    return (void*) SYSCALL6(SYSCALL_MMAP, addr, length, prot, flags, fd, offset);
#endif
}

int munmap(void *addr, size_t length){
#ifdef __is_libk
    return sys_munmap(addr, length);
#else
    return SYSCALL2(SYSCALL_MUNMAP, addr, length);
#endif
}

int exec(const char *path, int argc, char **argv){
#ifdef __is_libk
    return sys_exec(path, argc, argv);
#else
    return SYSCALL3(SYSCALL_EXEC, path, argc, argv);
#endif
}

void exit(int status){
#ifdef __is_libk
    sys_exit(status);
#else
    SYSCALL1(SYSCALL_EXIT, status);
#endif
    /* never returns */
    for (;;);
}
