#include <kernel/gdt.h>
#include <kernel/tss.h>

#define GDT_ENTRY(base, limit, access, flags)                                  \
    (GDT_entry_t){limit & 0xFFFF,                                                \
                  base & 0xFFFF,                                                 \
                  (base >> 16) & 0xFF,                                           \
                  access,                                                        \
                  ((limit >> 16) & 0xF) | (flags & 0xF0),                        \
                  (base >> 24) & 0xFF}

void GDT_load(GDT_descriptor_t *descriptor, uint16_t code_seg,
              uint16_t data_seg);

GDT_entry_t GDT_entries[6] = {
    /* Null descriptor */
    GDT_ENTRY(0, 0, 0, 0),

    /* Kernel 32-bit code segment */
    GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE_SEGMENT |
                  GDT_ACCESS_CODE_READABLE,
              (GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K)),
    /* Kernel 32-bit data segment */
    GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA_SEGMENT |
                  GDT_ACCESS_DATA_WRITEABLE,
              (GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K)),
    /* Userspace 32-bit code segment */
    GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODE_SEGMENT |
                  GDT_ACCESS_CODE_READABLE,
              (GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K)),
    /* Userspace 32-bit data segment */
    GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DATA_SEGMENT |
                  GDT_ACCESS_DATA_WRITEABLE,
              (GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K)),
    /* TSS ... */
};

GDT_descriptor_t GDT_descriptor = (GDT_descriptor_t){
    .limit = sizeof(GDT_entries) - 1, .gdt_p = (void *)&GDT_entries};

void GDT_init() {
    GDT_entries[5] =
        GDT_ENTRY((uint32_t)&g_tss, sizeof(g_tss) - 1,
                  GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 |
                      GDT_ACCESS_DESCRIPTOR_TSS | GDT_TYPE_TSS_AVAILABLE,
                  (GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_1B));

    GDT_load(&GDT_descriptor, GDT_CODE_SEGMENT, GDT_DATA_SEGMENT);
    flush_tss();
}
