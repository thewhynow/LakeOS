#define __UMM_H_INTERNAL
#include <kernel/umm.h>
#include <kernel/exe.h>

#define ABS(x)\
    ((x) < 0 ? (-(x)) : (x))


static void *const user_memory_start = (void*) 0x00000000,
            *const user_memory_end   = (void*) 0xC0000000
;

void umap_pages(t_Process *process, void *vaddr, size_t len, int prot, int flags){
    umm_block_t *new_block = kmalloc(sizeof(umm_block_t));
    *new_block = (umm_block_t){
        .start = vaddr,
        .len   = len,
        .prot  = prot,
        .flags = flags,
    };

    if (process->blocks && vaddr > process->blocks->start){
        umm_block_t *block = process->blocks;
        while (
            block->next &&
            block->next->start + block->next->len <= vaddr
        )
            block = block->next;

        new_block->next = block->next;
        block->next = new_block;
    }
    else {
        if (process->blocks && vaddr < process->blocks->start)
            new_block->next = process->blocks;
        else
            new_block->next = NULL;

        process->blocks = new_block;
    }

    for (void *i = vaddr; i < vaddr + len; i += 0x1000){
        void *paddr = alloc_page();
        vmm_map_page(
            paddr, i, !!(prot & UMM_BLOCK_PROT_WRITE), true
        );
    }
}

void umm_unmap_pages(t_Process *process, void *vaddr){
    for (
        umm_block_t *iter = process->blocks; 
        iter->next; 
        iter = iter->next
    )
        if (iter->next->start == vaddr){
            umm_block_t *old_block = iter->next;
            for (
                void *i = vaddr; i < vaddr + old_block->len; i += 0x1000
            )
                vmm_unmap_page(i);

            iter->next = iter->next->next;
            kfree(old_block);
        }
}

void *ualloc_pages(
    t_Process *process, size_t len, int prot, int flags
){
    umm_block_t *iter = process->blocks;
    void *first_fit_start;

    if (!process->blocks){
        first_fit_start = user_memory_start;
        goto end;
    }

    if (iter->start - user_memory_start >= len)
        first_fit_start = user_memory_start;
    else {
        for (; iter->next; iter = iter->next)
            if (iter->next->start - (iter->start + iter->len) >= len){
                first_fit_start = iter->start + iter->len;
                goto end;
            }

        if (user_memory_end - (iter->start + iter->len) >= len)
            first_fit_start = iter->start + iter->len;
        else
            return NULL;
    }
end:
    umap_pages(process, first_fit_start, len, prot, flags);
    return first_fit_start;
}

void umm_page_flt_handler(void *fault_addr){
    /* exe.c */
    extern t_Process *process_stack;

    if (fault_addr < process_stack->blocks->start){
        if (
            process_stack->blocks->start - user_memory_start >= 0x1000 * 2 &&
            process_stack->blocks->flags & UMM_BLOCK_FLAG_GROWSDOWN
        ){
            process_stack->blocks->start -= 0x1000;
            process_stack->blocks->len   += 0x1000;
            vmm_map_page(
                alloc_page(),
                process_stack->blocks->start,
                !!(process_stack->blocks->prot & UMM_BLOCK_PROT_WRITE),
                true
            );
            return;
        }
        else
            goto fail;
    }

    for (
        umm_block_t *iter = process_stack->blocks;
        iter->next;
        iter = iter->next
    ){
        if (
            iter->start + iter->len <= fault_addr &&
            fault_addr < iter->next->start
        ){
                
            if (
                iter->next->start - (iter->start + iter->len) >= 0x1000 * 2 &&
                iter->next->flags & UMM_BLOCK_FLAG_GROWSDOWN
            ){
                iter->next->start -= 0x1000;
                iter->next->len   += 0x1000;
                vmm_map_page(
                    alloc_page(), iter->next->start, 
                    !!(iter->next->prot & UMM_BLOCK_PROT_WRITE), 
                    true
                );
                return;
            }
            else
                goto fail;
        }
    }

fail:
    printf("\nFUCKASS USER PAGEFAULT\n");
    for (;;);
}
