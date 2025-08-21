#include "../include/pit.h"
#include "../include/rtc.h"
#include "../../libc/include/stdlib.h"

#define TIMER_IRQ 0x0

void PIT_init(){
    PIT_set_freq(1000);

    PIC_unmask(TIMER_IRQ);
}

void PIT_set_freq(int hz){
    int divisor = 1193182 / hz;
    port_write_byte(0x43, 0x36);
    port_write_byte(0x40, divisor & 0xFF);
    port_write_byte(0x40, divisor >> 8);
}

static bool carry_over;
static uint64_t ticks;

uint16_t inc_unit_of_time(uint16_t unit, uint16_t max){
    if (unit == max){
        carry_over = true;
        return 0;
    }
    else {
        carry_over = false;
        return ++unit;
    }
}

uint8_t max_monthday(uint16_t year, uint8_t month){
    if (month == 2){
        if (year % 4) return 28;
        else          return 29;
    }
    if (month % 2) return 30;
    else           return 31;
}

void inc_day_and_month(uint16_t year, uint8_t *month, uint8_t *monthday){
    if (*monthday == max_monthday(year, *month)){
        *monthday = 0;
        if (*month == 12){
            *month = 0;
            carry_over = true;
        }
        else
            (*month)++;
    }
    else
        (*monthday)++;
}

void IRQ_time_handler(){
    ++ticks;
    PIC_end_of_int(TIMER_IRQ);

    if (ticks % 1000) return;

    time.seconds = inc_unit_of_time(time.seconds, 59);
    if (carry_over) time.minutes = inc_unit_of_time(time.minutes, 59);
    if (carry_over) time.hours = inc_unit_of_time(time.hours, 23);
    if (carry_over) inc_day_and_month(time.year, &time.month, &time.monthday);
    if (carry_over) time.year = inc_unit_of_time(time.year, -1);
}

void PIT_sleep(uint64_t ms){
    uint64_t start = ticks;
    
    while (ticks - start < ms);
}
