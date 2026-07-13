#include <stddef.h>

#ifndef __UMM_H
#define __UMM_H

typedef struct umm_block_t umm_block_t;
/* don't want cyclical includes */
typedef struct t_Process t_Process;

/**
 * allocates len memory within user memory space
 *  for specified process.
 * assumes:
 *  len % 0x1000 == 0
 */
void *ualloc_pages(
    t_Process *process, size_t len, int prot, int flags
);

/**
 * allocates len memory within user memory space
 *  starting at vaddr
 * assumes:
 *  vaddr % 0x1000 == 0
 *  len % 0x1000 == 0
 *  ** there is no existing block at the range specified
 */
void umap_pages(t_Process *process, void *vaddr, size_t len, int prot, int flags);

/**
 * unmaps the block starting at vaddr 
 */
void umm_unmap_pages(t_Process *process, void *vaddr);

void umm_page_flt_handler(void *fault_addr);

struct umm_block_t {
    void *start;
    size_t len;
    int prot;
    int flags;
    struct umm_block_t *next;
};

typedef enum {
    UMM_BLOCK_PROT_NONE  = 0x0,
    UMM_BLOCK_PROT_READ  = 0x1,
    UMM_BLOCK_PROT_WRITE = 0x2,
    UMM_BLOCK_PROT_EXEC  = 0x4,
} e_UMM_BLOCK_PROT;

typedef enum {
    /* mutually exclusive */
    UMM_BLOCK_FLAG_SHARED    = 0x01,
    UMM_BLOCK_FLAG_PRIVATE   = 0x02,

    /* not */
    UMM_BLOCK_FLAG_ANONYMOUS = 0x20,
    UMM_BLOCK_FLAG_FIXED     = 0x10,
    UMM_BLOCK_FLAG_GROWSDOWN = 0x0100,
} e_UMM_BLOCK_FLAG;

#endif

