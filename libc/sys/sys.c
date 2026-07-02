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

int stat(const char *filename, const stat_t *statbuff){
#ifdef __is_libk
    sys_stat(filename, (void*) statbuff);
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


