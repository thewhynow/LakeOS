#include "../../libc/include/stdio.h"
#include "../include/kernel/tty.h"
#include "../../libc/include/stdlib.h"
#include "../include/kernel/gdt.h"
#include "../include/kernel/idt.h"
#include "../include/kernel/isr.h"
#include "../include/kernel/irq.h"
#include "../include/kernel/pit.h"

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
    printf("Welcome to lakeOS!\n");

    char string[0xFF];

    printf("World: Hello, User!\n");
    printf("What does the User respond?: ");

    gets(string);

    printf("\"%s!\", the User responds\n", string);

    for (;;);
}