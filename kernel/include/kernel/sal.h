/**
 * @file sal.h
 * @brief storage abstraction layer, the common interface the ata
 *  and floppy drivers sit behind, and what filesystems read through
 */

#ifndef _SAL_H
#define _SAL_H

#include "../../libc/include/types.h"

typedef struct storage_device_t storage_device_t;

/**
 * reads one sector
 *  device: the device being read
 *  buff:   where the sector goes, sector_size bytes
 *  lba:    sector number
 */
typedef void    (*f_ReadSector )(storage_device_t *device, void *buff, uint32_t lba);

/**
 * writes one sector
 *  device: the device being written to
 *  buff:   the sector to write, sector_size bytes
 *  lba:    sector number
 */
typedef void    (*f_WriteSector)(storage_device_t *device, const void *buff, uint32_t lba);

/**
 * a disk the kernel can read and write
 */
struct storage_device_t {
    /** the last sector that can be addressed */
    uint32_t      maxlba;
    /** bytes per sector */
    size_t        sector_size;
    /** human readable name */
    char         *name;
    /** which drive the driver should use, the driver decides what it means */
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

/**
 * @brief writes len bytes at a byte offset on the device, does
 *  a read-modify-write on the sectors that are only partly covered
 * @param device the device to write to
 * @param len how many bytes
 * @param offset byte offset from the start of the device
 * @param buff the data
 */
void SAL_write(storage_device_t *device, size_t len, uint32_t offset, void *buff);

/**
 * @brief reads len bytes from a byte offset on the device
 * @param device the device to read from
 * @param len how many bytes
 * @param offset byte offset from the start of the device
 * @param buff where the data goes
 */
void SAL_read (storage_device_t *device, size_t len, uint32_t offset, void *buff);

/**
 * @brief registers a device, the struct is copied
 * @param device the device to add
 */
void SAL_add_device(storage_device_t device);

/**
 * @brief gets the registered devices
 * @param out_num_devices set to how many there are
 * @return the array of devices
 */
storage_device_t *SAL_get_devices(uint32_t *out_num_devices);

/**
 * doesn't do anything but kept for legacy purposes
 */
void SAL_init();

#endif
