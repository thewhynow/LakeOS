#include "../../libc/include/stdio.h"
#include "../../libc/include/stdlib.h"
#include "../include/kernel/tty.h"
#include "../include/kernel/gdt.h"
#include "../include/kernel/idt.h"
#include "../include/kernel/isr.h"
#include "../include/kernel/irq.h"
#include "../include/kernel/pit.h"
#include "../include/kernel/pmm.h"
#include "../include/kernel/vmm.h"

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
    printf("Loading PMM...");
    PMM_init();
    printf("PMM Loaded!\n");
    printf("Loading VMM...");
    VMM_init();
    printf("VMM Loaded!\n");

    printf("Welcome to lakeOS!\n");

    char* string;

    // string = alloc_page();
    // vmm_map_page(string, string);

    // while (1) {
    //     memset(string, 0, 100);
    //     gets(string);
    //     printf("string: %s\n", string);

    //     if (!memcmp(string, "quit", 4))
    //         for (;;);
    // }

    for (;;);
}