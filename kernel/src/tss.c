#include <kernel/gdt.h>
#include <kernel/tss.h>

tss_entry_t g_tss;

/* from asm/boot.s */
extern uint8_t stack_top[];

void TSS_init() {
  g_tss = (tss_entry_t){0};
  g_tss.link = GDT_DATA_SEGMENT;
  g_tss.esp0 = (uint32_t)stack_top;
  g_tss.ss0 = GDT_DATA_SEGMENT;
}
