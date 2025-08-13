#define _ATA_H_INTERNAL
#include "../include/ata.h"
#include "../include/sal.h"

IDE_channel_regs_t channels[2];

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

uint8_t IDE_read(uint8_t channel, ATA_REG_PORT_OFFSETS reg){
    uint8_t res = 0;
    
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
    if (reg > 0x07 && reg < 0x0C)
        IDE_write(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);
    
    return res;
}

void IDE_write(uint8_t channel, ATA_REG_PORT_OFFSETS reg, uint8_t data){
    if (reg > 0x07 && reg < 0x0C)
        IDE_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].no_interrupt);
    if (reg < 0x08)
        port_write_byte(channels[channel].IO_base  + reg - 0x00, data);
    else if (reg < 0x0C)
        port_write_byte(channels[channel].IO_base  + reg - 0x06, data);
    else if (reg < 0x0E)
        port_write_byte(channels[channel].CTRL_base  + reg - 0x0A, data);
    else if (reg < 0x16)
        port_write_byte(channels[channel].bus_master_IDE + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        IDE_write(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);
}

void IDE_read_ident(uint8_t channel, uint32_t *buff){
    for (int i = 0; i < 128; ++i)
        buff[i] = port_read_long(channels[channel].IO_base + ATA_REG_DATA);
}

uint8_t IDE_poll(uint8_t channel, bool adv_check){
    /* delay a little more than 400ns */
    PIT_sleep(1);

    while (IDE_read(channel, ATA_REG_STATUS) & ATA_SR_MASK_BUSY);

    if (adv_check){
        uint8_t state  = IDE_read(channel, ATA_REG_STATUS);

        if (state & ATA_SR_MASK_ERROR)
            return 2;
        
        if (state & ATA_SR_MASK_DRIVE_FAULT)
            return 1;
        
        if (!(state & ATA_SR_MASK_DATA_REQ_READY))
            return 3;
    }

    return 0;
}

void IDE_init(){
    channels[ATA_PRIMARY] = (IDE_channel_regs_t){
        .IO_base        = 0x1F0,
        .CTRL_base      = 0x3F6,
        .bus_master_IDE = 0
    };

    channels[ATA_SECONDARY] = (IDE_channel_regs_t){
        .IO_base        = 0x170,
        .CTRL_base      = 0x376,
        .bus_master_IDE = 8
    };

    /* mask the IRQs (set nIEN in the control port) */
    IDE_write(ATA_PRIMARY  , ATA_REG_CONTROL, 2);
    IDE_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    int count = 0;
    for (int i = 0; i < 2; ++i){
        for (int j = 0; j < 2; ++j){
            uint8_t err = 0,
                    type = IDE_ATA,
                    status;

            /* select drive */
            IDE_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
            PIT_sleep(1);

            /* send ATA identity command */
            IDE_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            PIT_sleep(1);

            /* poll */
            if (!IDE_read(i, ATA_REG_STATUS))
                continue;

            while (true){
                status = IDE_read(i, ATA_REG_STATUS);
                /* if error, device is not ATA */
                if (status & ATA_SR_MASK_ERROR){
                    err = 1;
                    break;
                }
                /* we're all good */
                if (!(status & ATA_SR_MASK_BUSY) && (status & ATA_SR_MASK_DATA_REQ_READY))
                    break;
            }
        
            if (!err){
                uint8_t cl = IDE_read(i, ATA_REG_LBA1);
                uint8_t ch = IDE_read(i, ATA_REG_LBA2);

                if ((cl == 0x14 && ch == 0xEB) || (cl == 0x69 && ch == 0x96)){
                    type = IDE_ATAPI;
                    IDE_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                    PIT_sleep(1);
                }
            }
            else continue;

            IDE_read_ident(i, (uint32_t*) ide_buff);

            IDE_devices[count] = (struct IDE_device){
                .exists         = 1,
                .type           = type,
                .channel        = i,
                .is_slave_drive = j,
                .signature      = *(uint16_t*)(ide_buff + ATA_IDENT_DEVICETYPE),
                .features       = *(uint16_t*)(ide_buff + ATA_IDENT_CAPABILITIES),
                .command_sets   = *(uint32_t*)(ide_buff + ATA_IDENT_COMMANDSETS)
            };

            /* get the size */
            
            if (IDE_devices[count].command_sets & (1 << 26))
                /* 48-bit addressing */
                IDE_devices[count].size = *(uint32_t*)(ide_buff + ATA_IDENT_MAX_LBA_EXT);
            else
                IDE_devices[count].size = *(uint32_t*)(ide_buff + ATA_IDENT_MAX_LBA);

            /* get model string of device */
            for (int k = 0; k < 40; k += 2){
                IDE_devices[count].model[k] = ide_buff[ATA_IDENT_MODEL + k + 1];
                IDE_devices[count].model[k + 1] = ide_buff[ATA_IDENT_MODEL + k];
            }
            IDE_devices[count].model[40] = '\0';

            ++count;
        }
    }

    for (int i = 0; i < 4; ++i){
        if (IDE_devices[i].exists){
            SAL_add_device(
                (storage_device_t){
                    .maxlba = IDE_devices[i].size / IDE_ATA_SECTOR_SIZE - 1,
                    .sector_size = IDE_ATA_SECTOR_SIZE,
                    .name = IDE_devices[i].model,
                    .drive_num = i,

                    .read_sector  = ATA_read_sector,
                    .write_sector = ATA_write_sector,
                }
            );
        }
    }
}

