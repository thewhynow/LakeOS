#ifndef _SAL_H
#define _SAL_H

#include "../../libc/include/types.h"

typedef struct storage_device_t storage_device_t;

typedef void    (*f_ReadSector )(storage_device_t *device, void *buff, uint32_t lba);
typedef void    (*f_WriteSector)(storage_device_t *device, const void *buff, uint32_t lba);

struct storage_device_t {
    uint32_t      maxlba;
    size_t        sector_size;
    char         *name;
    uint8_t       drive_num;
    f_ReadSector  read_sector;
    f_WriteSector write_sector;
    /* can be used by other applications for extra data */
    void         *extra;
};


/**
 * these are utility functions for the f_*Sector storage_device member 
 *  functions to easily read/write at arbitrary offsets and lengths 
 *  within sectors of different sizes
 * 
 * assumes that the drive number is already set for relevant devices
 */

void SAL_write(storage_device_t *device, size_t len, uint32_t offset, void *buff);
void SAL_read (storage_device_t *device, size_t len, uint32_t offset, void *buff);

void SAL_add_device(storage_device_t device);
storage_device_t *SAL_get_devices(uint32_t *out_num_devices);

void SAL_init();

#endif
