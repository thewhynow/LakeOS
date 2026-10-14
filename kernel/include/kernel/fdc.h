/**
 * @file fdc.h
 * @brief the floppy disk controller, 1.44MB 3.5" drives only, reads
 *  and writes go through a one sector buffer using dma (dma.h)
 */

#ifndef _FDC_H
#define _FDC_H

#include "../../libc/include/types.h"
#include "../../libc/include/stdlib.h"
#include "sal.h"

/**
 * @brief looks in the cmos for floppy drives, registers the ones
 *  it finds with the SAL, and resets the controller
 *  does nothing if there are none
 */
void FDC_init();

/**
 * @brief IRQ6 handler, the controller raises it when a command is
 *  done. flags it for FDC_irq_wait and sends the end of interrupt
 */
void IRQ_FDC_handler();

#ifdef _FDC_H_INTERNAL

/**
 * @brief f_ReadSector for floppy drives
 * @param device a device from FDC_init, drive_num is the drive
 * @param buff where the 512 bytes go
 * @param lba the sector
 */
void FDC_read_sector(storage_device_t *device, void *buff, uint32_t lba);

/**
 * @brief f_WriteSector for floppy drives
 * @param device a device from FDC_init, drive_num is the drive
 * @param buff 512 bytes to write
 * @param lba the sector
 */
void FDC_write_sector(storage_device_t *device, const void *buff, uint32_t lba);

/**
 * @brief selects the drive the next commands talk to
 * @param drive 0-3
 */
void FDC_set_drive(uint8_t drive);

/* bytes in a sector */
#define FLOPPY_BYTES_PER_SECTOR 512

/**
 * @brief sends one byte of a command, waits until the controller
 *  is ready for it
 * @param command the byte
 */
static void FDC_write_cmd(uint8_t command);

/**
 * @brief points the dma channel at the sector buffer, for one sector
 */
static void FDC_DMA_init();

/**
 * @brief reads one byte of a command result, waits until the
 *  controller has one
 * @return the byte
 */
static uint8_t FDC_read_data();

/**
 * @brief runs the check interrupt command, which gives the result of
 *  the last seek/calibrate. hangs if st0 comes back as 0x80
 * @param st0 gets status register 0
 * @param cylinder gets the cylinder the head is on
 */
static void FDC_check_int(uint8_t *st0, uint8_t *cylinder);

/**
 * @brief spins up a drive's motor
 * @param drive 0-3
 */
static void FDC_start_motor(uint8_t drive);

/**
 * @brief stops the motors
 */
static void FDC_stop_motor();

/**
 * @brief turns the controller off (the reset bit in the dor)
 */
static void FDC_disable();

/**
 * @brief turns the controller on, in dma mode with the current drive
 */
static void FDC_enable();

/**
 * @brief waits for the controller's irq, and clears the flag
 */
static void FDC_irq_wait();

/**
 * @brief the specify command, gives the controller the drive timings
 * @param stepr step rate
 * @param loadt head load time
 * @param unloadt head unload time
 * @param dma true for dma mode, false for irq mode
 */
static void FDC_CMD_specify(uint32_t stepr, uint32_t loadt, uint32_t unloadt, bool dma);

/**
 * @brief moves the head to cylinder 0, retries 10 times
 * @param drive 0-3
 * @return 0 on success, -1 if it never got there
 */
static int FDC_CMD_callibrate(uint8_t drive);

/**
 * @brief moves the head to a cylinder, retries 10 times
 * @param cyl the cylinder
 * @param head 0 or 1
 * @return 0 on success, -1 if it never got there
 */
static int FDC_CMD_seek(uint8_t cyl, uint8_t head);

/* floppy is on dma channel 2 and irq 6, and a track has 18 sectors */
#define FLOPPY_CHANNEL 2
#define FLOPPY_IRQ 6
#define FLOPPY_SECTORS_PER_TRACK 18

