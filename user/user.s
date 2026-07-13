.section .text
.global _start
_start:
    movl $1, %eax
    movl $1, %edi
    movl $msg, %esi
    movl $msg_len, %edx
    int $0x80

    movl $60, %eax
    movl $29, %edi
    int $0x80

.section .rodata
msg:
    .ascii "Hello, World!\n"
msg_len = . - msg

