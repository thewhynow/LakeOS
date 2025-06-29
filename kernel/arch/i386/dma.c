#include "../../include/kernel/dma.h"
#include "../../include/kernel/io.h"

void DMA_init(){
    port_write_byte(DMA1_COMMAND_REG, 0x0);
}

void DMA_set_full_address(uint8_t channel, uint32_t addr){
    uint16_t port = 0;

    switch (channel){
        case 0: port = DMA0_CHAN0_ADDR_REG; break;
        case 1: port = DMA0_CHAN1_ADDR_REG; break;
        case 2: port = DMA0_CHAN2_ADDR_REG; break;
        case 3: port = DMA0_CHAN3_ADDR_REG; break;
        case 4: return;
        case 5: port = DMA1_CHAN5_ADDR_REG; break;
        case 6: port = DMA1_CHAN6_ADDR_REG; break;
        case 7: port = DMA1_CHAN7_ADDR_REG; break;
        default: return;
    }

    union {
        uint8_t bytes[4];
        uint32_t addr;
    } a = {
        .addr = addr
    };

    port_write_byte(port, a.bytes[0]); /* 0 byte */
    port_write_byte(port, a.bytes[1]); /* 1 byte */
    DMA_set_page(channel, a.bytes[2]); /* 2 byte */
}

void DMA_set_count(uint8_t channel, uint16_t count){
    uint16_t port;

    switch (channel){
        case 0: port = DMA0_CHAN0_COUNT_REG; break;
        case 1: port = DMA0_CHAN1_COUNT_REG; break;
        case 2: port = DMA0_CHAN2_COUNT_REG; break;
        case 3: port = DMA0_CHAN3_COUNT_REG; break;
        case 4: return;
        case 5: port = DMA1_CHAN5_COUNT_REG; break;
        case 6: port = DMA1_CHAN6_COUNT_REG; break;
        case 7: port = DMA1_CHAN7_COUNT_REG; break;
        default: return;
    }

    union {
        uint8_t bytes[2];
        uint16_t count;
    } c = {
        .count = count
    };

    port_write_byte(port, c.bytes[0]); /* low byte */
    port_write_byte(port, c.bytes[1]); /* high byte */
}

void DMA_set_page(uint8_t channel, uint8_t page){
    uint16_t port;

    switch(channel){
        case 0: port = DMA_PAGE_CHAN0; break;
        case 1: port = DMA_PAGE_CHAN1; break;
        case 2: port = DMA_PAGE_CHAN2; break;
        case 3: port = DMA_PAGE_CHAN3; break;
        case 4: return;
        case 5: port = DMA_PAGE_CHAN5; break;
        case 6: port = DMA_PAGE_CHAN6; break;
        case 7: port = DMA_PAGE_CHAN7; break;
        default: return;
    }

    port_write_byte(port, page);
}

void DMA_set_mask(uint8_t channel, bool mask){
    if (mask) {
        if (channel <= 4)
            port_write_byte(DMA0_CHANMASK_REG, 0b100 | channel);
        else
            port_write_byte(DMA1_CHANMASK_REG, 0b100 | (channel - 4));
        }
    else {
            if (channel <= 4)
                port_write_byte(DMA0_CHANMASK_REG, channel);
            else
                port_write_byte(DMA1_CHANMASK_REG, channel - 4);        
    }
}

void DMA_set_mode(uint8_t channel, uint8_t mode){
    DMA_set_mask(channel, 1);

    port_write_byte (
        channel < 4 ? DMA0_MODE_REG : DMA1_MODE_REG,
        channel - 4 * (channel >= 4) | mode
    );

    DMA_set_mask(channel, 0);
}

void DMA_reset_flipflop(uint8_t dma){
    port_write_byte(
        dma ? DMA1_CLEARBYTE_FLIPFLOP_REG : DMA0_CLEARBYTE_FLIPFLOP_REG,
        0x00
    );
}

void DMA_reset(uint8_t dma){
    port_write_byte(
        dma ? DMA1_MASTER_CLEAR_REG : DMA0_MASTER_CLEAR_REG,
        0x00
    );
}

void DMA_unmask_all(){
    port_write_byte(DMA1_UNMASK_ALL_REG, 0xFF);
}