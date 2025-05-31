#include "../../include/kernel/vmm.h"
#include "../../include/kernel/pmm.h"
#include "../../../libc/include/string.h"

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

/* allocates 4kb block of physical memory for PTE */
static bool vm_assign_entry(pt_entry_t *entry){
    void *block = alloc_page();

    if (!block)
        return false;
    else {
        ENTRY_SET_FRAME(*entry, block);
        ENTRY_ADD_ATTRIBUTE(*entry, PAGE_STRUCT_ENTRY_PRESENT);
        return true;
    }
}

/* frees a 4kb block of physical memory for PTE */
static void vm_unassign_entry(pt_entry_t *entry){
    void *block = (void*) ENTRY_GET_ATTRIBUTE(*entry, PAGE_STRUCT_PAGE_FRAME);
    if (block)
        free_page(block);
    
    ENTRY_DEL_ATTRIBUTE(*entry, PAGE_STRUCT_ENTRY_PRESENT);
}

static pt_entry_t *get_pte_from_vaddr(ptable_t *ptable, vaddr_t vaddr){
    return &ptable->entries[PAGE_TABLE_INDEX(vaddr)];
}

static pd_entry_t *get_pde_from_vaddr(pdirectory_t *pdirectory, vaddr_t vaddr){
    return &pdirectory->entries[PAGE_DIR_INDEX(vaddr)];
}

static pdirectory_t *curr_pd;

static void switch_pd(pdirectory_t *new){
    curr_pd = new;
    asm volatile("movl %0, %%cr3" :: "r"(new) : "memory");
}

static void flush_tlb_entry(vaddr_t vaddr){
    asm volatile (
        "cli\n"
        "invlpg (%0)\n"
        "sti\n"
    :: "r"(vaddr) : "memory");
}

void vmm_map_page(void *paddr, void *vaddr){
    pd_entry_t *pdir_entry = &curr_pd->entries[PAGE_DIR_INDEX((uint32_t)vaddr)];

    ptable_t *ptable;
    /* page table not present, allocate */
    if (!ENTRY_GET_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_PRESENT)){
        ptable = alloc_page();
        memset(ptable, 0, ENTRIES_PER_STRUCT * 4);
        
        /* set new page table */
        ENTRY_ADD_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_PRESENT);
        ENTRY_ADD_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_WRITEABLE);
        ENTRY_SET_FRAME(*pdir_entry, ptable);
    } 
    else
        ptable = (void*) PTE_FROM_PDIR(pdir_entry);
    
    /* get the page table entry */
    pt_entry_t *pte = &ptable->entries[PAGE_TABLE_INDEX((uint32_t)vaddr)];

    /* map the page table entry */
    ENTRY_ADD_ATTRIBUTE(*pte, PAGE_STRUCT_ENTRY_PRESENT);
    ENTRY_SET_FRAME(*pte, paddr);
}

void VMM_init() {
    /* default pt */
    ptable_t *table1 = alloc_page();
    /* used for identity mapping */
    ptable_t *table2 = alloc_page();

    memset(table1, 0, sizeof(ptable_t));
    memset(table2, 0, sizeof(ptable_t));

    /* linker symbols to denote start and end of kernel */
    extern uint8_t _begin, _end; 
    size_t kernel_size = &_end - &_begin;

    /* identity map the first 4MB */
    for (int i = 0; i < 1024; ++i){
        pt_entry_t *entry = &table2->entries[i];
        ENTRY_ADD_ATTRIBUTE(*entry, PAGE_STRUCT_ENTRY_PRESENT);
        ENTRY_SET_FRAME(*entry, i * 4096);
    }

    /* map physical 1MB to virtual 3GB */
    for (int i = 0, paddr = 0x100000; i < 1024; ++i, paddr += 4096){
        pt_entry_t *entry = &table1->entries[i];
        ENTRY_ADD_ATTRIBUTE(*entry, PAGE_STRUCT_ENTRY_PRESENT);
        ENTRY_SET_FRAME(*entry, paddr);
    }

    /* default dt */
    pdirectory_t *pdir = alloc_page();
    memset(pdir, 0, sizeof *pdir);

    /* 1MB -> 3GB Page Table */
    pd_entry_t *pd_entry = &pdir->entries[PAGE_DIR_INDEX(0xC0000000)];
    ENTRY_ADD_ATTRIBUTE(*pd_entry, PAGE_STRUCT_ENTRY_PRESENT);
    ENTRY_ADD_ATTRIBUTE(*pd_entry, PAGE_STRUCT_ENTRY_WRITEABLE);
    ENTRY_SET_FRAME(*pd_entry, table1);

    /* 0-4MB -> 0-4MB Page Table */
    pd_entry = &pdir->entries[PAGE_DIR_INDEX(0x0)];
    ENTRY_ADD_ATTRIBUTE(*pd_entry, PAGE_STRUCT_ENTRY_PRESENT);
    ENTRY_ADD_ATTRIBUTE(*pd_entry, PAGE_STRUCT_ENTRY_WRITEABLE);
    ENTRY_SET_FRAME(*pd_entry, table2);

    switch_pd(pdir);

    asm volatile (
        "movl %%cr0, %%eax\n"
        "orl $0b10000000000000000000000000000000, %%eax\n"
        "movl %%eax, %%cr0\n"
        "movl $1f, %%eax\n"
        "jmpl *%%eax\n"
        "1:\n"
    ::: "eax");

    pdir->entries[PAGE_DIR_INDEX(0x0)] = 0;
    switch_pd(pdir);
}