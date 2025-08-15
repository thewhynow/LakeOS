#define _KMM_H_INTERNAL
#include "../include/kmm.h"

static t_FreeBlock *freelist = HEAP_START;

void KMM_init(){
    freelist->next = freelist;
    freelist->prev = freelist;
    freelist->size = HEAP_SIZE;
}

void *kmalloc(size_t bytes){
    return find_first_fit(bytes);
}

void kfree(void *p){
    t_FreeBlock 
        *next,
        *iter   = freelist,
        *fblock = (t_FreeBlock*)((size_t*)p - 1);

    /* keep blocks sequential */
    
    while (iter->next < fblock) iter = iter->next;

    next = iter->next;
    
    next->prev = fblock;
    iter->next = fblock;

    fblock->next = next;
    fblock->prev = iter;

    coalesce_neighbors(fblock);
}

void *find_first_fit(size_t bytes){
    t_FreeBlock 
        *iter = freelist->next,
        *stop = freelist->next;
    
    /* store the size of the allocation as metadata */
    bytes += sizeof(size_t);

    bytes = bytes < sizeof(t_FreeBlock) 
        ? sizeof(t_FreeBlock) 
        : bytes;
    
    do {
        size_t *ret;
        /* does the block have enough room for node after alloc? */
        if (iter->size >= bytes + sizeof(t_FreeBlock)){
            iter->size -= bytes;
            ret = (size_t*)((uint8_t*)iter + iter->size);

            *ret = bytes;
            return ret + 1;
        }
        /* won't have enough room for metadata, delete */
        if (iter->size >= bytes){
            ret = (size_t*)iter;
            iter->prev->next = iter->next;
            *ret = bytes;
            return ret + 1;
        }

        iter = iter->next;
    } while(iter != stop);

    return NULL;
}

void coalesce_neighbors(t_FreeBlock *block){
    t_FreeBlock *prev = block->prev,
                *next = block->next;
    
    /* nothing to coalesce if only one block exists */
    if (block->next == block || block->prev == block)
        return;

    if ((uint8_t*)block + block->size == (uint8_t*)next){
        /* merge 'next' */
        next->next->prev = block;     
        block->next = next->next;

        block->size += next->size;
    }

    if ((uint8_t*)prev + prev->size == (uint8_t*)block){
        /* merge 'block' */
        prev->next = next;
        next->prev = prev;

        prev->size += block->size;
    }
}
