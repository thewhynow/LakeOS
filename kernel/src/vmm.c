#include <kernel/isr.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/umm.h>
#include <kernel/exe.h>
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

    if (!ENTRY_GET_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_PRESENT))
        return 0;

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

void *vmm_map_page(void *paddr, void *vaddr, bool write, bool ring3) {
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
    if (write)
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

    /* check if the entire page table is empty and free if so */
    for (int i = 0; i < 1024; ++i){
        pt_entry_t *pte = &ptable->entries[i];
        if (ENTRY_GET_ATTRIBUTE(*pte, PAGE_STRUCT_ENTRY_PRESENT))
            return;
    }

    free_page((void*) PTE_FROM_PDIR(pdir_entry));
    ENTRY_DEL_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_PRESENT);
}

void *valloc_page(void *vaddr) {
    void *page = alloc_page();
    return vmm_map_page(page, vaddr ? vaddr : page, true, false);
}

void vfree_page(void *vaddr){
    void *paddr = virt_to_phys(vaddr);

    if (paddr){
        free_page(paddr);
        vmm_unmap_page(vaddr);
    }
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
        vmm_map_page(bitmap, bitmap + 0xC0000000 + i * 4096, true, false);

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
        valloc_page((void *)fault_addr); else 
    if (fault_addr < 0xC0000000)
        umm_page_flt_handler((void*) fault_addr);
    else {
        printf("FATAL ERROR: PAGE FAULT AT ADDRESS %p\n"
               "SYSTEM HALTING, MANUAL SHIT OFF REQUIRED\n",
               (void *)fault_addr);
        for (;;)
            ;
    }
}

void copy_ptes(ptable_t *new_pt, ptable_t *old_pt, uint32_t start, uint32_t end){
    /* page table will be mapped to 0xC03FF000 */
    ptable_t *ptable = (ptable_t *)0xC03FF000;

    /* used to map the page table into memory */
    pt_entry_t *map_ptable_entry =
        &higher_half_page_table.entries[PAGE_TABLE_INDEX(0xC03FF000)];

    ENTRY_SET_FRAME(*map_ptable_entry, old_pt);
    flush_tlb_entry(0xC03FF000);

    static ptable_t temp_ptable;
    memcpy(
        temp_ptable.entries + start, 
        old_pt->entries + start, 
        (end - start) * sizeof(pt_entry_t)
    );

    ENTRY_SET_FRAME(*map_ptable_entry, new_pt);
    flush_tlb_entry(0xC03FF000);
    
    memcpy(
        new_pt->entries + start,
        temp_ptable.entries + start,
        (end - start) * sizeof(pt_entry_t)
    );
}

void new_page_table(pdirectory_t *pd, uint32_t pdi, bool ring3){
    void *new_pt = alloc_page();
    ENTRY_SET_FRAME(pd->entries[pdi], new_pt);
    ENTRY_ADD_ATTRIBUTE(pd->entries[pdi], PAGE_STRUCT_ENTRY_PRESENT);
    ENTRY_ADD_ATTRIBUTE(pd->entries[pdi], PAGE_STRUCT_ENTRY_WRITEABLE);

    if (ring3)
        ENTRY_ADD_ATTRIBUTE(pd->entries[pdi], PAGE_STRUCT_ENTRY_USER_ACCESS);

    /* page table will be mapped to 0xC03FF000 */
    ptable_t *ptable = (ptable_t *)0xC03FF000;

    /* used to map the page table into memory */
    pt_entry_t *map_ptable_entry =
        &higher_half_page_table.entries[PAGE_TABLE_INDEX(0xC03FF000)];

    ENTRY_SET_FRAME(*map_ptable_entry, new_pt);
    flush_tlb_entry(0xC03FF000);
    memset(ptable, 0, sizeof(ptable_t));
}

