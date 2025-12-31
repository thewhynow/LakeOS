#include "../../libc/include/stdio.h"
#include "../../libc/include/stdlib.h"
#include "../include/tty.h"
#include "../include/gdt.h"
#include "../include/idt.h"
#include "../include/isr.h"
#include "../include/irq.h"
#include "../include/pit.h"
#include "../include/pmm.h"
#include "../include/vmm.h"
#include "../include/fdc.h"
#include "../include/ata.h"
#include "../include/sal.h"
#include "../include/fat.h"
#include "../include/kmm.h"
#include "../include/rtc.h"
#include "../include/vfs.h"

void kernel_main() {
    terminal_init();
    printf("Loading GDT...");
    GDT_init();
    printf("GDT Loaded!\n");
    printf("Loading IDT...");
    IDT_init();
    printf("IDT Loaded!\n");
    printf("Loading ISR...");
    ISR_init();
    printf("ISR Loaded!\n");
    printf("Loading IRQ...");
    IRQ_init();
    printf("IRQ Loaded!\n");
    printf("Loading RTC...");
    RTC_init();
    printf("RTC Loaded!\n");
    printf("Loading PIT...");
    PIT_init();
    printf("PIT Loaded!\n");
    printf("Loading PMM...");
    PMM_init();
    printf("PMM Loaded!\n");
    printf("Loading VMM...");
    VMM_init();
    printf("VMM Loaded!\n");
    printf("Loading KMM...");
    KMM_init();
    printf("KMM Loaded!\n");
    printf("Loading SAL...");
    SAL_init();
    printf("SAL Loaded!\n");
    printf("Loading FDC...");
    FDC_init();
    printf("FDC Loaded!\n");
    printf("Loading IDE...");
    IDE_init();
    printf("IDE Loaded!\n");
    printf("Loading FAT...");
    FAT_init();
    printf("FAT Loaded!\n");
    printf("Loading VFS...\n");
    VFS_init();
    printf("VFS Loaded!\n");

    printf("the date is %d/%d/%d\n", time.month, time.monthday, time.year);
    printf("the time is %d:%d:%d\n", time.hours, time.minutes,  time.seconds);

    printf("Welcome to lakeOS!\n");
    
    char *string = kmalloc(100);
    
    while (true) {
        gets(string);
        printf("string: %s\n", string);
	
        if (!memcmp(string, "quit", 4))
            for (;;);
    }
}
