#define _SAL_H_INTERNAL
#define _ATA_H_INTERNAL
#include "../include/sal.h"
#include "../include/fdc.h"
#include "../include/ata.h"
#include "../include/pmm.h"
#include "../include/vmm.h"

/* 
writes data to offset of sector
assumes:
    drive is already set
    len < sector_size
 */
void SAL_write_unaligned (
    size_t len, uint32_t sector_off, uint32_t sector_num, 
    const void *buff, size_t sector_size, 
    void(*fn_read_sector) (void *buff, uint32_t lba),
    void(*fn_write_sector)(const void *buff, uint32_t lba)
){
    uint32_t lba = sector_num * sector_size;

    static uint8_t temp_buff[4096];

    if (4096 < sector_size) return;
    
    fn_read_sector(temp_buff, lba);

    memcpy(temp_buff + sector_off, buff, len);

    fn_write_sector(temp_buff, lba);
}

/*
reads data from offset of sector
assumes:
    drive is set
    len < sector_size
*/
void SAL_read_unaligned (
    size_t len, uint32_t sector_off, uint32_t sector_num, 
    void *buff, size_t sector_size, 
    void(*fn_read_sector) (void *buff, uint32_t lba)
){
    uint32_t lba = sector_num * sector_size;

    static uint8_t temp_buff[4096];

    if (4096 < sector_size) return;
    
    fn_read_sector(temp_buff, lba);

    memcpy(buff, temp_buff + sector_off, len);
}

void SAL_FDC_write(uint8_t drive, size_t len, uint32_t offset, const void *buff){
    FDC_set_drive(drive);

    size_t copied_len = 0;

    /* offset not on sector boundary */
    if (offset % FLOPPY_BYTES_PER_SECTOR){
        uint32_t sector_off = offset % FLOPPY_BYTES_PER_SECTOR;
        size_t    copy_len  = len < FLOPPY_BYTES_PER_SECTOR 
                            ? len 
                            : FLOPPY_BYTES_PER_SECTOR - sector_off;
        copied_len          += copy_len;
    
        SAL_write_unaligned (
            copy_len, sector_off,
            offset / FLOPPY_BYTES_PER_SECTOR, 
            buff, FLOPPY_BYTES_PER_SECTOR, 
            FDC_read_sector, FDC_write_sector
        );
    }

    while (len - copied_len >= FLOPPY_BYTES_PER_SECTOR){
        FDC_write_sector(buff + copied_len, offset + copied_len);
        copied_len += FLOPPY_BYTES_PER_SECTOR;
    }

    /* length ends on non-sector boundary */
    if (len - copied_len){
        uint32_t curr_off   = offset + copied_len;
        uint32_t sector_off = curr_off % FLOPPY_BYTES_PER_SECTOR;
        
        SAL_write_unaligned (
            len - copied_len, sector_off,
            curr_off / FLOPPY_BYTES_PER_SECTOR, 
            buff + copied_len, FLOPPY_BYTES_PER_SECTOR, 
            FDC_read_sector, FDC_write_sector
        );
    }
}

void SAL_FDC_read(uint8_t drive, size_t len, uint32_t offset, void *buff){
    FDC_set_drive(drive);

    size_t copied_len = 0;

    if (offset % FLOPPY_BYTES_PER_SECTOR){
        uint32_t sector_off = offset % FLOPPY_BYTES_PER_SECTOR;
        size_t    copy_len  = len < FLOPPY_BYTES_PER_SECTOR 
                            ? len 
                            : FLOPPY_BYTES_PER_SECTOR - sector_off;
        copied_len          += copy_len;

        SAL_read_unaligned(
            copy_len, sector_off,
            offset / FLOPPY_BYTES_PER_SECTOR,
            buff, FLOPPY_BYTES_PER_SECTOR,
            FDC_read_sector
        );
    }

    while (len - copied_len >= FLOPPY_BYTES_PER_SECTOR){
        FDC_read_sector(buff + copied_len, offset + copied_len);
        copied_len += FLOPPY_BYTES_PER_SECTOR;
    }

    if (len - copied_len){
        uint32_t curr_off   = offset + copied_len;
        uint32_t sector_off = curr_off % FLOPPY_BYTES_PER_SECTOR;
        
        SAL_read_unaligned (
            len - copied_len, sector_off,
            curr_off / FLOPPY_BYTES_PER_SECTOR, 
            buff + copied_len, FLOPPY_BYTES_PER_SECTOR, 
            FDC_read_sector
        );
    }
}

