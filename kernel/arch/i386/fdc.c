#define _FDC_H_INTERNAL

#include "../../include/kernel/fdc.h"
#include "../../include/kernel/irq.h"
#include "../../include/kernel/pic.h"
#include "../../include/kernel/dma.h"
#include "../../include/kernel/pmm.h"
#include "../../include/kernel/vmm.h"

static bool floppy_irq_fired;

uint8_t  current_drive;
void     *buff_paddr;

static void FDC_set_drive(uint8_t drive){
    port_write_byte(FDC_DOR, drive | FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA);
    current_drive = drive;
}

static void FDC_lba_to_chs(uint32_t lba, uint8_t *head, uint8_t *cylinder, uint8_t *sector){
    *head   = (lba / SECTORS_PER_TRACK) % 2;
    *cylinder  = (lba / SECTORS_PER_TRACK) / 2;
    *sector = lba % SECTORS_PER_TRACK + 1;
}

static void FDC_enable(){
    port_write_byte(FDC_DOR, current_drive | FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA);
}

static void FDC_disable(){
    port_write_byte(FDC_DOR, 0);
}

static void FDC_reset(){
    uint8_t st0, cyl;

    FDC_disable(); FDC_enable();
    FDC_irq_wait();

    for (int i = 0; i < 4; ++i)
        FDC_check_int(&st0, &cyl);
    
    /* transfer speed 500 KBPS */
    port_write_byte(FDC_CTRL, 0);

    FDC_CMD_specify(3, 16, 240, 1);

    FDC_CMD_callibrate(current_drive);
}

void IRQ_FDC_handler(){
    floppy_irq_fired = true;
    
    PIC_end_of_int(FLOPPY_IRQ);
}

static void FDC_irq_wait(){
    while (!floppy_irq_fired);

    floppy_irq_fired = false;
}

static void FDC_DMA_init(){
    DMA_set_mask(FLOPPY_CHANNEL, true);

    DMA_reset_flipflop(0);
    DMA_set_full_address(FLOPPY_CHANNEL, (uint32_t) buff_paddr);
    DMA_reset_flipflop(0);
    DMA_set_count(FLOPPY_CHANNEL, 214);

    DMA_set_mask(FLOPPY_CHANNEL, false);
}

static int FDC_check_floppies(){
    /* asm CMOS for drive information */
    port_write_byte(0x70, 0x10);
    uint8_t drives = port_read_byte(0x71);

    /* drive type 4 -> 1.44MB 3.5" */

    if (drives >> 4 == 4) {
        current_drive = 0;
        return 0;
    }
    if ((drives & 0xF) == 4){
        current_drive = 1;
        return 0;
    }

    /* there is no drive */
    return -1;
}

void FDC_init(){
    if (FDC_check_floppies()){
        printf("No floppy-disks mounted\n");
        return;
    }

    buff_paddr = alloc_page();
    if ((0xFFFFFF - (unsigned long)buff_paddr) < 4096)
        return;

    current_drive = 0;

    /* map the buffer so we can access it */
    vmm_map_page(buff_paddr, buff_paddr);

    memset(buff_paddr, 'A', 4095);

    PIC_unmask(FLOPPY_IRQ);
    FDC_set_drive(0);
    FDC_reset();
}

