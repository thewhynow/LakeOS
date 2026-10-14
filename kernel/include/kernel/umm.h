/**
 * @file umm.h
 * @brief user memory manager, keeps track of what is mapped in a
 *  process's address space (0x0 - 0xC0000000) as a list of blocks
 *  sorted by address
 */

#include <stddef.h>

#ifndef __UMM_H
#define __UMM_H

typedef struct umm_block_t umm_block_t;
/* don't want cyclical includes */
typedef struct t_Process t_Process;

/**
 * @brief allocates len memory within user memory space
 *  for specified process.
 *  assumes:
 *   len % 0x1000 == 0
 * @param process the process to give memory to
 * @param len how many bytes
 * @param prot UMM_BLOCK_PROT_* or'd together
 * @param flags UMM_BLOCK_FLAG_* or'd together
 * @return the start of the block, first fit from address 0,
 *  NULL if there is no gap that big
 */
void *ualloc_pages(
    t_Process *process, size_t len, int prot, int flags
);

/**
 * @brief allocates len memory within user memory space
 *  starting at vaddr
 *  assumes:
 *   vaddr % 0x1000 == 0
 *   len % 0x1000 == 0
 *   ** there is no existing block at the range specified
 * @param process the process to give memory to
 * @param vaddr where the block starts
 * @param len how many bytes
 * @param prot UMM_BLOCK_PROT_* or'd together
 * @param flags UMM_BLOCK_FLAG_* or'd together
 */
void umap_pages(t_Process *process, void *vaddr, size_t len, int prot, int flags);

/**
 * @brief unmaps the block starting at vaddr
 * @param process the process that owns it
 * @param vaddr the start of the block, has to match exactly
 */
void umm_unmap_pages(t_Process *process, void *vaddr);

/**
 * @brief handles a page fault below 0xC0000000, which only works
 *  for a stack (GROWSDOWN) block: maps one more page under it
 *  anything else is a fatal fault and halts
 * @param fault_addr the address that faulted (cr2)
 */
void umm_page_flt_handler(void *fault_addr);

/**
 * @brief unmaps a range page by page, so it can start or end in the
 *  middle of a block (splitting it)
 * @param process the process to unmap from
 * @param vaddr start of the range, page aligned
 * @param len how many bytes, page aligned
 */
void umm_unmap_range(t_Process *process, void *vaddr, size_t len);

/**
 * a contiguous mapping in a process, they form a
 *  linked list sorted by start
 */
struct umm_block_t {
    void *start;
    /** in bytes */
    size_t len;
    /** UMM_BLOCK_PROT_* */
    int prot;
    /** UMM_BLOCK_FLAG_* */
    int flags;
    struct umm_block_t *next;
};

/**
 * what a block allows, or'd together
 */
typedef enum {
    UMM_BLOCK_PROT_NONE  = 0x0,
    UMM_BLOCK_PROT_READ  = 0x1,
    /** the pages are mapped writeable */
    UMM_BLOCK_PROT_WRITE = 0x2,
    UMM_BLOCK_PROT_EXEC  = 0x4,
} e_UMM_BLOCK_PROT;

/**
 * how a block behaves, or'd together. same values as the
 *  SYSCALL_MMAP_FLAG_* ones in sys.h
 */
typedef enum {
    /** mutually exclusive with PRIVATE */
    UMM_BLOCK_FLAG_SHARED    = 0x01,
    /** mutually exclusive with SHARED */
    UMM_BLOCK_FLAG_PRIVATE   = 0x02,

    /** not mutually exclusive, the block isn't backed by a file */
    UMM_BLOCK_FLAG_ANONYMOUS = 0x20,
    /** not mutually exclusive, the block goes exactly where it was asked */
    UMM_BLOCK_FLAG_FIXED     = 0x10,
    /** not mutually exclusive, a fault right under the block grows it down a page (stack) */
    UMM_BLOCK_FLAG_GROWSDOWN = 0x0100,
} e_UMM_BLOCK_FLAG;

#endif

