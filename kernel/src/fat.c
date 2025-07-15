#define _FAT_H_INTERNAL
#include "../include/fat.h"
#include "../include/sal.h"
#include "../../libc/include/stdio.h"
#include "../../libc/include/stdlib.h"

/**
 * initializes context with the FAT type, assumes that
 * all relevant data from the BPB have been read from disk;
 */
e_FAT FAT_init_type(t_FATContext *context) {
    t_BootSectorCommon *boot_sec = &context->boot_sector;
    
    size_t total_sectors = 
        boot_sec->num_sectors
        ? boot_sec->num_sectors
        : boot_sec->long_sectors
    ;
    
    /* this computation rounds UP */
    size_t num_root_sectors = 
        ((boot_sec->num_root_entries * FAT_DIRECTORY_ENTRY_SZ) +
        (boot_sec->bytes_per_sector - 1)) / boot_sec->bytes_per_sector
    ;
    
    size_t num_data_sectors = 
        total_sectors - (
            boot_sec->reserved_sectors +
            SECTORS_PER_FAT(context) * boot_sec->num_fat +
            num_root_sectors
        )
    ;

    size_t num_clusters = num_data_sectors / boot_sec->sectors_per_cluster;

    if (num_clusters < 4085)
        return FAT_12; else
    if (num_clusters < 65525)
        return FAT_16; 
    else
        return FAT_32;
}

/**
 * returns the offset within the first FAT for a specified cluster number;
 * returns (unsigned)-1 on failure;
 */
size_t FAT_get_offset(t_FATContext *context, uint32_t cluster){
    switch(context->type){
        case FAT_12:
            return cluster + (cluster / 2);
        case FAT_16:
            return cluster * 2;
        case FAT_32:
            return cluster * 4;

        case FAT_NULL:
            return -1;
    }
}

/**
 * sets the FAT entry for a said cluster and said FAT;
 * returns ~data on failure, data on success;
 */
uint32_t FAT_set_entry(t_FATContext *context, uint8_t fat_num, uint32_t cluster, uint32_t data){
    size_t offset = context->boot_sector.reserved_sectors * context->boot_sector.bytes_per_sector;
    offset += fat_num * SECTORS_PER_FAT(context) * context->boot_sector.bytes_per_sector;
    offset += FAT_get_offset(context, cluster);

    uint32_t entry;

    switch(context->type){
        case FAT_12: {
            SAL_read(context->device, 2, offset, &entry);

            /* preserve either top or bottom nibble since entries are 12 bits */

            if (cluster % 2){
                data <<= 4;
                entry &= 0xF;
            }
            else {
                data  &= 0xFFF0;
                entry &= 0x000F;
            }

            data |= entry;

            /* fall through to FAT16 case */
        }

        case FAT_16: {
            SAL_write(context->device, 2, offset, &data);
            break;
        }

        case FAT_32: {
            SAL_read(context->device, 4, offset, &entry);

            /* top nibble must be preserved */

            data &= 0x0FFFFFFF;
            data |= entry & 0xF0000000;

            SAL_write(context->device, 4, offset, &data);

            break;
        }

        case FAT_NULL: return ~data;
    }

    return data;
}

/**
 * gets the FAT entry for said FAT and said cluster;
 * returns 0 on failure;
 */
uint32_t FAT_get_entry(t_FATContext *context, uint8_t fat_num, uint32_t cluster){
    size_t offset = context->boot_sector.reserved_sectors * context->boot_sector.bytes_per_sector;
    offset += fat_num * SECTORS_PER_FAT(context) * context->boot_sector.bytes_per_sector;
    offset += FAT_get_offset(context, cluster);

    uint32_t entry;

    switch(context->type){
        case FAT_12: {
            SAL_read(context->device, 2, offset, &entry);
            
            /* discard either top or bottom nibble since entries are 12 bits */

            if (cluster % 2)
                return entry >> 4;
            else
                return entry & 0x0FFF;
        }
        
        case FAT_16: {
            SAL_read(context->device, 2, offset, &entry);
            return entry;
        }
        
        case FAT_32: {
            SAL_read(context->device, 4, offset, &entry);

            /* discard top nibble since entries are 28 bits */

            return entry & ~0xF;
        }

        case FAT_NULL:
            return 0;
    }
}

/**
 * returns non-zero if the entry marks the end of a cluster
 * chain; 
 * else returns 0;
 * also returns 0 on context->type == FAT_NULL;
 */
bool FAT_is_eoc(t_FATContext *context, uint32_t fat_entry){
    switch(context->type){
        case FAT_12:
            return fat_entry >= 0xFF8;

        case FAT_16:
            return fat_entry >= 0xFFF8;

        case FAT_32:
            return fat_entry >= 0xFFFFFFF8;
        
        case FAT_NULL:
            return false;
    }
}

/**
 * initializes the BPB.sectors_per_fat field when formatting a drive;
 * assumes BPB.sectors_per_cluser, BPB.num_root_entries,
 * BPB.reserved_sectors, BPB.num_fats, and BPB.num_sectors are valid fields;
 */
void FAT_calc_size(t_FATContext *context){
    /**
     * """
     * Do not spend too much time trying to figure out why this math works. 
     * The basis for the computation is complicated; the important point is 
     * that this is how Microsoft operating systems do it, and it works.
     * """
     * - Microsoft Extensible Firmware Initiative FAT32 File System Specification, 
     * revision 1.03, page 24
     */

    t_BootSectorCommon *boot_sec = &context->boot_sector;

    size_t num_root_sectors = 
        ((boot_sec->num_root_entries * FAT_DIRECTORY_ENTRY_SZ) +
        (boot_sec->bytes_per_sector - 1)) / boot_sec->bytes_per_sector
    ;

    uint32_t tmp_1 = boot_sec->num_sectors - (boot_sec->reserved_sectors + num_root_sectors),
             tmp_2 = (256 * boot_sec->sectors_per_cluster) + boot_sec->num_fat
    ;

    if (context->type == FAT_32)
        tmp_2 /= 2;
    
    uint32_t fat_size = (tmp_1 + (tmp_2 - 1)) / tmp_2;

    if (context->type == FAT_32){
        boot_sec->sectors_per_fat = 0;
        context->fat32_ext.sectors_per_fat = fat_size;
    }
    else
        boot_sec->sectors_per_fat = fat_size;
}



void FAT_context_init(t_FATContext *context){
    SAL_read(context->device, sizeof(t_BootSectorCommon), 0, &context->boot_sector);

    
}