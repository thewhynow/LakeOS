/**
 * @file rtc.h
 * @brief the cmos real time clock, read once at boot. after that
 *  the pit keeps `time` ticking (see pit.h)
 */

#include "../../libc/include/types.h"

#ifndef _RTC_H
#define _RTC_H

/**
 * @brief reads the time from the cmos and fills in `time`
 *  reads until two reads in a row match so it doesn't catch an
 *  update halfway, converts bcd and 12-hour mode if the rtc is
 *  using them, and shifts it to EST (UTC-4)
 */
void RTC_init();

/**
 * a date and time, the year is the full year (2025 not 25)
 *  and hours are 24-hour
 */
typedef struct {
    uint16_t year;
    uint8_t  seconds,   ///< 0-59
             minutes,   ///< 0-59
             hours,     ///< 0-23
             weekday,
             monthday,  ///< 1-31
             month;     ///< 1-12
} t_RTCTime;

/**
 * the current time, only valid after RTC_init
 */
extern t_RTCTime time;

#endif

#ifdef _RTC_H_INTERNAL

/* the cmos is accessed by writing a register number to the select port, then reading the data port */
#define CMOS_REG_SELECT 0x70
#define CMOS_DATA_REG   0x71

/**
 * cmos register numbers for the time
 */
typedef enum {
    RTC_REG_SECONDS = 0x00 /* 0-59 */,
    RTC_REG_MINUTES = 0x02 /* 0-59 */,
    /**
     * 0-23 in 24-hour mode,
     * 0-12 in 12-hour mode,
     *  pm -> high bit set
     */
    RTC_REG_HOURS   = 0x04,
    /** 1-7 */
    RTC_REG_WEEKDAY = 0x06,
    RTC_REG_MONTHDAY= 0x07 /* 1-31 */,
    RTC_REG_MONTH   = 0x08 /* 1-12 */,
    RTC_REG_YEAR    = 0x09 /* 0-99 */,
    /**
     * maybe?
     *  19-20
     */
    RTC_REG_CENTURY = 0x32,
    /** has the update in progress bit */
    RTC_REG_STATUSA = 0x0A,
    /** has the 24-hour and binary format bits */
    RTC_REG_STATUSB = 0x0B
} e_RTCRegisters;

/**
 * bits of the status registers
 */
typedef enum {
    /** (status a) set while the rtc is changing the time, don't read then */
    RTC_REGA_UPD_IN_PROGRESS = 0b01000000,
    /** (status b) set -> hours are 0-23, clear -> 12-hour mode */
    RTC_REGB_24_HOUR_FORMAT  = 0b00000010,
    /** (status b) set -> values are binary, clear -> bcd */
    RTC_REGB_BINARY_FORMAT   = 0b00000100,
} e_RTCRegMasks;

/* the current year, used to work out the century since the century register isn't reliable */
#define CURR_YEAR 2025

#endif
