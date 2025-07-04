#include "../include/pit.h"

static uint64_t time;

#define TIMER_IRQ 0x0

void PIT_init(){
    /* one pulse per millisecond */
    PIT_set_freq(1000);

    PIC_unmask(TIMER_IRQ);
}

void PIT_set_freq(int hz){
    int divisor = 1193180 / hz;
    port_write_byte(0x43, 0x36);
    port_write_byte(0x40, divisor & 0xFF);
    port_write_byte(0x40, divisor >> 8);
}

void IRQ_time_handler(){
    ++time;
    PIC_end_of_int(TIMER_IRQ);
}

void PIT_sleep(uint64_t ms){
    uint64_t start = time;
    
    while (time - start < ms);
}