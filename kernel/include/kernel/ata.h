/**
 * @file ata.h
 * @brief ide/ata hard drives, pio mode, polling instead of irqs
 *  (suitable for singletasking). the driver registers every drive
 *  it finds with the SAL
 */

#ifndef _ATA_H
#define _ATA_H

#include "../../libc/include/types.h"
#include "io.h"
#include "pit.h"
#include "sal.h"

#ifdef _ATA_H_INTERNAL

/**
 * Status/Command port returns information regarding status of channel when read
 */
typedef enum {
    /** drive is working on something, nothing else is valid until this clears */
    ATA_SR_MASK_BUSY           = 0x80,
    ATA_SR_MASK_DRIVE_READY    = 0x40,
    /** the drive hit a fault */
    ATA_SR_MASK_DRIVE_FAULT    = 0x20,
    ATA_SR_MASK_DRIVE_SEEK_C   = 0x10, ///< drive seek complete
    /** drive is ready to send or take data */
    ATA_SR_MASK_DATA_REQ_READY = 0x08,
    ATA_SR_MASK_CORRECTED_DATA = 0x04,
    ATA_SR_MASK_INDEX          = 0x02,
    /** the last command failed */
    ATA_SR_MASK_ERROR          = 0x01
} ATA_STATUS_REG_MASKS;

/**
 * bits of the error register
 */
typedef enum {
    ATA_ER_MASK_BAD_BLOCK          = 0x80,
    ATA_ER_MASK_UNCORRECTABLE_DATA = 0x40,
    ATA_ER_MASK_MEDIA_CHANGED      = 0x20,
    ATA_ER_MASK_ID_MARK_NOT_FOUND  = 0x10,
    ATA_ER_MASK_MEDIA_CHANGE_RQ    = 0x08,
    ATA_ER_MASK_COMMAND_ABORTED    = 0x04,
    ATA_ER_MASK_TRACK_0_NOT_FOUND  = 0x02,
    ATA_ER_MASK_NO_ADDRESS_MARK    = 0x01,
} ATA_ERROR_REG_MASKS;

/**
 * what you can write to the command register
 */
typedef enum {
    /** read a sector, LBA28 */
    ATA_CMD_READ_PIO        = 0x20,
    /** read a sector, LBA48 */
    ATA_CMD_READ_PIO_EXT    = 0x24,
    ATA_CMD_READ_DMA        = 0xC8,
    ATA_CMD_READ_DMA_EXT    = 0x25,
    /** write a sector, LBA28 */
    ATA_CMD_WRITE_PIO       = 0x30,
    /** write a sector, LBA48 */
    ATA_CMD_WRITE_PIO_EXT   = 0x34,
    ATA_CMD_WRITE_DMA       = 0xCA,
    ATA_CMD_WRITE_DMA_EXT   = 0x35,
    /** makes the drive write its cache to the disk */
    ATA_CMD_CACHE_FLUSH     = 0xE7,
    /** same, for LBA48 */
    ATA_CMD_CACHE_FLUSH_EXT = 0xEA,
    ATA_CMD_PACKET          = 0xA0,
    /** identify for ATAPI devices */
    ATA_CMD_IDENTIFY_PACKET = 0xA1,
    /** asks the drive to send 512 bytes about itself */
    ATA_CMD_IDENTIFY        = 0xEC
} ATA_COMMANDS;

/**
 * ATA_CMD_IDENTIFY* return a buffer of 512 bytes, how to interpret said buffer:
 *  the values are byte offsets into it
 */
typedef enum {
    /** 16 bit */
    ATA_IDENT_DEVICETYPE   = 0,
    ATA_IDENT_CYLINDERS    = 2,
    ATA_IDENT_HEADS        = 6,
    ATA_IDENT_SECTORS      = 12,
    ATA_IDENT_SERIAL       = 20,
    /** 40 chars, every pair of them is swapped */
    ATA_IDENT_MODEL        = 54,
    /** bit 9 set means the drive does LBA */
    ATA_IDENT_CAPABILITIES = 98,
    ATA_IDENT_FIELDVALID   = 106,
    /** number of sectors, LBA28 */
    ATA_IDENT_MAX_LBA      = 120,
    /** bit 26 set means the drive does LBA48 */
    ATA_IDENT_COMMANDSETS  = 164,
    /** number of sectors, LBA48 */
    ATA_IDENT_MAX_LBA_EXT  = 200
} ATA_IDENTIFICATION_SPACE;

/**
 * the registers of a channel, given to IDE_read/IDE_write
 *  the values under 0x08 are just the offset from the IO base, the
 *  rest are made up and IDE_read/IDE_write work out the real port
 */