/**
 * FDC REGISTERS
 * 
 * DIGITAL OUTPUT REGISTER (DOR)
 *  [0:1] = Drive Select (0, 1, 2, 3)
 *  [2] = Reset (0 = reset, 1 = enabled)
 *  [3] = Mode (0 = IRQ, 1 = DMA)
 *  [4] = Drive 0 Motor Control (0 = stop, 1 = start)
 *  [5] = Drive 1 Motor Control 
 *  [6] = Drive 2 Motor Control  
 *  [7] = Drive 3 Motor Control 
 * 
 * MAIN STATUS REGISTER (MSR)
 *  [0] = FDD 0 (0 = not busy, 1 = busy)
 *  [1] = FDD 1
 *  [2] = FDD 2
 *  [3] = FDD 3
 *  [4] = FDC Busy (0 = not busy, 1 = busy)
 *  [5] = FDC !(DMA Mode) (0 = DMA, 1 = IRQ)
 *  [6] = DIO (0 = expects read, 1 = expects write)
 *  [7] = MQR (0 = FIFO not ready, 1 = FIFO ready for data write/read)
 * 
 * CONFIGURATION CONTROL REGISTER (CCR)
 *  [0:2] = Data Rate Select
 *      00 -> 500 KBPS
 *      10 -> 250 KBPS
 *      01 -> 300 KBPS
 *      11 -> 1   MBPS
 */

/**
 * the io ports of the controller
 */
typedef enum {
    /** digital output register */
    FDC_DOR	 = 0x3F2,
    /** main status register */
    FDC_MSR	 = 0x3F4,
    /** where command bytes go and results come from */
    FDC_FIFO = 0x3F5,
    /** configuration control register (CCR) */
    FDC_CTRL = 0x3F7
} FDC_IO;

/**
 * bits of the digital output register, or them together
 */
typedef enum {
    FDC_DOR_MASK_DRIVE0		  = 0b00000000,
    FDC_DOR_MASK_DRIVE1		  =	0b00000001,
    FDC_DOR_MASK_DRIVE2		  =	0b00000010,
    FDC_DOR_MASK_DRIVE3		  =	0b00000011,
    /** controller is on, clear holds it in reset */
    FDC_DOR_MASK_RESET		  =	0b00000100,
    /** dma mode, clear is irq mode */
    FDC_DOR_MASK_DMA		  =	0b00001000,
    FDC_DOR_MASK_IRQ		  =	0b00000000,
    /** motor of drive 0 is on */
    FDC_DOR_MASK_DRIVE0_MOTOR =	0b00010000,
    /** motor of drive 1 is on */
    FDC_DOR_MASK_DRIVE1_MOTOR =	0b00100000,
    /** motor of drive 2 is on */
    FDC_DOR_MASK_DRIVE2_MOTOR =	0b01000000,
    /** motor of drive 3 is on */
    FDC_DOR_MASK_DRIVE3_MOTOR =	0b10000000
} FDC_DOR_MASK;

/**
 * bits of the main status register
 */
typedef enum {
    FDC_MSR_MASK_DRIVE0_SEEK    = 0b00000001,
    FDC_MSR_MASK_DRIVE1_SEEK    = 0b00000010,
    FDC_MSR_MASK_DRIVE2_SEEK    = 0b00000100,
    FDC_MSR_MASK_DRIVE3_SEEK    = 0b00001000,
    FDC_MSR_MASK_BUSY   	    = 0b00010000,
    FDC_MSR_MASK_IRQ_MODE	    = 0b00100000,
    /** (DIO) set when the controller has data for us to read, clear when it wants data written */
    FDC_MSR_MASK_HAS_DATA       = 0b01000000,
    /** (MQR) the fifo is ready for a read or write */
    FDC_MSR_MASK_DATA_READY     = 0b10000000
} FDC_MSR_MASK;

/**
 * the commands the controller takes, sent a byte at a time through
 *  the fifo. each one is documented with its bytes: the parameters
 *  go after the command byte, the return comes back after
 */
