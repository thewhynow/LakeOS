/**
 * @file vmm.h
 * @brief virtual memory manager, 32-bit two level paging (4KB pages,
 *  and 4MB ones). the kernel lives in the top 1GB (0xC0000000 up),
 *  user stuff below it, see umm.h
 */

#ifndef VMM_H
#define VMM_H

/**
 * personal reference

Page Directory:
    * comprises of 1024 32-bit entries (PDE)
    * PDE can either be a page table or 4MB page
    * Page Table (PT):
        * [0]: present; 1
        * [1]: read/write; 1 = write
        * [2]: user/supervisor; 0 = kernel-only; 1 = user
        * [3]: PWT; {refer to PTE[3]}
        * [4]: PCD; {refer to PTE[4]}
        * [5]: accessed
        * [6]: ignored
        * [7]: 0 = PT; 1 = PDE
        * [8:11]: ignored
        * [12:31]: physical address of 4KB Page Table referenced by entry

        * comprises of 1024 32-bit entires (PTE)
        * Page Table Entry:
            * maps a 4KB page
            * [0]: present (1 to map)
            * [1]: read/write; if 0, writes not allowed
            * [2]: user/supervisor; 0 = kernel-only; 1 = user-accesible
            * [3]: Page Write-Through; 0 = write-back caching (better perf); 1 =
write-through caching
            * [4]: Page Cache Disable; 0 = enabled; 1 = disabled
            * [5]: indicates whether software has accesed the 4KB page
            * [6]: indicates whether sofwtare has written to the page
            * [7]: 0
            * [8]: Global; 0 = reloaded on context switch; 1 = not reloaded on
context switch
                -> used for consistently mapped pages i.e. kernel pages
            * [9:11]: ignored; used by OS
            * [12:31]: physical address of 4KB Page referenced by entry

    * Page Directory Entry (PDE)
        * [0]: present (1 to map)
        * [1]: read/write; 1=write
        * [2]: 0 = kernel-only; 1 = user-access
        * [3]: Page Write-Through; {refer to PTE[3]}
        * [4]: Page Cache Disable; {refer to PTE[4]}
        * [5]: Accessed?
        * [6]: Written?
        * [7]: 0 = PT; 1 = PDE
        * [8]: Global; {refer to Page Table[8]}
        * [9:11]: ignored
        * [12]: 0
        * [13:21]: 0
        * [22:31]: physical address of 4MiB Page
*/

#include <types.h>

/**
 * the bits of a page table entry or page directory entry
 *  (the reference above, the ones that line up)
 */
typedef enum {
    /** the entry maps something, any access faults if it's clear */
    PAGE_STRUCT_ENTRY_PRESENT       = 0b00000000000000000000000000000001,
    /** writes are allowed */
    PAGE_STRUCT_ENTRY_WRITEABLE     = 0b00000000000000000000000000000010,
    /** ring 3 can access it */
    PAGE_STRUCT_ENTRY_USER_ACCESS   = 0b00000000000000000000000000000100,
    PAGE_STRUCT_ENTRY_WRITE_THROUGH = 0b00000000000000000000000000001000,
    PAGE_STRUCT_ENTRY_CACHE_DISABLE = 0b00000000000000000000000000010000,
    PAGE_STRUCT_ENTRY_ACCESSED      = 0b00000000000000000000000000100000,
    PAGE_STRUCT_ENTRY_DIRTY         = 0b00000000000000000000000001000000,
    PDE_PAGE_SIZE                   = 0b00000000000000000000000010000000,
    PAGE_STRUCT_GLOBAL              = 0b00000000000000000000000100000000,
    /** the top 20 bits, the physical address of what the entry points to */
    PAGE_STRUCT_PAGE_FRAME          = 0b11111111111111111111000000000000,
} PAGE_STRUCT_ENTRY_MASKS;

/** a page table entry, maps a 4KB page */
typedef uint32_t pt_entry_t;
/** a page directory entry, points to a page table */
typedef uint32_t pd_entry_t;

/** a virtual address */
typedef uint32_t vaddr_t;
/** a physical address */
typedef uint32_t paddr_t;

/** sets the bits of attrib in an entry */
#define ENTRY_ADD_ATTRIBUTE(entry, attrib) ((entry) |= (uint32_t)(attrib))

/** clears the bits of attrib in an entry */
#define ENTRY_DEL_ATTRIBUTE(entry, attrib) ((entry) &= ~(uint32_t)(attrib))

/** the bits of attrib that are set in an entry (nonzero if any are) */
#define ENTRY_GET_ATTRIBUTE(entry, attrib) ((entry) & (uint32_t)(attrib))

/** sets the bits of attrib in an entry to the same bits of value, so it can clear them too */
#define ENTRY_SET_ATTRIBUTE(entry, attrib, value)\
    ((entry) = ((entry) & ~(uint32_t)attrib) |   \
               ((uint32_t)(value) & (uint32_t)attrib))

/** points an entry to a physical address, the low 12 bits of frame are dropped */
#define ENTRY_SET_FRAME(entry, frame)                                          \
    ((entry) = ((entry) & ~(uint32_t)PAGE_STRUCT_PAGE_FRAME) |                 \
               ((uint32_t)(frame) & (uint32_t)PAGE_STRUCT_PAGE_FRAME))

