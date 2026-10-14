/**
 * @file multiboot.h
 * @brief what the multiboot bootloader (grub) hands the kernel
 */

#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H

#include "../../libc/include/types.h"


/**
 * one region of the memory map
 *  entries are `size` + 4 bytes apart, `size` doesn't count itself
 */
typedef struct {
    /** size of the rest of the entry */
    uint32_t size;
    /** where the region starts */
    uint32_t address_low;
    uint32_t address_high; ///< ignored on 32-bit
    /** how many bytes long it is */
    uint32_t length_low;
    uint32_t length_high; ///< ignored on 32-bit
/* values for type */
/** ram the kernel is free to use */
#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5
    /** what the region is, one of the MULTIBOOT_MEMORY_* */
    uint32_t type;
} __attribute__((__packed__)) mmap_entry_t;

/**
 * the info struct grub passes in ebx
 *  like 3/4 of this is not relevant to me but i keep it anyway
 */
typedef struct {
    uint32_t flags;        ///< flags
    uint32_t mem_lower;    ///< lower memory (KB)
    uint32_t mem_upper;    ///< upper memory (KB)
    uint32_t boot_device;  ///< boot device ID
    uint32_t cmdline;      ///< address of kernel command line
    uint32_t mods_count;   ///< number of modules loaded
    uint32_t mods_addr;    ///< address of first module structure

    uint32_t irrelevant[4]; ///< this was really irrelevant XD

    /** memory map length, in bytes */
    uint32_t mmap_length;
    /** memory map address, it's an array of mmap_entry_t */
    uint32_t mmap_addr;
    uint32_t drives_length; ///< drive info length
    uint32_t drives_addr;   ///< drive info address
    uint32_t config_table;  ///< ROM configuration table
    uint32_t boot_loader_name; ///< bootloader name string address
    uint32_t apm_table;     ///< APM table address
    uint32_t vbe_control_info; ///< VBE control information
    uint32_t vbe_mode_info; ///< VBE mode information
    uint32_t vbe_mode;      ///< VBE mode
    uint16_t vbe_interface_seg;  ///< VBE interface segment
    uint16_t vbe_interface_off;  ///< VBE interface offset
    uint16_t vbe_interface_len;  ///< VBE interface length

    uint16_t framebuffer_addr;
    uint64_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint32_t framebuffer_bpp;
    uint8_t  framebuffer_type;

    uint8_t  color_info[6];
} __attribute__((__packed__)) multiboot_info_t;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * the info from the bootloader, set up in the boot asm
 */
extern multiboot_info_t* multiboot_info;
#ifdef __cplusplus
}
#endif


#endif
