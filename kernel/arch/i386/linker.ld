/* designated kernel entry point */
ENTRY(_start)

/* where to put the different sections */
SECTIONS {
    /* kernel starts @ 3GB + 1MB */
    . = 0xC0100000;
    
    _begin = . - 0xC0000000;

    .multiboot : {
        KEEP(*(.multiboot))
    }

    .text ALIGN(4K) : AT(ADDR(.text)-0xC0000000) {
        *(.text)
    }

    /* readonly data */
    .rodata ALIGN(4K) : AT(ADDR(.rodata)-0xC0000000) {
        *(.rodata)
    }

    /* initialized data */
    .data ALIGN(4K) : AT(ADDR(.data)-0xC0000000) {
        *(.data)
    }

    /* unintialized data + stack */
    .bss ALIGN(4K) : AT(ADDR(.bss)-0xC0000000) {
        *(COMMON)
        *(.bss)
    }

    _end = . - 0xC0000000;
}