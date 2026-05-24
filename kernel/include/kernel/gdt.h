#ifndef _GDT_H
#define _GDT_H

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

void GDT_init();

typedef struct {
  uint16_t limit_low;      /* limit: bits 0-15 */
  uint16_t base_low;       /* base:  bits 0-15 */
  uint8_t base_middle;     /* base:  bits 16-23 */
  uint8_t type;            /* access */
  uint8_t flags__limit_hi; /* limit: bits 16-19 | flags */
  uint8_t base_high;       /* base: bits 24-31 */
} __attribute__((__packed__)) GDT_entry_t;

typedef struct {
  uint16_t limit;     /* sizeof(GDT_t) - 1 */
  GDT_entry_t *gdt_p; /* &GDT */
} __attribute__((__packed__)) GDT_descriptor_t;

typedef enum {
  GDT_ACCESS_CODE_READABLE = 0x02,
  GDT_ACCESS_DATA_WRITEABLE = 0x02,

  GDT_ACCESS_CODE_CONFORMING = 0x04,
  GDT_ACCESS_DATA_DIRECTION_NORMAL = 0x00,
  GDT_ACCESS_DATA_DIRECTION_DOWN = 0x04,

  GDT_ACCESS_DATA_SEGMENT = 0x10,
  GDT_ACCESS_CODE_SEGMENT = 0x18,

  GDT_ACCESS_DESCRIPTOR_TSS = 0x00,

  GDT_TYPE_TSS_AVAILABLE = 0x09,
  GDT_TYPE_TSS_BUSY      = 0x08,

  GDT_ACCESS_RING0 = 0x00,
  GDT_ACCESS_RING1 = 0x20,
  GDT_ACCESS_RING2 = 0x40,
  GDT_ACCESS_RING3 = 0x60,

  GDT_ACCESS_PRESENT = 0x80,

} GDT_ACCESS_FLAGS;

typedef enum {
  GDT_FLAG_64BIT = 0x20,
  GDT_FLAG_32BIT = 0x40,
  GDT_FLAG_16BIT = 0x00,

  GDT_FLAG_GRANULARITY_1B = 0x00,
  GDT_FLAG_GRANULARITY_4K = 0x80,
} GDT_FLAGS;

#ifdef __cplusplus
}
#endif


#define GDT_CODE_SEGMENT 0x08
#define GDT_DATA_SEGMENT 0x10

#endif