pdirectory_t *new_page_directory(){
    pdirectory_t *new_pd = valloc_page(NULL);
    memset(new_pd, 0, sizeof *new_pd);

    /* exe.c */
    extern t_Process *process_stack;

    /* copy shared user mappings */
    for (umm_block_t *block = process_stack->blocks; block; block = block->next){
        if (!(block->flags | UMM_BLOCK_FLAG_SHARED))
            continue;

        uint32_t pti = PAGE_TABLE_INDEX((uint32_t)block->start),
                 pdi = PAGE_DIR_INDEX((uint32_t)block->start),
                 pti_end = PAGE_TABLE_INDEX((uint32_t)block->start + block->len),
                 pdi_end = PAGE_DIR_INDEX((uint32_t)block->start + block->len)
        ;

        if (pti % 1024){
            pd_entry_t *pde =
                curr_page_directory->entries + pdi;

            if (!ENTRY_GET_ATTRIBUTE(new_pd->entries[pdi], PAGE_STRUCT_ENTRY_PRESENT))
                new_page_table(new_pd, pdi, true);

            copy_ptes(
                (void*) ENTRY_GET_ATTRIBUTE(new_pd->entries[pdi], PAGE_STRUCT_PAGE_FRAME), 
                (void*) PTE_FROM_PDIR(pde),
                pti, pdi_end > pdi ? 1024 : pti_end
            );

            pti = pdi_end > pdi ? pti + pti % 1024 : pti_end;
            ++pdi;
        }

        uint32_t block_end = (uint32_t) block->start + block->len;

        for (; pdi < PAGE_DIR_INDEX(block_end); ++pdi, pti += 1024){
            pd_entry_t *pde =
               curr_page_directory->entries + pdi;

            if (!ENTRY_GET_ATTRIBUTE(new_pd->entries[pdi], PAGE_STRUCT_ENTRY_PRESENT))
                new_page_table(new_pd, pdi, true);

            copy_ptes(
                (void*) ENTRY_GET_ATTRIBUTE(new_pd->entries[pdi], PAGE_STRUCT_PAGE_FRAME), 
                (void*) PTE_FROM_PDIR(pde),
                0, 1024
            );
        }

        if (pti < pti_end){
            pd_entry_t *pde =
               curr_page_directory->entries + pdi;

            if (!ENTRY_GET_ATTRIBUTE(new_pd->entries[pdi], PAGE_STRUCT_ENTRY_PRESENT))
                new_page_table(new_pd, pdi, true);

            copy_ptes(
                (void*) ENTRY_GET_ATTRIBUTE(new_pd->entries[pdi], PAGE_STRUCT_PAGE_FRAME), 
                (void*) PTE_FROM_PDIR(pde),
                0, pti_end
            );
        }
    }

    /* copy kernel mappings */
    for (int i = 768; i < 1024; ++i)
        new_pd->entries[i] = kernel_page_directory.entries[i];

    vmm_unmap_page(new_pd);

    return new_pd;
}

void vmm_new_permissions(void *vaddr, bool write, bool ring3){
    pd_entry_t *pdir_entry =
        &curr_page_directory->entries[PAGE_DIR_INDEX((uint32_t)vaddr)];

    /* page table will be mapped to 0xC03FF000 */
    ptable_t *ptable = (ptable_t *)0xC03FF000;

    /* used to map the page table into memory */
    pt_entry_t *map_ptable_entry =
        &higher_half_page_table.entries[PAGE_TABLE_INDEX(0xC03FF000)];

    ENTRY_SET_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_USER_ACCESS, ring3);

    /* page table not present */
    if (!ENTRY_GET_ATTRIBUTE(*pdir_entry, PAGE_STRUCT_ENTRY_PRESENT))
        return;

    /* map the page table so it can be used */
    ENTRY_SET_FRAME(*map_ptable_entry, PTE_FROM_PDIR(pdir_entry));
    flush_tlb_entry(0xC03FF000);

    /* get the page table entry from the page table */
    pt_entry_t *pte = &ptable->entries[PAGE_TABLE_INDEX((uint32_t)vaddr)];

    ENTRY_SET_ATTRIBUTE(*pte, PAGE_STRUCT_ENTRY_WRITEABLE, write);
    ENTRY_SET_ATTRIBUTE(*pte, PAGE_STRUCT_ENTRY_USER_ACCESS, ring3);

    flush_tlb_entry((vaddr_t)vaddr);
}
