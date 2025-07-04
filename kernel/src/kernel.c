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

void kernel_main(){
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
    printf("Loading FDC...");
    FDC_init();
    printf("FDC Loaded!\n");
    printf("Loading IDE...");
    IDE_init();
    printf("IDE Loaded!\n");
    printf("Welcome to lakeOS!\n");

    char *data = alloc_page();
    data = vmm_map_page(data, data);

    memcpy(data, "Hello, World!", 14);

    IDE_ATA_write_sector(0, 0, data);
    memset(data, 'A', 14);
    IDE_ATA_read_sector(0, 0, data);

    printf("%s\n", data);

    char* string;

    string = alloc_page();
    string = vmm_map_page(string, string);

    while (1) {
        memset(string, 0, 100);
        gets(string);
        printf("string: %s\n", string);

        if (!memcmp(string, "quit", 4))
            for (;;);
    }

    for (;;);
}