#include "../../libc/include/types.h"

#ifndef _RTC_H
#define _RTC_H

void RTC_init();

typedef struct {
    uint16_t year;
    uint8_t  seconds,
             minutes,
             hours,
             weekday,
             monthday,
             month;
} t_RTCTime;

extern t_RTCTime time;

#endif

#ifdef _RTC_H_INTERNAL

#define CMOS_REG_SELECT 0x70
#define CMOS_DATA_REG   0x71

typedef enum {
    RTC_REG_SECONDS = 0x00 /* 0-59 */,
    RTC_REG_MINUTES = 0x02 /* 0-59 */,
    /**
     * 0-23 in 24-hour mode,
     * 0-12 in 12-hour mode,
     *  pm -> high bit set
     */
    RTC_REG_HOURS   = 0x04,
    RTC_REG_WEEKDAY = 0x06 /* 1-7 */,
    RTC_REG_MONTHDAY= 0x07 /* 1-31 */,
    RTC_REG_MONTH   = 0x08 /* 1-12 */,
    RTC_REG_YEAR    = 0x09 /* 0-99 */,
    /**
     * maybe?
     *  19-20
     */
    RTC_REG_CENTURY = 0x32,
    RTC_REG_STATUSA = 0x0A,
    RTC_REG_STATUSB = 0x0B
} e_RTCRegisters;

typedef enum {
    RTC_REGA_UPD_IN_PROGRESS = 0b01000000,
    RTC_REGB_24_HOUR_FORMAT  = 0b00000010,
    RTC_REGB_BINARY_FORMAT   = 0b00000100,
} e_RTCRegMasks;

#define CURR_YEAR 2025

#endif