static void FDC_CMD_read_sector(uint8_t head, uint8_t track, uint8_t sector){    
    FDC_DMA_init();
    DMA_SET_READ(FLOPPY_CHANNEL);
    
    FDC_write_cmd(FDC_CMD_READ_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
    
    FDC_write_cmd(head << 2 | current_drive);
    FDC_write_cmd(track);
    FDC_write_cmd(head);
    FDC_write_cmd(sector);
    FDC_write_cmd(FDC_CMD_DTL_512);
    FDC_write_cmd(sector + 1 >= SECTORS_PER_TRACK ? SECTORS_PER_TRACK : sector + 1);
    FDC_write_cmd(FDC_CMD_GAP3_LENGTH_3_5);
    FDC_write_cmd(0xFF);
    
    FDC_irq_wait();

    for (int i = 0; i < 7; ++i)
        FDC_read_data();
}

static void FDC_CMD_write_sector(uint8_t head, uint8_t track, uint8_t sector){
    FDC_DMA_init();
    DMA_SET_WRITE(FLOPPY_CHANNEL);

    FDC_write_cmd(FDC_CMD_WRITE_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
    FDC_write_cmd(head << 2 | current_drive);
    FDC_write_cmd(track);
    FDC_write_cmd(head);
    FDC_write_cmd(sector);
    FDC_write_cmd(FDC_CMD_DTL_512);
    FDC_write_cmd(sector + 1 >= SECTORS_PER_TRACK ? SECTORS_PER_TRACK : sector + 1);
    FDC_write_cmd(FDC_CMD_GAP3_LENGTH_3_5);
    FDC_write_cmd(0xFF);

    FDC_irq_wait();

    for (int i = 0; i < 7; ++i)
        FDC_read_data();
}

void FDC_read_sector(void *buff, uint32_t lba){
    uint8_t head, cylinder, sector;

    FDC_lba_to_chs(lba, &head, &cylinder, &sector);

    FDC_start_motor(current_drive);

    FDC_CMD_seek(cylinder, head);
        
    FDC_CMD_read_sector(head, cylinder, sector);

    FDC_stop_motor();

    memcpy(buff, (void*) buff_paddr, 512);
}

void FDC_write_sector(const void *buff, uint32_t lba){
    memcpy((void*)buff_paddr, buff, 512);

    uint8_t head, cylinder, sector;

    FDC_lba_to_chs(lba, &head, &cylinder, &sector);

    FDC_start_motor(current_drive);

    FDC_CMD_seek(cylinder, head);

    FDC_CMD_write_sector(head, cylinder, sector);
}

static void FDC_CMD_specify(uint32_t stepr, uint32_t loadt, uint32_t unloadt, bool dma){
    uint32_t data = 0;

    FDC_write_cmd(FDC_CMD_SPECIFY);

    data = ((stepr & 0xF) << 4) | (unloadt & 0xF);
    FDC_write_cmd(data);

    data = (loadt << 1) | !dma;
    FDC_write_cmd(data);
}

static int FDC_CMD_callibrate(uint8_t drive){
    uint8_t st0, cyl;

    FDC_start_motor(drive);

    for (int i = 0; i < 10; ++i){
        FDC_write_cmd(FDC_CMD_CALIBRATE);
        FDC_write_cmd(drive);

        FDC_irq_wait();

        FDC_check_int(&st0, &cyl);

        if (cyl == 0){
            FDC_stop_motor();
            return 0;
        }
    }
    
    printf("FDC_CMD_callibrate: status = fuck\n");
    FDC_stop_motor();
    return -1;
}

static int FDC_CMD_seek(uint8_t cyl, uint8_t head){
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

    return -1;
}

static void FDC_write_cmd(uint8_t command){

    uint8_t msr = port_read_byte(FDC_MSR);

    while (!(!(msr & FDC_MSR_MASK_HAS_DATA) && msr & FDC_MSR_MASK_DATA_READY))
        msr = port_read_byte(FDC_MSR);

    port_write_byte(FDC_FIFO, command);
}

static uint8_t FDC_read_data(){
    uint8_t msr = port_read_byte(FDC_MSR);

    while (!(msr & FDC_MSR_MASK_HAS_DATA && msr & FDC_MSR_MASK_DATA_READY))
        msr = port_read_byte(FDC_MSR);

    return port_read_byte(FDC_FIFO);
}

static void FDC_check_int(uint8_t *st0, uint8_t *cylinder){
    FDC_write_cmd(FDC_CMD_CHECK_INT);
    
    *st0 = FDC_read_data();

    /* error code, doesnt matter */
    if (*st0 == 0x80){
        printf("panic: st0==0x80\n");
        for (;;);
    }

    *cylinder = FDC_read_data();
}

static void FDC_start_motor(uint8_t drive){
    switch (drive){
        case 0: port_write_byte(FDC_DOR, current_drive | FDC_DOR_MASK_DRIVE0_MOTOR | FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA); break;
        case 1: port_write_byte(FDC_DOR, current_drive | FDC_DOR_MASK_DRIVE1_MOTOR | FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA); break;
        case 2: port_write_byte(FDC_DOR, current_drive | FDC_DOR_MASK_DRIVE2_MOTOR | FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA); break;
        case 3: port_write_byte(FDC_DOR, current_drive | FDC_DOR_MASK_DRIVE3_MOTOR | FDC_DOR_MASK_RESET | FDC_DOR_MASK_DMA); break;
    }

    /* give motor time to start up */
    PIT_sleep(50);
}

static void FDC_stop_motor(){
    port_write_byte(FDC_DOR, FDC_DOR_MASK_RESET);
}