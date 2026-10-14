/**
 * @file gdt.h
 * @brief global descriptor table. flat segments, layout is:
 *  null, kernel code (0x08), kernel data (0x10), user code (0x18),
 *  user data (0x20), tss (0x28)
 */

#ifndef _GDT_H
#define _GDT_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief fills in the tss entry, loads the GDT, reloads the segment
 *  registers and the task register
 */
void GDT_init();

/**
 * one descriptor of the GDT
 */
typedef struct {
    uint16_t limit_low;      /**< limit: bits 0-15 */
    uint16_t base_low;       /**< base:  bits 0-15 */
    uint8_t base_middle;     /**< base:  bits 16-23 */
    uint8_t type;            /**< access, see GDT_ACCESS_FLAGS */
    uint8_t flags__limit_hi; /**< limit: bits 16-19 | flags, see GDT_FLAGS */
    uint8_t base_high;       /**< base: bits 24-31 */
} __attribute__((__packed__)) GDT_entry_t;

/**
 * what lgdt wants
 */
typedef struct {
    uint16_t limit;     /**< sizeof(GDT_t) - 1 */
    GDT_entry_t *gdt_p; /**< &GDT */
} __attribute__((__packed__)) GDT_descriptor_t;

/**
 * the access byte of a descriptor, or them together
 */
typedef enum {
    /** (code) code can be read, not just executed */
    GDT_ACCESS_CODE_READABLE = 0x02,
    /** (data) data can be written, not just read */
    GDT_ACCESS_DATA_WRITEABLE = 0x02,

    GDT_ACCESS_CODE_CONFORMING = 0x04,
    GDT_ACCESS_DATA_DIRECTION_NORMAL = 0x00,
    GDT_ACCESS_DATA_DIRECTION_DOWN = 0x04,

    /** descriptor is a data segment */
    GDT_ACCESS_DATA_SEGMENT = 0x10,
    /** descriptor is a code segment */
    GDT_ACCESS_CODE_SEGMENT = 0x18,

    /** descriptor is a system segment (the tss), not code or data */
    GDT_ACCESS_DESCRIPTOR_TSS = 0x00,

    /** type of the tss descriptor, not busy */
    GDT_TYPE_TSS_AVAILABLE = 0x09,
    GDT_TYPE_TSS_BUSY      = 0x08,

    /** privilege level of the segment */
    GDT_ACCESS_RING0 = 0x00,
    GDT_ACCESS_RING1 = 0x20,
    GDT_ACCESS_RING2 = 0x40,
    GDT_ACCESS_RING3 = 0x60,

    /** segment is valid */
    GDT_ACCESS_PRESENT = 0x80,

} GDT_ACCESS_FLAGS;

/**
 * the flags nibble of a descriptor, or them together
 */
typedef enum {
    GDT_FLAG_64BIT = 0x20,
    /** 32 bit protected mode segment */
    GDT_FLAG_32BIT = 0x40,
    GDT_FLAG_16BIT = 0x00,

    /** limit is in bytes */
    GDT_FLAG_GRANULARITY_1B = 0x00,
    /** limit is in 4KB pages */
    GDT_FLAG_GRANULARITY_4K = 0x80,
} GDT_FLAGS;

#ifdef __cplusplus
}
#endif


/* selectors of the kernel segments, user ones are 0x18 and 0x20 (| 3 for ring 3) */
#define GDT_CODE_SEGMENT 0x08
#define GDT_DATA_SEGMENT 0x10

#endif
