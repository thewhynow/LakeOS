#include <kernel/vfs.h>
#include <kernel/kmm.h>
#include <string.h>
#include <kernel/elf.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>

void *elf_load(const void *file_buff){
    elf32_header_t *header = (elf32_header_t*) file_buff;

    if (!elf_validate_magic(header))
        return NULL;

    if (!elf_validate_supported(header))
        return NULL;

    elf_load_segments(header);
    
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

void elf_load_segments(elf32_header_t *header){
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

            /* map all of the pages in */
            for (elf32_addr page = page_begin; page < bss_end; page += 0x1000){
                void *phys_page = alloc_page(); 
                vmm_map_page(phys_page, (void*) page, true); 
            }

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

elf32_s_header_t *elf_null_s_header(elf32_header_t *header){
    return (void*)header + header->s_header_off;
}

elf32_s_header_t *elf_s_header(elf32_header_t *header, size_t idx){
    return elf_null_s_header(header) + idx;
}

const char *elf_string_table(elf32_header_t *header){
    elf32_s_header_t *str_table_s_header =
        elf_s_header(header, header->s_header_str_index);
    return (void*) header + str_table_s_header->offset;
}

const char *elf_lookup_string(elf32_header_t *header, size_t offset){
    return elf_string_table(header) + offset;
}

int elf_symbol_value(elf32_header_t *header, int table, uint32_t index){
    elf32_s_header_t *s_table = elf_s_header(header, table);

    elf32_sym_table_entry_t *s_entry =
        (elf32_sym_table_entry_t*)((void*)header + s_table->offset) + index;

    /* external symbol */
    if (s_entry->s_header_index == ELF32_S_HEADER_SPECIAL_UNDEFINED){
        /* now theoretically this means the symbol is defined externally ... */
        /* but i am a lazy bum and not writing a damn linker */
        /* so here is what ya'll get */
        if (ELF32_SYM_TABLE_ENTRY_BIND(s_entry->info) & ELF32_SYM_TABLE_ENTRY_BIND_WEAK)
            return 0;
            /* if symbol is defined as weak the value is 0 */
    }

    /* absolute value */
    if (s_entry->s_header_index == ELF32_S_HEADER_SPECIAL_ABSOLUTES)
        return s_entry->value;

    /* else internally defined symbol */
    elf32_s_header_t *target = elf_s_header(header, s_entry->s_header_index);
    return (int)((void*)header + target->offset + s_entry->value);
}

void elf_allocate_sections(elf32_header_t *header){
    elf32_s_header_t *null_header = elf_null_s_header(header);

    for (int i = 0; i < header->s_header_num_entries; ++i){
        elf32_s_header_t *section = null_header + i;

        /* section is BSS */
        if (section->type == ELF32_S_HEADER_TYPE_NOBITS){
            if (section->size == 0)
                continue;

            /* section occupies memory during execution */
            if (section->flags & ELF32_S_HEADER_FLAG_ALLOC){
                void *block = kmalloc(section->size);
                memset(block, 0, section->size);

                /* assign offset */
                section->offset = block - (void*)header;
            }
        }
    }
}

void elf_load_relocations(elf32_header_t *header){
    elf32_s_header_t *null_header = elf_null_s_header(header);

    for (int i = 0; i < header->s_header_num_entries; ++i){
        elf32_s_header_t *section = null_header + i;

        /* section is relocation */
        if (section->type == ELF32_S_HEADER_TYPE_RELOCATION_IMPLICIT){
            for (int j = 0; j < section->size / section->entry_size; ++j){
                elf32_relocation_t *relocation_table = 
                    (elf32_relocation_t*)((void*)header + section->offset) + j;

                elf_relocate(header, relocation_table, section);
            }
        }
    }
}

void elf_relocate(elf32_header_t *header, elf32_relocation_t *relocation, elf32_s_header_t *table){
    elf32_s_header_t *target = elf_s_header(header, table->info);

    void *address = (void*)header + target->offset;


}