void SAL_ATA_write(uint8_t drive, size_t len, uint32_t offset, const void *buff){
    IDE_set_drive(drive);
    
    size_t copied_len = 0;

    /* offset not on sector boundary */
    if (offset % IDE_ATA_SECTOR_SIZE){
        uint32_t sector_off = offset % IDE_ATA_SECTOR_SIZE;
        size_t    copy_len  = len < IDE_ATA_SECTOR_SIZE 
                            ? len 
                            : IDE_ATA_SECTOR_SIZE - sector_off;
        copied_len          += copy_len;
    
        SAL_write_unaligned (
            copy_len, sector_off,
            offset / IDE_ATA_SECTOR_SIZE, 
            buff, IDE_ATA_SECTOR_SIZE, 
            ATA_read_sector, ATA_write_sector
        );
    }

    while (len - copied_len >= IDE_ATA_SECTOR_SIZE){
        ATA_write_sector(buff, offset + copied_len);
        copied_len += IDE_ATA_SECTOR_SIZE;
    }

    /* write ends on non-sector boundary */
    if (len - copied_len){
        uint32_t curr_off   = offset + copied_len;
        uint32_t sector_off = curr_off % IDE_ATA_SECTOR_SIZE;
        
        SAL_write_unaligned (
            len - copied_len, sector_off,
            curr_off / IDE_ATA_SECTOR_SIZE, 
            buff, IDE_ATA_SECTOR_SIZE, 
            ATA_read_sector, ATA_write_sector
        );
    }
}

void SAL_ATA_read(uint8_t drive, size_t len, uint32_t offset, void *buff){
    IDE_set_drive(drive);
    
    size_t copied_len = 0;

    /* offset not on sector boundary */
    if (offset % IDE_ATA_SECTOR_SIZE){
        uint32_t sector_off = offset % IDE_ATA_SECTOR_SIZE;
        size_t    copy_len  = len < IDE_ATA_SECTOR_SIZE 
                            ? len 
                            : IDE_ATA_SECTOR_SIZE - sector_off;
        copied_len          += copy_len;
    
        SAL_read_unaligned (
            copy_len, sector_off,
            offset / IDE_ATA_SECTOR_SIZE, 
            buff, IDE_ATA_SECTOR_SIZE, 
            ATA_read_sector
        );
    }

    while (len - copied_len >= IDE_ATA_SECTOR_SIZE){
        ATA_read_sector(buff, offset + copied_len);
        copied_len += IDE_ATA_SECTOR_SIZE;
    }

    /* write ends on non-sector boundary */
    if (len - copied_len){
        uint32_t curr_off   = offset + copied_len;
        uint32_t sector_off = curr_off % IDE_ATA_SECTOR_SIZE;
        
        SAL_read_unaligned (
            len - copied_len, sector_off,
            curr_off / IDE_ATA_SECTOR_SIZE, 
            buff, IDE_ATA_SECTOR_SIZE, 
            ATA_read_sector
        );
    }
}

/**
 * ABSTRACTION LAYER DEFINITIONS
 */

 static struct {
    storage_device_t *start;
    size_t num_blocks;
    size_t num_devices;
    size_t block_offset; /* technically can be computed using num_devices but easier this way */
} SAL_arena;

void SAL_add_device(const storage_device_t *device){
    /* need to allocate a new block for the device list */
    if (4096 - SAL_arena.block_offset < sizeof(*device)){
        void *block = alloc_page();
        vmm_map_page(block, SAL_arena.start + 4096 * ++SAL_arena.num_blocks);
    }
    
    *(SAL_arena.start + SAL_arena.num_devices++) = *device;
    SAL_arena.block_offset = (size_t)(SAL_arena.start + SAL_arena.num_devices) % 4096;
}

void SAL_write(storage_device_t *device, size_t len, uint32_t offset, const void *buff){
    if (device->type == STORAGE_TYPE_FLOPPY)
        SAL_FDC_write(device->drive_num, len, offset, buff); else
    if (device->type == STORAGE_TYPE_ATA)
        SAL_ATA_write(device->drive_num, len, offset, buff);
}

void SAL_read(storage_device_t *device, size_t len, uint32_t offset, void *buff){
    if (device->type == STORAGE_TYPE_FLOPPY)
        SAL_FDC_read(device->drive_num, len, offset, buff); else
    if (device->type == STORAGE_TYPE_ATA)
        SAL_ATA_read(device->drive_num, len, offset, buff);
}

storage_device_t *SAL_get_devices(uint32_t *out_num_devices){
    *out_num_devices = SAL_arena.num_devices;
    return (storage_device_t*) SAL_arena.start;
}

/* initializes simple arena allocator for the device list */
void SAL_init(){
    SAL_arena.start = alloc_page();
    SAL_arena.start = vmm_map_page(SAL_arena.start, SAL_arena.start + 0xC0000000);
}