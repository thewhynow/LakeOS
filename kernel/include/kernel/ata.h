#ifndef _ATA_H
#define _ATA_H

#include "../../../libc/include/types.h"
#include "io.h"

#ifdef _ATA_H_INTERNAL

/* Status/Command port returns information regarding status of channel when read */
typedef enum {
    ATA_SR_MASK_BUSY           = 0x80,
    ATA_SR_MASK_DRIVE_READY    = 0x40,
    ATA_SR_MASK_DRIVE_FAULT    = 0x20,
    ATA_SR_MASK_DRIVE_SEEK_C   = 0x10, /* drive seek complete */
    ATA_SR_MASK_DATA_REQ_READY = 0x08,
    ATA_SR_MASK_CORRECTED_DATA = 0x04,
    ATA_SR_MASK_INDEX          = 0x02,
    ATA_SR_MASK_ERROR          = 0x01
} ATA_STATUS_REG_MASKS;

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

typedef enum {
    ATA_CMD_READ_PIO        = 0x20,
    ATA_CMD_READ_PIO_EXT    = 0x24,
    ATA_CMD_READ_DMA        = 0xC8,
    ATA_CMD_READ_DMA_EXT    = 0x25,
    ATA_CMD_WRITE_PIO       = 0x30,
    ATA_CMD_WRITE_PIO_EXT   = 0x34,
    ATA_CMD_WRITE_DMA       = 0xCA,
    ATA_CMD_WRITE_DMA_EXT   = 0x35,
    ATA_CMD_CACHE_FLUSH     = 0xE7,
    ATA_CMD_CACHE_FLUSH_EXT = 0xEA,
    ATA_CMD_PACKET          = 0xA0,
    ATA_CMD_IDENTIFY_PACKET = 0xA1,
    ATA_CMD_IDENTIFY        = 0xEC
} ATA_COMMANDS;

/* ATA_CMD_IDENTIFY* return a buffer of 512 bytes, how to interpret said buffer: */
typedef enum {
    ATA_IDENT_DEVICETYPE   = 0,
    ATA_IDENT_CYLINDERS    = 2,
    ATA_IDENT_HEADS        = 6,
    ATA_IDENT_SECTORS      = 12,
    ATA_IDENT_SERIAL       = 20,
    ATA_IDENT_MODEL        = 54,
    ATA_IDENT_CAPABILITIES = 98,
    ATA_IDENT_FIELDVALID   = 106,
    ATA_IDENT_MAX_LBA      = 120,
    ATA_IDENT_COMMANDSETS  = 164,
    ATA_IDENT_MAX_LBA_EXT  = 200
} ATA_IDENTIFICATION_SPACE;

typedef enum {
    ATA_REG_DATA       = 0x00, /* BAR0+0, R/W */
    ATA_REG_ERROR      = 0x01, /* BAR0+1, W */
    ATA_REG_FEATURES   = 0x01, /* BAR0+1, R */
    ATA_REG_SECCOUNT0  = 0x02, /* BAR0+2, R/W */
    ATA_REG_LBA0       = 0x03, /* BAR0+3, R/W */
    ATA_REG_LBA1       = 0x04, /* BAR0+4, R/W */
    ATA_REG_LBA2       = 0x05, /* BAR0+5, R/W */
    ATA_REG_HDDEVSEL   = 0x06, /* BAR0+6, R/W, used to select drive in channel */
    ATA_REG_COMMAND    = 0x07, /* BAR0+7, W */
    ATA_REG_STATUS     = 0x07, /* BAR0+7, R */

    ATA_REG_SECCOUNT1  = 0x08, 
    ATA_REG_LBA3       = 0x09, 
    ATA_REG_LBA4       = 0x0A, 
    ATA_REG_LBA5       = 0x0B,
    ATA_REG_CONTROL    = 0x0C, /* BAR1+2, W */
    ATA_REG_ALTSTATUS  = 0x0C, /* BAR1+2, R */
    ATA_REG_DEVADDRESS = 0x0D  /* BAR1+3, purpose unknown */
} ATA_REG_PORT_OFFSETS;

#define ATA_PRIMARY   0x00
#define ATA_SECONDARY 0x01
#define ATA_READ      0x00
#define ATA_WRITE     0x01

/*
BAR0 -> start of ports used by primary channel
BAR1 -> start of ports to control primary channel
BAR2 -> start of ports used by secondary channel
BAR3 -> start of ports to control secondary channel
BAR4 -> start of 8 ports which control primary channel IDE
BAR4+8 -> start of 8 ports which control secondary channel IDE
*/
struct IDE_channel_regs_t {
    uint16_t IO_base;
    uint16_t CTRL_base;
    uint16_t bus_master_IDE;
    uint16_t no_interrupt;
} channels[2];

uint8_t ide_buff[2048];
uint8_t ide_irq_fired;
uint8_t atapi_packet[12] = {[0] = 0xA8};

struct IDE_device {
    /* 0 (empty) 1 (drive exists) */
    uint8_t exists;
    /* 0 (primary channel) 1 (secondary channel) */
    uint8_t channel;
    /* 0 (master drive) 1 (slave drive) */
    uint8_t is_slave_drive;
    /* 0 (ATA) 1 (ATAPI) */
    uint16_t type;
    uint16_t signature;
    uint16_t features;
    uint32_t command_sets;
    /* size in sectors */
    uint32_t size;
    /* model name as string */
    char model[41];
} IDE_devices[4];

uint8_t IDE_read(uint8_t channel, ATA_REG_PORT_OFFSETS reg);
void IDE_write(uint8_t channel, ATA_REG_PORT_OFFSETS reg, uint8_t data);

#endif

#endif