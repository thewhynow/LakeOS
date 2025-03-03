#ifndef _GDT_H
#define _GDT_H

#include "../../../libc/include/types.h"

void GDT_init();

#define GDT_CODE_SEGMENT 0x08
#define GDT_DATA_SEGMENT 0x10

#endif