#include <kernel/gdt.h>

#ifndef _TSS_H
#define _TSS_H

typedef struct tss_entry_t {
  /* the previous segment */
  /* in our case will be kernel GDT data segment */
  uint16_t link;

  /* reserved */
  uint16_t _reserved_0;

  /* previous esp (in ring 0) */
  uint32_t esp0;

  /**
   * everything below here is
   * unused but i'll do it
   * anyway
   */

  /* the stack segment (typically ring 0) */
  uint16_t ss0;
  uint16_t _reserved_1;
  uint32_t esp1;
  uint16_t ss1;
  uint16_t _reserved_2;
  uint32_t esp2;
  uint16_t ss2;
  uint16_t _reserved_3;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;
  uint16_t es;
  uint16_t _reserved_4;
  uint16_t cs;
  uint16_t _reserved_5;
  uint16_t ss;
  uint16_t _reserved_6;
  uint16_t ds;
  uint16_t _reserved_7;
  uint16_t fs;
  uint16_t _reserved_8;
  uint16_t gs;
  uint16_t _reserved_9;
  uint16_t ldtr;
  uint32_t _reserved_10;
  uint16_t iopb;
  uint32_t ssp;
} tss_entry_t;

void TSS_init();
extern void flush_tss();

extern tss_entry_t g_tss;
#endif
