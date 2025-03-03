/* designated kernel entry point */
ENTRY(_start)

/* where to put the different sections */
SECTIONS {
    /* no fucking idea what this does */
    . = 2M;

    /* place the multiboot header at the beginning of the executeable */
    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text)
    }

    /* readonly data */
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    /* initialized data */
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    /* unintialized data + stack */
    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}