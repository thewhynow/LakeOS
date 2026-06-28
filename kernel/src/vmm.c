#include <kernel/isr.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <string.h>

pdirectory_t *curr_page_directory;

static void flush_pd() {
/* to shut apple intellisense up */
#ifndef __APPLE__
    asm volatile("movl %%cr3, %%eax\n"
                 "movl %%eax, %%cr3\n" ::
                     : "eax");
#endif
}

static void flush_tlb_entry(vaddr_t vaddr) {
#ifndef __APPLE__
    asm volatile("cli\n"
                 "invlpg (%0)\n"
                 "sti\n" ::"r"(vaddr)
                 : "memory");
#endif
}

void *virt_to_phys(void *vaddr) {
    pd_entry_t *pdir_entry =
        &curr_page_directory->entries[PAGE_DIR_INDEX((uint32_t)vaddr)];

    /* temporarily map the page table containing vaddr to 0xC03FF000 */
    ptable_t *ptable = (ptable_t *)0xC03FF000;
    pt_entry_t *map_ptable_entry =
        &higher_half_page_table.entries[PAGE_TABLE_INDEX(0xC03FF000)];

    ENTRY_SET_FRAME(*map_ptable_entry, PTE_FROM_PDIR(pdir_entry));
    flush_tlb_entry(0xC03FF000);

    pt_entry_t *pte = &ptable->entries[PAGE_TABLE_INDEX((uint32_t)vaddr)];

    if (!ENTRY_GET_ATTRIBUTE(*pte, PAGE_STRUCT_ENTRY_PRESENT))
        return 0;

    /* all usecases of this function only deal with page-aligned addresses  */
    return (void*) ENTRY_GET_ATTRIBUTE(*pte, PAGE_STRUCT_PAGE_FRAME);
}

void switch_pd(pdirectory_t *new_pd) {
    curr_page_directory = new_pd;

#ifndef __APPLE__
    asm volatile("movl %0, %%cr3" ::"r"(new_pd) : "memory");
#endif
}

void *vmm_map_big_page(void *paddr, void *vaddr) {
    pd_entry_t *pde = &curr_page_directory->entries[PAGE_DIR_INDEX((size_t)vaddr)];

    ENTRY_ADD_ATTRIBUTE(*pde, PAGE_STRUCT_ENTRY_PRESENT);
    ENTRY_ADD_ATTRIBUTE(*pde, PAGE_STRUCT_ENTRY_WRITEABLE);
    ENTRY_SET_FRAME(*pde, (size_t)paddr);

    flush_tlb_entry((size_t)vaddr);

    return vaddr;
}

void *vmm_map_page(void *paddr, void *vaddr, bool ring3) {
    pd_entry_t *pdir_entry =
        &curr_page_directory->entries[PAGE_DIR_INDEX((uint32_t)vaddr)];

    /* page table will be mapped to 0xC03FF000 */
    ptable_t *ptable = (ptable_t *)0xC03FF000;

    /* used to map the page table into memory */
    pt_entry_t *map_ptable_entry =
        &higher_half_page_table.entries[PAGE_TABLE_INDEX(0xC03FF000)];

    if (ring3)
        ENTRY_ADD_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_USER_ACCESS);

    /* page table not present, allocate */
    if (!ENTRY_GET_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_PRESENT)) {
        /* map the page table so it can be used */
        ENTRY_SET_FRAME(*map_ptable_entry, alloc_page());
        flush_tlb_entry(0xC03FF000);

        memset(ptable, 0, ENTRIES_PER_STRUCT * sizeof(pt_entry_t));

        /* set new page table as writeable and present */
        ENTRY_ADD_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_PRESENT);
        ENTRY_ADD_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_WRITEABLE);

        ENTRY_SET_FRAME(*pdir_entry, ENTRY_GET_ATTRIBUTE(*map_ptable_entry,
                                                         PAGE_STRUCT_PAGE_FRAME));
    } else {
        /* map the page table so it can be used */
        ENTRY_SET_FRAME(*map_ptable_entry, PTE_FROM_PDIR(pdir_entry));
        flush_tlb_entry(0xC03FF000);
    }

    /* get the page table entry from the page table */
    pt_entry_t *pte = &ptable->entries[PAGE_TABLE_INDEX((uint32_t)vaddr)];

    /* map the page table entry */
    ENTRY_ADD_ATTRIBUTE(*pte, PAGE_STRUCT_ENTRY_PRESENT);
    ENTRY_ADD_ATTRIBUTE(*pte, PAGE_STRUCT_ENTRY_WRITEABLE);
    if (ring3)
        ENTRY_ADD_ATTRIBUTE(*pte, PAGE_STRUCT_ENTRY_USER_ACCESS);
    ENTRY_SET_FRAME(*pte, paddr);

    flush_tlb_entry((vaddr_t)vaddr);

    return vaddr;
}

