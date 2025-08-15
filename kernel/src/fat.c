#define _FAT_H_INTERNAL
#include "../include/fat.h"
#include "../include/sal.h"
#include "../../libc/include/stdlib.h"
#include "../../libc/include/string.h"
#include "../../libc/include/stdio.h"

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
                entry &= 0x000F;
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
e_LongDirEntryType DirEntryType(const t_LongDirEntry *entry){
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
    size_t num_clusters = context->boot_sector.num_sectors * context->boot_sector.sectors_per_cluster;
    for (size_t i = 2; i < num_clusters; ++i){
        uint32_t entry = FAT_get_entry(context, 0, i);
        
        if (!entry) return i;
    }

    return 0;
}

/**
 * reads all the appropriate fields from disk
 *  and determines the FAT type
 * 
 * assumes:
 *  context->device is set
 *  context->partition_start is set
 */
void FAT_context_init(t_FATContext *context){
    SAL_read(
        context->device, sizeof(t_BootSectorCommon), 
        0 + context->partition_start, &context->boot_sector
    );

    /* validate that it is FAT */

    uint16_t boot_signature;

    SAL_read(context->device, 2, 0x1FE, &boot_signature);

    if (boot_signature != 0xAA55){
        context->type = FAT_NULL;
        return;
    }

    FAT_init_type(context);

    /* round up */
    uint32_t root_sectors = (context->boot_sector.num_root_entries * 32 + context->boot_sector.bytes_per_sector - 1) 
                            / context->boot_sector.bytes_per_sector;

    context->data_region_start  = context->boot_sector.reserved_sectors;
    context->data_region_start += context->boot_sector.num_fat * context->boot_sector.sectors_per_fat;
    context->data_region_start += root_sectors;
    context->data_region_start *= context->boot_sector.bytes_per_sector;

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

/**
 * creates a file named "HELLO.TXT"
 *  in the root directory
 */
void FAT_test(t_FATContext *ctx){
    FAT_create(ctx, "/MYDIR", ENTRY_ATTR_DIRECTORY);
    FAT_create(ctx, "/MYDIR/HELLO.TXT", 0);

    printf("/MYDIR found @ cluster %d\n", FAT_file_cluster(ctx, "/MYDIR"));
    printf("/MYDIR/HELLO.TXT found @ cluster %d\n", FAT_file_cluster(ctx, "/MYDIR/../MYDIR/HELLO.TXT"));
}

/**
 * returns the final offset for file offset
 *  offset, for a file starting at cluster
 * 
 * follows the FAT cluster chain.. essentially
 */
uint32_t FAT_absolute_offset(t_FATContext *ctx, uint32_t cluster, size_t offset, uint32_t *clus_fail){
    size_t bytes_per_cluster = ctx->boot_sector.sectors_per_cluster * ctx->boot_sector.bytes_per_sector;

    if (cluster < 2)
        return RootDirSector(ctx) * ctx->boot_sector.bytes_per_sector + offset;

    if (offset < bytes_per_cluster)
        return FAT_clus_to_off(ctx, cluster) + offset;
    
    while (offset > bytes_per_cluster){
        uint32_t entry = FAT_get_entry(ctx, 0, cluster);

        if (FAT_is_eoc(ctx, entry)){
            if (clus_fail) *clus_fail = cluster;
            return 0;
        }

        cluster = entry;

        offset -= bytes_per_cluster;
    }

    return cluster * bytes_per_cluster + offset;
}

/**
 * reads entry_num entry from the directory
 *  starting at cluster into out_entry
 *
 * on success, returns 0
 * on failure, returns the last cluster that
 *  was allocated by the cluster chain
 */
uint32_t FAT_dir_entry(t_FATContext *ctx, uint32_t cluster, uint32_t entry_num, void *out_entry){
    size_t entry_offset = entry_num * sizeof(t_ShortDirEntry);
    uint32_t last_free_cluster;

    size_t final_offset = FAT_absolute_offset(ctx, cluster, entry_offset, &last_free_cluster);
    if (final_offset){
        SAL_read(ctx->device, sizeof(t_ShortDirEntry), final_offset, out_entry);
        return 0;
    }
    else
        return last_free_cluster;
}

/**
 * converts a cluster number to a disk
 *  offset, treats cluster 0 as root 
 */
size_t FAT_clus_to_off(t_FATContext *ctx, uint32_t cluster){
    if (cluster > 1)
        return ctx->data_region_start + 
            (cluster - 2) * ctx->boot_sector.sectors_per_cluster * ctx->boot_sector.bytes_per_sector;
    else
        return RootDirSector(ctx) * ctx->boot_sector.bytes_per_sector;
}

bool FAT_is_root(t_FATContext *ctx, uint32_t cluster){
	if (ctx->type != FAT_32)
		return cluster < 2;
     
     size_t root_off = RootDirSector(ctx) * ctx->boot_sector.bytes_per_sector;
     size_t clus_off = FAT_clus_to_off(ctx, cluster);

     return  !!FAT_absolute_offset(ctx, 0, clus_off - root_off, NULL);
}

/**
 * finds a file in the directory
 *  starting at cluster and returns
 *  the cluster number of the file,
 *  -1 on file-not-found
 */
uint32_t FAT_find_file(t_FATContext *ctx, uint32_t cluster, const char *_name){
    int eoc = 0, 
    entry_num = 0; 
    
    t_ShortDirEntry entry;

    eoc = FAT_dir_entry(ctx, cluster, entry_num, &entry);
   
    char name[11];
    FAT_conv_fname(_name, name);

    while (!eoc && memcmp(entry.name, name, 11)){
        ++entry_num;
        eoc = FAT_dir_entry(ctx, cluster, entry_num, &entry);	
    }
    
    if (eoc)
        return -1;
    else
        return entry.first_cluster_low;
}

/**
 * returns the cluster number of the specified file
 *  since directories are just files, this function
 *  also handles them and returns the cluster number
 *  of the directory
 * in the root directory special case on FAT12/16,
 *  this function returns 0 as the cluster number
 */
uint32_t FAT_file_cluster(t_FATContext *ctx, const char *_path){
     
    static char path[0xFFF];
    memset(path, 0, 0xFFF);
    memcpy(path, _path, strlen(_path));

    uint32_t clus =  ctx->type == FAT_32
         ? ctx->fat32_ext.root_cluster_num
         : 0;

    char *name = strtok(path, "/");

    while (name){
        clus = FAT_find_file(ctx, clus, name);

        if (clus == 0xFFFFFFFF)
            return -1;

        name = strtok(NULL, "/");
    }

    return clus;
}

void FAT_write_entry(t_FATContext *ctx, uint32_t cluster, void *entry){
    int entry_num = 0;
    t_ShortDirEntry read_entry;
    int eoc = FAT_dir_entry(ctx, cluster, entry_num, &read_entry);   

    for (; !eoc; ++entry_num){
        retry_search:

        eoc = FAT_dir_entry(ctx, cluster, entry_num, &read_entry);
        if (read_entry.name[0] == 0x00 || read_entry.name[0] == 0xE5){
            size_t offset = FAT_clus_to_off(ctx, cluster) + entry_num * sizeof(t_ShortDirEntry); 
            SAL_write(ctx->device, sizeof(t_ShortDirEntry), offset, entry);
            return;
        }
    }

    /* reaching this point means a new cluster needs to be allocated */
    
    uint32_t new_cluster = FindFreeCluster(ctx);

    for (int i = 0; i < ctx->boot_sector.num_fat; ++i)
        FAT_set_entry(ctx, i, eoc, new_cluster);

    eoc = 0;

    goto retry_search;
}

/**
 * assumes out is a 
 * string that contains at minimum
 * 11 characters
 */
void FAT_conv_fname(const char *name, char *out){
    int i = 0;
    int l = strlen(name);
   
    /* handle '.' and '..' special case */
    if (name[0] == '.' && l < 3){
        memcpy(out, name, l);
        memset(out + l, ' ', 11 - l);
        return;
    }

    for (; i < l && i < 8 && name[i] != '.'; ++i)
        out[i] = name[i];
    
    int j = i;
    for (; j < 8; ++j)
        out[j] = ' ';

    ++i;

    for (; i < l && i < 11; ++i, ++j)
        out[j] = name[i];

    for (; j < 11; ++j)
        out[j] = ' ';
}


/* FUNCTIONS DEFINED BELOW THIS SUBHEADING ARE FOR THE VFS API */

/**
 * this function creates a file mentioned by the specified path
 *  doesn't do m(any) sanity checks and will silently (or very loudly)
 *  fail if YOU fuck something up in the parameters !!
 */
void FAT_create(t_FATContext *ctx, const char *_path, uint32_t attribs){
    char path[0xFFF]; 
    memcpy(path, _path, strlen(_path));
    
    int fname_begin = 0;
    for (int i = 0; path[i]; ++i)
        if (path[i] == '/')
            fname_begin = i;
    
    path[fname_begin++] = '\0';

    /* get the cluster of the parent directory */
    uint32_t dir_cluster = FAT_file_cluster(ctx, path);

    char fname[11];
    FAT_conv_fname(path + fname_begin, fname);

    t_ShortDirEntry entry = {
        .attributes = attribs,
        .reserved = 0,
        .creation_tenths = 0,
        .creation_time = 0,
        .creation_date = 0,
        .last_access_date = 0,
        .first_cluster_high = 0,
        .last_write_time = 0,
        .last_write_date = 0,
        .first_cluster_low = FindFreeCluster(ctx),
        .file_size = ctx->boot_sector.bytes_per_sector * ctx->boot_sector.sectors_per_cluster
    };

    memcpy(entry.name, fname, 11);
    FAT_write_entry(ctx, dir_cluster, &entry);

    for (uint16_t i = 0; i < ctx->boot_sector.num_fat; ++i)
        FAT_set_entry(ctx, i, entry.first_cluster_low, 0xFFFFFFFF);

    /* creating '.' and '..' entries in directories */
    if (attribs | ENTRY_ATTR_DIRECTORY){
        memcpy(entry.name, ".          ", 11);
        
        FAT_write_entry(ctx, entry.first_cluster_low, &entry);

        uint32_t clus = entry.first_cluster_low;

        entry.name[1] = '.';
        entry.first_cluster_low = dir_cluster;

        FAT_write_entry(ctx, clus, &entry);
    }
}


