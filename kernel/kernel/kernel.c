#include "../../libc/include/stdio.h"
#include "../include/kernel/tty.h"
#include "../../libc/include/stdlib.h"
#include "../include/kernel/gdt.h"
#include "../include/kernel/idt.h"
#include "../include/kernel/isr.h"
#include "../include/kernel/irq.h"
#include "../include/kernel/pit.h"
#include "../include/kernel/pmm.h"

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
    // printf("Loading PMM...");
    PMM_init();
    // printf("PMM Loaded!\n");

    int* first_alloc = (int*)0;
    *first_alloc = 12;
    printf("first alloc: %i\n", *first_alloc);
    kfree(first_alloc);

    printf("Welcome to lakeOS!\n");

    char string[0xFF];

    while (1){
        gets(string);
        printf("string: %s\n", string);
        memset(string, 0, 0xFF - 1);
    }

    for (;;);
}