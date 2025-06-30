#define _ATA_H_INTERNAL
#include "../../include/kernel/ata.h"

uint8_t IDE_read(uint8_t channel, ATA_REG_PORT_OFFSETS reg){
    uint8_t res;
    
    if (0x07 < reg && reg < 0x0C)
        IDE_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].no_interrupt);
    if (reg < 0x08)
        res = port_read_byte(channels[channel].IO_base + reg); else
    if (reg < 0x0C)
        res = port_read_byte(channels[channel].IO_base + reg - 0x06); else
    if (reg < 0x0E)
        res = port_read_byte(channels[channel].IO_base + reg - 0x0A); else
    if (reg < 0x16)
        res = port_read_byte(channels[channel].IO_base + reg - 0x0E);
}