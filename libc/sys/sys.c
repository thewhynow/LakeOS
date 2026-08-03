#include <sys/sys.h>
#include <kernel/sys.h>

size_t read(int fd, void *buff, size_t count){
#ifdef __is_libk
    sys_read(fd, buff, count);
#else

#endif
}

size_t write(int fd, const void *buff, size_t count){
#ifdef __is_libk
    sys_write(fd, buff, count);
#else

#endif
}

int open(const char *filename, uint8_t mode){
#ifdef __is_libk
    sys_open(filename, mode);
#else

#endif
}

int close(int fd){
#ifdef __is_libk
    sys_close(fd);
#else

#endif
}

int stat(const char *filename, stat_t *statbuff){
#ifdef __is_libk
    sys_stat(filename, (void*) statbuff);
#else

#endif
}

int fstat(int fd, stat_t *statbuff){
#ifdef __is_libk
    sys_fstat(fd, (void*) statbuff);
#else

#endif
}

size_t lseek(int fd, ssize_t offset, int whence){
#ifdef __is_libk
    sys_lseek(fd, offset, whence);
#else

#endif
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, size_t offset){
#ifdef __is_libk
    sys_mmap(addr, length, prot, flags, fd, offset);
#else

#endif
}

int munmap(void *addr, size_t length){
#ifdef __is_libk
    sys_munmap(addr, length);
#else

#endif
}

int exec(const char *path, int argc, char **argv){
#ifdef __is_libk
    sys_exec(path, argc, argv);
#else

#endif
}

void exit(int status){
#ifdef __is_libk
    sys_exit(status);
#else

#endif
}


