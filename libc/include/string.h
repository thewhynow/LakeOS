#ifndef _STRING_H
#define _STRING_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void *, const void *, size_t);
void *memcpy(void *__restrict, const void *__restrict, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
unsigned long strlen(const char *);
char *strncat(char *__restrict, const char *__restrict, size_t);
char *strtok(char *str, const char *sep);
char *strdup(const char *str);
char *strcpy(char *, const char *);
int strcmp(const char *, const char *);
char *strchr(const char *str, int sep);
char *strpbrk(const char *str, const char *set);

#ifdef __cplusplus
}
#endif

#endif