void vmm_unmap_page(void *vaddr) {
    pd_entry_t *pdir_entry =
        &curr_page_directory->entries[PAGE_DIR_INDEX((size_t)vaddr)];

    /* page table will be mapped to 0xC03FF000 */
    ptable_t *ptable = (ptable_t *)0xC03FF000;

    /* used to map the page table into memory */
    pt_entry_t *map_ptable_entry =
        &higher_half_page_table.entries[PAGE_TABLE_INDEX(0xC03FF000)];

    /* map the page table */
    ENTRY_SET_FRAME(*map_ptable_entry, PTE_FROM_PDIR(pdir_entry));
    flush_tlb_entry(0xC03FF000);

    /* get the page table entry from the page table */
    pt_entry_t *pte = &ptable->entries[PAGE_TABLE_INDEX((size_t)vaddr)];

    ENTRY_DEL_ATTRIBUTE(*pte, PAGE_STRUCT_ENTRY_PRESENT);
}

void *valloc_page(void *vaddr) {
    void *page = alloc_page();
    return vmm_map_page(page, vaddr ? vaddr : page, false);
}

void vfree_page(void *vaddr){
    free_page(virt_to_phys(vaddr));
    vmm_unmap_page(vaddr);
}

void *valloc_big_page(void *vaddr) {
    void *page = alloc_pages(0x400);

    return vmm_map_big_page(page, vaddr ? vaddr : page);
}



void VMM_init() {

    extern uint8_t *bitmap;
    extern size_t bitmap_len;

    size_t bitmap_size = bitmap_len / 8 + !!(bitmap_len % 8);
    size_t num_pages = bitmap_size / 4096 + !!(bitmap_size % 4096);

    if ((size_t)bitmap % 4096) {
        printf("Bitmap not page-aligned! im too lazy to fix this 'bug'\n");
        for (;;)
            ;
    }

    curr_page_directory = &kernel_page_directory;

    /* map kernel heap into memory */
    for (size_t i = 0; i < num_pages; ++i)
        vmm_map_page(bitmap, bitmap + 0xC0000000 + i * 4096, false);

    /* map one userspace page into memory (TEMP) */
    void *user_stack_bottom = (void *)(0xBFFFF000 - 4096);
    vmm_map_page(alloc_page(), user_stack_bottom, true);

    /* just invalidate the identity-mapping page directory entry */
    curr_page_directory->entries[PAGE_DIR_INDEX(0x0)] = 0;

    bitmap += 0xC0000000;

    flush_pd();
}

void ISR_page_flt_handler(registers_t *regs) {
    uint64_t fault_addr;

#ifndef __APPLE__
    asm volatile("movl %%cr2, %0" : "=r"(fault_addr)::);
#endif

    /* page is in kernel heap */
    if (0xD0000000 <= fault_addr && fault_addr < 0xE0000000)
        valloc_page((void *)fault_addr);
    else {
        printf("FATAL ERROR: PAGE FAULT AT ADDRESS %p\n"
               "SYSTEM HALTING, MANUAL SHIT OFF REQUIRED\n",
               (void *)fault_addr);
        for (;;)
            ;
    }
}
