#include <kernel/vfs.h>
#include <kernel/kmm.h>
#include <string.h>
#include <kernel/elf.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/umm.h>

void *elf_load(t_Process *process, const void *file_buff){
    elf32_header_t *header = (elf32_header_t*) file_buff;

    if (!elf_validate_magic(header))
        return NULL;

    if (!elf_validate_supported(header))
        return NULL;

    elf_load_segments(process, header);
    
    return (void*) header->entry;
}

bool elf_validate_magic(elf32_header_t *header){
    static const char magic[4] = {
        0x7F, 'E', 'L', 'F'
    };

    return !memcmp(header->identification, magic, 4);
}

bool elf_validate_supported(elf32_header_t *header){
    if (
        header->identification[ELF32_IDENTIFICATION_INDEX_CLASS]
        != ELF32_IDENTIFICATION_CLASS_32
    )
        return false;

    if (
        header->identification[ELF32_IDENTIFICATION_INDEX_DATA]
        != ELF32_IDENTIFICATION_DATA_2LSB
    )
        return false;

    if (header->machine != ELF32_MACHINE_386)
        return false;

    if (
        header->identification[ELF32_IDENTIFICATION_INDEX_VERSION]
        != ELF32_VERSION_CURR
    )
        return false;

    if (
        header->type != ELF32_TYPE_REL
        && header->type != ELF32_TYPE_EXEC
    )
        return false;

    return true;
}

elf32_p_header_t *elf_p_header(elf32_header_t *header, int i){
    elf32_p_header_t *base_p_header = (void*) header + header->p_header_off;
    return base_p_header + i;
}

void elf_load_segments(t_Process *process, elf32_header_t *header){
    if (header->type != ELF32_TYPE_EXEC)
        return;

    for (int i = 0; i < header->p_header_num_entries; ++i){
        elf32_p_header_t *p_header =  elf_p_header(header, i);

        if (p_header->type == ELF32_P_HEADER_TYPE_LOADABLE_SEG){
            /* round down to page boundary */
            elf32_addr page_begin = p_header->vaddr - (p_header->vaddr % 0x1000);

            elf32_addr file_end = p_header->vaddr + p_header->file_size,
                       mem_end  = page_begin + p_header->mem_size,
                       /* round up to page boundary */
                       bss_end  = ((mem_end + 0x1000 - 1) / 0x1000) * 0x1000
            ;

            umap_pages(
                process, 
                (void*) page_begin, bss_end - page_begin, 
                UMM_BLOCK_PROT_EXEC | UMM_BLOCK_PROT_READ | UMM_BLOCK_PROT_WRITE, 
                UMM_BLOCK_FLAG_PRIVATE | UMM_BLOCK_FLAG_ANONYMOUS | UMM_BLOCK_FLAG_FIXED
            );

            memcpy(
                (void*) p_header->vaddr, 
                (void*) header + p_header->offset, 
                p_header->file_size
            );
            /* zero trailing bss */
            memset((void*) file_end, 0, bss_end - file_end);
        }
    }
}
