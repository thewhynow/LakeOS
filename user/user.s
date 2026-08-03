.section .text
.global _start
.type _start, @function
_start:
    movl $1, %eax
    movl $1, %ebx
    movl $msg, %ecx
    movl $msg_len, %edx
    int $0x80

    movl $60, %eax
    movl $29, %ebx
    int $0x80

.section .rodata
msg:
    .ascii "Hello, World!\n"
msg_len = . - msg

