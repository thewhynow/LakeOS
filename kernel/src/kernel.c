#include <kernel/ata.h>
#include <kernel/fat.h>
#include <kernel/fdc.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/isr.h>
#include <kernel/kmm.h>
#include <kernel/pit.h>
#include <kernel/pmm.h>
#include <kernel/rtc.h>
#include <kernel/sal.h>
#include <kernel/tss.h>
#include <kernel/tty.h>
#include <kernel/vfm.h>
#include <kernel/vfs.h>
#include <kernel/vmm.h>
#include <stdio.h>

void kernel_main() {
    terminal_init();
    printf("Loading TSS...");
    TSS_init();
    printf("TSS Loaded!\n");
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
    printf("Loading VFM...");
    VFM_init();
    printf("VFM Loaded!\n");
    printf("Loading VFS...");
    VFS_init();
    printf("VFS Loaded!\n");

    printf("The date is %d/%d/%d\n", time.month, time.monthday, time.year);
    printf("The time is %d:%d:%d\n", time.hours, time.minutes, time.seconds);

    printf("Welcome to lakeOS!\n");

    printf("Warping into userspace...\n");

    extern void jump_ring3();
//    jump_ring3();
}
