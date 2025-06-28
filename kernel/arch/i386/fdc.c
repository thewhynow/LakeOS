#include "../../include/kernel/fdc.h"
#include "../../include/kernel/irq.h"
#include "../../include/kernel/pic.h"
#include "../../include/kernel/dma.h"
#include "../../include/kernel/pmm.h"
#include "../../include/kernel/vmm.h"

static bool floppy_irq_fired;

uint8_t  current_drive;
uint32_t buff_paddr;
uint16_t buff_len;

void FDC_CMD_read_sector(uint8_t head, uint8_t track, uint8_t sector);

void FDC_lba_to_chs(uint32_t lba, uint8_t *head, uint8_t *track, uint8_t *sector){
    *head   = (lba / SECTORS_PER_TRACK) % 2;
    *track  = lba / (SECTORS_PER_TRACK * 2);
    *sector = lba % SECTORS_PER_TRACK + 1;
}

void FDC_enable(){
    port_write_byte(FDC_DOR, FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA);
}

void FDC_disable(){
    port_write_byte(FDC_DOR, 0);
}

void FDC_reset(){
    uint8_t st0, cyl;

    FDC_disable(); FDC_enable();
    FDC_irq_wait();

    FDC_check_int(&st0, &cyl);
    
    /* transfer speed 500 KBPS */
    port_write_byte(FDC_CTRL, 0);

    // FDC_CMD_drive_data(3, 16, 240, true);
    FDC_write_cmd(FDC_CMD_SPECIFY);
    FDC_write_cmd(0xDF);
    FDC_write_cmd(0x02);

    FDC_CMD_callibrate(current_drive);
}

void IRQ_FDC_handler(){
    floppy_irq_fired = true;
    
    PIC_end_of_int(FLOPPY_IRQ);
}

void FDC_irq_wait(){
    while (!floppy_irq_fired);

    floppy_irq_fired = false;
}

void FDC_DMA_init(){
    DMA_set_mask(FLOPPY_CHANNEL, true);

    DMA_reset_flipflop(0);
    DMA_set_full_address(FLOPPY_CHANNEL, buff_paddr);
    DMA_reset_flipflop(0);
    DMA_set_count(FLOPPY_CHANNEL, buff_len - 1);
    DMA_SET_READ(FLOPPY_CHANNEL);

    DMA_set_mask(FLOPPY_CHANNEL, false);
}

void FDC_init(){
    void *buff = alloc_page();
    if ((0xFFFFFF - (unsigned long)buff) < 4096)
        return;

    buff_paddr = (uint32_t) buff;
    buff_len = 4096;
    current_drive = 0;

    /* map the buffer so we can access it */
    vmm_map_page(buff, buff);

    memset(buff, 'A', 4095);

    PIC_unmask(FLOPPY_IRQ);

    FDC_DMA_init();

    FDC_reset();

    FDC_CMD_drive_data(13, 1, 0xF, true);
}

