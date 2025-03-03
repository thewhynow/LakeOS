/* constants for multiboot header */
.set ALIGN,    1<<0             /* aligns modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* multiboot flag field */
.set MAGIC,    0x1badB002       /* magic number that lets bootloader find header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above */

/* 
    declare multiboot header - makes program the kernel
    bootloader searches for this signature in the first 8KiB
    of the kernel file, aligned to 4 bytes

    signature in own section so header can be forced to be within first 8KiB of file
*/

.section .multiboot
    .align 4
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

/*
    create a "stack" -> just static memory
    since stack grows downwards, lowest address = bottom of stack
*/
.section .bss
    .align 16 // stack must be 16-byte aligned according to System-V
    stack_bottom:
        .skip 16384 // 16 KiB
    stack_top:

/* entry point of kernel */
.section .text
.global _start
.type _start, @function
_start:
    /* 
    set up the "stack" with the stack pointer and 
    base pointer that will be used by C programs
    */ 

    movl $stack_top, %esp // use 32-bit assembly, no lea required here
    movl %esp, %ebp

    cli // disable interrupts

    call kernel_main // hand over control to the C portion, everything initialized

    /*
    if the system has nothing more to do, infinite loop:
        1) disable interrupts with cli (clear interrupt enable in eflags)
        2) halt the CPU until next interrupt ... which will never come
        3) if the interrupt does somehow come go back to the halt
    */

    cli
hlt:hlt
    jmp hlt

/*
apparently useful for debugging later
*/
.size _start, . - _start
