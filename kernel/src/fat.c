#define _FAT_H_INTERNAL
#include "../include/fat.h"
#include "../include/sal.h"
#include "../include/pmm.h"
#include "../include/vmm.h"
#include "../../libc/include/stdio.h"
#include "../../libc/include/stdlib.h"
#include "../../libc/include/string.h"

/**
 * initializes context with the FAT type, assumes that
 * all relevant data from the BPB have been read from disk;
 */
void FAT_init_type(t_FATContext *context) {
    t_BootSectorCommon *boot_sec = &context->boot_sector;
    
    size_t total_sectors = 
        boot_sec->num_sectors
        ? boot_sec->num_sectors
        : boot_sec->long_sectors
    ;
    
    /* this computation rounds UP */
    size_t num_root_sectors = 
        ((boot_sec->num_root_entries * sizeof(t_ShortDirEntry)) +
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
        context->type = FAT_12; else
    if (num_clusters < 65525)
        context->type = FAT_16; 
    else
        context->type = FAT_32;
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
                data  &= 0x0FFF;
                entry &= 0xF000;
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
        ((boot_sec->num_root_entries * sizeof(t_ShortDirEntry)) +
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

/**
 * computes a checksum for the short_name at the end of a series of
 *  long directory entries, which is used in said entries as a field
 * 
 * the checksum code is borrowed from
 *  Microsoft Extensible Firmware Initiative FAT32 File System Specification, 
 *  revision 1.03, page 33
 */
uint8_t LongDirEntry_checksum(const uint8_t *short_name){
    uint8_t sum = 0;

    for (int i = 0; i < 11; ++i)
        /* the operation is apprently a unsigned byte rotate right */
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *short_name++;

    return sum;
}

/**
 * converts a long FAT name to a short name using the algorithm provided
 *  by microsoft
 * 
 * assumes:
 *  long_name: a mutable string containing the long-name that needs to be converted
 *  short_name: output parameter; a string that is at least 11 characters long
 */
void LongToShortName(char *long_name, char *short_name){
    char *short_base = short_name;
    char *short_ext  = short_name + 8;
    
    int long_s = strlen(long_name);

    int i = 0;

    /* remove leading periods */
    while (long_name[i] == '.') ++i;
    memmove(long_name + i, long_name + i + 1, long_s - i);

    /* convert to uppercase && remove spaces */
    for (i = 0; i < long_s; ++i)
        if ('a' <= long_name[i] && long_name[i] <= 'z')
            long_name[i] -= 'A' - 'a'; else
        if (long_name[i] == ' ')
            memmove(long_name + i, long_name + i + 1, long_s - i);    

    /* copy the base name */
    for (i = 0; i < long_s && i < 8 && long_name[i] != '.'; ++i)
        short_base[i] = long_name[i];

    /* scan for last period */
    int last_period;

    for (; i < long_s; ++i)
        if (i == '.')
            last_period = i;

    /* copy the extension */
    for (i = last_period; i < long_s && last_period - i < 3; ++i)
        short_ext[i] = long_name[i];

    /**
     * TODO: implement Numeric Tail-Generation Algorithm based on other short names
     *  in the same directory
     */
}

/**
 * determines the type of directory entry
 * returns DIR_ENTRY_INVALID on failure / invalid entry
 */
e_DirEntryType DirEntryType(const t_LongDirEntry *entry){
    if ((entry->attribute & ENTRY_ATTR_MASK_LONG_NAME) == ENTRY_ATTR_LONG_NAME && entry->order != 0xE5)
        return DIR_ENTRY_LONG_NAME;

    if ((entry->attribute & (ENTRY_ATTR_VOLUME_ID | ENTRY_ATTR_VOLUME_ID)) == 0)
        return DIR_ENTRY_FILE;
    
    if ((entry->attribute & (ENTRY_ATTR_VOLUME_ID | ENTRY_ATTR_DIRECTORY)) == ENTRY_ATTR_DIRECTORY)
        return DIR_ENTRY_DIRECTORY;
    
    if ((entry->attribute & (ENTRY_ATTR_VOLUME_ID | ENTRY_ATTR_DIRECTORY)) == ENTRY_ATTR_VOLUME_ID)
        return DIR_ENTRY_VOL_LABEL;

    return DIR_ENTRY_INVALID;
}

/**
 * returns the sector number of the root directory
 */
size_t RootDirSector(t_FATContext *context){
    t_BootSectorCommon *boot_sector = &context->boot_sector;

    size_t data_region_start = boot_sector->reserved_sectors + (boot_sector->num_fat * boot_sector->sectors_per_fat);
    
    if (context->type == FAT_12 || context->type == FAT_16)
        return data_region_start;
    else
        return (context->fat32_ext.root_cluster_num - 2) * boot_sector->sectors_per_cluster + data_region_start;
}

/**
 * searches the FAT for a free cluster and returns the
 *  cluster number
 * 
 * returns 0 on failure
 */
uint32_t FindFreeCluster(t_FATContext *context){
    for (size_t i = 2; i < context->boot_sector.num_sectors * context->boot_sector.sectors_per_cluster; ++i){
        uint32_t entry = FAT_get_entry(context, 0, i);
        
        if (!entry) return i;
    }

    return 0;
}

/**
 * creates a file named "A.TXT"
 *  in the root directory
 */
void FAT_test(t_FATContext *context){
    t_ShortDirEntry entry = {
        .name = {
            'H', 'E', 'L', 'L', 'O', ' ', ' ', ' ', 'T', 'X', 'T'
        },
        .attributes = DIR_ENTRY_FILE,
        .reserved = 0,
        .creation_tenths = 0,
        .creation_time = 0,
        .creation_date = 0,
        .last_access_date = 0,

        .first_cluster_high = 0,

        .last_write_time = 0,
        .last_write_date = 0,

        .first_cluster_low = FindFreeCluster(context),
        .file_size = 0
    };

    for (uint8_t i = 0; i < context->boot_sector.num_fat; ++i)
        FAT_set_entry(context, i, entry.first_cluster_low, 0xFFF);

    size_t root_dir_off = RootDirSector(context) * context->boot_sector.bytes_per_sector;

    SAL_write(context->device, sizeof(t_ShortDirEntry), root_dir_off, &entry); 
    memset(&entry, 0, sizeof entry);
    SAL_read (context->device, sizeof(t_ShortDirEntry), root_dir_off, &entry);
}

/**
 * reads all the appropriate fields from disk
 *  and determines the FAT type
 */
void FAT_context_init(t_FATContext *context){
    SAL_read(context->device, sizeof(t_BootSectorCommon), 0 + context->partition_start, &context->boot_sector);

    /* validate that it is FAT */

    uint16_t boot_signature;

    SAL_read(context->device, 2, 0x1FE, &boot_signature);

    if (boot_signature != 0xAA55){
        context->type = FAT_NULL;
        return;
    }

    FAT_init_type(context);

    size_t offset = sizeof(t_BootSectorCommon);

    if (context->type == FAT_32){
        SAL_read(context->device, sizeof(t_BootSector32Ext), offset, &context->fat32_ext);
        offset += sizeof(t_BootSector32Ext);
        SAL_read(context->device, sizeof(t_BootSectorCommonExt), offset, &context->fat32_common_ext);

        /**
         * I could (and technically should) read the FSINFO sector here, but
         *  until I write a FAT formatter I am not going to be doing so
         */
    }
    else
        SAL_read(context->device, sizeof(t_BootSectorCommonExt), offset, &context->old_ext);
}