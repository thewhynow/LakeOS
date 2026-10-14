/**
 * @file tss.h
 * @brief task state segment, only here so the cpu knows which stack to
 *  use when an interrupt comes in from ring 3
 */

#include <kernel/gdt.h>

#ifndef _TSS_H
#define _TSS_H

/**
 * the hardware layout of a tss
 *  the only things that get set are link, esp0 and ss0,
 *  everything else is unused but i'll do it anyway
 */
typedef struct tss_entry_t {
    /**
     * the previous segment
     *  in our case will be kernel GDT data segment
     */
    uint16_t link;

    /** reserved */
    uint16_t _reserved_0;

    /**
     * previous esp (in ring 0)
     *  the stack an interrupt from ring 3 lands on, so exe.c
     *  points it at the top of the running process's kernel stack
     */
    uint32_t esp0;

    /** the stack segment (typically ring 0) */
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

/**
 * @brief zeroes g_tss and points the kernel stack (esp0, ss0) at the
 *  boot stack
 */
void TSS_init();

/**
 * @brief loads the task register with the tss entry from the gdt (ltr),
 *  GDT_init calls this
 */
extern void flush_tss();

/**
 * the one tss, the gdt has an entry for it
 */
extern tss_entry_t g_tss;
#endif
