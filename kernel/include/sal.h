#ifndef _SAL_H
#define _SAL_H

#include "../../libc/include/types.h"

typedef struct {
    uint32_t maxlba;
    char    *name;
#define STORAGE_TYPE_UNAVAIL 0
#define STORAGE_TYPE_FLOPPY  1
#define STORAGE_TYPE_ATA     2
    uint8_t type;
    uint8_t drive_num;
} storage_device_t;

void SAL_add_device(const storage_device_t *device);
void SAL_write(storage_device_t *device, size_t len, uint32_t offset, const void *buff);
void SAL_read(storage_device_t *device, size_t len, uint32_t offset, void *buff);
storage_device_t *SAL_get_devices(uint32_t *out_num_devices);
void SAL_init();

#endif