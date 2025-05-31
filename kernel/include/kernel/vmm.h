#ifndef VMM_H
#define VMM_H

/* personal reference */
/*
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
            * [3]: Page Write-Through; 0 = write-back caching (better perf); 1 = write-through caching
            * [4]: Page Cache Disable; 0 = enabled; 1 = disabled
            * [5]: indicates whether software has accesed the 4KB page
            * [6]: indicates whether sofwtare has written to the page
            * [7]: 0
            * [8]: Global; 0 = reloaded on context switch; 1 = not reloaded on context switch 
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

#include "../../../libc/include/types.h"

typedef enum {
    PAGE_STRUCT_PRESENT       = 0b00000000000000000000000000000001,
    PAGE_STRUCT_WRITEABLE     = 0b00000000000000000000000000000010,
    PAGE_STRUCT_USER_ACCESS   = 0b00000000000000000000000000000100,
    PAGE_STRUCT_WRITE_THROUGH = 0b00000000000000000000000000001000,
    PAGE_STRUCT_CACHE_DISABLE = 0b00000000000000000000000000010000,
    PAGE_STRUCT_ACCESSED      = 0b00000000000000000000000000100000,
    PAGE_STRUCT_DIRTY         = 0b00000000000000000000000001000000,
    PTE_GLOBAL        = 0b00000000000000000000000100000000,
    PTE_PAGE_FRAME    = 0b11111111111111111110000000000000
} PAGE_STRUCT_MASKS;

typedef uint32_t pt_entry;

typedef enum {
    PDE_PRESENT       = 0b00000000000000000000000000000001,
    PDE_WRITEABLE     = 0b00000000000000000000000000000010,
    PDE_USER_ACCESS   = 0b00000000000000000000000000000100,
    PDE_WRITE_THROUGH = 0b00000000000000000000000000001000,
    PDE_CACHE_DISABLE = 0b00000000000000000000000000010000,
    PDE_ACCESSED      = 0b00000000000000000000000000100000,
    PDE_DIRTY         = 0b00000000000000000000000001000000,
    PDE_PAGE_SIZE     = 0b00000000000000000000000010000000,
    PDE_GLOBAL        = 0b00000000000000000000000100000000,
    PDE_PAGE_TABLE    = 0b11111111111111111110000000000000,
} PAGE_PDE_MASKS;

#define ENTRY_ADD_ATTRIBUTE(entry, attrib)\
    ((entry) |= (attrib))

#define ENTRY_DEL_ATTRIBUTE(entry, attrib)\
    ((entry) &= ~(attrib))

#define ENTRY_GET_ATTRIBUTE(entry, attrib)\
    ((entry) & (attrib))

#define ENTRY_SET_FRAME(entry, mask, frame)\
    ((entry) |= (mask) & (frame))

#ifdef __cplusplus
extern "C" {
#endif

void VMM_init();

#ifdef __cplusplus
}
#endif

#endif