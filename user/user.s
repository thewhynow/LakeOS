.section .text
.global _start
.type _start, @function
_start:
// write(stdout, msg, msg_len)
    movl $1, %eax
    movl $1, %ebx
    movl $msg, %ecx
    movl $msg_len, %edx
    int $0x80

// fd = open("/USER/HELLO.TXT", SYSCALL_OPEN_READ | SYSCALL_OPEN_WRITE | SYSCALL_OPEN_CREAT)
    movl $2, %eax
    movl $fname, %ebx
    movl $0b10011, %ecx
    int $0x80
    movl %eax, fd

// write(fd, msg, msg_len)
    movl $1, %eax
    movl fd, %ebx
    movl $msg, %ecx
    movl $msg_len, %edx
    int $0x80

// close(fd)
    movl $3, %eax
    movl $fd, %ebx
    int $0x80

// exit(29)
    movl $60, %eax
    movl $29, %ebx
    int $0x80

.section .data
fd:
    .long 0

.section .rodata
msg:
    .asciz "Hello, World!\n"
msg_len = . - msg

fname:
    .asciz "/USER/HELLO.TXT"
fname_len = . - fname