/** entries in a page table or page directory */
#define ENTRIES_PER_STRUCT 1024

/** index into the page directory for a virtual address (top 10 bits) */
#define PAGE_DIR_INDEX(vaddr) (((vaddr) >> 22) & 0x3FF)

/** index into the page table for a virtual address (the 10 bits after that) */
#define PAGE_TABLE_INDEX(vaddr) (((vaddr) >> 12) & 0x3FF)

/** the physical address of the page table a page directory entry points to, takes a pointer to the entry */
#define PTE_FROM_PDIR(page) (*page & ~0xFFF)

/** how much memory one page table maps (4MB) and one page directory (4GB) */
#define PTABLE_ADDRESS_SPACE 0x400000
#define DTABLE_ADDRESS_SPACE 0x100000000

/** bytes in a page */
#define PAGE_SIZE 4096

/** a page table, 1024 entries */
typedef struct {
    pt_entry_t entries[ENTRIES_PER_STRUCT];
} ptable_t;

/** a page directory, 1024 entries */
typedef struct {
    pd_entry_t entries[ENTRIES_PER_STRUCT];
} pdirectory_t;

/**
 * the directory the kernel boots with, its upper half (768 and up)
 *  is what every other directory copies
 */
extern pdirectory_t kernel_page_directory;
/**
 * the page table that maps the first 4MB at 0xC0000000. its last
 *  entry is the window the vmm points at other page tables to edit
 *  them (shows up at 0xC03FF000)
 */
extern ptable_t higher_half_page_table;

/** the directory in use right now, kernel_page_directory until a process is made */
extern pdirectory_t *curr_page_directory;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief maps the pmm bitmap into the higher half, drops the identity
 *  mapping of the first 4MB, and flushes the page directory
 *  the bitmap has to be page aligned or it hangs
 */
void VMM_init();

/**
 * @brief maps a physical page at a virtual address in the current
 *  directory, making the page table if there isn't one
 * @param paddr physical address of the page
 * @param vaddr where to put it
 * @param write false makes it read-only
 * @param ring3 true lets userspace touch it
 * @return vaddr
 */
void *vmm_map_page(void *paddr, void *vaddr, bool write, bool ring3);

/**
 * @brief unmaps a page, and frees the page table too if that
 *  was the last page in it. doesn't free the page itself
 * @param vaddr the page to unmap
 */
void vmm_unmap_page(void *vaddr);

/**
 * @brief switches to another page directory
 *  assumes that the physical address is being passed
 * @param new_pd PHYSICAL address of the directory
 */
void switch_pd(pdirectory_t *new_pd);

/**
 * @brief allocates a page physically and maps it to the specified address
 *  if vaddr == NULL, identity-maps the page
 * @param vaddr where to map it, or NULL
 * @return the address it was mapped at
 */
void *valloc_page(void *vaddr);

/**
 * @brief unmaps a page and gives the physical page back to the pmm
 * @param vaddr the page, nothing happens if it isn't mapped
 */
void vfree_page(void *vaddr);

/**
 * @brief maps a 4MB page with a page directory entry
 * @param paddr physical address, 4MB aligned
 * @param vaddr where to put it, 4MB aligned
 * @return vaddr
 */
void *vmm_map_big_page(void *paddr, void *vaddr);

/**
 * @brief allocates 4MB of contiguous physical memory and maps it
 *  with a big page
 * @param vaddr where to map it, or NULL to identity-map
 * @return the address it was mapped at
 */
void *valloc_big_page(void *vaddr);

/**
 * @brief looks up what a virtual address is mapped to
 * @param vaddr a page aligned address
 * @return the physical address of the page, 0 if it isn't mapped
 */
void *virt_to_phys(void *vaddr);

/**
 * @brief copies the page table entries from old_pt to new_pt starting
 *  at start up to end
 * @param new_pt PHYSICAL address of the table to copy to
 * @param old_pt PHYSICAL address of the table to copy from
 * @param start first entry
 * @param end one past the last entry
 */
void copy_ptes(ptable_t *new_pt, ptable_t *old_pt, uint32_t start, uint32_t end);

/**
 * @brief creates a new page table at page directory index pdi
 *  in page directory pd
 * @param pd the directory to put it in
 * @param pdi index in the directory
 * @param ring3 true lets userspace into it
 */
void new_page_table(pdirectory_t *pd, uint32_t pdi, bool ring3);

/**
 * @brief creates a new page directory and copies all the relevant
 *  mappings from other direcrtories
 *  that's the kernel half, and the shared blocks of the current process
 * @return the PHYSICAL address of the new directory, which is what switch_pd wants
 */
pdirectory_t *new_page_directory();

/**
 * @brief updates the permissions for the PTE relating to
 *  vaddr
 * @param vaddr the page
 * @param write true makes it writeable
 * @param ring3 true lets userspace touch it
 */
void vmm_new_permissions(void *vaddr, bool write, bool ring3);

#ifdef __cplusplus
}
#endif

#endif
