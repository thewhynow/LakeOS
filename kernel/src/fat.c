#define _FAT_H_INTERNAL
#include "../include/fat.h"
#include "../include/sal.h"
#include "../include/kmm.h"
#include "../include/rtc.h"
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

    uint32_t entry = 0;

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

static void FAT_set_entries(t_FATContext *ctx, uint32_t num, uint32_t data){
    for (uint8_t i = 0; i < ctx->boot_sector.num_fat; ++i)
        FAT_set_entry(ctx, i, num, data);
}

/**
 * gets the FAT entry for said FAT and said cluster;
 * returns 0 on failure;
 */
uint32_t FAT_get_entry(t_FATContext *context, uint8_t fat_num, uint32_t cluster){
    size_t offset = context->boot_sector.reserved_sectors * context->boot_sector.bytes_per_sector;
    offset += fat_num * SECTORS_PER_FAT(context) * context->boot_sector.bytes_per_sector;
    offset += FAT_get_offset(context, cluster);

    uint32_t entry = 0;

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
t_FATContext *FAT_mount(storage_device_t *dev){
    t_FATContext *context = kmalloc(sizeof(t_FATContext));
    context->device = dev;
    context->partition_start = 0;

    SAL_read(
        context->device, sizeof(t_BootSectorCommon), 
        0 + context->partition_start, &context->boot_sector
    );

    /* validate that it is FAT */

    uint16_t boot_signature;

    SAL_read(context->device, 2, 0x1FE, &boot_signature);

    if (boot_signature != 0xAA55){
        kfree(context);
        return NULL;
    }

    FAT_init_type(context);

    /* round up */
    uint32_t root_sectors = 
        (context->boot_sector.num_root_entries * 32 + context->boot_sector.bytes_per_sector - 1) 
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

    context->bytes_p_clus = 
        context->boot_sector.sectors_per_cluster * context->boot_sector.bytes_per_sector;

    context->root = (t_FATHandle){
        .start_cluster = context->type == FAT_32
            ? context->fat32_ext.root_cluster_num : 0,
        .attribs = DIR_ENTRY_DIRECTORY,
        .dir_cluster = 0, 
        .ctx = context,
        .name[0] = '\0'
    };
    return context;
}

/**
 * returns the final offset for file offset
 *  offset, for a file starting at cluster
 * returns 0 on failure
 * 
 * follows the FAT cluster chain.. essentially
 */
uint32_t FAT_absolute_offset(t_FATContext *ctx, uint32_t cluster, size_t offset, uint32_t *clus_fail){
    if (cluster < 2)
        return RootDirSector(ctx) * ctx->boot_sector.bytes_per_sector + offset;

    if (offset < ctx->bytes_p_clus)
        return FAT_clus_to_off(ctx, cluster) + offset;

    while (offset >= ctx->bytes_p_clus){
        uint32_t entry = FAT_get_entry(ctx, 0, cluster);

        if (FAT_is_eoc(ctx, entry)){
            if (clus_fail) *clus_fail = cluster;
            return 0;
        }

        cluster = entry;

        offset -= ctx->bytes_p_clus;
    }

    return FAT_clus_to_off(ctx, cluster) + offset;
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

    FAT_allocate_cluster(ctx, cluster);

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

/**
 * converts from a FAT internal FS name
 *  to an actual string
 */
void FAT_fname_conv(const char *fname, char *name){
    size_t i = 0;
    for (; i < 8 && fname[i] != ' '; ++i)
        name[i] = fname[i];

    if (fname[i] != ' ') name[i++] = '.';
   
    for (size_t j = 8; j < 11 && fname[j] != ' '; ++i, ++j)
        name[i] = fname[j];

    name[i] = '\0';
}

/**
 * initializes a directory that
 * starts at cluster_num and is
 * the child of a directory 
 * starting in parent_clus with `.`
 * and `..` entries, and sets the
 * entry cluster to 0
 */
void FAT_init_dir(t_FATContext *ctx, uint32_t cluster_num, uint32_t parent_clus){
    uint8_t attribs = ENTRY_ATTR_DIRECTORY | ENTRY_ATTR_HIDDEN | ENTRY_ATTR_SYSTEM;
    
    t_ShortDirEntry entry = {0};
    entry = (t_ShortDirEntry){
        .attributes = attribs,
        .first_cluster_low = cluster_num,
    };
    memcpy(entry.name, ".          ", 11);
    
    uint8_t *zero = kmalloc(ctx->bytes_p_clus);
    memset(zero, 0, ctx->bytes_p_clus);

    SAL_write(ctx->device, ctx->bytes_p_clus, FAT_clus_to_off(ctx, cluster_num), zero);

    kfree(zero);

    FAT_write_entry(ctx, entry.first_cluster_low, &entry);

    uint32_t clus = entry.first_cluster_low;

    entry.name[1] = '.';
    entry.first_cluster_low = parent_clus;

    FAT_write_entry(ctx, clus, &entry);
}

/**
 * inserts a null terminator within
 *  the path, seperating the final
 *  file and the parent directory,
 *  returns the offset of the first
 *  character of the filename
 */
uint32_t FAT_split_path(char *path){
    uint32_t fname_begin = 0;
    for (uint32_t i = 0; path[i]; ++i)
        if (path[i] == '/')
            fname_begin = i;
    
    path[fname_begin++] = '\0';    

    return fname_begin;
}

/**
 * frees the cluster chain starting at
 * cluster, marking all the FAT entries
 * as free
 */
void FAT_free_cluster_chain(t_FATContext *ctx, uint32_t cluster){
    while (!FAT_is_eoc(ctx, cluster)){
        uint32_t next = FAT_get_entry(ctx, 0, cluster);

        FAT_set_entries(ctx, cluster, 0x0);

        cluster = next; 
    }
    
    /* set last cluster to free */
    FAT_set_entries(ctx, cluster, 0x0);
}

void FAT_file_entry(t_FATHandle *file, t_ShortDirEntry *out_entry){
    char conv_fname[11];
    FAT_conv_fname(file->name, conv_fname);

    bool eoc = FAT_dir_entry(file->ctx, file->dir_cluster, 0, out_entry);

    for (uint32_t i = 0; !eoc; ++i){
        if (!memcmp(out_entry, conv_fname, 11)) return;
        FAT_dir_entry(file->ctx, file->dir_cluster, i + 1, out_entry);
    }

    memset(out_entry, 0, sizeof *out_entry);
}

/**
 * allocates a new cluster for the
 *  cluster chain starting at cluster
 */
void FAT_allocate_cluster(t_FATContext *ctx, uint32_t _cluster){
    uint32_t cluster = _cluster;
    uint32_t next = FAT_get_entry(ctx, 0, cluster);
    while (!FAT_is_eoc(ctx, next)){
        cluster = next;
        next    = FAT_get_entry(ctx, 0, next);
    }

    uint32_t free_clus = FindFreeCluster(ctx);
    FAT_set_entries(ctx, cluster, free_clus);
    FAT_set_entries(ctx, free_clus, 0xFFFFFFFF);

}

/**
 * updates the size of the file entry
 *  corresponding to the file fname
 *  in the parent directory dir
 */
void FAT_upd_entry(t_FATContext *ctx, t_FATFile *file){
    bool eoc = false;
    t_ShortDirEntry entry;
    uint32_t entry_num = 0;
    do {
        eoc = FAT_dir_entry(ctx, file->parent_clus, entry_num, &entry);
        if (file->starting_clus == entry.first_cluster_low){
            entry.file_size = file->size;
            entry.last_access_date = FAT_get_date();
            if (file->flags & FAT_FILE_WRITE){
                entry.last_write_date = FAT_get_date();
                entry.last_write_time = FAT_get_time();
            }
            size_t off = FAT_absolute_offset(ctx, file->parent_clus, entry_num * sizeof(entry), NULL);  
            SAL_write(ctx->device, sizeof(entry), off, &entry);
            return;
        }
        ++entry_num;
    } while (!eoc);
}

uint16_t FAT_get_date(){
    uint16_t fat_date = 0;
    fat_date =  (time.monthday);
    fat_date |= time.month << 5;
    fat_date |= (time.year - 1980) << 9; 

    return fat_date;
}

uint16_t FAT_get_time(){
    uint16_t fat_time = 0;
    fat_time = time.seconds / 2;
    fat_time |= (time.minutes) << 5;
    fat_time |= (time.hours) << 11;

    return fat_time;
}

void FAT_conv_to_chrono(uint16_t fat_date, uint16_t fat_time, t_FileChrono *out){
    *out = (t_FileChrono){
        .day_of_month  = fat_date & 0x1F,
        .month_of_year = (fat_date >> 5) & 0xF,
        .year          = (fat_date >> 9) & 0x7F + 1980,
        .seconds       = (fat_time & 0x1F) * 2,
        .minutes       = (fat_time >> 5) & 0x3F,
        .hours         = (fat_time >> 11) & 0x1F
    };
}

uint8_t FAT_conv_attrib(uint8_t vfsattrib){
    uint8_t res = 0;
    if (vfsattrib & FILE_ATTRIB_DIRECTORY) res |= ENTRY_ATTR_DIRECTORY;
    if (vfsattrib & FILE_ATTRIB_HIDDEN)    res |= ENTRY_ATTR_HIDDEN;
    if (vfsattrib & FILE_ATTRIB_SYSTEM)    res |= ENTRY_ATTR_SYSTEM;
    if (vfsattrib & FILE_ATTRIB_READ_ONLY) res |= ENTRY_ATTR_READ_ONLY;
    return res;
}

/* FUNCTIONS DEFINED BELOW THIS SUBHEADING ARE FOR THE VFS API */

void FAT_unmount(t_FATContext *ctx){
    kfree(ctx);
}

t_FATHandle *FAT_get_root(t_FATContext *ctx){
    return &ctx->root;
}

t_FATHandle *FAT_lookup(t_FATContext *ctx, t_FATHandle *dir, const char *name){
    t_ShortDirEntry entry;
    t_FATHandle temp = (t_FATHandle){
        .ctx = ctx,
        .dir_cluster = dir->start_cluster
    };
    memcpy(temp.name, name, 11);
    FAT_file_entry(&temp, &entry);
    t_FATHandle *handle;

    if (entry.first_cluster_low){
        handle = kmalloc(sizeof (t_FATHandle));
        *handle = (t_FATHandle){
            .start_cluster = entry.first_cluster_low,
            .dir_cluster = dir->start_cluster,
            .attribs = entry.attributes,
            .ctx = dir->ctx
        };
        memcpy(handle->name, name, 13);
        return handle;
    }
    
    return NULL;
}

void FAT_create(t_FATHandle *dir, const char *name, uint8_t attribs){
    t_ShortDirEntry entry = {0};
    entry = (t_ShortDirEntry){
        .attributes = FAT_conv_attrib(attribs),
        .first_cluster_low = FindFreeCluster(dir->ctx),
        .creation_date = FAT_get_date(),
        .creation_time = FAT_get_time(),
    };

    FAT_conv_fname(name, (char*)&entry.name);
    FAT_write_entry(dir->ctx, dir->start_cluster, &entry);

    FAT_set_entries(dir->ctx, entry.first_cluster_low, 0xFFFFFFFF);

    if (attribs & FILE_ATTRIB_DIRECTORY)
        FAT_init_dir(dir->ctx, entry.first_cluster_low, dir->start_cluster);
}

void FAT_remove(t_FATHandle *file){
    t_ShortDirEntry entry;
    bool is_eoc = 
        FAT_dir_entry(file->ctx, file->dir_cluster, 0, &entry);

    char conv_fname[11];
    FAT_conv_fname(file->name, conv_fname);
    for (uint32_t i = 0; !is_eoc; ++i){
        if (!memcmp(entry.name, conv_fname, 11)){
            FAT_free_cluster_chain(file->ctx, entry.first_cluster_low);
            size_t entry_off = i * sizeof(t_ShortDirEntry); 
            size_t final_off = 
                FAT_absolute_offset(file->ctx, file->dir_cluster, entry_off, NULL);
            entry = (t_ShortDirEntry){0};
            SAL_write(file->ctx->device, sizeof(t_ShortDirEntry), final_off, &entry);
        }

        is_eoc = FAT_dir_entry(file->ctx, file->dir_cluster, i + 1, &entry);
    }
}

t_FATFile *FAT_open(t_FATHandle *handle, uint8_t mode){
    uint32_t starting_clus = handle->start_cluster,
             parent_clus   = handle->dir_cluster;
    size_t   file_size;

    t_ShortDirEntry entry;
    FAT_file_entry(handle, &entry);
    file_size = entry.file_size;
        
    t_FATFile *file = kmalloc(sizeof *file);
    
    if (file)
        *file = (t_FATFile){
            .starting_clus = starting_clus,
            .parent_clus   = parent_clus,
            .size          = file_size,
            .flags         = mode,
            .ctx           = handle->ctx
        };

    return file;
}

void FAT_close(t_FATFile *file){
    FAT_upd_entry(file->ctx, file);

    kfree(file);
}

size_t FAT_write(t_FATFile *file, size_t len, void *data){
    if (!(file->flags | FAT_FILE_WRITE)) return 0;
    t_FATContext *ctx = file->ctx;
    size_t full_off = FAT_absolute_offset(ctx, file->starting_clus, file->size, NULL),
           bytes_written = 0, 
           bpc  = ctx->bytes_p_clus;

    if (file->size && !(file->size % bpc))
        FAT_allocate_cluster(ctx, file->starting_clus);

    /* start on unaligned to cluster */
    if (file->size % bpc){
        size_t clust_off = file->size % bpc,
               write_len = bpc - clust_off >= len ? len : bpc - clust_off;
        SAL_write(ctx->device, write_len, full_off, data); 
        bytes_written += write_len;
        file->size    += write_len;
    }

    while (len - bytes_written >= bpc){
        FAT_allocate_cluster(ctx, file->starting_clus);
        full_off = FAT_absolute_offset(ctx, file->starting_clus, file->size, NULL);
        SAL_write(ctx->device, bpc, full_off, data + bytes_written); 
        bytes_written += bpc;
        file->size    += bpc;
    }

    /* end on unaligned to cluster */
    if (len - bytes_written){
        if (file->size) FAT_allocate_cluster(ctx, file->starting_clus);
        full_off = FAT_absolute_offset(ctx, file->starting_clus, file->size, NULL);
        size_t copy_len = len - bytes_written;
        SAL_write(ctx->device, copy_len, full_off, data + bytes_written);
        bytes_written += copy_len;
        file->size    += copy_len;
    }

    return bytes_written;
}

size_t FAT_read(t_FATFile *file, size_t len, void *data){
    if (!(file->flags | FAT_FILE_READ)) return 0;
    t_FATContext *ctx = file->ctx;
    size_t full_off = FAT_absolute_offset(ctx, file->starting_clus, file->position, NULL),
           bytes_read = 0, 
           bpc  = ctx->bytes_p_clus;

    /* start on unaligned to cluster */
    if (file->position % bpc){
        size_t clust_off = file->position % bpc,
               read_len  = bpc - clust_off >= len ? len : bpc - clust_off;
        SAL_read(ctx->device, read_len, full_off, data); 
        bytes_read     += read_len;
        file->position += read_len;
        if (bytes_read > file->size) return 0;
    }

    while (len - bytes_read >= bpc){
        full_off = FAT_absolute_offset(ctx, file->starting_clus, file->position, NULL);
        SAL_read(ctx->device, bpc, full_off, data + bytes_read); 
        bytes_read += bpc;
        file->position += bpc;
        if (bytes_read > file->size) return 0;
    }

    /* end on unaligned to cluster */
    if (len - bytes_read){
        full_off = FAT_absolute_offset(ctx, file->starting_clus, file->position, NULL);
        size_t read_len = len - bytes_read;
        SAL_read(ctx->device, read_len, full_off, data + bytes_read);
        bytes_read += read_len;
        file->position += read_len;
        if (bytes_read > file->size) return 0;
    }

    return bytes_read;
}

t_FATHandle *FAT_read_dir(t_FATHandle *dir, size_t n){
    t_ShortDirEntry entry;
    uint32_t fail = 
        FAT_dir_entry(dir->ctx, dir->start_cluster, n, &entry);
    
    if (!fail) return NULL;
        
    t_FATHandle *new = kmalloc(sizeof(t_FATHandle));
    *new = (t_FATHandle){
            .start_cluster = entry.first_cluster_low,
            .dir_cluster = dir->start_cluster,
            .attribs = entry.attributes,
            .ctx = dir->ctx
    };
    FAT_fname_conv((char*)entry.name, new->name);
    
    return new; 
}

const char *FAT_handle_name(t_FATHandle *handle){ 
    return handle->name; 
}

void FAT_file_stat(t_FATHandle *file, t_FileStat *out){
    t_ShortDirEntry entry;
    FAT_file_entry(file, &entry);
    FAT_conv_to_chrono(entry.creation_date, entry.creation_time, &out->created);
    FAT_conv_to_chrono(entry.last_write_date, entry.last_write_time, &out->modified);
    out->size = entry.file_size;
    out->flags = 0;
    if (entry.attributes | ENTRY_ATTR_DIRECTORY) out->flags |= FILE_ATTRIB_DIRECTORY;
    if (entry.attributes | ENTRY_ATTR_READ_ONLY) out->flags |= FILE_ATTRIB_READ_ONLY;
    if (entry.attributes | ENTRY_ATTR_HIDDEN)    out->flags |= FILE_ATTRIB_HIDDEN;
    if (entry.attributes | ENTRY_ATTR_SYSTEM)    out->flags |= FILE_ATTRIB_SYSTEM;
}



void FAT_init(){
    t_VFSOperations driver = (t_VFSOperations){
        .f_Mount    = (void*)FAT_mount,
        .f_UnMount  = (void*)FAT_unmount,
        .f_Root     = (void*)FAT_get_root,
        .f_Lookup   = (void*)FAT_lookup,
        .f_Create   = (void*)FAT_create,
        .f_Remove   = (void*)FAT_remove,
        .f_Open     = (void*)FAT_open,
        .f_Close    = (void*)FAT_close,
        .f_Read     = (void*)FAT_read,
        .f_Write    = (void*)FAT_write,
        .f_ReadDir  = (void*)FAT_read_dir,
        .f_NodeName = (void*)FAT_handle_name,
        .f_Stat     = (void*)FAT_file_stat
    };
    VFS_register_fs(&driver);
}
