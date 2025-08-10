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
#define _FAT_H_INTERNAL
#include "../include/fat.h"

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
    printf("Loading PIT...");
    PIT_init();
    printf("PIT Loaded!\n");
    printf("Loading PMM...");
    PMM_init();
    printf("PMM Loaded!\n");
    printf("Loading VMM...");
    VMM_init();
    printf("VMM Loaded!\n");
    printf("Loading SAL...");
    SAL_init();
    printf("SAL Loaded!\n");
    printf("Loading FDC...");
    FDC_init();
    printf("FDC Loaded!\n");
    printf("Loading IDE...");
    IDE_init();
    printf("IDE Loaded!\n");

    uint32_t num_devices;
    
    printf("Dumping Storage Devices...\n");
    storage_device_t *devices = SAL_get_devices(&num_devices);
    for (int i = 0; i < (int) num_devices; ++ i)
        printf("Device %i name: %s\n", i, devices[i].name);

    t_FATContext ctx = (t_FATContext){ .device = devices + 0, .partition_start = 0 };
    FAT_context_init(&ctx);

    FAT_test(&ctx);

    printf("Welcome to lakeOS!\n");
    
    char *string = valloc_page(NULL);
    
    while (1) {
        memset(string, 0, 100);
        gets(string);
        printf("string: %s\n", string);

        if (!memcmp(string, "quit", 4))
            for (;;);
    }
}