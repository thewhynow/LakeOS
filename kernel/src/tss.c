#include <kernel/gdt.h>
#include <kernel/tss.h>

tss_entry_t g_tss;

/* from asm/boot.s */
extern void *stack_bottom;

void TSS_init() {
  g_tss = (tss_entry_t){0};
  g_tss.link = GDT_DATA_SEGMENT;
  g_tss.esp0 = (uint32_t)stack_bottom;
}
