#include <types.h>

#ifndef __SYS_H
#define __SYS_H

typedef struct chrono_t {
    uint16_t year;
    uint8_t  seconds,
             minutes,
             hours,
             day_of_month,
             month_of_year;
} chrono_t;

typedef struct stat_t {
    chrono_t created,
             modified;
    size_t   size,
             flags;
} stat_t;

size_t read(int fd, void *buff, size_t count);
size_t write(int fd, const void *buff, size_t count);
int    open(const char *filename, uint8_t mode);
int    close(int fd);
int    stat(const char *filename, const stat_t *statbuff);
int    exec(const char *path, char *const *argv, char *const *envp);
void   exit(int status);

#endif
