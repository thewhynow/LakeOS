#ifndef _PMM_H
#define _PMM_H

#include "../../../libc/include/types.h"

#include "multiboot.h"

typedef struct {
    uint32_t addr;
    uint32_t len;
#define PMM_REGION_FREE 0
    uint32_t used;
} mem_region_state_t;

void PMM_init();

typedef struct {
    size_t size;
    uint8_t data[];
} allocated_chunk_t;

typedef struct free_chunk_t free_chunk_t;
struct free_chunk_t {
    size_t size;
    free_chunk_t* front;
    free_chunk_t* back;
};

void* kmalloc(size_t size);
void kfree(void* ptr);

#endif