void FDC_CMD_read_sector(uint8_t head, uint8_t track, uint8_t sector){
    FDC_CMD_seek(0, 0);
    FDC_CMD_seek(0, 1);
    
    FDC_start_motor(current_drive);
    
    FDC_DMA_init();
    DMA_SET_READ(FLOPPY_CHANNEL);
    
    FDC_write_cmd(FDC_CMD_READ_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
    
    FDC_write_cmd(head << 2 | current_drive);
    FDC_write_cmd(track);
    FDC_write_cmd(head);
    FDC_write_cmd(sector);
    FDC_write_cmd(FDC_CMD_DTL_512);
    FDC_write_cmd(SECTORS_PER_TRACK);
    FDC_write_cmd(FDC_CMD_GAP3_LENGTH_3_5);
    FDC_write_cmd(0xFF);
    
    FDC_irq_wait();

    for (int i = 0; i < 7; ++i)
        FDC_read_data();

    FDC_stop_motor();
}

void *FDC_read_sector(uint32_t lba){
    uint8_t head, track, sector;

    FDC_lba_to_chs(lba, &head, &track, &sector);

    FDC_start_motor(current_drive);

    if (FDC_CMD_seek(track, head) != 0)
        return NULL;
        
    FDC_CMD_read_sector(head, track, sector);

    FDC_stop_motor();

    return (void*) buff_paddr;
}

void FDC_CMD_drive_data(uint32_t stepr, uint32_t loadt, uint32_t unloadt, bool dma){
    uint32_t data = 0;

    FDC_write_cmd(FDC_CMD_SPECIFY);

    data = ((stepr & 0xF) << 4) | (unloadt & 0xF);
    FDC_write_cmd(data);

    data = (loadt << 1) | !dma;
    FDC_write_cmd(data);
}

int FDC_CMD_callibrate(uint8_t drive){
    uint8_t st0, cyl;

    FDC_start_motor(drive);

    for (int i = 0; i < 10; ++i){
        FDC_write_cmd(FDC_CMD_CALIBRATE);
        FDC_write_cmd(drive);

        FDC_irq_wait();

        FDC_check_int(&st0, &cyl);

        if (st0 & 0xC0)
            printf("FDC_CMD_callibrate: status = fuck\n");
        
        if (!cyl){
            FDC_stop_motor();
            return 0;
        }
    }
    
    printf("FDC_CMD_callibrate: status = fuck\n");
    FDC_stop_motor();
    return -1;
}

int FDC_CMD_seek(uint8_t cyl, uint8_t head){
    uint8_t st0, cyl0;

    for (int i = 0; i < 10; ++i){
        FDC_write_cmd(FDC_CMD_SEEK);
        FDC_write_cmd(head << 2 | current_drive);
        FDC_write_cmd(cyl);

        FDC_irq_wait();

        FDC_check_int(&st0, &cyl0);

        if (cyl0 == cyl)
            return 0;
    }

    FDC_stop_motor();
    return -1;
}

void FDC_read_init(){
    DMA_SET_READ(FLOPPY_CHANNEL);
}

void FDC_write_init(){
    DMA_SET_WRITE(FLOPPY_CHANNEL);
}

void FDC_write_cmd(uint8_t command){
    while (!(port_read_byte(FDC_MSR) & FDC_MSR_MASK_DATA_READY));

    port_write_byte(FDC_FIFO, command);
}

uint8_t FDC_read_data(){
    while (!(port_read_byte(FDC_MSR) & FDC_MSR_MASK_HAS_DATA)) ;
    return port_read_byte(FDC_FIFO);
}

void FDC_check_int(uint8_t *st0, uint8_t *cylinder){
    FDC_write_cmd(FDC_CMD_CHECK_INT);
    
    *st0 = FDC_read_data();

    /* error code, doesnt matter */
    if (*st0 == 0x80){
        printf("panic: st0==0x80\n");
        for (;;);
    }

    *cylinder = FDC_read_data();
}

void FDC_start_motor(uint8_t drive){
    switch (drive){
        case 0: port_write_byte(FDC_DOR, FDC_DOR_MASK_DRIVE0_MOTOR | FDC_DOR_MASK_RESET); break;
        case 1: port_write_byte(FDC_DOR, FDC_DOR_MASK_DRIVE1_MOTOR | FDC_DOR_MASK_RESET); break;
        case 2: port_write_byte(FDC_DOR, FDC_DOR_MASK_DRIVE2_MOTOR | FDC_DOR_MASK_RESET); break;
        case 3: port_write_byte(FDC_DOR, FDC_DOR_MASK_DRIVE3_MOTOR | FDC_DOR_MASK_RESET); break;
    }

    /* give motor time to start up */
    for (int i = 0; i < 10000; ++i);
}

void FDC_stop_motor(){
    port_write_byte(FDC_DOR, FDC_DOR_MASK_RESET);
}