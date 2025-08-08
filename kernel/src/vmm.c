#include "../include/vmm.h"
#include "../include/pmm.h"
#include "../../libc/include/string.h"

#define ENTRIES_PER_STRUCT 1024

#define PAGE_DIR_INDEX(vaddr)\
    (((vaddr) >> 22) & 0x3FF)

#define PAGE_TABLE_INDEX(vaddr)\
    (((vaddr) >> 12) &0x3FF)

#define PTE_FROM_PDIR(page)\
    (*page & ~0xFFF)

#define PTABLE_ADDRESS_SPACE 0x400000
#define DTABLE_ADDRESS_SPACE 0x100000000

#define PAGE_SIZE 4096

typedef struct {
    pt_entry_t entries[ENTRIES_PER_STRUCT];
} ptable_t;

typedef struct {
    pd_entry_t entries[ENTRIES_PER_STRUCT];
} pdirectory_t;

extern pdirectory_t page_directory;
extern ptable_t     higher_half_page_table;

static void flush_pd(){
/* to shut apple intellisense up */
#ifndef __APPLE__
    asm volatile (
        "movl %%cr3, %%eax\n"
        "movl %%eax, %%cr3\n"
        ::: "eax"
    );
#endif
}

static void flush_tlb_entry(vaddr_t vaddr){
    asm volatile (
        "cli\n"
        "invlpg (%0)\n"
        "sti\n"
    :: "r"(vaddr) : "memory");
}

void *vmm_map_page(void *paddr, void *vaddr){
    pd_entry_t *pdir_entry = &page_directory.entries[PAGE_DIR_INDEX((uint32_t)vaddr)];

    /* page table will be mapped to 0xC03FF000 */
    ptable_t *ptable = (ptable_t*) 0xC03FF000;

    /* used to map the page table into memory */
    pt_entry_t *map_ptable_entry = &higher_half_page_table.entries[PAGE_TABLE_INDEX(0xC03FF000)];

    /* page table not present, allocate */
    if (!ENTRY_GET_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_PRESENT)){
        /* map the page table so it can be used */
        ENTRY_SET_FRAME(*map_ptable_entry, alloc_page());
        flush_tlb_entry(0xC03FF000);
        
        memset(ptable, 0, ENTRIES_PER_STRUCT * sizeof(pt_entry_t));

        /* set new page table as writeable and present */
        ENTRY_ADD_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_PRESENT);
        ENTRY_ADD_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_WRITEABLE);
        ENTRY_SET_FRAME(*pdir_entry, ENTRY_GET_ATTRIBUTE(*map_ptable_entry, PAGE_STRUCT_PAGE_FRAME));
    } 
    else {
        /* map the page table so it can be used */
        ENTRY_SET_FRAME(*map_ptable_entry, PTE_FROM_PDIR(pdir_entry));
        flush_tlb_entry(0xC03FF000);
    }

    /* get the page table entry from the page table */
    pt_entry_t *pte = &ptable->entries[PAGE_TABLE_INDEX((uint32_t)vaddr)];
    
    /* map the page table entry */
    ENTRY_ADD_ATTRIBUTE(*pte, PAGE_STRUCT_ENTRY_PRESENT);
    ENTRY_ADD_ATTRIBUTE(*pte, PAGE_STRUCT_ENTRY_WRITEABLE);
    ENTRY_SET_FRAME(*pte, paddr);

    flush_tlb_entry((vaddr_t) vaddr);

    return vaddr;
}

void *valloc_page(void *vaddr){
    void *page = alloc_page();
    return vmm_map_page(page, vaddr ? vaddr : page);
}

void VMM_init() {
    
    extern uint8_t *bitmap;
    extern size_t  bitmap_len;
    
    size_t bitmap_size = bitmap_len / 8 + !!(bitmap_len % 8);
    size_t num_pages = bitmap_size / 4096 + !!(bitmap_size % 4096);
    
    if ((size_t)bitmap % 4096) {
        printf("Bitmap not page-aligned! im too lazy to fix this 'bug'\n");
        for (;;);
    }
    
    for (size_t i = 0; i < num_pages; ++i)
        vmm_map_page(bitmap, bitmap + 0xC0000000 + i * 4096);
    
    /* just invalidate the identity-mapping page directory entry */
    page_directory.entries[PAGE_DIR_INDEX(0x0)] = 0;

    bitmap += 0xC0000000;
    
    flush_pd();
}