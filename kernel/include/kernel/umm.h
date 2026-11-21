#include <types.h>

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
 * maps a file into memory at given vaddr
 * assumes:
 *  vaddr was either mapped into memory through umap_pages or returned by ualloc_pages
 *  len is the size of the mapped memory through umap_pages or returned by ualloc_pages
 *  fd is a valid, OPEN, file descriptor
 *  offset < fd.len
 */
void umap_file(t_Process *process, void *vaddr, size_t len, int fd, size_t offset);

/**
 * updates mapping flags for range of address
 * assumes:
 *  vaddr % 0x1000 == 0
 *  len % 0x1000 == 0
 *  fd is a valid open file descriptor, -1 if anonymous mapping
 *  the flags of the existing mapping are consistent with the option above
 *  ** memory in the range of vaddr to len is mapped
 */
void umm_update_mapping(t_Process *process, void *vaddr, size_t len, int prot, int fd, size_t offset);

/**
 * syncs the mapping of a file back to disk,
 */
void umm_sync_file(t_Process *process, void *vaddr, size_t len);

/**
 * unmaps the block starting at vaddr 
 */
void umm_unmap_pages(t_Process *process, void *vaddr);

void umm_page_flt_handler(void *fault_addr);

/**
 * unmaps the area of memory starting at vaddr and ending at vaddr + len
 */
void umm_unmap_range(t_Process *process, void *vaddr, size_t len);

struct umm_block_t {
    void *start;
    struct umm_block_t *next;
    size_t len;
    size_t fd_offset;
    int prot;
    int flags;
    int fd;
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
