#ifndef _KMM_H
#define _KMM_H

#include "../../libc/include/types.h"

void  KMM_init();

void *kmalloc(size_t len);
void  kfree(void *p);

void *krealloc(void *p, size_t new_size);
void *kexpand(void *p, size_t increment);

size_t ksize(void *p);

#endif

#ifdef _KMM_H_INTERNAL

typedef struct t_FreeBlock t_FreeBlock;

struct t_FreeBlock {
    size_t size; 
    t_FreeBlock *next;
    t_FreeBlock *prev;
};

#define HEAP_START ((void*)0xD0000000)
#define HEAP_SIZE  ((size_t)0x10000000)

void *find_first_fit(size_t bytes);

void coalesce_neighbors(t_FreeBlock *block);

#endif
