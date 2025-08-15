#define _SAL_H_INTERNAL
#define _ATA_H_INTERNAL
#include "../include/sal.h"
#include "../include/fdc.h"
#include "../include/ata.h"
#include "../include/kmm.h"

/*
writes data to offset of sector
assumes:
    drive is already set
    len < sector_size
 */
void SAL_write_unaligned (
    size_t len, uint32_t sector_off, uint32_t lba, 
    const void *buff, storage_device_t *device
){
    static uint8_t temp_buff[4096];

    if (4096 < device->sector_size) return;
    
    device->read_sector(device, temp_buff, lba);

    memcpy(temp_buff + sector_off, buff, len);

    device->write_sector(device,temp_buff, lba);
}

/*
reads data from offset of sector
assumes:
    drive is set
    len < sector_size
*/
void SAL_read_unaligned (
    size_t len, uint32_t sector_off, uint32_t lba, 
    void *buff, storage_device_t *device
){
    static uint8_t temp_buff[4096];

    if (4096 < device->sector_size) return;
    
    device->read_sector(device, temp_buff, lba);

    memcpy(buff, temp_buff + sector_off, len);
}

/**
 * ABSTRACTION LAYER DEFINITIONS
 */

void SAL_read (storage_device_t *device, size_t len, uint32_t offset, void *buff){
    size_t copied_len = 0;
    size_t sector_size = device->sector_size;

    if (offset % sector_size){
        uint32_t sector_off = offset % sector_size;
        size_t    copy_len  = len < sector_size 
                            ? len 
                            : sector_size - sector_off;
        copied_len          += copy_len;

        SAL_read_unaligned(copy_len, sector_off, offset / sector_size, buff, device);
    }

    while (len - copied_len >= sector_size){
        device->read_sector(device, buff + copied_len, (offset + copied_len) / sector_size);
        copied_len += sector_size;
    }

    if (len - copied_len){
        uint32_t curr_off   = offset + copied_len;
        uint32_t sector_off = curr_off % sector_size;
        
        SAL_read_unaligned(len - copied_len, sector_off, curr_off / sector_size, buff + copied_len, device);
    }
}

void SAL_write(storage_device_t *device, size_t len, uint32_t offset, const void *buff){
    size_t copied_len = 0;
    size_t sector_size = device->sector_size;

    /* offset not on sector boundary */
    if (offset % sector_size){
        uint32_t sector_off = offset % sector_size;
        size_t    copy_len  = len < sector_size 
                            ? len 
                            : sector_size - sector_off;
        copied_len          += copy_len;
    
        SAL_write_unaligned(copy_len, sector_off, offset / sector_size, buff, device);
    }

    while (len - copied_len >= sector_size){
        device->write_sector(device, buff + copied_len, (offset + copied_len) / sector_size);
        copied_len += sector_size;
    }

    /* length ends on non-sector boundary */
    if (len - copied_len){
        uint32_t curr_off   = offset + copied_len;
        uint32_t sector_off = curr_off % sector_size;
        
        SAL_write_unaligned(len - copied_len, sector_off, curr_off / sector_size, buff + copied_len, device);
    }
}

static storage_device_t *SAL_device_list;
static uint32_t          SAL_num_devices;

void SAL_add_device(storage_device_t device){
    storage_device_t *new_list = kmalloc(sizeof(storage_device_t) * (SAL_num_devices + 1));
    
    if (SAL_device_list){
        memcpy(new_list, SAL_device_list, sizeof(storage_device_t) * SAL_num_devices);
        kfree(SAL_device_list);
    }

    SAL_device_list = new_list;

    SAL_device_list[SAL_num_devices++] = device;
}

storage_device_t *SAL_get_devices(uint32_t *out_num_devices){
    *out_num_devices = SAL_num_devices;
    return SAL_device_list;
}

/* doesn't do anything but kept for legacy purposes */
void SAL_init() {}
