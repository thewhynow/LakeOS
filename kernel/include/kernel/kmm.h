/**
 * @file kmm.h
 * @brief kernel heap, first-fit allocator over a doubly linked
 *  free list. lives at HEAP_START and pages are demand-mapped by
 *  the page fault handler
 */

#ifndef _KMM_H
#define _KMM_H

#include "../../libc/include/types.h"

/**
 * @brief sets up the whole heap as one big free block
 */
void  KMM_init();

/**
 * @brief allocates len bytes, the size is stored right before
 *  the pointer so ksize and kfree can find it
 * @param len how many bytes
 * @return pointer to the memory, NULL if no free block is big enough
 */
void *kmalloc(size_t len);

/**
 * @brief frees memory from kmalloc and merges it with the
 *  free blocks next to it
 * @param p pointer from kmalloc/krealloc/kexpand
 */
void  kfree(void *p);

/**
 * @brief resizes an allocation, grows in place if the free block
 *  after it is big enough, otherwise moves it
 *  does nothing if the new size isn't bigger
 * @param p the allocation, NULL acts like kmalloc
 * @param new_size the size wanted in total
 * @return the (possibly moved) allocation
 */
void *krealloc(void *p, size_t new_size);

/**
 * @brief grows an allocation by increment bytes
 * @param p the allocation, NULL acts like kmalloc
 * @param increment how many bytes to add
 * @return the (possibly moved) allocation
 */
void *kexpand(void *p, size_t increment);

/**
 * @brief gets the size of an allocation
 * @param p pointer from kmalloc, NULL gives 0
 * @return the size stored by the allocator, this includes
 *  the size field itself
 */
size_t ksize(void *p);

#endif

#ifdef _KMM_H_INTERNAL

/**
 * a free chunk of the heap, the chunk's
 *  memory is used for the node itself
 */
typedef struct t_FreeBlock t_FreeBlock;

struct t_FreeBlock {
    /** size of the whole chunk in bytes */
    size_t size;
    t_FreeBlock *next;
    t_FreeBlock *prev;
};

/* where the heap is and how big it is */
#define HEAP_START ((void*)0xD0000000)
#define HEAP_SIZE  ((size_t)0x10000000)

/**
 * @brief finds the first free block that fits and carves the
 *  allocation off the end of it
 * @param bytes size wanted, the size field gets added on top
 * @return pointer past the size field, NULL if nothing fits
 */
void *find_first_fit(size_t bytes);

/**
 * @brief merges a just-freed block with the one after it,
 *  or with the one before it
 * @param block the block that was just inserted in the free list
 */
void coalesce_neighbors(t_FreeBlock *block);

#endif
