#define _RTC_H_INTERNAL
#define _PIT_H_INTERNAL
#include "../include/rtc.h"
#include "../include/pit.h"
#include "../include/io.h"
#include "../../libc/include/stdlib.h"

t_RTCTime time;

void CMOS_write(uint8_t port, uint8_t data){
    port_write_byte(CMOS_REG_SELECT, port);
    port_write_byte(CMOS_DATA_REG, data);
}

uint8_t CMOS_read(uint8_t port){
    port_write_byte(CMOS_REG_SELECT, port);
    return port_read_byte(CMOS_DATA_REG);
}

bool comp_time(t_RTCTime *timea, t_RTCTime *timeb){
    /* dont compare century as it is not guranteed */
    return
        timea->seconds  == timeb->seconds  &&
        timea->minutes  == timeb->minutes  &&
        timea->hours    == timeb->hours    &&
        timea->monthday == timeb->monthday &&
        timea->month    == timeb->month    &&
        timea->year     == timeb->year;
}

static bool carry;
uint16_t RTC_sub_unit(uint16_t unit, uint16_t max){
    if ((carry = !unit))
        return max;
    else
        return unit - 1;
}

void RTC_sub_month_and_day(uint16_t year, uint8_t *month, uint8_t *monthday){
    if (*monthday){
        (*monthday)--;
        return;
    }

    if (*month){
        (*month)--;
        *monthday = max_monthday(year, *month);
    }
    else {
        *month = 12;
        *monthday = max_monthday(year, 12);
        carry = true;
    }
}

void RTC_sub_hours(t_RTCTime *time, uint8_t num_hours){
    uint8_t carry = 0;
    if (time->hours >= num_hours)
        time->hours -= num_hours;
    else {
        time->hours = (time->hours + 24) - num_hours;          
        RTC_sub_month_and_day(time->year, &time->month, &time->monthday);
        if (carry) --time->year;
    }
}

void RTC_read_time(t_RTCTime *time){
    *time = (t_RTCTime){
        .seconds  = CMOS_read(RTC_REG_SECONDS),
        .minutes  = CMOS_read(RTC_REG_MINUTES),
        .hours    = CMOS_read(RTC_REG_HOURS),
        .monthday = CMOS_read(RTC_REG_MONTHDAY),
        .month    = CMOS_read(RTC_REG_MONTH),
        .year     = CMOS_read(RTC_REG_YEAR),
    };
}

void RTC_wait_update(){
    while(CMOS_read(RTC_REG_STATUSA) & RTC_REGA_UPD_IN_PROGRESS);
}

uint8_t BCD_to_bin(uint8_t x){
    return (x & 0x0F) + ((x / 16) * 10);
}

void RTC_init(){
    RTC_wait_update();
    RTC_read_time(&time); 

    t_RTCTime tmp_time;
    do {
        tmp_time = time;
        RTC_wait_update();
        RTC_read_time(&time);
    } while(!comp_time(&tmp_time, &time));
    
    uint8_t regb = CMOS_read(RTC_REG_STATUSB);

    /* convert BCD to binary */
    if (!(regb & RTC_REGB_BINARY_FORMAT)){
        time = (t_RTCTime){
            .seconds  = BCD_to_bin(time.seconds),
            .minutes  = BCD_to_bin(time.minutes),
            .hours    = BCD_to_bin(time.hours),
            .monthday = BCD_to_bin(time.monthday),
            .month    = BCD_to_bin(time.month),
            .year     = BCD_to_bin(time.year)
        };
    }

    /* convert 24-hour to 12 */
    if (!(regb & RTC_REGB_24_HOUR_FORMAT) && (time.hours & 0x80))
        time.hours = ((time.hours & 0x7F) + 12) % 24;

    time.year += (CURR_YEAR / 100) * 100;
    if (time.year < CURR_YEAR) time.year += 100; 

    /* since EST is UTC-4 */
    RTC_sub_hours(&time, 4);
}