typedef enum {
    ATA_REG_DATA       = 0x00, ///< BAR0+0, R/W
    ATA_REG_ERROR      = 0x01, ///< BAR0+1, W
    ATA_REG_FEATURES   = 0x01, ///< BAR0+1, R
    ATA_REG_SECCOUNT0  = 0x02, ///< BAR0+2, R/W
    ATA_REG_LBA0       = 0x03, ///< BAR0+3, R/W
    ATA_REG_LBA1       = 0x04, ///< BAR0+4, R/W
    ATA_REG_LBA2       = 0x05, ///< BAR0+5, R/W
    ATA_REG_HDDEVSEL   = 0x06, ///< BAR0+6, R/W, used to select drive in channel
    ATA_REG_COMMAND    = 0x07, ///< BAR0+7, W
    ATA_REG_STATUS     = 0x07, ///< BAR0+7, R

    /** high order byte of the sector count, LBA48 only */
    ATA_REG_SECCOUNT1  = 0x08,
    /** LBA48 only */
    ATA_REG_LBA3       = 0x09,
    /** LBA48 only */
    ATA_REG_LBA4       = 0x0A,
    /** LBA48 only */
    ATA_REG_LBA5       = 0x0B,
    ATA_REG_CONTROL    = 0x0C, ///< BAR1+2, W
    ATA_REG_ALTSTATUS  = 0x0C, ///< BAR1+2, R
    ATA_REG_DEVADDRESS = 0x0D  ///< BAR1+3, purpose unknown
} ATA_REG_PORT_OFFSETS;

/* the two channels */
#define ATA_PRIMARY   0x00
#define ATA_SECONDARY 0x01
#define ATA_READ      0x00
#define ATA_WRITE     0x01

/* what kind of device it is */
#define IDE_ATA        0x00
#define IDE_ATAPI      0x01
/* which drive on a channel */
#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

/**
 * the ports of one channel
 *
 * BAR0 -> start of ports used by primary channel
 * BAR1 -> start of ports to control primary channel
 * BAR2 -> start of ports used by secondary channel
 * BAR3 -> start of ports to control secondary channel
 * BAR4 -> start of 8 ports which control primary channel IDE
 * BAR4+8 -> start of 8 ports which control secondary channel IDE
 */
typedef struct {
    /** where the command block registers start */
    uint16_t IO_base;
    /** where the control registers start */
    uint16_t CTRL_base;
    uint16_t bus_master_IDE;
    /** what gets written to the control register to keep the nIEN bit set (2 == no irqs) */
    uint16_t no_interrupt;
} IDE_channel_regs_t;



/**
 * @brief reads a register of a channel
 * @param channel ATA_PRIMARY or ATA_SECONDARY
 * @param reg the register
 * @return the value
 */
uint8_t IDE_read(uint8_t channel, ATA_REG_PORT_OFFSETS reg);

/**
 * @brief writes a register of a channel
 * @param channel ATA_PRIMARY or ATA_SECONDARY
 * @param reg the register
 * @param data the value
 */
void    IDE_write(uint8_t channel, ATA_REG_PORT_OFFSETS reg, uint8_t data);

/*
    ATA/ATAPI Read/Write Modes:
        Addressing:
            LBA28
            LBA48
            CHS
        Reading:
            PIO Modes
            Single Word DMA Mode
            Double Word DMA Mode
            Ultra DMA
        Polling:
            - IRQs
            - Polling Status --> suitable for singletasking (us)
*/

/*
    LBA0 -> [0:7]  of LBA28/48  + sector num of CHS
    LBA1 -> [8:15] of LBA28/48  + cylinder[0:7] of CHS
    LBA2 -> [16:23] of LBA28/48 + cylinder[8:15] of CHS
    LBA3 -> [24:31] of LBA48
    LBA4 -> [32:39] of LBA48
    LBA5 -> [40:47] of LBA48

    HDDEVSEL[0:3] -> [28:31] of LBA28

    HDDEVSEL[6] -> LBA : CHS
*/

/**
 * @brief writes one sector to a drive
 * @param drive index of the drive, 0-3
 * @param lba the sector
 * @param buff 512 bytes to write
 * @return 0 on success, 1 if the drive doesn't exist or the lba is too big
 */
uint8_t IDE_ATA_write(uint8_t drive, uint32_t lba, void *buff);

/**
 * @brief reads one sector from a drive
 * @param drive index of the drive, 0-3
 * @param lba the sector
 * @param buff where the 512 bytes go
 * @return 0 on success, 1 if the drive doesn't exist or the lba is too big
 */
uint8_t IDE_ATA_read (uint8_t drive, uint32_t lba, void *buff);

/* bytes in a sector */
#define IDE_ATA_SECTOR_SIZE 512

/**
 * @brief f_WriteSector for ide drives
 * @param device a device from IDE_init, drive_num is the drive
 * @param buff 512 bytes to write
 * @param lba the sector
 */
void ATA_write_sector(storage_device_t *device, const void *buff, uint32_t lba);

/**
 * @brief f_ReadSector for ide drives
 * @param device a device from IDE_init, drive_num is the drive
 * @param buff where the 512 bytes go
 * @param lba the sector
 */
void ATA_read_sector (storage_device_t *device, void *buff, uint32_t lba);

#endif

/**
 * @brief finds the ata drives on both channels (master and slave
 *  each) with IDENTIFY and registers them with the SAL
 *  needs the PIT
 */
void IDE_init();

#endif
