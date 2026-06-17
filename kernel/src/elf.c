#include <kernel/vfs.h>
#include <kernel/kmm.h>
#include <string.h>
#include <kernel/elf.h>

void elf_load(const char *path){
    char *file = file_load(path);
}

char *file_load(const char *path){
    void *file = VFS_open(path, VFS_FILE_READ);
    size_t size = VFS_size(file);
    void *buff = kmalloc(size);
    VFS_read(file, buff, size);
    VFS_close(file);
    return buff;
}

bool elf_validate_magic(elf32_header_t *header){
    static const char magic[4] = {
        0x7F, 'E', 'L', 'F'
    };

    if (memcmp(header->identification, magic, 4))
        return false;
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



