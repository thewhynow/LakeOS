/**
 * @file pmm.h
 * @brief physical memory manager, one bit per 4KB block
 *  returns PHYSICAL addresses
 */

#ifndef _PMM_H
#define _PMM_H

#include "multiboot.h"

#include "../../libc/include/string.h"
#include "../../libc/include/stdio.h"
#include "../../libc/include/stdlib.h"

/* size of one block (page) of physical memory */
#define MEMORY_BLOCK_SIZE 0x1000

/* what a bit in the bitmap means */
#define BITMAP_BLOCK_FREE 0x0
#define BITMAP_BLOCK_USED 0x1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief builds the bitmap from the multiboot memory map
 *  everything is reserved except the available regions,
 *  and the kernel and the bitmap itself are carved out of those
 */
void PMM_init();

/**
 * @brief grabs one free page
 * @return the physical address, NULL if out of memory
 */
void* alloc_page();

/**
 * @brief gives a page back
 * @param page the physical address from alloc_page
 */
void free_page(void* page);

/**
 * @brief grabs n pages in a row
 *  up to the caller to keep track of allocation sizes >:)
 * @param n how many pages
 * @return the physical address of the first one, NULL if there
 *  is no gap that big
 */
void* alloc_pages(size_t n);

/**
 * @brief frees n pages in a row
 * @param page the physical address from alloc_pages
 * @param n how many pages, same as what was allocated
 */
void free_pages(void* page, size_t n);

#ifdef __cplusplus
}
#endif

#endif
