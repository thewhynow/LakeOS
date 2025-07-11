#ifndef _STRING_H
#define _STRING_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
size_t strlen(const char*);
char* strncat(char* __restrict, const char* __restrict, size_t);

#ifdef __cplusplus
}
#endif

#endif