typedef enum {
    /**
     * Read / Write Sector
     *  reads or writes a sector from/to the FDD into the DMA-specified memory
     *  format: M F 0 0 1 1 0
     *  parameters:
     *      x x x x x HD DR1 DR0
     *      Cylinder
     *      Head
     *      Sector Number
     *      Sector Size
     *      Track Length / Max Sector Number
     *      Length of GAP3
     *      Data Length
     *  return:
     *      ST0
     *      ST1
     *      ST2
     *      Current Cylinder
     *      Current Head
     *      Sector Number
     *      Sector Size
     */
    FDC_CMD_READ_SECT	 = 0x6,
    /** write a sector, same format as FDC_CMD_READ_SECT */
    FDC_CMD_WRITE_SECT	 = 0x5,

    /**
     * Fix Drive Data
     *  pass controlling info about the mechanical drive to FDC
     *  format: 0 0 0 0 0 0 1 1
     *  parameters:
     *      S S S S H H H H   : S = Step Rate, H = Head Unload Time
     *      H H H H H H H NDM : H = Head Load Time, NDM = !(DMA Mode)
     *              
     *      Step Rate, Head Unload Time, Head Load Time dependent on Data Transfer Rate in CCR
     *      https://www.isdaman.com/alsos/hardware/fdc/floppy.htm 
     *  return: None
     */
    FDC_CMD_SPECIFY		 = 0x3,

    /**
     * Check Drive Status
     *  format: 0 0 0 0 0 1 0 0
     *  parameters:
     *      x x x x x HD DR1 DR0
     *  return:
     *      Status Register 3
     */
    FDC_CMD_CHECK_STAT	 = 0x4,

    /**
     * Calibrate Drive
     *  moves the read/write head to cylinder 0, check if on right track after completion
     *  format:  0 0 0 0 0 1 1 1
     *  parameters:
     *      x x x x x 0 DR1 DR0
     *  return: None
     */
    FDC_CMD_CALIBRATE	 = 0x7,

    /**
     * Check Interrupt Status
     *  check the status of the FDC when interrupt returns
     *  format: 0 0 0 0 1 0 0 0
     *  parameters: None
     *  return:
     *      Status Register 0
     *      Current Cylinder
     */
    FDC_CMD_CHECK_INT	 = 0x8,

    /**
     * Seek Head
     *  move the read/write head to a specific cylinder, check if on right one after completion
     *  format: 0 0 0 0 1 1 1 1
     *  parameters:
     *      x x x x x HD DR1 DR0
     *      Cylinder
     *  return: None
     */
    FDC_CMD_SEEK		 = 0xF,
} FDC_COMMAND;

/**
 * EXTENDED COMMAND BITS
 * MultiTrack Operation (M)
 *  0 = one track, 1 = both tracks
 * FM/MFM Mode Setting (F)
 *  0 = FM (Single Density), 1 = MFM (Double Density)
 * Skip Mode Setting (S)
 *  0 = Skip Deleted Address Marks, 1 = Do Not Skip Deleted Address marks
 * Head Number (HD)
 * Drive Number Bits [DR0:DR1]
 *  00 -> Drive 0
 *  01 -> Drive 1
 *  10 -> Drive 2
 *  11 -> Drive 3
 *
 * or'd onto the command byte
 */
typedef enum {
    FDC_CMD_EXT_SKIP      = 0b00100000, ///< S
    FDC_CMD_EXT_DENSITY    = 0b01000000, ///< F
    FDC_CMD_EXT_MULTITRACK = 0b10000000 ///< M
} FDC_COMMAND_EXT;

/**
 * the length of gap3 (the gap between sectors) for read/write
 */
typedef enum {
    FDC_CMD_GAP3_LENGTH_STD  = 42,
    FDC_CMD_GAP3_LENGTH_5_14 = 32,
    /** gap3 length for 3.5" drives */
    FDC_CMD_GAP3_LENGTH_3_5  = 27
} FDC_COMMAND_GAP3_LEN;

/**
 * the sector size code that read/write take
 */
typedef enum {
    FDC_CMD_DTL_128  = 0,
    FDC_CMD_DTL_256  = 1,
    /** 512 byte sectors */
    FDC_CMD_DTL_512  = 2,
    FDC_CMD_DTL_1024 = 4
} FDC_COMMAND_DTL;

#endif
#endif