#define IDE_ATA_WRITE 1
#define IDE_ATA_READ  0

void IDE_ATA_operation(uint8_t direction, uint8_t drive, uint32_t lba, uint16_t *buff){
    uint8_t  lba_mode,
             lba_io[6],
             head,
             sector;
    uint16_t cylinder;
    uint32_t channel = IDE_devices[drive].channel;

    channels[channel].no_interrupt = 0x02;
    IDE_write(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);

    if (lba >= 0x10000000){
        /* LBA48 */
        lba_mode  = 2;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head      = 0;
    } else
    /* does the drive support LBA? */
    if (IDE_devices[drive].features & 0x200){
        /* LBA 28 */
        lba_mode  = 1;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head      = (lba & 0xF000000) >> 24;
    }
    else {
        /* CHS */
        lba_mode  = 0;
        sector    = (lba % 63) + 1;
        cylinder  = (lba + 1  - sector) / (16 * 63);
        lba_io[0] = sector;
        lba_io[1] = (cylinder >> 0) & 0xFF;
        lba_io[2] = (cylinder >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head      = (lba + 1  - sector) % (16 * 63) / (63);
    }

    while (IDE_read(channel, ATA_REG_STATUS) & ATA_SR_MASK_BUSY);

    if (!lba_mode)
        IDE_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (IDE_devices[drive].is_slave_drive << 4) | head); /* Drive & CHS */
    else
        IDE_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (IDE_devices[drive].is_slave_drive << 4) | head); /* Drive & LBA */

    /* write params */

    if (lba_mode == 2){
        IDE_write(channel, ATA_REG_SECCOUNT1, 0);
        IDE_write(channel, ATA_REG_LBA3, lba_io[3]);
        IDE_write(channel, ATA_REG_LBA4, lba_io[4]);
        IDE_write(channel, ATA_REG_LBA5, lba_io[5]);
    }
    IDE_write(channel, ATA_REG_SECCOUNT0, 1);
    IDE_write(channel, ATA_REG_LBA0, lba_io[0]);
    IDE_write(channel, ATA_REG_LBA1, lba_io[1]);
    IDE_write(channel, ATA_REG_LBA2, lba_io[2]);

    /* select command & send it */

    uint8_t command;

    /* write */
    if (direction){
        if (lba_mode == 2)
            command = ATA_CMD_WRITE_PIO_EXT;
        else
            command = ATA_CMD_WRITE_PIO;
    }
    /* read */
    else {
        if (lba_mode == 2)
            command = ATA_CMD_READ_PIO_EXT;
        else
            command = ATA_CMD_READ_PIO;
    }

    IDE_write(channel, ATA_REG_COMMAND, command);

    IDE_poll(channel, false);
    
    /* write */
    if (direction)
        for (int i = 0; i < IDE_ATA_SECTOR_SIZE / 2; ++i)
            port_write_word(channels[channel].IO_base, buff[i]);
    /* read */
    else {
        for (int i = 0; i < IDE_ATA_SECTOR_SIZE / 2; ++i)
            buff[i] = port_read_word(channels[channel].IO_base);

        static char commands[] = {
            ATA_CMD_CACHE_FLUSH,
            ATA_CMD_CACHE_FLUSH,
            ATA_CMD_CACHE_FLUSH_EXT,
        };
    
        IDE_write(channel, ATA_REG_COMMAND, commands[lba_mode]);
    
        IDE_poll(channel, false);
    }
}

uint8_t IDE_ATA_write_sector(uint8_t drive, uint32_t lba, const void *buff){
    if (drive > 3 || !IDE_devices[drive].exists)
        return 1;

    if (lba > IDE_devices[drive].size)
        return 1;

    IDE_ATA_operation(IDE_ATA_WRITE, drive, lba, buff);

    return 0;
}

uint8_t IDE_ATA_read_sector(uint8_t drive, uint32_t lba, void *buff){
    if (drive > 3 || !IDE_devices[drive].exists)
        return 1;

    if (lba > IDE_devices[drive].size)
        return 1;
    
    IDE_ATA_operation(IDE_ATA_READ, drive, lba, buff);

    return 0;
}

static uint8_t current_drive;

void ATA_write_sector(storage_device_t *device, const void *buff, uint32_t lba){
    IDE_ATA_write_sector(device->drive_num, lba, buff);
}

void ATA_read_sector (storage_device_t *device, void *buff, uint32_t lba){
    IDE_ATA_read_sector(device->drive_num, lba, buff);
}
